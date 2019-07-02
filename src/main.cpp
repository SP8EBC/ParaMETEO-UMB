//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------

#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stm32f10x.h>
#include <stm32f10x_rcc.h>
#include "stm32f10x_iwdg.h"
#include "diag/Trace.h"
#include "drivers/serial.h"
#include "drivers/umb-slave.h"
//#include "drivers/tm_stm32f1_onewire.h"
#include "drivers/dallas.h"
#include "drivers/ms5611.h"
#include "drivers/tx20.h"
#include "drivers/i2c.h"
#include "drivers/adc.h"
#include "drivers/_dht22.h"
#include "TimerConfig.h"
#include "LedConfig.h"

#include "aprs/ax25.h"
#include "KissCommunication.h"

#include "rte_main.h"
#include "rte_wx.h"

#include "wx_handler.h"

//#define _KOZIA_GORA

dht22Values dht, dht_valid;

UmbMeteoData u;

volatile int i = 0;

//volatile uint8_t commTimeoutCounter = 0;

uint32_t master_time = 0;


// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic ignored "-Wempty-body"

int
main(int argc, char* argv[])
{

  RCC->APB1ENR |= (RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM7EN | RCC_APB1ENR_TIM4EN);
  RCC->APB2ENR |= (RCC_APB2ENR_IOPAEN |
				  RCC_APB2ENR_IOPBEN |
				  RCC_APB2ENR_IOPCEN |
				  RCC_APB2ENR_IOPDEN |
				  RCC_APB2ENR_AFIOEN |
				  RCC_APB2ENR_TIM1EN);

  u.humidity = 0;
  u.qfe = 0;
  u.qnh = 0;
  u.temperature = 0;
  u.winddirection = 0;
  u.windgusts = 0.0f;
  u.windspeed = 0.0f;

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

  // choosing the signal source for the SysTick timer.
  SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);

  // Configuring the SysTick timer to generate interrupt 100x per second (one interrupt = 10ms)
  SysTick_Config(SystemCoreClock / SYSTICK_TICKS_PER_SECONDS);

  // setting an Systick interrupt priority
  NVIC_SetPriority(SysTick_IRQn, 5);

  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  IWDG_SetPrescaler(IWDG_Prescaler_128);
  IWDG_SetReload(0xFFF);
//  IWDG_Enable();
//  IWDG_ReloadCounter();


  LedConfig();
  TimerConfig();

#ifndef _KOZIA_GORA
  dallas_init(GPIOC, GPIO_Pin_6, GPIO_PinSource6);
#else
  dallas_init(GPIOC, GPIO_Pin_7, GPIO_PinSource7);
#endif
  srl_init();
  TX20Init();
  i2cConfigure();
  dht22_init();

  ms5611_reset(&rte_wx_ms5611_qf);
  ms5611_read_calibration(SensorCalData, &rte_wx_ms5611_qf);
  ms5611_trigger_measure(0, 0);

  wx_get_all_measurements();

  GPIO_ResetBits(GPIOC, GPIO_Pin_8 | GPIO_Pin_9);
  EventTimerConfig();
  UmbSlaveListen();

  // Infinite loop
  while (1)
    {

	  if (srl_rx_state == SRL_RX_DONE || srl_rx_state == SRL_RX_IDLE || srl_rx_state == SRL_WAITING_TO_RX) {
		  IWDG_ReloadCounter();
	  }

		dht22_timeout_keeper();

		switch (dht22State) {
			case DHT22_STATE_IDLE:
				dht22_comm(&dht);
				break;
			case DHT22_STATE_DATA_RDY:
				dht22_decode(&dht);
				break;
			case DHT22_STATE_DATA_DECD:
				dht_valid = dht;			// powrot do stanu DHT22_STATE_IDLE jest w TIM3_IRQHandler
				dht22State = DHT22_STATE_DONE;
				break;
			default: break;
		}

	  UmbSlavePool();
	  if (umbSlaveState == 3) {
		  NVIC_SystemReset();
	  }
	  if (umbSlaveState == 2) {
		  GPIO_SetBits(GPIOC, GPIO_Pin_8);
		  rte_main_umb_comm_timeout_cntr = 0;

		  switch (umbMessage.cmdId) {
			  case 0x26:
				  UmbClearMessageStruct(0);
				  UmbStatusRequestResponse();
				  srl_start_tx(UmbPrepareFrameToSend(&umbMessage, srl_tx_buffer));
				  //while(srlTXing != 0);
				  umbSlaveState = 4;
				  break;
			  case 0x2D:
				  UmbDeviceInformationRequestResponse();
				  srl_start_tx(UmbPrepareFrameToSend(&umbMessage, srl_tx_buffer));
				  umbSlaveState = 4;
				  break;
			  case 0x23:
//				  UmbClearMessageStruct(0);
				  u.humidity = (char)dht_valid.humidity;
				  u.fTemperature = rte_wx_temperature_dallas_valid;
				  u.temperature = (char)rte_wx_temperature_dallas_valid;
				  u.qfe = rte_wx_pressure_valid;
				  u.qnh = CalcQNHFromQFE(rte_wx_pressure_valid, 674, rte_wx_temperature_dallas_valid);
				  u.winddirection = TX20.HistoryAVG[0].WindDirX;
				  u.windspeed = TX20.HistoryAVG[0].WindSpeed;
				  u.windgusts = TX20FindMaxSpeed();
				  UmbOnlineDataRequestResponse(&u, 0);
				  srl_start_tx(UmbPrepareFrameToSend(&umbMessage, srl_tx_buffer));
				  umbSlaveState = 4;
				  break;
			  case 0x2F:
				  UmbMultiOnlineDataRequestResponse(&u, 0);
				  srl_start_tx(UmbPrepareFrameToSend(&umbMessage, srl_tx_buffer));
				  umbSlaveState = 4;
				  break;
			  default:
				  UmbClearMessageStruct(0);
				  UmbSlaveListen();
				  break;
		  }
		  GPIO_ResetBits(GPIOC, GPIO_Pin_8);
	  }
	  if (umbSlaveState == 4 && srl_tx_state == SRL_TX_IDLE)
		  UmbSlaveListen();


    }
  // Infinite loop, never return.
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
