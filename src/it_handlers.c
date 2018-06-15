/*
 * it_handlers.c
 *
 *  Created on: 25.06.2017
 *      Author: mateusz
 */

#include <stm32f10x.h>
#include "main.h"
#include "diag/Trace.h"
#include "drivers/dallas.h"
#include "drivers/ms5611.h"
#include "drivers/serial.h"
#include "drivers/tx20.h"
#include "drivers/_dht22.h"
#include "drivers/umb-slave.h"
#include "KissCommunication.h"
#include <stdio.h>

static char s = -1;
static char wx_freq = 3, wxi = 2;

extern UmbMeteoData u;

void TIM3_IRQHandler(void) {


	TIM3->SR &= ~(1<<0);
	ds_t = DallasQuery();
	u.temperature = (char)ds_t;
	 GPIO_SetBits(GPIOC, GPIO_Pin_8);

//	trace_puts("TIM3\r\n");
	switch (s) {
	case 0:
		ds_t = DallasQuery();
//		trace_printf("temperatura DS: %d\r\n", (int)ds_t);
		s = 1;
		break;
	case 1:
		  ms_t = SensorBringTemperature();
//		  trace_printf("temperatura MS: %d\r\n", (int)ms_t);
		  s = 2;
		  break;
	case 2:
		  ms_p = (float)SensorBringPressure();
//		  trace_printf("cisnienie MS: %d\r\n", (int)ms_p);
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
//	trace_printf("TIM4: %d\r\n", (int)wxi);
	if (wxi++ == wx_freq){
//		trace_printf("TIM4: sending...\r\n");
//
//		float max_wind_speed = 0.0, temp;
//		unsigned char wind_speed_mph, wind_gusts_mph, d;
//		for(d = 1; d <= 19 ; d++)
//			if (VNAME.HistoryAVG[d].WindSpeed > max_wind_speed)
//					max_wind_speed = VNAME.HistoryAVG[d].WindSpeed;		// Wyszukiwane najwiekszej wartosci
//		temp = VNAME.HistoryAVG[0].WindSpeed;
//		temp /= 0.45;																						// Konwersja na mile na godzine
//		max_wind_speed /= 0.45;
//		if ((temp - (short)temp) >= 0.5)												// Zaokraglanie wartosci
//			/* Odejmuje od wartosci zmiennoprzecinkowej w milach nad godzine wartosc
//				 po zrzutowaniu na typ short czyli z odcienta czescia po przecinku.
//				 Jezeli wynik jest wiekszy albo rowny 0.5 to trzeba zaokraglic w gore */
//			wind_speed_mph = (short)temp + 1;
//		else
//			/* A jezeli jest mniejsza niz 0.5 to zaokraglamy w dol */
//			wind_speed_mph = (short)temp;
//		if ((max_wind_speed - (short)max_wind_speed) >= 0.5)
//			/* Analogiczna procedura ma miejsce dla porywow wiatru*/
//			wind_gusts_mph = (short)max_wind_speed + 1;
//		else
//			wind_gusts_mph = (short)max_wind_speed;
//
//		short int temperatura;
//		temperatura = ds_t*1.8+32;
//
//		int qnh = (int)CalcQNHFromQFE(ms_p, 215.0f, ds_t);
//
//		memset(aprs_msg, 0x00, 128);
//		aprs_msg_len = sprintf(aprs_msg, "%s%03d/%03dg%03dt%03dr...p...P...b%05d", "!4948.82N/01903.50E_", /* kierunek */(short)VNAME.HistoryAVG[0].WindDirX, /* predkosc*/(int)wind_speed_mph, /* porywy */(short)wind_gusts_mph,/*temperatura */temperatura, qnh*10);
//		aprs_msg[aprs_msg_len] = 0;
//		ax25_sendVia(&ax25, path, (sizeof(path) / sizeof(*(path))), aprs_msg, aprs_msg_len);
//		while(!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5));
//		while(srlTXing == 1);
//		SrlStartTX(SendKISSToHost(0x00, (char*)a.tx_buf + 1, a.tx_fifo.tail - a.tx_fifo.head - 4, srlTXData));
//		while(srlTXing == 1);
//		AFSK_Init(&a);


//		trace_printf("TIM4: sent\r\n");

		wxi = 1;
	}
	else;

}
