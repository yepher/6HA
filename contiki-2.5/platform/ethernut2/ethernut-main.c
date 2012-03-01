
/* Copyright (c) 2005, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the Contiki OS
 *
 * $Id: ethernut-main.c,v 1.4 2010/12/03 21:39:33 dak664 Exp $
 *
 */

#include "contiki.h"
#include "contiki-net.h"
#include "dev/serial-line.h"
#include "dev/slip.h"
#include "dev/rs232.h"

#include <avr/interrupt.h>
/*static void setup_xram(void) __attribute__ ((naked)) \
     __attribute__ ((section (".init1")));

static void
setup_xram(void)
{
  outp(BV(SRE) | BV(SRW), MCUCR);
}*/

static struct uip_fw_netif slipif =
  {UIP_FW_NETIF(0,0,0,0, 0,0,0,0, slip_send)};

PROCESS(serial_test, "Serial test");

PROCESS_THREAD(serial_test, ev, data)
{
  PROCESS_BEGIN();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
    rs232_print(RS232_PORT_0, data);
  }
  PROCESS_END();
}

PROCINIT(&etimer_process, &serial_line_process, &slip_process,
	 &uip_fw_process);

int
main(void)
{
  uip_ipaddr_t addr;

  clock_init();
  rs232_init(RS232_PORT_0, USART_BAUD_57600,USART_PARITY_NONE | USART_STOP_BITS_1 | USART_DATA_BITS_8);
  rs232_set_input(RS232_PORT_0, slip_input_byte);

  sei();

  /* Initialize drivers and event kernal */
  process_init();

  uip_ipaddr(&addr, 172,16,0,2);
  uip_sethostaddr(&addr);

  procinit_init();
  autostart_start(autostart_processes);
  uip_fw_default(&slipif);

  rs232_print_p(RS232_PORT_0, PSTR("Initialized\n"));

  while(1) {
    process_run();
  }

  return 0;
}
