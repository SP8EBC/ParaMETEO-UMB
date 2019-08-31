#ifndef RTEWX_H_
#define RTEWX_H_

#include <stdint.h>
#include "drivers/_dht22.h"
#include "drivers/dallas.h"
#include "drivers/ms5611.h"
#include "drivers/umb-slave.h"

extern float rte_wx_temperature_dallas, rte_wx_temperature_dallas_valid;
extern float rte_wx_temperature, rte_wx_temperature_valid;
extern float rte_wx_pressure, rte_wx_pressure_valid;

extern uint8_t rte_wx_tx20_excessive_slew_rate;

extern dht22Values rte_wx_dht, rte_wx_dht_valid;

extern DallasQF rte_wx_current_dallas_qf, rte_wx_error_dallas_qf;
extern ms5611_qf_t rte_wx_ms5611_qf;

extern umbMeteoData_t rte_wx_umb;

#endif
