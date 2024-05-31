#ifndef _USART_H
#define _USART_H

#include <stdio.h>
#include <stm32f10x.h>
#include "stm32f10x_usart.h"
void UART1_Config(void);
void UART2_Config(void);
void UART3_Config(void);
void put_char(uint8_t a);

#endif /*_USART_H*/
