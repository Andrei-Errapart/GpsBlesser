#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include "usart.h"

#define NUMBER_OF_UARTS 4

static uint16_t rx_head[NUMBER_OF_UARTS], rx_tail[NUMBER_OF_UARTS];
static uint16_t tx_head[NUMBER_OF_UARTS], tx_tail[NUMBER_OF_UARTS];
static volatile uint16_t rx_count[NUMBER_OF_UARTS];
static volatile uint16_t tx_count[NUMBER_OF_UARTS];
uint8_t uart0_rx_buffer[UART0_RX_BUFFER_SIZE];
uint8_t uart0_tx_buffer[UART0_TX_BUFFER_SIZE];
uint8_t uart1_rx_buffer[UART1_RX_BUFFER_SIZE];
uint8_t uart1_tx_buffer[UART1_TX_BUFFER_SIZE];
uint8_t uart2_rx_buffer[UART2_RX_BUFFER_SIZE];
uint8_t uart2_tx_buffer[UART2_TX_BUFFER_SIZE];
uint8_t uart3_rx_buffer[UART3_RX_BUFFER_SIZE];
uint8_t uart3_tx_buffer[UART3_TX_BUFFER_SIZE];


/*****************************************************/
void uart_Init(void)
/*****************************************************/
{
	int i;

	for(i = 0; i < NUMBER_OF_UARTS; i++)
	{
		rx_head[i] = rx_tail[i] = 0;
		tx_head[i] = tx_tail[i] = 0;
		rx_count[i] = 0;
		tx_count[i] = 0;
	}	

	UBRR0 = UBRR0_RELOAD;
	UCSR0A = 0; //_BV(U2X0);
	UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0) | _BV(TXCIE0);
	UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);

	UBRR1 = UBRR1_RELOAD;
	UCSR1A = 0; //_BV(U2X1);
	UCSR1B = _BV(RXEN1) | _BV(TXEN1) | _BV(RXCIE1) | _BV(TXCIE1);
	UCSR1C = _BV(UCSZ10) | _BV(UCSZ11);

	UBRR2 = UBRR2_RELOAD;
	UCSR2A = 0;
	UCSR2B = _BV(RXEN2) | _BV(TXEN2) | _BV(RXCIE2) | _BV(TXCIE2);
	UCSR2C = _BV(UCSZ20) | _BV(UCSZ21);
	
	UBRR3 = UBRR3_RELOAD;
	UCSR3A = 0;
	UCSR3B = _BV(RXEN3) | _BV(TXEN3) | _BV(RXCIE3) | _BV(TXCIE3);
	UCSR3C = _BV(UCSZ30) | _BV(UCSZ31);
}


/*****************************************************/
ISR(USART0_RX_vect)
/*****************************************************/
{
	uint8_t status, data;

	status = UCSR0A;
	data = UDR0;

	if(!(status & (_BV(FE0) | _BV(UPE0) | _BV(DOR0))))
	{
		if(rx_count[0] < UART0_RX_BUFFER_SIZE)
		{

			uart0_rx_buffer[rx_tail[0]++] = data;
			if(rx_tail[0] >= UART0_RX_BUFFER_SIZE)
			rx_tail[0] = 0;

			rx_count[0]++;
		}
	}
}

/*****************************************************/
ISR(USART1_RX_vect)
/*****************************************************/
{
	uint8_t status, data;

	status = UCSR1A;
	data = UDR1;

	if(!(status & (_BV(FE1) | _BV(UPE1) | _BV(DOR1))))
	{
		
		uart1_rx_buffer[rx_tail[1]++] = data;
		if(rx_tail[1] >= UART1_RX_BUFFER_SIZE)
			rx_tail[1] = 0;

		rx_count[1]++;
	}
}

/*****************************************************/
ISR(USART2_RX_vect)
/*****************************************************/
{
	uint8_t status, data;

	status = UCSR2A;
	data = UDR2;

	if(!(status & (_BV(FE2) | _BV(UPE2) | _BV(DOR2))))
	{
		
		uart2_rx_buffer[rx_tail[2]++] = data;
		if(rx_tail[2] >= UART2_RX_BUFFER_SIZE)
			rx_tail[2] = 0;

		rx_count[2]++;
	}
}

/*****************************************************/
ISR(USART3_RX_vect)
/*****************************************************/
{
	uint8_t status, data;

	status = UCSR3A;
	data = UDR3;

	if(!(status & (_BV(FE3) | _BV(UPE3) | _BV(DOR3))))
	{
		
		uart3_rx_buffer[rx_tail[3]++] = data;
		if(rx_tail[3] >= UART3_RX_BUFFER_SIZE)
			rx_tail[3] = 0;

		rx_count[3]++;
	}
}

/*****************************************************/
ISR(USART0_TX_vect)
/*****************************************************/
{
	if(tx_count[0])
	{
		UDR0 = uart0_tx_buffer[tx_head[0]++];
		if(tx_head[0] >= UART0_TX_BUFFER_SIZE)
			tx_head[0] = 0;
		tx_count[0]--;
	}
}

/*****************************************************/
ISR(USART1_TX_vect)
/*****************************************************/
{
	if(tx_count[1])
	{
		UDR1 = uart1_tx_buffer[tx_head[1]++];
		if(tx_head[1] >= UART1_TX_BUFFER_SIZE)
			tx_head[1] = 0;
		tx_count[1]--;
	}
}

/*****************************************************/
ISR(USART2_TX_vect)
/*****************************************************/
{
	if(tx_count[2])
	{
		UDR2 = uart2_tx_buffer[tx_head[2]++];
		if(tx_head[2] >= UART2_TX_BUFFER_SIZE)
			tx_head[2] = 0;
		tx_count[2]--;
	}
}

