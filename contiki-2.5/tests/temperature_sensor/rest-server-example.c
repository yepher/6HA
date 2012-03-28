#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "rest.h"
#include <avr/sleep.h>
#include <avr/wdt.h>
#include "net/netstack.h"

#define DEBUG 1
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

char temp[100];


//#########################################

#include "i2cm.h"
#include <compat/twi.h>
//#define DEBUG DEBUG_PRINT
//#include "net/uip-debug.h"

// Initialize
void i2cm_init(void)
{

	TWBR = I2C_BITRATE;
	TWCR = (1<<TWEN) | (1<<TWINT);

	if(I2C_PRESCALER & 1)
		TWSR|=1<<TWPS0;
	if(I2C_PRESCALER & 2)
		TWSR|=1<<TWPS1;
}


// Send a n-byte data frame to an address
void i2cm_send(uint8_t slave_addr, uint8_t n, const uint8_t* data)
{
	int i;

	I2C_START();

	// Slave address + Write bit (0)
	I2C_SEND(slave_addr<<1);

	// Data
	for( i=0; i<n; i++ )
	{
		I2C_SEND(data[i]);
	}

	// Stop and wait
	I2C_STOP();
	//wait_4cyc(100);
}

void i2c_lm75_shutdown()
{
	I2C_START();

	// Slave address + Write bit (0)
	I2C_SEND(0x48<<1);
	I2C_SEND(0x01); //pointer to conf register
	
	I2C_START();//it's a repeated start !!!!
	I2C_SEND(0x48<<1);
	I2C_SEND(0x01); //shutdown !
	I2C_STOP();

}

void i2c_lm75_powerup()
{
	I2C_START();

	// Slave address + Write bit (0)
	I2C_SEND(0x48<<1);
	I2C_SEND(0x01); //pointer to conf register
	
	I2C_START();//it's a repeated start !!!!
	I2C_SEND(0x48<<1);
	I2C_SEND(0x00); //power up !
	I2C_STOP();

}

// Receive a frame from an address
// Slave must respect the i2c_ryder protocol : first byte tells if there are
// data to transmit or not
// Return number of read bytes
uint8_t i2cm_rcv(uint8_t slave_addr, uint8_t n, uint8_t* data)
{
	int i;

	I2C_START();

	// Slave address + Read bit (1)
	I2C_SEND((slave_addr<<1)+1);

	// No ACK, no data
	if( TW_STATUS != TW_MR_SLA_ACK )
	{
		I2C_STOP();
		//wait_4cyc(100);
		return -1;
	}

	// Read all data
	for( i=0; i<n; i++ )
	{
		I2C_ACK();
		data[i] = TWDR;
	}

	I2C_NACK();

	I2C_STOP();
	//wait_4cyc(100);

	return 1;
}


// Receive one data byte from an address, -1 on error
int i2cm_rcv_single(uint8_t slave_addr)
{
	I2C_START();

	// Slave address + Read bit (1)
	I2C_SEND((slave_addr<<1)+1);

	// No ACK, no data
	if( TW_STATUS != TW_MR_SLA_ACK )
	{
		printf("no ack\n");
		I2C_STOP();
		//wait_4cyc(100);
		return -1;
	}

	I2C_ACK();
	I2C_STOP();
	return TWDR;
}


#include "buffer.h"
#define SERVER_NODE(ipaddr)   uip_ip6addr(ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0x1)
#define LOCAL_PORT 61620
#define REMOTE_PORT 61616

char temp[100];
int xact_id;
int id_validated;
static uip_ipaddr_t server_ipaddr;
static struct uip_udp_conn *client_conn;
#define MAX_PAYLOAD_LEN   100

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
}

	static void
send_data(void)
{
	char buf[MAX_PAYLOAD_LEN];
	char buf2[15]={0};
	uint8_t data[2]={0};
	if (init_buffer(COAP_DATA_BUFF_SIZE)) {
		int data_size = 0;
		//int service_id = random_rand() % NUMBER_OF_URLS;
		coap_packet_t* request = (coap_packet_t*)allocate_buffer(sizeof(coap_packet_t));
		init_packet(request);

		//coap_set_method(request, COAP_GET);
		coap_set_method(request, COAP_POST);
		request->tid = xact_id++;
		request->type = MESSAGE_TYPE_CON;
		coap_set_header_uri(request, /*service_urls[service_id]*/ "temperature");


		i2cm_rcv(0x48, 2, data);
		data[1]&=0x80;
		sprintf(buf2, "value=%2d.%d",data[0],(data[1])?5:0);
		printf(" (msg: %s)\n", buf2);

		coap_set_payload(request,buf2,10);
		data_size = serialize_packet(request, buf);

		PRINTF("Client sending request to:[");
		PRINT6ADDR(&client_conn->ripaddr);
		PRINTF("]:%u/%s\n", (uint16_t)REMOTE_PORT, /*service_urls[service_id]*/ "temperature");
		uip_udp_packet_send(client_conn, buf, data_size);

		delete_buffer();
	}
}

	static void
