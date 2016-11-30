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
struct SPI_Data sendData;
 
unsigned short HLT_maxVol = 1000;        //volume of water in HLT
unsigned short HLT_temp = 0;             //current temperature
unsigned short HLT_desiredTemp = 0x00F8; //desired HLT temperature - 0x00F8

unsigned short MT_mashTime = 1000;
unsigned short MT_temp = 0;              //current temperature
unsigned short MT_desiredTemp = 0x00F8;  //desired HLT temperature - 0x00F8

unsigned short BK_maxVol = 1;            //maxvolume of BK
unsigned short BK_boilTime = 3000;          //time to keep at boil
unsigned short BK_temp = 0;              //current temperature
unsigned short BK_desiredTemp = 0x00F8;  //desired BK temperature - 0x00F8
unsigned short BK_desiredTempLow = 0x0028;	 //desired low BK temperature 0x0028

void SPI_handleReceivedData(void) {
	return;
}

/******************************* TMASTER TASK *******************************/
enum TMasterState { INIT, GET_INPUT } TMaster_state;

void TMaster_Init(){
    TMaster_state = INIT;
}

void TMaster_Tick()
{
	unsigned short data[2];
    /* actions */
    switch(TMaster_state){
        case INIT:
			//PORTB = PORTB | 0x07;
            break;
        case GET_INPUT:
			//PORTB = PORTB | 0x07;
            if (~PINA & 0x01) {
                /* send HEAT WATER to HLT */
                PORTB = PORTB & 0xFE;

				/* for struct data */
				sendData.flag = 0;
				sendData.temp = HLT_desiredTemp;
				sendData.time = 0;
				sendData.vol = HLT_maxVol;
				SPI_Transmit_Data(sendData);

                PORTB = (PORTB & 0xFE) | 0x01;
            } else if (~PINA & 0x02) {
                /* send PERSIST HEAT WATER to HLT */
                PORTB = PORTB & 0xFE;

				/* for struct data */
				sendData.flag = 1;
				sendData.temp = HLT_desiredTemp;
				sendData.time = 0;
				sendData.vol = HLT_maxVol;
				SPI_Transmit_Data(sendData);

                PORTB = (PORTB & 0xFE) | 0x01;
            } else if (~PINA & 0x04) {
                /* send START MASH to MT */
                PORTB = PORTB & 0xFD;

				/* for struct data */
				sendData.flag = 0;
				sendData.temp = MT_desiredTemp;
				sendData.time = MT_mashTime;
				sendData.vol = 0;
				SPI_Transmit_Data(sendData);

                PORTB = (PORTB & 0xFD) | 0x02;
            } else if (~PINA & 0x08) {
                /* send START BOIL to BK */
                PORTB = PORTB & 0xFB;

				/* for struct data */
				sendData.flag = 0;
				sendData.temp = BK_desiredTemp;
				sendData.time = BK_boilTime;
				sendData.vol = BK_desiredTempLow;
				SPI_Transmit_Data(sendData);

                PORTB = (PORTB & 0xFB) | 0x04;
            }
            break;
        default:
            break;
    }

    /* transitions */
    switch(TMaster_state){
        case INIT:
            TMaster_state = GET_INPUT;
            break;
        case GET_INPUT:
            break;
        default:
            TMaster_state = INIT;
            break;
    }
}

void TMaster_Task()
{
    TMaster_Init();
    for(;;)
    {
        TMaster_Tick();
        vTaskDelay(100);
    }
}
/******************************* TMASTER TASK *******************************/

void StartSecPulse(unsigned portBASE_TYPE Priority)
{
    xTaskCreate(TMaster_Task, (signed portCHAR *)"TMaster_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
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


