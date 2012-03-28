#include "contiki.h"
#include <avr/io.h>
#include "usart_as_spi.h"

//#define UCPHA0 UCSZ00

void usart_spi_init(void)
{
	UBRR0 = 0;
	DDRE |= (1<<2);
	//UCSR0C = (1<<UMSEL01)|(1<<UMSEL00)|(0<<UCPHA0)|(0<<UCPOL0);
	UCSR0C = 0xC3;
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	UBRR0 = 2;
}

uint8_t USART_trx( uint8_t data )
{
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) );
	/* Put data into buffer, sends the data */
	UDR0 = data;
	/* Wait for data to be received */
	while ( !(UCSR0A & (1<<RXC0)) );
	/* Get and return received data from buffer */
	return UDR0;
}

