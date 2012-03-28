#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "rest.h"
#include <avr/sleep.h>
#include <avr/wdt.h>
#include "net/netstack.h"
#include "usart_as_spi.h"
#include "m25p80.h"
#include "crc16.h"
#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

PROCESS(coap_request, "CoAP request handler");
char temp[100];
process_event_t coap_msg_event;


#include "buffer.h"
#define SERVER_NODE(ipaddr)   uip_ip6addr(ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0x1)
#define LOCAL_PORT 61620
#define REMOTE_PORT 61616

char temp[100];
int xact_id;
static uip_ipaddr_t server_ipaddr;
static struct uip_udp_conn *client_conn;
#define MAX_PAYLOAD_LEN   100

/*
    static void
response_handler(coap_packet_t* response)
{
    uint16_t payload_len = 0;
    uint8_t* payload = NULL;
    payload_len = coap_get_payload(response, &payload);
    if(response->tid==xact_id-1){
        id_validated=1;
    }
    PRINTF("Response transaction id: %u\n", response->tid);
    if (payload) {
        memcpy(temp, payload, payload_len);
        temp[payload_len] = 0;
        PRINTF(" payload: %s\n", temp);
    }
}*/

    static void
send_data(coap_method_t method, char * uri, char * payload)
{
    char buf[MAX_PAYLOAD_LEN];
    if (init_buffer(COAP_DATA_BUFF_SIZE)) {
        int data_size = 0;
        //int service_id = random_rand() % NUMBER_OF_URLS;
        coap_packet_t* request = (coap_packet_t*)allocate_buffer(sizeof(coap_packet_t));
        init_packet(request);

        //coap_set_method(request, COAP_GET);
        coap_set_method(request, method);
        request->tid = ++xact_id;
        request->type = MESSAGE_TYPE_CON;
        coap_set_header_uri(request, /*service_urls[service_id]*/ uri);
        if(payload!=NULL) coap_set_payload(request,payload,strlen(payload));

        data_size = serialize_packet(request, buf);

        PRINTF("Client sending request to:[");
        PRINT6ADDR(&client_conn->ripaddr);
        PRINTF("]:%u/%s\n", (uint16_t)REMOTE_PORT, /*service_urls[service_id]*/ uri);
        uip_udp_packet_send(client_conn, buf, data_size);

        process_post_synch(&coap_request, coap_msg_event, NULL);
        delete_buffer();
    }
}

    static coap_packet_t *
handle_incoming_data()
{
    PRINTF("Incoming packet size: %u \n", (u16_t)uip_datalen());
    if (init_buffer(COAP_DATA_BUFF_SIZE)) {
        if (uip_newdata()) {
            coap_packet_t* response = (coap_packet_t*)allocate_buffer(sizeof(coap_packet_t));
            if (response) {
                parse_message(response, uip_appdata, uip_datalen());
                //response_handler(response);
                return response;
            }
        }
        //delete_buffer();
    }
    return 0;
}



//########################################


PROCESS(rest_server_example, "Rest Server Example");
AUTOSTART_PROCESSES(&rest_server_example, &coap_request);

PROCESS_THREAD(coap_request, ev, data)
{

    static struct etimer et;
    /*static coap_msg_t * msg;
    static char in_progress=0;
    static int id;*/
    PROCESS_BEGIN();
    coap_msg_event = process_alloc_event();
    etimer_set(&et, 2 * CLOCK_SECOND);
    etimer_stop(&et);

    SERVER_NODE(&server_ipaddr);
    client_conn = udp_new(&server_ipaddr, UIP_HTONS(REMOTE_PORT), NULL);
    udp_bind(client_conn, UIP_HTONS(LOCAL_PORT));

    PRINTF("Created a connection with the server ");
    PRINT6ADDR(&client_conn->ripaddr);
    PRINTF(" local/remote port %u/%u\n",
            UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));


    while(1)
    {
        PROCESS_WAIT_EVENT();
        if(ev == coap_msg_event)
        {
            etimer_restart(&et);
        }
        else if(ev == tcpip_event)
        {
            process_post(&rest_server_example, coap_msg_event, handle_incoming_data());
            etimer_stop(&et);

        }
        else if(etimer_expired(&et))
        {
            process_post(&rest_server_example, coap_msg_event, 0);
        }

    }

    PROCESS_END()

}

