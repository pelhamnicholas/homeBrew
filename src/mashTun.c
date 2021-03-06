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

//extern unsigned long receivedData; // for long data
extern volatile struct SPI_Data receivedData; // for struct data
unsigned char volume; 

signed short mashTime = 0;
unsigned short temp = 0;             //current temperature
unsigned short desiredTemp = 0;      //desired HLT temperature - 0x00F8
signed char rotationcounter = 0;     //stepper motor rotation flag

// adc min value = 0x00B8

//lowest desired temp = 0x001F
//highest desired temp = 0x00FE
    
/******************************* FILL ************************************/

enum fillState { checkVol } fill_state;

void fill_init() {
    fill_state = checkVol;
}

void fill_tick() {
    switch(fill_state) {
        case checkVol:
            if (~PINA & 0x02) {
                volume = NOT_EMPTY;
				if (~PINA & 0x04) {
					volume = FULL;
				}
            }
            else if (!(~PINA & 0x02)) {
                volume = EMPTY;
			}
            else if (!(~PINA & 0x04)) {
                volume = NOT_EMPTY;
            }
            break; 
        default:
            break;
    }

    switch(fill_state) {
        case checkVol:
            break;
        default:
            fill_state = checkVol; 
            break;
    }
}

void fill_task() {
    fill_init();
    for(;;) {
        fill_tick();
        vTaskDelay(100);
    }
}

/****************************** STIRRER *********************************/

enum stirState { OFF, ON } stir_state;

void stir_init() {
    stir_state = OFF;
}

void stir_tick() {
    switch(stir_state) {
        case OFF:
            if (volume == NOT_EMPTY) {
                stir_state = ON;
            }
            break;
        case ON:
            if (volume == EMPTY) {
                stir_state = OFF;
            }
            break;
        default:
            break;
    }

    switch(stir_state) {
        case OFF:
            rotationcounter = 0;
            PORTB = PORTB & 0xF7; 
            break;
        case ON:
            rotationcounter = 100;
            PORTB = (PORTB & 0xF7) | 0x08;
            break;
        default:
            break;
    }
}

void stir_task() {
    stir_init();
    for(;;) {
        stir_tick();
        vTaskDelay(100);
    }
}

/****************************** MASH TUN *********************************/

const unsigned char MASH_TUN_PERIOD = 100;

enum mashTunState { WAIT, FILL, BELOW_TEMP, AT_TEMP, COOL, FINISHED } mashTun_state;

void mashTun_init() {
    mashTun_state = WAIT;
}

void mashTun_tick() {
    switch(mashTun_state) {
        case WAIT:
            temp = ADC_read(0);
			//PORTB = PORTB & 0xFE;
//             set_sleep_mode(SLEEP_MODE_EXT_STANDBY);
//             cli();
//             // if ( ... ) {
//                 sleep_enable();
//                 sei();
//                 sleep_cpu();
//                 sleep_disable();
//             // }
//             sei();
            break;
        case FILL:
            temp = ADC_read(0);
			//portb = portb & 0xfe;
            break;
        case AT_TEMP:
            temp = ADC_read(0);
            /* spi transfer don't heat water */
            //portb = portb & 0xfe;
            mashTime = mashTime - MASH_TUN_PERIOD;
            break;
        case BELOW_TEMP:
            temp = ADC_read(0);
            /* spi transfer heat water */
            //portb = portb | 0x01;
            mashTime = mashTime - MASH_TUN_PERIOD;
            break;
        case FINISHED:
            /* wait for master to finish sparge */
            desiredTemp = 0;
            break;
        default:
            break;
    }

    switch(mashTun_state) {
        case WAIT:
            if (desiredTemp > 0 && mashTime > 0) {
                mashTun_state = FILL;
            }
            break;
        case FILL:
            if (volume == FULL) {
                mashTun_state = AT_TEMP;
            }
            break;
        case AT_TEMP:
            if (mashTime <= 0) {
                mashTun_state = FINISHED;
            } else if (temp < desiredTemp) {
                mashTun_state = BELOW_TEMP;
            }
            break;
        case BELOW_TEMP:
            if (mashTime <= 0) {
                mashTun_state = FINISHED;
            } else if (temp >= desiredTemp) {
                mashTun_state = AT_TEMP;
            }
            break;
        case FINISHED:
            /* wait for ping to move from finished? */
            mashTun_state = WAIT;
            break;
        default:
            break;
    }
}

void mashTun_task() {
    mashTun_init();
    for(;;) {
        mashTun_tick();
        vTaskDelay(MASH_TUN_PERIOD);
    }
}
/******************************* HLT TASK *******************************/

/******************************* MOTOR TASK *******************************/
enum MOTORState {INIT, MOTOROFF, A, AB, B, BC, C, CD, D, DA} motor_state;

void Motor_Init(){
    motor_state = INIT;
}

