#include <avr/io.h>
#include <avr/interrupt.h>

#include "light_ws2812.h"

#define LEDS 150
#define PACKET_SZ ( (LEDS * 3) + 3 )

volatile unsigned char serial_buffer[PACKET_SZ];
volatile unsigned int head = 0;
volatile unsigned int start;
volatile unsigned int checksum_1;
volatile unsigned int checksum_0;

struct cRGB led;

//USART Receive interrupt pushes the incoming byte into the buffer
ISR(USART_RX_vect)
{
    serial_buffer[head] = UDR0;

    if( head >= (PACKET_SZ - 1) )
    {
      start = 0;
      checksum_1 = head;
      checksum_0 = head - 1;
      head = 0;
    }
    else
    {
      start = head + 1;
      checksum_1 = head;
      if( head == 0 )
      {
        checksum_0 = PACKET_SZ - 1;
      }
      else
      {
        checksum_0 = head - 1;
      }
      head++;
    }
}

void serial_init(unsigned int baud)
{
	//Set baud rate
	UBRR0H = (unsigned char) (baud >> 8);
	UBRR0L = (unsigned char) (baud);

	//Set frame format: 8 data, no parity, 2 stop bits
	UCSR0C = (0<<UMSEL00) | (0<<UPM00) | (0<<USBS0) | (3<<UCSZ00);

	//Enable receiver and transmitter
	UCSR0B = (1<<RXCIE0 | 1<<RXEN0) | (1<<TXEN0);
}

int main()
{
	int i;

    serial_init( 3 );

	for( i = 0; i < LEDS; i++ )
	{
		led.r = 0;
		led.g = 0;
		led.b = 0;
		ws2812_sendarray((uint8_t *)&led, 3);
	}
	
	sei();

    while(1)
	{
	if( serial_buffer[start] == 0xAA )
	    {
	      unsigned short sum = 0;

	      for( int i = 0; i < checksum_0; i++ )
	      {
	        sum += serial_buffer[i];
	      }

	      if( start > 0 )
	      {
	        for( int i = start; i < PACKET_SZ; i++ )
	        {
	          sum += serial_buffer[i];
	        }
	      }

		  serial_buffer[start] = 0;
      
	      //Test if valid write packet
	      if( ( ( (unsigned short)serial_buffer[checksum_0] << 8 ) | serial_buffer[checksum_1] ) == sum )
	      {
		  	cli();
	        for( int i = 0; i < LEDS; i++ )
	        {
	          int idx = start + 1 + ( 3 * i );
  
	          if( idx >= (PACKET_SZ - 1) )
	          {
	            idx = idx - PACKET_SZ;
	          }
			  led.r = serial_buffer[idx];
			  led.g = serial_buffer[idx+1];
			  led.b = serial_buffer[idx+2];
			  ws2812_sendarray((uint8_t *)&led, 3);
	        }
	      }

		  sei();
	    }
	}
}
