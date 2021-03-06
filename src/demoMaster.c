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

#include "spiMaster.h"

//extern unsigned long receivedData; // for unsigned long data
struct SPI_Data receivedData; // for struct data
unsigned char receivedFlag;

/* SLAVES */
#define HLT 0
#define MT  1
#define BK  2
#define NONE 9

unsigned char slave = NONE;
unsigned char spitarget = NONE;

enum slaveStates { INIT, WAIT, FILL, BELOW_TEMP, AT_TEMP, COOL, FINISHED } HLT_state, MT_state, BK_state;
 
//unsigned short HLT_maxVol = 1000;        //volume of water in HLT
unsigned short HLT_temp = 0;             //current temperature
unsigned short HLT_desiredTemp = 0x00F8;
unsigned short HLT_desiredTemp_high = 0x00F8; //desired HLT temperature - 0x00F8
unsigned short HLT_desiredTemp_low = 0x0000; //desired HLT temperature - 0x00F8
unsigned char  HLT_vol = 0;

unsigned char fillValve = 0; // flag for HLT valve

unsigned short MT_mashTime = 10000;
unsigned short MT_temp = 0;              //current temperature
unsigned short MT_desiredTemp = 0x00F8;  //desired HLT temperature - 0x00F8
unsigned char  MT_vol = 0;

//unsigned short BK_maxVol = 1;            //maxvolume of BK
unsigned short BK_boilTime = 10000;          //time to keep at boil
unsigned short BK_temp = 0;              //current temperature
unsigned short BK_desiredTemp = 0x00F8;  //desired BK temperature - 0x00F8
unsigned char  BK_desiredTemp_Low = 0x00D8;  //desired BK temperature - 0x00F8
unsigned char  BK_vol = 0;

/* VALVES */
#define HLT_MT     0x04A9
#define MT_MT_HEAT 0x0B0A
#define MT_MT_LEAD 0x050A
#define MT_BK      0x0512
#define BK_BK      0x0AD4
unsigned short valves;
unsigned char pump;

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

void SPI_handleReceivedData(struct SPI_Data receivedData) {
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
            MT_mashTime = receivedData.time;
            MT_vol = receivedData.vol;
            receivedFlag = 1;
            break;
        case BK:
            BK_state = receivedData.flag;
            BK_temp = receivedData.temp;
            BK_boilTime = receivedData.time;
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
	if(slave != HLT) {
		deselectSlave(slave);
		selectSlave(HLT);
		_delay_ms(10);
	}
	
    struct SPI_Data data;
    data.flag = persist;
    data.time = 0xF0;
    data.temp = HLT_desiredTemp;
    data.vol  = 0;	
	receivedData = SPI_Transmit_Data(data);
	SPI_handleReceivedData(receivedData);
	
	if (slave != HLT) {
		deselectSlave(HLT);
		selectSlave(slave);
		_delay_ms(10);
	}
    return;
}

void start_MT(void) {
	if(slave != MT) {
		deselectSlave(slave);
		selectSlave(MT);
		_delay_ms(10);
	}
	
    struct SPI_Data data;
    data.flag = 0;
    data.time = MT_mashTime;
    data.temp = MT_desiredTemp;
    data.vol  = 0;	
	receivedData = SPI_Transmit_Data(data);
	SPI_handleReceivedData(receivedData);
	
	if (slave != MT) {
		deselectSlave(MT);
		selectSlave(slave);
		_delay_ms(10);
	}

    return;
}

void start_BK(void) {
	if(slave != BK) {
		deselectSlave(slave);
		selectSlave(BK);
		_delay_ms(10);
	}
	
    struct SPI_Data data;
    data.flag = 0;
    data.time = BK_boilTime;
    data.temp = BK_desiredTemp;
    data.vol  = BK_desiredTemp_Low;
	receivedData = SPI_Transmit_Data(data);
	SPI_handleReceivedData(receivedData);
	
	if (slave != BK) {
		deselectSlave(BK);
		selectSlave(slave);
		_delay_ms(10);
	}

    return;
}

