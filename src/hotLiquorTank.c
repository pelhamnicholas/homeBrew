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
#include <avr/sleep.h>

//FreeRTOS include files
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"

//ADC header file
#include "adc.h"
#include "spi.h"

//extern unsigned long receivedData;
extern struct SPI_Data receivedData;

/******************************* HLT TASK *******************************/
unsigned short volume = 0;            //volume of water in HLT
unsigned short maxVol = 50;           //maxvolume of HLT
unsigned short temp = 0;              //current temperature
unsigned short desiredTemp = 0;       //desired HLT temperature - 0x00F8
unsigned short persist = 0;           // 0 = heat, 1 = heat_persist
unsigned char heater = 0;             //heater flag: 0 = off, 1 = on

/* state order is standardized for SPI communication protocols */
enum HLTState { INIT, WAIT, FILL, BELOW_TEMP, AT_TEMP, FINISHED } hlt_state;

void HLT_Init(){
    hlt_state = init;
}

void HLT_Tick(){
    //Actions
    switch(hlt_state){
        case INIT:
            volume = 0;
            heater = 0;
            desiredTemp = 0;
            hlt_state = wait;
            break;

        case WAIT:
            temp = ADC_read(0);
            heater = 0;
            // TODO: fill in sleep mode and conditional
            set_sleep_mode(...);
            cli();
            if (...) {
                sleep_enable();
                sei();
                sleep_cpu();
                sleep_disable();
            }
            sei();
            break;

        case FILL:
            temp = ADC_read(0);
            heater = 0;
            volume++;
            break;

        case BELOW_TEMP:
            temp = ADC_read(0);
            heater = 1;
            break;

        case AT_TEMP:
            temp = ADC_read(0);
            heater = 0;
            break;

        case FINISHED:
            /* waits to be pinged for state value?*/
//            sendData.flag = HLT_state;
//            sendData.time = 0;
//            sendData.temp = temp;
//            sendData.vol = volume;
//            SPI_Transmit_Data();
            break;

        default:
            heater = 0;
            break;
    }

    //Transitions
    switch(hlt_state){
        case INIT: 
            hlt_state = WAIT; 
            break;

        case WAIT:
            if (desiredTemp > 0) hlt_state = FILL; 
            break;

        case FILL:
            if (volume >= maxVol) {
                heater = 0;
                hlt_state = BELOW_TEMP;
            } 
            break;

        case BELOW_TEMP:
            if (!persist && temp_reached) {
                hlt_state = FINISHED;
            } else if (temp >= desiredTemp) {
                hlt_state = AT_TEMP;
            }
            break;

        case AT_TEMP:
            if (!persist) {
                hlt_state = FINISHED;
            }
            break;

        case FINISHED:
            hlt_state = WAIT;

        default:
            hlt_state = INIT;
            break;
    }
}

void HLT_Task()
{
    HLT_Init();
    for(;;)
    {
        HLT_Tick();
        vTaskDelay(100);
    }
}

/******************************* HLT TASK *******************************/

void handleReceivedData(void) {
    struct SPI_Data sendData;

    if (receivedData.flag == 0xFF) {
        sendData.flag = HLT_state;
        sendData.time = 0;
        sendData.temp = temp;
        sendData.vol = volume;
        SPI_Transmit_Data(sendData);
    } else {
        persist = sendData.flag;
        desiredTemp = sendData.temp;
        maxVol = sendData.vol;
    }
}

void StartSecPulse(unsigned portBASE_TYPE Priority)
{
    xTaskCreate(HLT_Task, (signed portCHAR *)"HLT_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

int main(void)
{
    //DDR and Port settings:
    //DDR : 0x00 = input, 0xFF = output
    //PORT: 0xFF = input, 0x00 = output
    DDRA = 0x00; PORTA = 0xFF;
    DDRB = 0xFF; PORTB = 0x00;
    DDRC = 0xFF; PORTC = 0x00;
    DDRD = 0x00; PORTD = 0xFF;
    //Start Tasks
    StartSecPulse(1);
    //init ADC
    ADC_init();
    Set_A2D_Pin(0);
    SPI_SlaveInit();
    //RunSchedular
    vTaskStartScheduler();
    
    return 0;
}
