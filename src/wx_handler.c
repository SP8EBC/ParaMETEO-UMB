/*
 * wx_handler.c
 *
 *  Created on: 26.01.2019
 *      Author: mateusz
 */

#include <math.h>
#include <rte_wx.h>
#include "drivers/_dht22.h"
#include "drivers/ms5611.h"
#include "drivers/tx20.h"

#include "../include/station_config.h"


void wx_get_all_measurements(void) {

	int32_t return_value = 0;

	// quering MS5611 sensor for temperature
	return_value = ms5611_get_temperature(&rte_wx_temperature_ms, &rte_wx_ms5611_qf);

	if (return_value == MS5611_OK) {
		rte_wx_temperature_ms_valid = rte_wx_temperature_ms;

	}

	// quering dallas DS12B20 thermometer for current temperature
	rte_wx_temperature_dallas = dallas_query(&rte_wx_current_dallas_qf);

	// checking if communication was successfull
	if (rte_wx_temperature_dallas != -128.0f) {

		// store current value
		rte_wx_temperature_dallas_valid = rte_wx_temperature_dallas;

		// include current temperature into the average
		dallas_average(rte_wx_temperature_dallas, &rte_wx_dallas_average);

		// update the current temperature with current average
		rte_wx_temperature_average_dallas_valid = dallas_get_average(&rte_wx_dallas_average);

		// update current minimal temperature
		rte_wx_temperature_min_dallas_valid = dallas_get_min(&rte_wx_dallas_average);

		// and update maximum also
		rte_wx_temperature_max_dallas_valid = dallas_get_max(&rte_wx_dallas_average);
	}
	else {
		// if there were a communication error set the error to unavaliable
		rte_wx_error_dallas_qf = DALLAS_QF_NOT_AVALIABLE;

	}
	//else
	//	rte_wx_temperature_dallas_valid = 0.0f;

	// quering MS5611 sensor for pressure
	return_value = ms5611_get_pressure(&rte_wx_pressure,  &rte_wx_ms5611_qf);

	if (return_value == MS5611_OK) {
		rte_wx_pressure_valid = rte_wx_pressure;
	}

	// if humidity sensor is idle trigger the communiction & measuremenets
	if (dht22State == DHT22_STATE_DONE || dht22State == DHT22_STATE_TIMEOUT)
		dht22State = DHT22_STATE_IDLE;

}

void wx_pool_dht22(void) {

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
			//rte_wx_dht_valid.qf = DHT22_QF_FULL;
			dht22State = DHT22_STATE_DONE;

#ifdef _DBG_TRACE
			trace_printf("DHT22: temperature=%d,humi=%d\r\n", dht_valid.scaledTemperature, dht_valid.humidity);
#endif
			break;
		case DHT22_STATE_TIMEOUT:
			rte_wx_dht_valid.qf = DHT22_QF_UNAVALIABLE;
			break;
		default: break;
	}

}

void wx_copy_to_rte_meteodata(void) {
	  rte_wx_umb.humidity = lround(rte_wx_dht_valid.humidity);
	  rte_wx_umb.float_temperature = rte_wx_temperature_dallas_valid;
	  rte_wx_umb.temperature = lround(rte_wx_temperature_dallas_valid);
	  rte_wx_umb.float_max_temperature = rte_wx_temperature_max_dallas_valid;
	  rte_wx_umb.float_min_temperature = rte_wx_temperature_min_dallas_valid;
	  rte_wx_umb.max_temperature = lround(rte_wx_temperature_max_dallas_valid);
	  rte_wx_umb.min_temperature = lround(rte_wx_temperature_min_dallas_valid);
	  rte_wx_umb.float_avg_temperature = rte_wx_temperature_average_dallas_valid;
	  rte_wx_umb.avg_temperature = lround(rte_wx_temperature_average_dallas_valid);
	  rte_wx_umb.qfe = rte_wx_pressure_valid;
	  rte_wx_umb.sqfe = round(rte_wx_pressure_valid);
	  rte_wx_umb.qnh = CalcQNHFromQFE(rte_wx_pressure_valid, ALTITUDE, rte_wx_temperature_dallas_valid);
	  rte_wx_umb.sqnh = round(CalcQNHFromQFE(rte_wx_pressure_valid, ALTITUDE, rte_wx_temperature_dallas_valid));
	  rte_wx_umb.winddirection = TX20.HistoryAVG[0].WindDirX;
	  rte_wx_umb.windspeed = TX20.HistoryAVG[0].WindSpeed;
	  rte_wx_umb.windgusts = TX20FindMaxSpeed();
	  rte_wx_umb.windspeedmin = TX20FindMinSpeed();
}
