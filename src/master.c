/*  Partner(s) Name & E-mail: Nicholas Pelham <npelh001@ucr.edu>
 *  Lab Section: 22
 *  Assignment: Lab #2  Exercise #1 
 *  
 *  I acknowledge all content contained herein, excluding template or example
 *  code, is my own original work.
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/portpins.h>
#include <avr/pgmspace.h>

//FreeRTOS include files
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"

#include "spi.h"

//extern unsigned long receivedData; // for unsigned long data
extern struct SPI_Data receivedData; // for struct data
unsigned char receivedFlag;

/* SLAVES */
#define HLT 0
#define MT  1
#define BK  2
#define NONE 9

unsigned char slave = NONE;
enum slaveStates { INIT, WAIT, FILL, BELOW_TEMP, AT_TEMP, COOL, FINISHED } HLT_state, MT_state, HLT_state;
 
//unsigned short HLT_maxVol = 1000;        //volume of water in HLT
unsigned short HLT_temp = 0;             //current temperature
unsigned short HLT_desiredTemp = 0x00F8; //desired HLT temperature - 0x00F8
unsigned char  HLT_vol = 0;

unsigned short MT_mashTime = 1000;
unsigned short MT_temp = 0;              //current temperature
unsigned short MT_desiredTemp = 0x00F8;  //desired HLT temperature - 0x00F8
unsigned char  MT_vol = 0;

//unsigned short BK_maxVol = 1;            //maxvolume of BK
unsigned short BK_boilTime = 0;          //time to keep at boil
unsigned short BK_temp = 0;              //current temperature
unsigned short BK_desiredTemp = 0x00F8;  //desired BK temperature - 0x00F8
unsigned char  BK_vol = 0;

/* VALVES */
#define HLT_MT     0x04A9
#define MT_MT_HEAT 0x0B0A
#define MT_MT_LEAD 0x050A
#define MT_BK      0x0512
#define BK_BK      0x0AD4
unsigned short valves;

/* PUMP */
#define OFF 0
#define ON  1

/* VOLUME */
#define EMPTY 0
#define NOT_EMPTY 1
#define FULL 2

static inline void selectSlave(unsigned char ss) {
    if (ss != NONE) {
        slave = ss;
        PORTB = (PORTB & ~(1<<ss));
    }
}

static inline void deselectSlave(unsigned char ss) {
    if (ss == slave) {
        PORTB = (PORTB | (1<<ss));
        slave = NONE;
    }
}

void SPI_handleReceivedData(void) {
    switch(slave) {
        case HLT:
            HLT_state = receivedData.flag;
            HLT_temp = receivedData.temp;
            HLT_vol = receivedData.vol;
            receivedFlag = 1; // used to wait for response after ping
            break;
        case MT:
            MT_state = receivedData.flag;
            MT_temp = receivedData.temp;
            MT_time = receivedData.time;
            MT_vol = receivedData.vol;
            receivedFlag = 1;
            break;
        case BK:
            BK_state = receivedData.flag;
            BK_temp = receivedData.temp;
            BK_time = receivedData.time;
            BK_vol = receivedData.vol;
            receivedFlag = 1;
            break;
        default:
            /* Do nothing */
            break;
    }
    return;
}

void start_HLT(unsigned char persist) {
    struct SPI_Data sendData;

    sendData.flag = persist;
    sendData.time = 0;
    sendData.temp = HLT_desiredTemp;
    sendData.vol  = 0;

    return;
}

void start_MT(void) {
    struct SPI_Data sendData;

    sendData.flag = 0;
    sendData.time = MT_mashTime;
    sendData.temp = MT_desiredTemp;
    sendData.vol  = 0;

    return;
}

void start_BK(void) {
    struct SPI_Data sendData;

    sendData.flag = 0;
    sendData.time = BK_boilTime;
    sendData.temp = BK_desiredTemp;
    sendData.vol  = 0;

    return;
}

void pingSlave(void) {
    struct SPI_Data sendData;

    sendData.flag = 0xFF;
    sendData.time = 0;
    sendData.temp = 0;
    sendData.vol  = 0;

    return;
}

/***************************** HLT_FILL TASK *****************************/
enum hltFillState { WAITING, FILLING } hlt_fill_state;

void HLTFill_Init() {
    hlt_fill_state = WAITING;
}

