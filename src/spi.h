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

struct SPI_Data {
	unsigned char flag;
	unsigned short temp;
	signed short time;
	unsigned char vol; // 0 if empty, 1 if not empty, 2 if full
	unsigned char unused; // stores the last byte of data sent by the master
}; 
struct SPI_Data receivedData;
struct SPI_Data tmpData;
struct SPI_Data sendData;

volatile unsigned char * pData = (unsigned char *) &tmpData;
volatile unsigned char * pSendData = (unsigned char *) &sendData;
volatile unsigned char byte = 0;

void SPI_SlaveInit(void)
{
    SPI_DDR = (SPI_DDR & 0x0F) | (1<<MISO);
	SPI_PORT = (SPI_PORT & 0x0F) | ~(1<<MISO);
    SPCR = (1<<SPIE)|(1<<SPE) ;
    sei();
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
	SPDR = pSendData[byte];
	byte = (byte + 1) % sizeof(struct SPI_Data); // for struct data
	if (byte == 0) {
		receivedData = tmpData;
		SPI_handleReceivedData();
	}
}

#endif
