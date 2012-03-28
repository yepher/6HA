#include "contiki.h"
#include <avr/io.h>
#include "m25p80.h"
#include "usart_as_spi.h"

#define USART_dummy() USART_trx(0x00)
#define USART_read() USART_trx(0x00)

void m25p80_init_cs(void)
{
	M25P80_CS_OUTPUT();
	M25P80_CS_H();
}

void m25p80_wren(void)
{
	M25P80_CS_L();
	USART_trx(0x06);
	M25P80_CS_H();
}

void m25p80_wrdi(void)
{
	M25P80_CS_L();
	USART_trx(0x04);
	M25P80_CS_H();
}

void m25p80_dp(void)
{
	M25P80_CS_L();
	USART_trx(0xB9);
	M25P80_CS_H();
}

uint8_t m25p80_res(void)
{
	uint8_t temp;
	
	M25P80_CS_L();
	USART_trx(0xAB);
	USART_dummy();
	USART_dummy();
	USART_dummy();
	temp=USART_read();
	M25P80_CS_H();

	return temp;
}

void m25p80_be(void)
{
	m25p80_wren();

	M25P80_CS_L();
	USART_trx(0xC7);
	M25P80_CS_H();
}

uint8_t m25p80_rdsr(void)
{
	uint8_t temp;
	M25P80_CS_L();
	USART_trx(0x05);
	temp=USART_read();
	M25P80_CS_H();
	return temp;
}

void m25p80_wrsr(uint8_t data)
{
	m25p80_wren();

	M25P80_CS_L();
	USART_trx(0x01);
	USART_trx(data);
	M25P80_CS_H();
}

uint8_t m25p80_pp(uint32_t addr, uint16_t bytes, uint8_t * pdata)
{
	uint16_t i;
	/* Check address and number of bytes to write */
	if(addr>0xFFFFF) return 0xFF;
	if( (bytes==0)||(bytes>256)) return 0xFE;

	m25p80_wren();

	M25P80_CS_L();
	USART_trx(0x02);
	
	/* Write address */
	USART_trx( (addr>>16)&0xFF );
	USART_trx( (addr>>8)&0xFF );
	USART_trx( addr&0xFF );

	/* Write data */
	for(i=0; i<bytes; i++)
	{
		USART_trx(pdata[i]);
	}
	M25P80_CS_H();

	return 0;
}

uint8_t m25p80_read(uint32_t addr, uint16_t bytes, uint8_t  * pdata)
{
	uint16_t i;
	/* Check address */
	if(addr>0xFFFFF) return 0xFF;
	
	M25P80_CS_L();
	USART_trx(0x03);

	/* Write address */
	USART_trx( (addr>>16)&0xFF );
	USART_trx( (addr>>8)&0xFF );
	USART_trx( addr&0xFF );

	for(i=0; i<bytes; i++)
	{
		*pdata++ = USART_read();
	}
	M25P80_CS_H();

	return 0;
}

uint8_t m25p80_se(uint32_t addr)
{
	/* Check address */
	if(addr>0xFFFFF) return 0xFF;

	m25p80_wren();

	M25P80_CS_L();
	USART_trx(0xD8);
	
	/* Write address */
	USART_trx( (addr>>16)&0xFF );
	USART_trx( (addr>>8)&0xFF );
	USART_trx( addr&0xFF );

	M25P80_CS_H();

	return 0;
}

/*
uint8_t m25p80_WRITE( uint32_t addr, uint32_t bytes, uint8_t * pdata)
{
  uint32_t written_bytes = 0;
  uint8_t i;
  uint16_t temp;
  
  for(;bytes > 0;)
  {
    //temp = ~((address + written_bytes)&0xFF);
    temp = 256 - ((addr)&0xFF);
      
    if( bytes > (temp))
    {
       i=m25p80_pp(addr, temp, pdata);
       written_bytes+=temp;
       bytes-=temp;
    }
    else
    {
      i=m25p80_pp(addr, bytes, pdata);
      written_bytes+=bytes;
      bytes=0;
    }
    if(i!=0)
    {
	return i;
    }
    addr+=temp;
    pdata+=temp;
  }
  
  
  return written_bytes;
}
*/
