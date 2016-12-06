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

extern volatile struct SPI_Data receivedData;

/******************************* BK TASK *******************************/
unsigned short volume = 0;                //volume of water in BK
unsigned short maxVol = 0;            //maxvolume of BK
unsigned short BKtemp = 0;              //current temperature
unsigned short desiredTempHigh = 0;  //desired BK temperature - 0x00F8
unsigned short desiredTempLow = 0;
unsigned short displayTemp = 0;
unsigned short boilTime = 0;
unsigned char heater = 0;             //heater flag: 0 = off, 1 = on
unsigned char BK_PERIOD = 100;

/* state order is standardized for SPI communication protocols */
enum BKState { INIT, WAIT, FILL, BELOW_TEMP, AT_TEMP, COOL, FINISHED } BK_state;

void BK_Init(){
    BK_state = INIT;
}

void BK_Tick(){
    //Actions
    switch(BK_state){
        case INIT:
            heater = 0;
            maxVol = 0;
            desiredTempHigh = 0;
            desiredTempLow = 0;
            boilTime = 0;
            break;

        case WAIT:
            BKtemp = ADC_read(0);
            heater = 0;
            // TODO: fill in sleep mode and conditional
//            set_sleep_mode(SLEEP_MODE_PWR_SAVE);
//            cli();
//            //  if (...) {
//                sleep_enable();
//                sei();
//                sleep_cpu();
//                sleep_disable();
//            // }
//            sei();
            break;

        case FILL:
            BKtemp = ADC_read(0);
            heater = 0;
            break;

        case BELOW_TEMP:
            BKtemp = ADC_read(0);
            heater = 1;
            boilTime = boilTime - BK_PERIOD;
            break;

        case AT_TEMP:
            BKtemp = ADC_read(0);
            heater = 0;
            boilTime = boilTime - BK_PERIOD;
            break;

        case COOL:
            BKtemp = ADC_read(0);
            heater = 0;
            /* send signal to cool */
            break; 
           
        case FINISHED:
            BKtemp = ADC_read(0);
            heater = 0;
            PORTC = 0;
            PORTD = 0x80;
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
			desiredTempHigh = 0;
            break;

        case WAIT:
            if (desiredTempHigh > 0 && boilTime > 0) {
                BK_state = FILL; 
            }
            break;

        case FILL:
            if (volume == FULL) {
                BK_state = AT_TEMP;
            } 
            break;

        case BELOW_TEMP:
            if (boilTime <= 0) {
                BK_state = COOL;
            } 
            else if (BKtemp >= desiredTempHigh || boilTime <= 0) {
                BK_state = AT_TEMP;
            }
            break;

        case AT_TEMP:
            if (boilTime <= 0) {
                BK_state = COOL;
            }
            else if (BKtemp < desiredTempHigh || boilTime <= 0) {
                BK_state = BELOW_TEMP;
            } 
            break;

        case COOL:
            if (BKtemp <= desiredTempLow) {
                BK_state = FINISHED;
            }
            break;
            
        case FINISHED:
			if (volume == EMPTY) {
				BK_state = WAIT;
			}
            /* stay in finish until signaled? */
            //BK_state = WAIT;
            break;

        default:
            BK_state = INIT;
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
/******************************* BK TASK *******************************/
/******************************* INPUT TASK *******************************/
enum inputStates {input} input_state;

void Input_Init()
{
    input_state = input;
}

void Input_Tick()
{
    //actions
    switch(input_state) {
        case input:
            if (~PINA & 0x02) {
                volume = FULL; 
            }
            else {
                volume = EMPTY;
            }
            break;
        default:
            break;
    }
    //transitions
    switch(input_state) {
        case input:
            break;
        default:
            input_state = input;
            break;
    }
}

void Input_Task()
{
    Input_Init();
    for(;;)
    {
        Input_Tick();
        vTaskDelay(BK_PERIOD);
    }
}
/******************************* INPUT TASK *******************************/

/******************************* OUTPUT TASK *******************************/
enum outputStates {output_init, output} output_state;

void Output_Init()
{
    output_state = output_init;
}

void Output_Tick()
{
    //actions
    switch (output_state){
        case output_init:
            PORTB = 0x00;
            break;
        case output:
            if (volume == FULL) {
                PORTB = PORTB | 0x02;
            }
            else {
                PORTB = PORTB & 0xFD;
            }
            if (heater == 1 && volume == FULL) {
                PORTB = PORTB | 0x01;
            }
            else {
                PORTB = PORTB & 0xFE;
                PORTC = 0;
            }
            //TODO: disable heater when volume is empty   
			displayTemp = (BK_state == COOL) ? desiredTempLow : desiredTempHigh;
            PORTC = displayTemp;   
            PORTD = (displayTemp & 0x0300) >> 2;
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
void SPI_handleReceivedData(void) {
    //struct SPI_Data sendData;

    if (receivedData.flag == 0xFF) {
        /* pinged for data */
        sendData.flag = BK_state;
        sendData.time = boilTime;
        sendData.temp = BKtemp;
        sendData.vol = volume;
        //SPI_Transmit_Data(sendData);
    } else {
        boilTime = receivedData.time;
        desiredTempHigh = receivedData.temp;
        desiredTempLow = receivedData.vol;
        
    }
}

void StartSecPulse(unsigned portBASE_TYPE Priority)
{
    xTaskCreate(BK_Task, (signed portCHAR *)"BK_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
    xTaskCreate(Output_Task, (signed portCHAR *)"Output_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
    xTaskCreate(Input_Task, (signed portCHAR *)"Input_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
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
    //init ADC
    ADC_init();
    Set_A2D_Pin(0);
    SPI_SlaveInit();
    //RunSchedular
    vTaskStartScheduler();
    
    return 0;
}
