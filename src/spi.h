#ifndef SPI_H
#define SPI_H

unsigned long recievedData;
unsigned char * cData = &receivedData;

void SPI_MasterInit(void) {
	// Set DDR to have MOSI, SCK, and SS as output and MISO as input
	// Set SPCR register to enable SPO, enable master, and use SCK frequency
	//    of fosc/16 (pg. 168)
	/* Set MOSI and SCK output, all others input */
	SPI_DDR = (1<<MOSI) | (1<<SCK);
	SPI_PORT = ~((1<<MOSI) | (1<<SCK));
	/* Enable SPI, Master, set clock rate fck/16 */
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);//|(1<<SPIE);
	// Make sure global interrupts are enabled on SREG register (pg. 9)
	sei();
}

void SPI_SlaveInit(void)
{
    DDRB = (1<<6);
    SPCR = (1<<SPIE) | (1<<SPE) ;
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
unsigned short SPI_Transmit_Short(unsigned short sData) {
	unsigned char cData;

	cData = (unsigned char) (sData >> 8);
	SPI_Transmit(cData);
	cData = (unsigned char) (sData & 0x00FF);
	SPI_Transmit(cData);

	return sData;
}

/* Assumes slave select is dealt with elsewhere */
unsigned long SPI_Transmit_Long(unsigned long lData) {
	//unsigned short sData;

	//sData = (unsigned short) (lData >> 16);
	//SPI_Transmit_Short(sData);
	//sData = (unsigned short) (lData & 0x0000FFFF);
	//SPI_Transmit_Short(sData);

    unsigned char * cData;

    cData = &lData;
    for (i = 0; i < 4; i++) {
        SPI_Transmit(cData++);
    }

	return lData;
}

unsigned char SPI_Receive(void)
{
    while(!(SPSR & (1 << SPIF)));
    return SPDR;
}

unsigned short SPI_Recieve_Short(void) {
	unsigned short sData;

	sData = ((unsigned short) SPI_Receive()) << 8;
	sData = sData + SPI_Receive();

	return sData;
}

unsigned long SPI_Receive_Long(void) {
	unsigned long lData;

	lData = ((unsigned long) SPI_Receive_Short()) << 16;
	lData = lData + SPI_Receive_Short();

	return lData;
}

ISR(SPI_STC_vect)
{
    n = (n + 1) % 4;
    *(cData + n) = SPI_Receive();
    //receivedData = SPI_Receive_Long();
}

#endif