char atoh(uint8_t * buf, char * str)
{
    int i,j;
    char b;
    int len= strlen(str);
    //printf("atoh, len : %d ::",len);
    for(i=0; i<len; i++)
    {
        j=i/2;
        b=str[i];
        //printf("%c",b);
        buf[j]<<=4;
        if( (b>='0')&&(b<='9') )
        {
            buf[j]+=(b-'0');
        }
        else if( (b>='A')&&(b<='F') )
        {
            buf[j]+=(b-'A'+0xA);
        }
        else if( (b>='a')&&(b<='f') )
        {
            buf[j]+=(b-'a'+0xA);
        }

        else return 0xFF;
    }
    //printf("\n");
    return 0;
}
#include <avr/pgmspace.h>
PROCESS_THREAD(rest_server_example, ev, data)
{

    static uint32_t size;
    static uint16_t chksum;
    coap_packet_t * buf=NULL;
    int i;
    uint16_t j;
    static int s_i;
    uint16_t fvalue;
    static uint16_t lines;
    uint8_t data_temp[20]={0};
    uint8_t temp_b;
    PROCESS_BEGIN();

    DDRB|=0xE0;
    PORTB&=0x1F;
    DDRE|=0xC0;
    PORTE&=0x3F;

    static struct etimer et;
    static int retries=0;
    uint16_t payload_len = 0;
    uint8_t* payload = NULL;


    m25p80_init_cs();	
    usart_spi_init();

    PORTB|=(1<<7);
    etimer_set(&et, 1 * CLOCK_SECOND);
    do{
        PROCESS_YIELD();
    }while(!etimer_expired(&et));
    PORTB&=0x1F;
    PORTB|=(1<<6);
    etimer_set(&et, 1 * CLOCK_SECOND);
    do{
        PROCESS_YIELD();
    }while(!etimer_expired(&et));

    PORTB&=0x1F;
    PORTB|=(1<<5);
    etimer_set(&et, 1 * CLOCK_SECOND);
    do{
        PROCESS_YIELD();
    }while(!etimer_expired(&et));

    PORTB&=0x1F;
    etimer_set(&et, 10 * CLOCK_SECOND);
    etimer_stop(&et);
    i=m25p80_res();
    printf("signature: %2x\n", i);


    m25p80_wait_end_write();
    m25p80_be();
    m25p80_wait_end_write();
    /* new connection with server */
    // while(1)
    {
        //for(i=0;i<10;i++) printf("counter: %d\n",i);
        /*for(retries=0;(retries<3)&&(!buf);retries++)
          {

          printf("try n: %d\n",retries);
        //   etimer_set(&et, 2 * CLOCK_SECOND);

        send_data(METHOD_GET, "uptime", NULL);
        PROCESS_WAIT_EVENT_UNTIL(ev==coap_msg_event);
        buf=data;
        }
        if(!buf) printf("not ");
        printf("validated\n");
        etimer_restart(&et);
        do{
        PROCESS_YIELD();
        }while(!etimer_expired(&et));
         */
        for(retries=0;(retries<3)&&(!buf);retries++)
        {

            //printf("try n: %d\n",retries);
            //   etimer_set(&et, 2 * CLOCK_SECOND);

            send_data(METHOD_GET, "firmware?line=size", NULL);
            PROCESS_WAIT_EVENT_UNTIL(ev==coap_msg_event);
            buf=data;
            if(buf)
            {
                payload_len = coap_get_payload(buf, &payload);
                if (payload) {
                    memcpy(temp, payload, payload_len);
                    temp[payload_len] = 0;
                    size=atol(temp);                
                }
            }
        }
        buf=0;
        for(retries=0;(retries<3)&&(!buf);retries++)
        {

            //printf("try n: %d\n",retries);
            //   etimer_set(&et, 2 * CLOCK_SECOND);

            send_data(METHOD_GET, "firmware?line=checksum", NULL);
            PROCESS_WAIT_EVENT_UNTIL(ev==coap_msg_event);
            buf=data;
            if(buf)
            {
                payload_len = coap_get_payload(buf, &payload);
                if (payload) {
                    memcpy(temp, payload, payload_len);
                    temp[payload_len] = 0;
                    chksum=atoi(temp);                
                }
            }
        }

        printf("results: size=%ld, chksum=%4.x\n",size,chksum);

        lines = (size >> 4)+ ((size&0xF)?1:0);
        printf("lines: %d\n", lines);
        for(s_i=0; s_i< lines; s_i++)
        {
            buf=0;
            for(retries=0;(retries<3)&&(!buf);retries++)
            {

                //printf("try n: %d\n",retries);
                //   etimer_set(&et, 2 * CLOCK_SECOND);
                sprintf(data_temp, "firmware?line=%d",s_i);               
                send_data(METHOD_GET, data_temp, NULL);
                PROCESS_WAIT_EVENT_UNTIL(ev==coap_msg_event);
                buf=data;
                if(buf)
                {
                    payload_len = coap_get_payload(buf, &payload);
                    if (payload) {
                        memcpy(temp, payload, payload_len);
                        temp[payload_len] = 0;
                        memset(data_temp, 0, 20);
                        //printf("flbl: %s,%d\n",temp, strlen(temp));
                        atoh(data_temp, temp);

                       // for(i=0; i<(payload_len/2);i++) printf("%.2x",data_temp[i]);
                        //printf("\n");

                        m25p80_wait_end_write();
                        m25p80_pp(((uint32_t)s_i)<<4, payload_len/2, data_temp);
                    }
                }
            }

        }

        m25p80_wait_end_write();
        fvalue=0;
        for(s_i=0; s_i<(size>>4);s_i++)
        {
            //printf("s_i:%d --",s_i);
            m25p80_read(((uint32_t)s_i)<<4, 16, data_temp);
            /*for(i=0;i<16;i++)
            {
                //printf("%.2x",data_temp[i]);
                fvalue+=data_temp[i];
            }*/
            fvalue=crc16_data(data_temp,16,fvalue);
            //printf("\n");
        }

        m25p80_read((uint32_t)s_i<<4, size&0xF, data_temp);
        fvalue=crc16_data(data_temp, size&0xF, fvalue);
        //for(i=0;i<(size&0xF);i++) fvalue+=data_temp[i];
        printf("chsum: %4.x, read: %4.x\n", chksum, fvalue);
        fvalue=0;
        for(j=0; j<size;j++)
        {   
            temp_b=pgm_read_byte(j);
            m25p80_read(j, 1, data_temp);
            if(temp_b!= data_temp[0])
                printf("byte: %d, in int.flash: %2x, in ext.flash: %2x\n",j,temp_b,data_temp[0]);
            fvalue=crc16_add(temp_b,fvalue);
        }
        printf("crc in flash: %4.x\n", fvalue);
        /*etimer_restart(&et);
          do{
          PROCESS_YIELD();
          }while(!etimer_expired(&et));*/


    }
    PROCESS_END();
}


ISR(WDT_vect){
} 