handle_incoming_data()
{
	PRINTF("Incoming packet size: %u \n", (u16_t)uip_datalen());
	if (init_buffer(COAP_DATA_BUFF_SIZE)) {
		if (uip_newdata()) {
			coap_packet_t* response = (coap_packet_t*)allocate_buffer(sizeof(coap_packet_t));
			if (response) {
				parse_message(response, uip_appdata, uip_datalen());
				response_handler(response);
			}
		}
		delete_buffer();
	}
}



//########################################



PROCESS(rest_server_example, "Rest Server Example");
AUTOSTART_PROCESSES(&rest_server_example);

PROCESS_THREAD(rest_server_example, ev, data)
{
	int i;
	PROCESS_BEGIN();
	/*
#ifdef WITH_COAP
PRINTF("COAP Server\n");
#else
PRINTF("HTTP Server\n");
#endif

rest_init();*/
	DDRB|=0xE0;
	PORTB&=0x1F;
	DDRE|=0xC0;
	PORTE&=0x3F;

	static struct etimer et;
	static int retries=0;

	i2cm_init();
	SERVER_NODE(&server_ipaddr);

	PRINTF("Unpowering unnecessary modules (ADCs, USARTs, etc...)...\n");
	PRR0|=(1<<PRUSART0); //M25P80 USART
	PRR0|=(1<<PRTIM2)|(1<<PRADC);
	PRR1|=(1<<PRTIM5)|(1<<PRTIM4)|(1<<PRTIM3)|(1<<PRUSART3)|(1<<PRUSART2);


	PRINTF("Waiting 20 seconds to start...\n");
	
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
	etimer_set(&et, 17 * CLOCK_SECOND);
	do{
		PROCESS_YIELD();
	}while(!etimer_expired(&et));

	


	/*PRINTF("Unpowering JTAG.\n");
	MCUCR=(1<<JTD);
	MCUCR=(1<<JTD);*/

	/* new connection with server */
	client_conn = udp_new(&server_ipaddr, UIP_HTONS(REMOTE_PORT), NULL);
	udp_bind(client_conn, UIP_HTONS(LOCAL_PORT));

	PRINTF("Created a connection with the server ");
	PRINT6ADDR(&client_conn->ripaddr);
	PRINTF(" local/remote port %u/%u\n",
			UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

	//init WD, prescaler 8s
	WDTCSR|=(1<<WDCE)|(1<<WDE);
	WDTCSR=(0x31);
	_WD_CONTROL_REG = _BV(WDIE);
	
	while(1){
		id_validated=0;
		etimer_set(&et, 2 * CLOCK_SECOND);
		
		//for(i=0;i<10;i++) printf("counter: %d\n",i);
		for(retries=0;(retries<3)&&(!id_validated);retries++)
		{
			printf("try n: %d\n",retries);
			etimer_set(&et, 2 * CLOCK_SECOND);
			send_data();

			do{
				PROCESS_YIELD();
				if(ev==tcpip_event){
					handle_incoming_data();
				}
			}while(!etimer_expired(&et) && !id_validated);

		}
		if(!id_validated) printf("not ");
		printf("validated\n");

		//####
		NETSTACK_RADIO.off();
		i2c_lm75_shutdown();
		PRR0|=(1<<PRTIM0)|(1<<PRTIM1)|(1<<PRTWI);
		
		for(i=0;i<37;i++)
		{
			set_sleep_mode(SLEEP_MODE_PWR_DOWN); 
			sleep_mode();
		}
		PRR0&=~((1<<PRTIM0)|(1<<PRTIM1)|(1<<PRTWI));
		
		i2cm_init();
		i2c_lm75_powerup();
		NETSTACK_RADIO.on();

	}
	PROCESS_END();
}


ISR(WDT_vect){
} 
