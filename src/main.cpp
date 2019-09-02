//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------

#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <main.h>
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
#include "rte_umb.h"

#include "wx_handler.h"

#include "station_config.h"

//#define _KOZIA_GORA


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

  rte_wx_umb.humidity = 0;
  rte_wx_umb.qfe = 0;
  rte_wx_umb.qnh = 0;
  rte_wx_umb.temperature = 0;
  rte_wx_umb.winddirection = 0;
  rte_wx_umb.windgusts = 0.0f;
  rte_wx_umb.windspeed = 0.0f;

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

  // Configuting LEDs
  LedConfig();

  // Configuring timers for Dallas OneWire delay & WX measurements starting
  TimerConfig();

#ifndef _KOZIA_GORA
	#ifndef _DALLAS_SPLIT_PIN
	  dallas_init(GPIOC, GPIO_Pin_6, GPIO_PinSource6);
	#else
	  // If split - ping OneWire is enabled to work with opto separation.
	  // Pin defined here is an output. Next pin in the same port is an input
	  dallas_init(GPIOB, GPIO_Pin_5, GPIO_PinSource5);
	#endif
#else
	  // SR9WXS specific configuration
  dallas_init(GPIOC, GPIO_Pin_7, GPIO_PinSource7);
#endif

  // Initializing UART
  srl_init();

  // Initializing TX20 anemometer driver
  TX20Init();

  // Initializing i2c driver
  i2cConfigure();

  // Initializing dht22 humidity sensor
  dht22_init();

  ms5611_reset(&rte_wx_ms5611_qf);
  ms5611_read_calibration(SensorCalData, &rte_wx_ms5611_qf);
  ms5611_trigger_measure(0, 0);

  wx_get_all_measurements();

  GPIO_ResetBits(GPIOC, GPIO_Pin_8 | GPIO_Pin_9);
  EventTimerConfig();
  umb_slave_listen();

  // Infinite loop
  while (1)
    {

	  if (srl_rx_state == SRL_RX_DONE || srl_rx_state == SRL_RX_IDLE || srl_rx_state == SRL_WAITING_TO_RX) {
		  IWDG_ReloadCounter();
	  }

		dht22_timeout_keeper();

		switch (dht22State) {
			case DHT22_STATE_IDLE:
				dht22_comm(&rte_wx_dht);
				break;
			case DHT22_STATE_DATA_RDY:
				dht22_decode(&rte_wx_dht);
				break;
			case DHT22_STATE_DATA_DECD:
				rte_wx_dht_valid = rte_wx_dht;			// powrot do stanu DHT22_STATE_IDLE jest w TIM3_IRQHandler
				dht22State = DHT22_STATE_DONE;
				break;
			default: break;
		}

	  umb_slave_pooling();
	  if (umb_slave_state == UMB_STATE_MESSAGE_RXED_WRONG_CRC) {
		  umb_slave_listen();
	  }
	  if (umb_slave_state == UMB_STATE_MESSAGE_RXED) {
		  GPIO_SetBits(GPIOC, GPIO_Pin_8);
		  rte_main_umb_comm_timeout_cntr = 0;

		  switch (umb_message.cmdId) {
			  case 0x26:
				  umb_clear_message_struct(0);
				  umb_callback_status_request_0x26();
				  srl_start_tx(umb_prepare_frame_to_send(&umb_message, srl_tx_buffer));
				  umb_slave_state = UMB_STATE_PROCESSING_DONE;
				  break;
			  case 0x2D:
				  umb_callback_device_information_0x2d();
				  srl_start_tx(umb_prepare_frame_to_send(&umb_message, srl_tx_buffer));
				  umb_slave_state = UMB_STATE_PROCESSING_DONE;
				  break;
			  case 0x23:
//				  UmbClearMessageStruct(0);
				  rte_wx_umb.humidity = (char)rte_wx_dht_valid.humidity;
				  rte_wx_umb.fTemperature = rte_wx_temperature_dallas_valid;
				  rte_wx_umb.temperature = (char)rte_wx_temperature_dallas_valid;
				  rte_wx_umb.qfe = rte_wx_pressure_valid;
				  rte_wx_umb.sqfe = round(rte_wx_umb.qfe);
				  rte_wx_umb.qnh = CalcQNHFromQFE(rte_wx_pressure_valid, 674, rte_wx_temperature_dallas_valid);
				  rte_wx_umb.sqnh = round(CalcQNHFromQFE(rte_wx_pressure_valid, 674, rte_wx_temperature_dallas_valid));
				  rte_wx_umb.winddirection = TX20.HistoryAVG[0].WindDirX;
				  rte_wx_umb.windspeed = TX20.HistoryAVG[0].WindSpeed;
				  rte_wx_umb.windgusts = TX20FindMaxSpeed();
				  umb_callback_online_data_request_0x23(&rte_wx_umb, 0);
				  srl_start_tx(umb_prepare_frame_to_send(&umb_message, srl_tx_buffer));
				  umb_slave_state = UMB_STATE_PROCESSING_DONE;
				  break;
			  case 0x2F:
				  umb_callback_multi_online_data_request_0x2f(&rte_wx_umb, 0);
				  srl_start_tx(umb_prepare_frame_to_send(&umb_message, srl_tx_buffer));
				  umb_slave_state = UMB_STATE_PROCESSING_DONE;
				  break;
			  default:
				  rte_umb_wrong_service++;
				  umb_clear_message_struct(0);
				  umb_slave_listen();
				  break;
		  }
		  GPIO_ResetBits(GPIOC, GPIO_Pin_8);
	  }
	  if (umb_slave_state == UMB_STATE_PROCESSING_DONE && srl_tx_state == SRL_TX_IDLE)
		  umb_slave_listen();


    }
  // Infinite loop, never return.
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
