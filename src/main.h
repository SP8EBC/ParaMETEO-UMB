/*
 * main.h
 *
 *  Created on: 25.06.2017
 *      Author: mateusz
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "aprs/ax25.h"

extern float ds_t;
extern float ms_t;
extern double ms_p;

extern AX25Ctx ax25;
extern Afsk a;
extern AX25Call path[2];
extern uint8_t aprs_msg_len;
extern char aprs_msg[128];



#endif /* MAIN_H_ */
