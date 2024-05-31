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
**/
#include "stm32f1_delay.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO  uint32_t delay_us;				// Bo dem cho ham Delay_ms();
__IO  uint32_t Counter_us;		// Bo dem cho ham read timer 100us

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	delay_us++;				// Tang gia tri delay len 1 lan
	Counter_us++;			// Tang gia tri dem len;
}

/* Setup SysTick Timer for 1 msec interrupts.
------------------------------------------
	1. The SysTick_Config() function is a CMSIS function which configure:
	- The SysTick Reload register with value passed as function parameter.
	- Configure the SysTick IRQ priority to the lowest value (0x0F).
	- Reset the SysTick Counter register.
	- Configure the SysTick Counter clock source to be Core Clock Source (HCLK).
	- Enable the SysTick Interrupt.
	- Start the SysTick Counter.
    
	2. You can change the SysTick Clock source to be HCLK_Div8 by calling the
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8) just after the
	SysTick_Config() function call. The SysTick_CLKSourceConfig() is defined
	inside the misc.c file.

	3. You can change the SysTick IRQ priority by calling the
	NVIC_SetPriority(SysTick_IRQn,...) just after the SysTick_Config() function 
	call. The NVIC_SetPriority() is defined inside the core_cm3.h file.

	4. To adjust the SysTick time base, use the following formula:
	
	Reload Value = SysTick Counter Clock (Hz) x  Desired Time base (s)
	
	- Reload Value is the parameter to be passed for SysTick_Config() function
	- Reload Value should not exceed 0xFFFFFF
*/

void Delay_Init(void)
{
  if (SysTick_Config(SystemCoreClock / 1000000))
  { 
    /* Capture error */ 
    while (1);
  }
	/* Configure the SysTick handler priority */
	NVIC_SetPriority(SysTick_IRQn, 0x0F);
}

/**
  * @brief  Initialize Delay
  * @note   Su dung chuc nang systick cua vdk lam delay
  * @param  None
  * @retval None
  */
void Delay_ms(uint32_t ms)
{
	delay_us=0;					// reset bo dem ve 0, (bat dau dem tu 0)
	while(delay_us<ms*1000);
}

/**
  * @brief  Initialize Delay
  * @note   Su dung chuc nang systick cua vdk lam delay
  * @param  None
  * @retval None
  */
void Delay_us(uint32_t us)
{
	delay_us=0;					// reset bo dem ve 0, (bat dau dem tu 0)
	while(delay_us<us);
}


/**
  * @brief  Initialize Delay
  * @note   Su dung chuc nang systick cua vdk lam delay
  * @param  None
  * @retval None
  */
void Timer_reset(void)
{
	Counter_us = 0;				// Dat gia tri bat dau dem len
}


/**
  * @brief  Read_Timer
  * @note   Su dung ham nay de doc ra thoi gian he thong bat dau tinh tu khi goi ham Set_Timer
  * @param  None
	* @retval thoi gian ms tu khi goi ham
  */
 uint32_t Timer_read_ms(void)
{
	return Counter_us/1000;
}

/**
  * @brief  Read_Timer
  * @note   Su dung ham nay de doc ra thoi gian he thong bat dau tinh tu khi goi ham Set_Timer
  * @param  None
	* @retval thoi gian ms tu khi goi ham Set_Timer: Thoi gian = Read_Timer() - Read_Timer(0)
  */
 uint32_t Timer_read_us(void)
{
	return Counter_us;
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

