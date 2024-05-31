/**
  ******************************************************************************
  * @file    SysTick/SysTick_Example/main.h 
  * @author  MCD Application Team
  * @version V1.4.0
  * @date    04-August-2014
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */
#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"		


void SysTick_Handler(void);
void Delay_Init(void);
void Delay_ms(uint32_t ms);
void Delay_us(uint32_t us);
void Timer_reset(void);
 uint32_t  Timer_read_ms(void);
 uint32_t  Timer_read_us(void);

#endif