/*****************************************************/
ISR(USART3_TX_vect)
/*****************************************************/
{
	if(tx_count[3])
	{
		UDR3 = uart3_tx_buffer[tx_head[3]++];
		if(tx_head[3] >= UART3_TX_BUFFER_SIZE)
			tx_head[3] = 0;
		tx_count[3]--;
	}
}

/*****************************************************/
uint8_t uart0_IsRxEmpty(void)
/*****************************************************/
{
	return (!rx_count[0]);	
}

/*****************************************************/
uint8_t uart1_IsRxEmpty(void)
/*****************************************************/
{
	return (!rx_count[1]);	
}

/*****************************************************/
uint8_t uart2_IsRxEmpty(void)
/*****************************************************/
{
	return (!rx_count[2]);	
}

/*****************************************************/
uint8_t uart3_IsRxEmpty(void)
/*****************************************************/
{
	return (!rx_count[3]);	
}

/*****************************************************/
void uart0_FlushRX(void)
/*****************************************************/
{	
	cli();

	rx_head[0] = rx_tail[0] = 0;
	rx_count[0] = 0;

	sei();
}

/*****************************************************/
void uart1_FlushRX(void)
/*****************************************************/
{	
	cli();

	rx_head[1] = rx_tail[1] = 0;
	rx_count[1] = 0;

	sei();
}

/*****************************************************/
void uart2_FlushRX(void)
/*****************************************************/
{	
	cli();

	rx_head[2] = rx_tail[2] = 0;
	rx_count[2] = 0;

	sei();
}

/*****************************************************/
void uart3_FlushRX(void)
/*****************************************************/
{	
	cli();

	rx_head[3] = rx_tail[3] = 0;
	rx_count[3] = 0;

	sei();
}


/*****************************************************/
uint8_t uart0_GetChar(void)
/*****************************************************/
{
	uint8_t data;

	while(!rx_count[0]);

	cli();

	data = uart0_rx_buffer[rx_head[0]++];
	if(rx_head[0] >= UART0_RX_BUFFER_SIZE)
		rx_head[0] = 0;
	rx_count[0]--;

	sei();

	return data;
}

/*****************************************************/
uint8_t uart1_GetChar(void)
/*****************************************************/
{
	uint8_t data;

	while(!rx_count[1]);

	cli();

	data = uart1_rx_buffer[rx_head[1]++];
	if(rx_head[1] >= UART1_RX_BUFFER_SIZE)
		rx_head[1] = 0;
	rx_count[1]--;

	sei();

	return data;
}

/*****************************************************/
uint8_t uart2_GetChar(void)
/*****************************************************/
{
	uint8_t data;

	while(!rx_count[2]);

	cli();

	data = uart2_rx_buffer[rx_head[2]++];
	if(rx_head[2] >= UART2_RX_BUFFER_SIZE)
		rx_head[2] = 0;
	rx_count[2]--;

	sei();

	return data;
}

/*****************************************************/
uint8_t uart3_GetChar(void)
/*****************************************************/
{
	uint8_t data;

	while(!rx_count[3]);

	cli();

	data = uart3_rx_buffer[rx_head[3]++];
	if(rx_head[3] >= UART3_RX_BUFFER_SIZE)
		rx_head[3] = 0;
	rx_count[3]--;

	sei();

	return data;
}

/*****************************************************/
void uart0_PutChar(uint8_t data)
/*****************************************************/
{
	while(tx_count[0] >= UART0_TX_BUFFER_SIZE);

	cli();
	
	if(tx_count[0] || (!(UCSR0A & _BV(UDRE0))))
	{
		uart0_tx_buffer[tx_tail[0]++] = data;
		if(tx_tail[0] >= UART0_TX_BUFFER_SIZE)
			tx_tail[0] = 0;
		tx_count[0]++;
	}
	else
		UDR0 = data;

	sei();
}

/*****************************************************/
void uart1_PutChar(uint8_t data)
/*****************************************************/
{
	while(tx_count[1] >= UART1_TX_BUFFER_SIZE);

	cli();
	
	if(tx_count[1] || (!(UCSR1A & _BV(UDRE1))))
	{
		uart1_tx_buffer[tx_tail[1]++] = data;
		if(tx_tail[1] >= UART1_TX_BUFFER_SIZE)
			tx_tail[1] = 0;
		tx_count[1]++;
	}
	else
		UDR1 = data;

	sei();
}

/*****************************************************/
void uart2_PutChar(uint8_t data)
/*****************************************************/
{
	while(tx_count[2] >= UART2_TX_BUFFER_SIZE);

	cli();
	
	if(tx_count[2] || (!(UCSR2A & _BV(UDRE2))))
	{
		uart2_tx_buffer[tx_tail[2]++] = data;
		if(tx_tail[2] >= UART2_TX_BUFFER_SIZE)
			tx_tail[2] = 0;
		tx_count[2]++;
	}
	else
		UDR2 = data;

	sei();
}

/*****************************************************/
void uart3_PutChar(uint8_t data)
/*****************************************************/
{
	while(tx_count[3] >= UART3_TX_BUFFER_SIZE);

	cli();
	
	if(tx_count[3] || (!(UCSR3A & _BV(UDRE3))))
	{
		uart3_tx_buffer[tx_tail[3]++] = data;
		if(tx_tail[3] >= UART3_TX_BUFFER_SIZE)
			tx_tail[3] = 0;
		tx_count[3]++;
	}
	else
		UDR3 = data;

	sei();
}
