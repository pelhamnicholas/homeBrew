#ifndef SPI_H
#define SPI_H

#define SPI_DDR 	DDRB
#define SPI_PORT	PORTB
#define SS 			4
#define MOSI 		5
#define MISO 		6
#define SCK 		7

#define F_CPU 1000000UL
#include <util/delay.h>

#define EMPTY     0
#define NOT_EMPTY 1
#define FULL      2

/* Temp in Fahrenheit */
#define MAXTEMP 300
#define MINTEMP 0
#define TEMP(A) (A * (MAXTEMP - MINTEMP) + MINTEMP)

/* This is more accurate but requires floating point arithmetic
   If the thermistors were available a look up table would be more appropriate
double celcius(unsigned short ADC) { 
    double temp;

    temp = log(((1024/RawADC) - 1));
    temp = 1 / (0.001129148 + (0.000234125 + 
                (0.0000000876741 * Temp * Temp ))* Temp );
    temp = temp - 273.15;

    return temp;
}

double fahrenheit(unsigned short ADC) {
    return (celcius * 9.0)/ 5.0 + 32.0; 
}
*/

struct SPI_Data {
	unsigned char flag;
	unsigned short temp;
	signed short time;
	unsigned char vol; // 0 if empty, 1 if not empty, 2 if full
} receivedData, tmpData;
unsigned long dataSize = sizeof(struct SPI_Data);

//unsigned long recievedData, tmpData;
unsigned char * pData = (unsigned char *) &tmpData;
unsigned char byte = 0;

void SPI_MasterInit(void) {
	// Set DDR to have MOSI, SCK, and SS as output and MISO as input
	// Set SPCR register to enable SPO, enable master, and use SCK frequency
	//    of fosc/16 (pg. 168)
	/* Set MOSI and SCK output, all others input */
	SPI_DDR = (SPI_DDR & 0x0F) | (1<<MOSI) | (1<<SCK);
	SPI_PORT = (SPI_PORT & 0x0F) | ~((1<<MOSI) | (1<<SCK));
	/* Enable SPI, Master, set clock rate fck/16 */
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);//|(1<<SPIE);
	// Make sure global interrupts are enabled on SREG register (pg. 9)
	sei();
}

void SPI_SlaveInit(void)
{
    SPI_DDR = (SPI_DDR & 0x0F) | (1<<MISO);
	SPI_PORT = (SPI_PORT & 0x0F) | ~(1<<MISO);
    SPCR = (1<<SPIE)|(1<<SPE) ;
    sei();
}

/* Assumes slave select is dealt with elsewhere */
unsigned char SPI_Transmit(unsigned char cData) {

	/* data in SPDR will be transmitter, e.g. */
	SPDR = cData;
	while(!(SPSR & (1<<SPIF))) /* wait for transmission to complete */
		;
	
	return SPDR;
}

/* Assumes slave select is dealt with elsewhere */
unsigned long SPI_Transmit_Long(unsigned long lData) {
    unsigned char * cData;

    cData = &lData;
    for (i = 0; i < 4; i++) {
        SPI_Transmit(cData[i]);
    }

	return lData;
}

/* */
void SPI_Transmit_Data(struct SPI_Data sendData) {
	unsigned char i = 0;
	unsigned char * cData = (unsigned char *) &sendData;

	for (i = 0; i < sizeof(sendData); i++) {
		SPI_Transmit(cData[i]);
		_delay_ms(100);
	}
}

unsigned char SPI_Receive(void)
{
    while(!(SPSR & (1 << SPIF)));
    return SPDR;
}

void SPI_handleReceivedData(void);

ISR(SPI_STC_vect)
{
    pData[byte] = SPDR;
    //byte = (byte + 1) % 4; // for unsigned long data
	byte = (byte + 1) % sizeof(struct SPI_Data); // for struct data
	if (byte == 0) {
		receivedData = tmpData;
		SPI_handleReceivedData();
	}
}

#endif
