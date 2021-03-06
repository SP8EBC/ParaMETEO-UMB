/*
 * main.h
 *
 *  Created on: 25.06.2017
 *      Author: mateusz
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdint.h>

#define _SERIAL_BAUDRATE 19200

#define SYSTICK_TICKS_PER_SECONDS 100
#define SYSTICK_TICKS_PERIOD 10

extern uint32_t master_time;

extern uint16_t main_wx_measuremenets_counter;

extern float ds_t;
extern float ms_t;
extern double ms_p;

//extern volatile uint8_t commTimeoutCounter;



#endif /* MAIN_H_ */
