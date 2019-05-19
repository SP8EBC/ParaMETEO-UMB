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

#include "Timer.h"
#include "BlinkLed.h"

#include "aprs/ax25.h"
#include "KissCommunication.h"

#define _KOZIA_GORA

float ds_t = 0.0;
float ms_t = 0.0f;
double ms_p = 0.0;

dht22Values dht, dht_valid;

UmbMeteoData u;

volatile int i = 0;

volatile uint8_t commTimeoutCounter = 0;

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

  NVIC_SetPriorityGrouping(0);

  // Send a greeting to the trace device (skipped on Release).
//  trace_puts("Hello ARM World!");

  // At this stage the system clock should have already been configured
  // at high speed.
//  trace_printf("System clock: %u Hz\n", SystemCoreClock);

  char rsoutput[20];

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
  IWDG_Enable();
  IWDG_ReloadCounter();


  LedConfig();
  TimerConfig();

#ifndef _KOZIA_GORA
  DallasInit(GPIOC, GPIO_Pin_6, GPIO_PinSource6);
#else
  DallasInit(GPIOC, GPIO_Pin_7, GPIO_PinSource7);
#endif
  srl_init();
  TX20Init();
  i2cConfigure();
  dht22_init();

  SensorReset(0xEC);
  ds_t = DallasQuery();
  SensorReadCalData(0xEC, SensorCalData);
  SensorStartMeas(0);

  ds_t = DallasQuery();
  ms_t = SensorBringTemperature();
  ds_t = DallasQuery();
  ms_p = (float)SensorBringPressure();

	u.temperature = (char)ds_t;

	  GPIO_ResetBits(GPIOC, GPIO_Pin_8 | GPIO_Pin_9);
	  EventTimerConfig();
  UmbSlaveListen();

  // Infinite loop
  while (1)
    {

	  if (srl_rx_state == SRL_RX_DONE || srl_rx_state == SRL_RX_IDLE) {
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
		  commTimeoutCounter = 0;
//	  	  trace_printf("UmbSlave: Received cmdId 0x%02x with data %d bytes long\n", umbMessage.cmdId, umbMessage.payloadLn);
		  for (i = 0; i <= 0x1FFFF; i++);
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
//				  trace_printf("UmbSlave: cmd[DEVICE_INFO] sending payload for option 0x%02X with status %d crc: 0x%04X\n",
//						  	 umbMessage.payload[1],
//							 umbMessage.payload[0],
//							 umbMessage.checksum
//							 );
				  break;
			  case 0x23:
//				  UmbClearMessageStruct(0);
				  u.humidity = (char)dht_valid.humidity;
				  u.fTemperature = ds_t;
				  u.temperature = ds_t;
				  u.qfe = ms_p;
				  u.qnh = CalcQNHFromQFE(ms_p, 674, ds_t);
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
	  if (umbSlaveState == 4 && srlTXing == 0)
		  UmbSlaveListen();


    }
  // Infinite loop, never return.
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
