/*
 * it_handlers.c
 *
 *  Created on: 25.06.2017
 *      Author: mateusz
 */

#include <stm32f10x.h>
#include "main.h"
#include "delay.h"
#include "diag/Trace.h"
#include "drivers/dallas.h"
#include "drivers/ms5611.h"
#include "drivers/serial.h"
#include "drivers/tx20.h"
#include "drivers/_dht22.h"
#include "drivers/i2c.h"
#include "drivers/umb-slave.h"
#include "../include/rte_main.h"
#include "../include/rte_wx.h"
#include "KissCommunication.h"
#include <stdio.h>

static char s = -1;
static char wx_freq = 3, wxi = 2;

extern UmbMeteoData u;

// Systick interrupt used for time measurements, checking timeouts and SysTick_Handler
void SysTick_Handler(void) {

	// systick interrupt is generated every 10ms
	master_time += SYSTICK_TICKS_PERIOD;

	srl_keep_timeout();

	i2cKeepTimeout();

	delay_decrement_counter();

}

void USART1_IRQHandler(void) {
	NVIC_ClearPendingIRQ(USART1_IRQn);
	srl_irq_handler();
}

void I2C1_EV_IRQHandler(void) {
	NVIC_ClearPendingIRQ(I2C1_EV_IRQn);

	i2cIrqHandler();

}

void I2C1_ER_IRQHandler(void) {
	i2cErrIrqHandler();
}

// przerwanie wyzwalne co ~40 sekund
void TIM3_IRQHandler(void) {


	TIM3->SR &= ~(1<<0);

	rte_main_umb_comm_timeout_cntr++;

	if (rte_main_umb_comm_timeout_cntr > 8)
		NVIC_SystemReset();

	ds_t = DallasQuery();
	u.temperature = (char)ds_t;
	 GPIO_SetBits(GPIOC, GPIO_Pin_8);

	switch (s) {
	case 0:
		ds_t = DallasQuery();
		s = 1;
		break;
	case 1:
		  ms_t = SensorBringTemperature();
		  s = 2;
		  break;
	case 2:
		  ms_p = (float)SensorBringPressure();
		  s = 3;
		break;
	case 3:
		if (dht22State == DHT22_STATE_DONE || dht22State == DHT22_STATE_TIMEOUT)
			dht22State = DHT22_STATE_IDLE;
		s = 0;
		break;
	default: s = 0; break;

	}
	 GPIO_ResetBits(GPIOC, GPIO_Pin_8);
	// co 10 sekund
}

void TIM4_IRQHandler(void) {
	TIM4->SR &= ~(1<<0);
	if (wxi++ == wx_freq){

		wxi = 1;
	}
	else {
		;
	}

}
