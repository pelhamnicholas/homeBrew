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

/******************************* BK TASK *******************************/
unsigned char volume = 0;             //volume of water in BK
unsigned short maxVol = 1;            //maxvolume of BK
unsigned short temp = 0;              //current temperature
unsigned short desiredtemp = 0x00F8;  //desired BK temperature - 0x00F8
unsigned short boilTime = 0x00;

enum BKState {init, wait, fill, heat, cool, finish} BK_state;

void BK_Init(){
    BK_state = init;
}

void BK_Tick(){
    //Actions
    switch(BK_state){
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
                    boilTime = (unsigned short) (receivedData & 0x0000FFFF);
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
            BKtemp = ADC_read(0);
            BKvolume++;
            break;

        case heat:
            if (BKtemp >= BKdesiredtemp) {
                PORTB = 0x00;
            } else {
                PORTB = 0x01;
            }
            Boiltime--;
            BKtemp = ADC_read(0);
            break;

        case cool:
            PORTB = 0x00;
            PORTC = 0x01;
            BKtemp = ADC_read(0);
            break; 
           
        case finish:
        //signal back to LCD screen to say it's done
            break;

        default:
            PORTA = 0;
            break;
    }
    
    //Transitions
    switch(BK_state){
        case init: 
            BK_state = wait; 
            break;

        case wait:
            if (desiredTemp > 0) BK_state = fill; 
            break;

        case fill:
            if (volume >= maxVol)
            {
                PORTB = 0x00;
                boilTime = 150;
                BK_state = heat;
            } 
            break;

        case heat:
            if (boilTime == 0) 
            {
                volume = 0;
                PORTB = 0;
                desiredTemp = 0x005A;
                BK_state = cool;
            }
            break;

        case cool:
            if (temp <= desiredTemp)
            {
                volume = 0;
                PORTC = 0;
                desiredTemp = 0;
                BK_state = finish;
            }
            break;
            
        case finish:
            //signal from LCD screen sends back to wait
            BK_state = wait;
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
    //RunSchedular
    vTaskStartScheduler();
    
    return 0;
}
