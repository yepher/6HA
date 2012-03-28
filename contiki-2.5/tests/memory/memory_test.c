#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
//#include "i2cm.h"
//#include "ping6.h"
#include <stdio.h>
#include <string.h>
#include "usart_as_spi.h"
#include "m25p80.h"

//#define LED_INTERVAL 1*CLOCK_SECOND
#define SEND_INTERVAL 2*CLOCK_SECOND
//#include "i2cm.h"
//#include <compat/twi.h>
#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"
#define MAX_PAYLOAD_LEN		40
/*
   typedef struct memory_write_tag
   {

   uint32_t addr;
   uint32_t len;
   uint8_t data[256];
   } memory_write_t;

   memory_write_t wdata;
 */
PROCESS(write_process, "write process");
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
//AUTOSTART_PROCESSES(&ping6_process, &hello_world_process);
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(hello_world_process, ev, data)
{
    static struct etimer et;
    int temp, i;
    //uint8_t * pdata;
    uint8_t data_s[20];
    PROCESS_BEGIN();
    /*DDRE|=(1<<3);
      PORTE|=(1<<3);*/
    m25p80_init_cs();	

    DDRB|=(1<<7)|(1<<5)|(1<<6);
    PORTB&=0x7F;
    //PORTB|=(1<<6);
    usart_spi_init();

    printf("Hello, world\n");

    /*m25p80_pp(0, 18, "written in memory");
      m25p80_wait_end_write();
      m25p80_pp(0x1000, 11, "tagada !!!");*/
    /*wdata.addr=0x0010;
      wdata.len=256;
      for(temp=0;temp<256;temp++)
      {
      wdata.data[temp]='a'+(temp%26);
      }
      pdata = &wdata;
      for(i=0;i<16;i++)
      {
      printf("pdata: %.8x -- i=%d -- value:%.2x\n",pdata, i, pdata[i]);
      }
      printf("bulk erase\n");
      m25p80_be();
      printf("wdata addr: %8.x\n", (uint32_t)&wdata);*/
    //process_start(&write_process,(void *)&wdata);
    process_start(&write_process,NULL);
    printf("I'm here !!!\n");
    PROCESS_WAIT_WHILE( process_is_running( &write_process ));

    //PROCESS_YIELD();
    printf("data written\n");

    //PROCESS_WAIT_WHILE(!m25p80_WIP_read());
    //m25p80_read(78,10,data_s);
    //printf("%10s\n",data_s);

    etimer_set(&et, SEND_INTERVAL);
    while(1)
    {
        PORTB^=(1<<6);
        PROCESS_YIELD();
        if(etimer_expired(&et))
        {
            PORTB^=(1<<5);
            /*PORTE&=(~(1<<3));
              USART_trx(0xAB);
              USART_trx(0);
              USART_trx(0);
              USART_trx(0);
              temp=USART_trx(0);
              printf("signature: %2x\n", temp);
              PORTE|=(1<<3);*/
            temp=m25p80_res();
            printf("signature: %2x\n", temp);
            if (temp ==0x13)
            {
                PORTB^=(1<<7);
            }
            /*if( !m25p80_WIP_read())
              {
              m25p80_read(0, 18, data_s);
              printf("%s\n", data_s);
              m25p80_read(0x1000, 11, data_s);
              printf("%s\n", data_s);

              }*/
            etimer_restart(&et);
        }
    }

    PROCESS_END();
}

PROCESS_THREAD(write_process, ev, data)
{
    PROCESS_BEGIN();

    /*printf("wdata addr: %8.x\n", (uint32_t)data);
      addr=(uint32_t)wdata.addr;
      len=(uint32_t)wdata.len;
      pdata=&(wdata.data[0]);
      printf("&len:%.8x  pdata:%.8x\n", &(wdata.len), pdata);
      printf("write begin addr: %.8x\n    len: %.8x\n    pdata[0]:%d\n",addr,len,pdata[0]);
      while( len > 0 )
      {
      printf("process yield\n");

      do
      {
      PROCESS_YIELD();	
      }while( m25p80_WIP_read() );
      printf("process unyield\n");
      temp = 256 - (addr&0xFF);
      if( len > (temp))
      {
      m25p80_pp(addr, temp, pdata);
      len-=temp;
      PROCESS_YIELD();
      }
      else
      {
      m25p80_pp(addr, len, pdata);
      len=0;
      }
      addr+=temp;
      pdata+=temp;

      }*/
    printf("write_process quit\n");
    PROCESS_END();

}
/*---------------------------------------------------------------------------*/