void Motor_Tick(){
    //Actions
    switch(motor_state){
        case INIT:
        PORTC = 0;
        break;
        
        case MOTOROFF:
        PORTC = 0;
        break;
        
        case A:
        PORTC = 1;
        break;
        
        case AB:
        PORTC = 3;
        break;
        
        case B:
        PORTC = 2;
        break;
        
        case BC:
        PORTC = 6;
        break;
        
        case C:
        PORTC = 4;
        break;
        
        case CD:
        PORTC = 12;
        break;
        
        case D:
        PORTC = 8;
        break;
        
        case DA:
        PORTC = 9;
        break;
        
        default:
        PORTC = 0;
        break;
    }

    //Transitions
    switch(motor_state){
        case INIT:
        motor_state = A;
        break;
        
        case MOTOROFF:
        if (rotationcounter > 0) motor_state = A;
        else motor_state = OFF;
        break;
        
        case A:
        if (rotationcounter > 0) motor_state = AB;
        else motor_state = A;
        break;
        
        case AB:
        if (rotationcounter > 0) motor_state = B;
        else motor_state = AB;
        break;
        
        case B:
        if (rotationcounter > 0) motor_state = BC;
        else motor_state = B;
        break;
        
        case BC:
        if (rotationcounter > 0) motor_state = C;
        else motor_state = BC;
        break;
        
        case C:
        if (rotationcounter > 0) motor_state = CD;
        else motor_state = C;
        break;
        
        case CD:
        if (rotationcounter > 0) motor_state = D;
        else motor_state = CD;
        break;
        
        case D:
        if (rotationcounter > 0) motor_state = DA;
        else motor_state = D;
        break;
        
        case DA:
        if (rotationcounter > 0) motor_state = A;
        else motor_state = DA;
        break;
        
        default:
        motor_state = INIT;
        break;
    }
}

void MotorTask()
{
    Motor_Init();
    for(;;)
    {
        Motor_Tick();
        vTaskDelay(1);
    }
}

/******************************* TEST TASK *******************************/
enum TESTstates {D_INIT, get_input} TESTstates;
unsigned short adctemp = 0;


void TEST_Init(){
    TESTstates = D_INIT;
}

void TEST_Tick()
{
    //actions
    switch(TESTstates){
        case D_INIT:
            break; 

        case get_input:
            //PORTD = (adctemp & 0x0300) >> 2;
            PORTD = (char) (desiredTemp & 0x00FF);
            break; 

        default:
            break;
    }
    //transitions
    switch(TESTstates){
        case D_INIT:
            TESTstates = get_input;
            break;

        case get_input:
            break;

        default:
            TESTstates = D_INIT;
            break;
    }
}

void TESTtask()
{
    TEST_Init();
    for(;;)
    {
        TEST_Tick();
        vTaskDelay(100);
    }
}

/******************************* TEST TASK *******************************/


/******************************* OUTPUT TASK *******************************/
enum outputStates {output_Init, output} output_state;

void Output_Init()
{
    output_state = output_Init;
}

void Output_Tick()
{
    //actions
    switch(output_state) {
        case output_Init:
            PORTB = PORTB & 0x00; 
            break;

        case output:
            if (volume == EMPTY) {
                PORTB = PORTB & 0xFC;
            }
            else if (volume == NOT_EMPTY) {
                PORTB = (PORTB & 0xF9) | 0x02;
            }
            else if (volume == FULL) {
                PORTB = (PORTB & 0xF9) | 0x04;
            }
            if (mashTun_state == FILL) {
                PORTB = PORTB & 0xFE;
            }
            else if (mashTun_state == AT_TEMP) {
                PORTB = PORTB & 0xFE;
            }
            else if (mashTun_state == BELOW_TEMP) {
                PORTB = PORTB | 0x01;
            }
            else if (mashTun_state == FINISHED) {
                PORTB = PORTB & 0xFE;
            }
            break;

        default:
            break;
    }
    //transitions
    switch(output_state) {
        case output_Init:
            output_state = output;
            break;

        case output:
            break;

        default:
            output_state = output;
            break;
    }
}

void OutputTask()
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
        sendData.flag = mashTun_state;
        sendData.time = mashTime;
        sendData.temp = temp;
        sendData.vol = volume;
        SPI_Transmit_Data(sendData);
        } else {
        mashTime = receivedData.time;
        desiredTemp = receivedData.temp;
    }
}

void StartSecPulse(unsigned portBASE_TYPE Priority)
{
    xTaskCreate(fill_task, (signed portCHAR *)"fill_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
    xTaskCreate(stir_task, (signed portCHAR *)"stir_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
    xTaskCreate(mashTun_task, (signed portCHAR *)"mashTun_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
    xTaskCreate(TESTtask, (signed portCHAR *)"TEST_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
    xTaskCreate(MotorTask, (signed portCHAR *)"MotorTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
    xTaskCreate(OutputTask, (signed portCHAR *)"OutputTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

int main(void)
{
    //DDR and Port settings:
    //DDR:  0x00 = input, 0xFF = output
    //PORT: 0xFF = input, 0x00 = output
    DDRA = 0x00; PORTA = 0xFF;
    DDRB = 0x0F; PORTB = 0xF0;
    DDRC = 0xFF; PORTB = 0x00;
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
