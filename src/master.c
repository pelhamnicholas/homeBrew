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
struct SPI_Data sendData;

enum slaveSelect { HLT, MT, BK, NONE } slave;
enum slaveStates { INIT, WAIT, FILL, BELOW_TEMP, AT_TEMP, COOL, FINISHED } HLT_state, MT_state, HLT_state;
 
unsigned short HLT_maxVol = 1000;        //volume of water in HLT
unsigned short HLT_temp = 0;             //current temperature
unsigned short HLT_desiredTemp = 0x00F8; //desired HLT temperature - 0x00F8

unsigned short MT_mashTime = 1000;
unsigned short MT_temp = 0;              //current temperature
unsigned short MT_desiredTemp = 0x00F8;  //desired HLT temperature - 0x00F8

unsigned short BK_maxVol = 1;            //maxvolume of BK
unsigned short BK_boilTime = 0;          //time to keep at boil
unsigned short BK_temp = 0;              //current temperature
unsigned short BK_desiredTemp = 0x00F8;  //desired BK temperature - 0x00F8

static inline void slaveSelect(unsigned char ss) {
    if (ss != NONE) {
        slave = ss;
        PORTB = (PORTB & ~(1<<ss));
    }
}

static inline void slaveDeselect(unsigned char ss) {
    if (ss == SLAVE) {
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
            receivedFlag = 1;
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

/******************************* MASTER TASK *******************************/
enum masterState { INIT, GET_INPUT } master_state;

void Master_Init(){
    master_state = INIT;
}

void Master_Tick()
{
    /* actions */
    switch(master_state){
        default:
            break;
    }

    /* transitions */
    switch(master_state){
        default:
            master_state = INIT;
            break;
    }
}

void Master_Task()
{
    Master_Init();
    for(;;)
    {
        Master_Tick();
        vTaskDelay(100);
    }
}
/******************************* MASTER TASK *******************************/

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
