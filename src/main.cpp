//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

// ----------------------------------------------------------------------------
//
// Standalone STM32F1 led blink sample (trace via DEBUG).
//
// In debug configurations, demonstrate how to print a greeting message
// on the trace device. In release configurations the message is
// simply discarded.
//
// Then demonstrates how to blink a led with 1 Hz, using a
// continuous loop and SysTick delays.
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//
// The external clock frequency is specified as a preprocessor definition
// passed to the compiler via a command line option (see the 'C/C++ General' ->
// 'Paths and Symbols' -> the 'Symbols' tab, if you want to change it).
// The value selected during project creation was HSE_VALUE=8000000.
//
// Note: The default clock settings take the user defined HSE_VALUE and try
// to reach the maximum possible system clock. For the default 8 MHz input
// the result is guaranteed, but for other values it might not be possible,
// so please adjust the PLL settings in system/src/cmsis/system_stm32f10x.c
//

// Definitions visible only within this translation unit.
namespace
{
  // ----- Timing definitions -------------------------------------------------

  // Keep the LED on for 2/3 of a second.
  constexpr Timer::ticks_t BLINK_ON_TICKS = Timer::FREQUENCY_HZ * 3 / 4;
  constexpr Timer::ticks_t BLINK_OFF_TICKS = Timer::FREQUENCY_HZ
      - BLINK_ON_TICKS;
}

float ds_t = 0.0;
float ms_t = 0.0f;
double ms_p = 0.0;

dht22Values dht, dht_valid;

AX25Ctx ax25;
Afsk a;
AX25Call path[2];
uint8_t aprs_msg_len;
char aprs_msg[128];

UmbMeteoData u;

volatile int i = 0;

//uint16_t adc = 0;


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

  memset(aprs_msg, 0x00, 128);

  memcpy(path[0].call, "AKLPRZ", 6), path[0].ssid = 0;
  memcpy(path[1].call, "SP8EBC", 6), path[1].ssid = 1;

  u.humidity = 0;
  u.qfe = 0;
  u.qnh = 0;
  u.temperature = 0;
  u.winddirection = 0;
  u.windgusts = 0.0f;
  u.windspeed = 0.0f;

  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  IWDG_SetPrescaler(IWDG_Prescaler_128);
  IWDG_SetReload(0xFFF);
  IWDG_Enable();
  IWDG_ReloadCounter();


  LedConfig();
  TimerConfig();
//  SendingTimerConfig();
  DallasInit(GPIOC, GPIO_Pin_7, GPIO_PinSource7);
  SrlConfig();
  TX20Init();
  i2cConfigure();
  dht22_init();

  SensorReset(0xEC);
  ds_t = DallasQuery();
  SensorReadCalData(0xEC, SensorCalData);
  SensorStartMeas(0);

//  EventTimerConfig();

//  memset(srlTXData, 0x00, 128);
//  aprs_msg_len = sprintf(aprs_msg, ">ParaMETEO by SP8EBC - v27072017");
//  aprs_msg[aprs_msg_len] = 0;
//  ax25_sendVia(&ax25, path, (sizeof(path) / sizeof(*(path))), aprs_msg, aprs_msg_len);
//  SrlStartTX(SendKISSToHost(0x00, (char*)a.tx_buf + 1, a.tx_fifo.tail - a.tx_fifo.head - 4, srlTXData));
//  while(srlTXing == 1);
////  memset(aprs_msg, 0x00, 128);
//  AFSK_Init(&a);

  ds_t = DallasQuery();
//  trace_printf("temperatura DS: %d\r\n", (int)ds_t);
  ms_t = SensorBringTemperature();
//  trace_printf("temperatura MS: %d\r\n", (int)ms_t);
  ds_t = DallasQuery();
  ms_p = (float)SensorBringPressure();
//  trace_printf("cisnienie MS: %d\r\n", (int)ms_p);

	u.temperature = (char)ds_t;

	  GPIO_ResetBits(GPIOC, GPIO_Pin_8 | GPIO_Pin_9);
	EventTimerConfig();
  UmbSlaveListen();

  // Infinite loop
  while (1)
    {
//	  if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5))
//		  GPIO_ResetBits(GPIOC, GPIO_Pin_8);
//	  else
//		  GPIO_SetBits(GPIOC, GPIO_Pin_8);
//	  ds_t = DallasQuery();
//	  ms_t = SensorBringTemperature();
//	  trace_printf("temperatura DS: %d\r\n", (int)ds_t);
//	  trace_printf("temperatura MS: %d\r\n", (int)ms_t);
//	  ds_t = DallasQuery();
//	  ms_p = (float)SensorBringPressure();
//
//	  memset(rsoutput, 0x00, 22);
//	  sprintf(rsoutput, "temperatura DS: %d\r\n", (int)ds_t);
//	  SrlSendData(rsoutput, 0, 0);
//	  while(srlTXing == 1);
//	  while(srlRXing == 1);

	  if (srlRXing == 0) {
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
#ifdef _DBG_TRACE
				trace_printf("DHT22: temperature=%d,humi=%d\r\n", dht_valid.scaledTemperature, dht_valid.humidity);
#endif
				break;
			default: break;
		}

	  UmbSlavePool();
	  if (umbSlaveState == 2) {
		  GPIO_SetBits(GPIOC, GPIO_Pin_8);
//	  	  trace_printf("UmbSlave: Received cmdId 0x%02x with data %d bytes long\n", umbMessage.cmdId, umbMessage.payloadLn);
		  for (i = 0; i <= 0x1FFFF; i++);
		  switch (umbMessage.cmdId) {
			  case 0x26:
				  UmbClearMessageStruct(0);
				  UmbStatusRequestResponse();
				  SrlStartTX(UmbPrepareFrameToSend(&umbMessage, srlTXData));
				  //while(srlTXing != 0);
				  umbSlaveState = 4;
				  break;
			  case 0x2D:
				  UmbDeviceInformationRequestResponse();
				  SrlStartTX(UmbPrepareFrameToSend(&umbMessage, srlTXData));
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
				  SrlStartTX(UmbPrepareFrameToSend(&umbMessage, srlTXData));
				  umbSlaveState = 4;
				  break;
			  case 0x2F:
				  UmbMultiOnlineDataRequestResponse(&u, 0);
				  SrlStartTX(UmbPrepareFrameToSend(&umbMessage, srlTXData));
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
