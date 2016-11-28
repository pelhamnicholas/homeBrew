#ifndef ADC_H
#define ADC_H

void transmit_data(unsigned short data)
{
    unsigned char i = 0x00;
    PORTC = 0x08;
    for(i = 0; i < 16; i++)
    {
        PORTC = 0x08;
        PORTC |= ((data & 0x8000) >> 15);
        PORTC |= 0x04;
        data = data << 1;
    }
    PORTC |= 0x02;
    PORTC = 0x00;
    return;
}

void A2D_init() {
    // ADEN:  Enables analog-to-digital conversion
    // ADSC:  Starts analog-to-digital conversion
    // ADATE: Enables auto-triggering, allowing for constant analog to digital
    //        conversions.
    ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
}

// Pins on PORTA are used as input for A2D conversion
//    The default channel is 0 (PA0)
// The value of pinNum determines the pin on PORTA used for A2D conversion
// Valid values range between 0 and 7, where the value represents the desired 
//    pin for A2D conversion.
void Set_A2D_Pin(unsigned char pinNum) {
    ADMUX = (pinNum <= 0x07) ? pinNum : ADMUX;
    // Allow channel to stabilize
    static unsigned char i = 0;
    for (i = 0; i < 15; ++i) { asm("nop"); }
}


void ADC_init(void) {
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t ADC_read (uint8_t channel) {
    channel &= 0x07;
    ADMUX = (ADMUX & 0xF8) | channel;
    
    ADCSRA |= (1 << ADSC); 
    
    while(!(ADCSRA & (1 << ADIF)));
    
    ADCSRA |= (1 << ADIF);
    return(ADC);
}

#endif ADC_H