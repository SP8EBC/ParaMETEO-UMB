/*
 * TimerConfig.c
 *
 *  Created on: 19.03.2017
 *      Author: mateusz
 */

#include <stm32f10x.h>
#include <stm32f10x_tim.h>
#include "TimerConfig.h"

void TimerConfig(void) {
	///////////////////////////////////////////
	/// konfiguracja TIM2 -- dallas delay 	///
	///////////////////////////////////////////
	NVIC_SetPriority(TIM2_IRQn, 2);
	TIM2->PSC = 0;
	TIM2->ARR = 119;
	TIM2->CR1 |= TIM_CR1_DIR;
	TIM2->CR1 &= (0xFFFFFFFF ^ TIM_CR1_DIR);
//	TIM2->CR1 |= TIM_CR1_CEN;
	TIM2->DIER |= 1;
	NVIC_EnableIRQ( TIM2_IRQn );
}


void EventTimerConfig(void) {
	NVIC_SetPriority(TIM3_IRQn, 4);
	TIM3->PSC = 65535;
	TIM3->ARR = 13440;   // co 40 sekund
	TIM3->CR1 |= TIM_CR1_DIR;
	TIM3->CR1 &= (0xFFFFFFFF ^ TIM_CR1_DIR);
	TIM3->CR1 |= TIM_CR1_CEN;
	TIM3->DIER |= 1;
	NVIC_EnableIRQ( TIM3_IRQn );

}

void SendingTimerConfig(void) {
	NVIC_SetPriority(TIM4_IRQn, 3);
	TIM4->PSC = 65535;
	TIM4->ARR = 21960;   // co 60 sekund
	TIM4->CR1 |= TIM_CR1_DIR;
	TIM4->CR1 &= (0xFFFFFFFF ^ TIM_CR1_DIR);
	TIM4->CR1 |= TIM_CR1_CEN;
	TIM4->DIER |= 1;
	NVIC_EnableIRQ( TIM4_IRQn );

}

