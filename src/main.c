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

#include "athai005_npelh001_spi.h"

extern unsigned long receivedData;
 
unsigned short HLT_maxVol = 0;           //volume of water in HLT
unsigned short HLT_temp = 0;             //current temperature
unsigned short HLT_desiredTemp = 0;      //desired HLT temperature - 0x00F8

unsigned short MT_mashTime = 0;
unsigned short MT_temp = 0;              //current temperature
unsigned short MT_desiredTemp = 0;       //desired HLT temperature - 0x00F8

unsigned short BK_maxVol = 1;            //maxvolume of BK
unsigned short BK_boilTime = 0;          //time to keep at boil
unsigned short BK_temp = 0;              //current temperature
unsigned short BK_desiredTemp = 0x00F8;  //desired BK temperature - 0x00F8

/******************************* TMASTER TASK *******************************/
enum TMasterState { INIT, GET_INPUT } TMaster_state;

void TMaster_Init(){
    TMaster_state = INIT;
}

void TMaster_Tick()
{
    /* actions */
    switch(TMaster_state){
        case INIT:
            break;
        case GET_INPUT:
            if (~PINA & 0x01) {
                /* send HEAT WATER to HLT */
                PORTB = (PORTB & 0xF0) | 0x01;
                SPI_Transmit_Long((1 << 16) + HLT_maxVol);
                SPI_Transmit_Long(2 << 16);
                SPI_Transmit_Long((3 << 16) + HLT_desiredTemp);
                PORTB = PORTB & 0xF0;
            } else if (~PINA & 0x02) {
                /* send PERSIST HEAT WATER to HLT */
                PORTB = (PORTB & 0xF0) | 0x01;
                SPI_Transmit_Long((1 << 16) + HLT_maxVol);
                SPI_Transmit_Long((2 << 16) + 1);
                SPI_Transmit_Long((3 << 16) + HLT_desiredTemp);
                PORTB = PORTB & 0xF0;
            } else if (~PINA & 0x04) {
                /* send START MASH to MT */
                PORTB = (PORTB & 0xF0) | 0x02;
                SPI_Transmit_Long((1 << 16) + MT_mashTime);
                SPI_Transmit_Long((3 << 16) + MT_desiredTemp);
                PORTB = PORTB & 0xF0;
            } else if (~PINA & 0x08) {
                /* send START BOIL to BK */
                PORTB = (PORTB & 0xF0) | 0x04;
                SPI_Transmit_Long((1 << 16) + BK_maxVol);
                SPI_Transmit_Long((2 << 16) + BK_boilTime);
                SPI_Transmit_Long((3 << 16) + BK_desiredTemp);
                PORTB = PORTB & 0xF0;
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
