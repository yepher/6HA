all: memory_test
APPS=ping6

UIP_CONF_IPV6=1
WITH_COAP = 1
CFLAGS += -DWITH_COAP
APPS += rest-coap

PROJECT_SOURCEFILES += usart_as_spi.c m25p80.c

CONTIKI = ../..
include $(CONTIKI)/Makefile.include