void pollSlave(void) {
    //struct SPI_Data data;

	/* assume slave has just been selected and give it time to set up */
	/*_delay_ms(10);
    data.flag = 0xFF;
    data.time = 0;
    data.temp = 0;
    data.vol  = 0;
	receivedData = SPI_Transmit_Data(data);
	SPI_handleReceivedData(receivedData);*/

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
				HLT_desiredTemp = HLT_desiredTemp_high;
                hlt_fill_state = FILLING;
            } else if (~PINA & 0x10) {
				HLT_desiredTemp = HLT_desiredTemp_low;
				hlt_fill_state = FILLING;
			}
            break;
        case FILLING:
            if (!(~PINA & 0x11)) {
	            start_HLT(0);
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
unsigned short timer = 0;

enum pumpState { pumpOFF, HLT_TO_MT, MT_TO_MT_HEAT, MT_TO_MT_LEAD, 
    MT_TO_BK, BK_TO_BK, HLT_TO_MT_ } pump_state;

void Pump_Init(){
    pump_state = pumpOFF;
}

void Pump_Tick()
{
    /* actions */
    switch(pump_state){
        case pumpOFF:
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
		case HLT_TO_MT_:
			pump = ON;
            valves = HLT_MT;
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
        case pumpOFF:
            if (~PINA & 0x02) {
                pump_state = HLT_TO_MT;
            } else if (~PINA & 0x04) {
                pump_state = MT_TO_MT_HEAT;
            } else if (~PINA & 0x08) {
				timer = 10000;
                pump_state = MT_TO_MT_LEAD;
            } else if (~PINA & 0x10) {
                pump_state = HLT_TO_MT_;
            } else if (~PINA & 0x20) {
                pump_state = BK_TO_BK;
            }
            break;
        case HLT_TO_MT:
            if (!(~PINA & 0x02)) {
	            start_HLT(1);
	            start_MT();
                pump_state = pumpOFF;
            }
            break;
        case MT_TO_MT_HEAT:
            if (!(~PINA & 0x04)) {
                pump_state = pumpOFF;
            }
            break;
        case MT_TO_MT_LEAD:
            if (timer <= 0) {
                pump_state = MT_TO_BK;
            }
            break;
        case MT_TO_BK:
            if (!(~PINA & 0x08)) {
	            start_BK();
                pump_state = pumpOFF;
            }
            break;
		case HLT_TO_MT_:
			if (!(~PINA & 0x10)) {
				HLT_desiredTemp = HLT_desiredTemp_low;
				pump_state = pumpOFF;
			}
			break;
        case BK_TO_BK:
            if (!(~PINA & 0x20)) {
                pump_state = pumpOFF;
            }
            break;
        default:
            pump_state = pumpOFF;
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

/******************************* OUTPUT TASK *******************************/
enum outputStates {output_init, output} output_state;
const unsigned char const * vout = (unsigned char * ) &valves;

void Output_Init()
{
	output_state = output_init;
}

void Output_Tick()
{
	//actions
	switch (output_state){
		case output_init:
			PORTC = 0;
			PORTD = 0;
			break;
		case output:
			PORTC = (unsigned char) ((valves & 0xFF00) >> 8);
			PORTD = (unsigned char) (valves & 0x00FF); 
			break;
		default:
			break;
	}
	//transitions
	switch (output_state) {
		case output_init:
			output_state = output;
			break;
		case output:
			break;
		default:
			output_state = output;
			break;
	}
}

void Output_Task()
{
	Output_Init();
	for(;;)
	{
		Output_Tick();
		vTaskDelay(100);
	}
}
/******************************* OUTPUT TASK *******************************/

void StartSecPulse(unsigned portBASE_TYPE Priority)
{
    xTaskCreate(Pump_Task, (signed portCHAR *)"Pump_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	xTaskCreate(HLTFill_Task, (signed portCHAR *)"HLTFill_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	xTaskCreate(Output_Task, (signed portCHAR *)"Output_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

int main(void)
{
    //DDR and Port settings:
    //DDR:  0x00 = input, 0xFF = output
    //PORT: 0xFF = input, 0x00 = output
    DDRA = 0x00; PORTA = 0xFF;
    DDRB = 0xFF; PORTB = 0x00;
    DDRC = 0xFF; PORTC = 0x00;
    DDRD = 0xFF; PORTD = 0x00;
    //Start Tasks
    StartSecPulse(1);
    /* Init SPI */
    SPI_MasterInit();
    //RunScheduler
    vTaskStartScheduler();
    
    return 0;
}
