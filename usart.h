#ifndef _USART_H
#define _USART_H

#include <stdint.h>	// uint8_t

#define UART0_BAUD_RATE 38400ul
#define UART0_RX_BUFFER_SIZE 256
#define UART0_TX_BUFFER_SIZE 256

#define UART1_BAUD_RATE 38400ul
#define UART1_RX_BUFFER_SIZE 256
#define UART1_TX_BUFFER_SIZE 256

#define UART2_BAUD_RATE 9600ul
#define UART2_RX_BUFFER_SIZE 256
#define UART2_TX_BUFFER_SIZE 256

#define UART3_BAUD_RATE 38400ul
#define UART3_RX_BUFFER_SIZE 128
#define UART3_TX_BUFFER_SIZE 128


// NB! All work fine without X2.
#define UBRR0_RELOAD ((F_CPU / (UART0_BAUD_RATE  * 16)) - 1)
#define UBRR1_RELOAD ((F_CPU / (UART1_BAUD_RATE  * 16)) - 1)
#define UBRR2_RELOAD ((F_CPU / (UART2_BAUD_RATE  * 16)) - 1)
#define UBRR3_RELOAD ((F_CPU / (UART3_BAUD_RATE  * 16)) - 1)

void uart_Init(void);
uint8_t uart0_IsRxEmpty(void);
uint8_t uart1_IsRxEmpty(void);
uint8_t uart2_IsRxEmpty(void);
uint8_t uart3_IsRxEmpty(void);
void uart0_FlushRX(void);
void uart1_FlushRX(void);
void uart2_FlushRX(void);
void uart3_FlushRX(void);
uint8_t uart0_GetChar(void);
uint8_t uart1_GetChar(void);
uint8_t uart2_GetChar(void);
uint8_t uart3_GetChar(void);
void uart0_PutChar(uint8_t data);
void uart1_PutChar(uint8_t data);
void uart2_PutChar(uint8_t data);
void uart3_PutChar(uint8_t data);



#endif
