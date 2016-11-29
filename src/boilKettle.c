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

extern struct SPI_Data receivedData;

/******************************* BK TASK *******************************/
unsigned short vol= 0;                //volume of water in BK
unsigned short maxVol = 0;            //maxvolume of BK
unsigned short temp = 0;              //current temperature
unsigned short desiredtemp = 0x00F8;  //desired BK temperature - 0x00F8
unsigned short boilTime = 0;
unsigned char heater = 0;             //heater flag: 0 = off, 1 = on

/* state order is standardized for SPI communication protocols */
enum BKState { INIT, WAIT, FILL, BELOW_TEMP, AT_TEMP, FINISHED, COOL } BK_state;

void BK_Init(){
    BK_state = INIT;
}

void BK_Tick(){
    //Actions
    switch(BK_state){
        case INIT:
            heater = 0;
            maxVol = 0;
            desiredTemp = 0;
            boilTime = 0;
            break;

        case WAIT:
            temp = ADC_read(0);
            heater = 0;
            // TODO: fill in sleep mode and conditional
            set_sleep_mode(SLEEP_MODE_PWR_SAVE);
            cli();
            //  if (...) {
                sleep_enable();
                sei();
                sleep_cpu();
                sleep_disable();
            // }
            sei();
            break;

        case FILL:
            temp = ADC_read(0);
            heater = 0;
            vol++;
            break;

        case BELOW_TEMP:
            BKtemp = ADC_read(0);
            heater = 1;
            boilTime = boilTime - BK_PERIOD;
            break;

        case AT_TEMP:
            temp = ADC_read(0);
            heater = 0;
            boilTime = boilTime - BK_PERIOD;
            break;

        case COOL:
            temp = ADC_read(0);
            heater = 0;
            /* send signal to cool */
            break; 
           
        case FINISHED:
            temp = ADC_read(0);
            heater = 0;
            /* send signal finished */
            break;

        default:
            heater = 0;
            break;
    }
    
    //Transitions
    switch(BK_state){
        case INIT: 
            BK_state = WAIT; 
            break;

        case WAIT:
            if (desiredTemp > 0 $$ boilTime > 0) {
                BK_state = FILL; 
            }
            break;

        case FILL:
            if (vol >= maxVol) {
                BK_state = HEAT;
            } 
            break;

        case BELOW_TEMP:
            if (boilTime <= 0) {
                BK_state = COOL;
            }
            break;

        case AT_TEMP:
            if (boilTime <= 0) {
                BK_state = COOL;
            }
            break;

        case COOL:
            if (temp <= desiredTemp) {
                BK_state = FINISH;
            }
            break;
            
        case FINISHED:
            /* stay in finish until signaled? */
            BK_state = WAIT;
            break;

        default:
            BK_state = init;
            break;
    }
}

void BK_Task()
{
    BK_Init();
    for(;;)
    {
        BK_Tick();
        vTaskDelay(100);
    }
}

void handleReceivedData(void) {
    struct SPI_Data sendData;

    if (receivedData.flag == 0xFF) {
        /* pinged for data */
        sendData.flag = BK_state;
        sendData.time = boilTime;
        sendData.temp = temp;
        sendData.vol = vol;
        SPI_Transmit_Data(sendData);
    } else {
        mashTime = receivedData.time;
        desiredTemp = receivedData.temp;
    }
}

void StartSecPulse(unsigned portBASE_TYPE Priority)
{
    xTaskCreate(BK_Task, (signed portCHAR *)"BK_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
    xTaskCreate(TESTtask, (signed portCHAR *)"TEST_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
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
    //init ADC
    ADC_init();
    Set_A2D_Pin(0);
    SPI_SlaveInit();
    //RunSchedular
    vTaskStartScheduler();
    
    return 0;
}
