/*
 * rte_wx.c
 *
 *  Created on: 19.05.2019
 *      Author: mateusz
 */

#include <rte_wx.h>

float rte_wx_temperature_dallas = 0.0f, rte_wx_temperature_dallas_valid = 0.0f;
float rte_wx_temperature_average_dallas_valid = 0.0f;
float rte_wx_temperature_min_dallas_valid = 0.0f, rte_wx_temperature_max_dallas_valid = 0.0f;
float rte_wx_temperature_ms = 0.0f, rte_wx_temperature_ms_valid = 0.0f;
float rte_wx_pressure = 0.0f, rte_wx_pressure_valid = 0.0f;

uint8_t rte_wx_tx20_excessive_slew_rate = 0;

dht22Values rte_wx_dht, rte_wx_dht_valid;		// quality factor inside this structure
DallasQF rte_wx_current_dallas_qf, rte_wx_error_dallas_qf = DALLAS_QF_UNKNOWN;
DallasAverage_t rte_wx_dallas_average;
ms5611_qf_t rte_wx_ms5611_qf;

umbMeteoData_t rte_wx_umb;