void HLTFill_Tick() {
    /* actions */
    switch(hlt_fill_state) {
        case WAITING:
            fillValve = OFF;
            break;
        case FILLING:
            fillValve = ON;
            break;
        default:
            break;
    }

    /* transitions */
    switch(hlt_fill_state) {
        case WAITING:
            if (~PINA & 0x01) {
                start_HLT(0);
                hlt_fill_state = FILLING;
            }
            break;
        case FILLING:
            if (HLT_state != FILL) {
                hlt_fill_state = WAITING;
            }
            break;
        default:
            break;
    }
}

void HLTFill_Task()
{
    HLTFill_Init();
    for(;;)
    {
        HLTFill_Tick();
        vTaskDelay(100);
    }
}
/***************************** HLT_FILL TASK *****************************/

/******************************* PUMP TASK *******************************/
static const unsigned char PUMP_PERIOD = 100;

enum pumpState { OFF, HLT_TO_MT, MT_TO_MT_HEAT, MT_TO_MT_LEAD, 
    MT_TO_BK, BK_TO_BK } pump_state;

void Pump_Init(){
    pump_state = OFF;
}

void Pump_Tick()
{
    /* actions */
    switch(pump_state){
        case OFF:
            pump = OFF;
            valves = 0;
            break;
        case HLT_TO_MT:
            pump = ON;
            valves = HLT_MT;
            break;
        case MT_TO_MT_HEAT:
            pump = ON;
            valves = MT_MT_HEAT;
            break;
        case MT_TO_MT_LEAD:
            timer = timer - PUMP_PERIOD;
            pump = ON;
            valves = MT_MT_LEAD;
            break;
        case MT_TO_BK:
            pump = ON;
            valves = MT_BK;
            break;
        case BK_TO_BK:
            pump = ON;
            valves = BK_BK;
            break;
        default:
            break;
    }

    /* transitions */
    switch(pump_state){
        case OFF:
            if (HLT_state == FINISHED) {
                pump_state = HLT_TO_MT;
            } else if (MT_temp < desiredTemp && HLT_temp == HLT_desiredTemp) {
                pump_state = MT_TO_MT_HEAT;
            } else if (MT_state == FINISHED && MT_vol == FULL
                    && BK_state == FILL && BK_vol != FULL) {
                pump_state = MT_TO_BK;
            } else if (MT_state == FINISHED && BK_state == FILL) {
                pump_state = MT_TO_MT_LEAD;
            } else if (BK_state == COOL) {
                pump_state = BK_TO_BK;
            }
            break;
        case HLT_TO_MT:
            if (MT_vol == FULL || HLT_vol == EMPTY) {
                start_MT();
                pump_state = OFF;
            }
            break;
        case MT_TO_MT_HEAT:
            if (MT_temp >= MT_desiredTemp) {
                pump_state = OFF;
            }
            break;
        case MT_TO_MT_LEAD:
            /* based on a timer */
            if (timer <= 0) {
                pump_state = OFF;
            }
            break;
        case MT_TO_BK:
            if (BK_vol == FULL) {
                pump_state = OFF;
                start_BK();
            } else if (MT_vol == EMPTY) {
                pump_state = HLT_TO_MT;
            }
            break;
        case BK_TO_BK:
            if (BK_state == FINISHED) {
                pump_state == off;
            }
            break;
        default:
            pump_state = OFF;
            break;
    }
}

void Pump_Task()
{
    Pump_Init();
    for(;;)
    {
        Pump_Tick();
        vTaskDelay(PUMP_PERIOD);
    }
}
/******************************* PUMP TASK *******************************/

void StartSecPulse(unsigned portBASE_TYPE Priority)
{
    xTaskCreate(Master_Task, (signed portCHAR *)"Master_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

int main(void)
{
    //DDR and Port settings:
    //DDR:  0x00 = input, 0xFF = output
    //PORT: 0xFF = input, 0x00 = output
    DDRA = 0x00; PORTA = 0xFF;
    DDRB = 0xFF; PORTB = 0x00;
    DDRC = 0xFF; PORTC = 0x00;
    DDRD = 0x00; PORTD = 0xFF;
    //Start Tasks
    StartSecPulse(1);
    /* Init SPI */
    SPI_MasterInit();
    //RunScheduler
    vTaskStartScheduler();
    
    return 0;
}
