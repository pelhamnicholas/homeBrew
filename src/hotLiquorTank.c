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

//ADC header file
#include "athai005_npelh001_adc.h"
#include "athai005_npelh001_spi.h"

extern unsigned long receivedData;

/******************************* HLT TASK *******************************/
unsigned short volume = 0;            //volume of water in HLT
unsigned short maxVol = 50;           //maxvolume of HLT
unsigned short temp = 0;              //current temperature
unsigned short desiredTemp = 0;       //desired HLT temperature - 0x00F8
unsigned short persist = 0;           // 0 = heat, 1 = heat_persist

enum HLTState {init, wait, fill, heat, heat_persist} hlt_state;

void HLT_Init(){
    hlt_state = init;
}

void HLT_Tick(){
    //Actions
    switch(hlt_state){
        case init:
            break;

        case wait:
            temp = ADC_read(0);
            switch(receivedData >> 16) {
                case 1:
                    maxVol = (unsigned short) (receivedData & 0x0000FFFF);
                    receivedData = 0;
                    break;
                case 2:
                    persist = (unsigned short) (receivedData & 0x0000FFFF);
                    receivedData = 0;
                    break;
                case 3:
                    desiredTemp = (unsigned short) (receivedData & 0x0000FFFF);
                    receivedData = 0;
                    break;
                default:
                    receivedData = 0;
                    break;
            }
            break;

        case fill:
            PORTB = 0x02;
            temp = ADC_read(0);
            volume++;
            break;

        case heat:
            PORTB = 0x01;
            temp = ADC_read(0);
            break;

        case heat_persist:
            if (temp >= desiredTemp) PORTB = 0x00;
            else PORTB = 0x01;
            temp = ADC_read(0);
            break; 

        default:
            PORTA = 0;
            break;
    }
    //Transitions
    switch(hlt_state){
        case init: 
            hlt_state = wait; 
            break;

        case wait:
            if (desiredTemp > 0) hlt_state = fill; 
            break;

        case fill:
            if (volume >= maxVol)
            {
                PORTB = 0x00;
                hlt_state = (persist == 1) ? heat_persist : heat;
            } 
            break;

        case heat:
            if (temp >= desiredTemp) 
            {
                volume = 0;
                PORTB = 0;
                desiredTemp = 0;
                hlt_state = wait;
            }
            break;

        case heat_persist:
            if (persist == 0)
            {
                volume = 0;
                PORTB = 0;
                desiredTemp = 0;
                hlt_state = wait;
            }
            break;

        default:
            hlt_state = init;
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

void StartSecPulse(unsigned portBASE_TYPE Priority)
{
    xTaskCreate(HLT_Task, (signed portCHAR *)"HLT_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
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
    //RunSchedular
    vTaskStartScheduler();
    
    return 0;
}
