/*
 * KissCommunication.h
 *
 *  Created on: 29.04.2017
 *      Author: mateusz
 */

#ifndef KISSCOMMUNICATION_H_
#define KISSCOMMUNICATION_H_

#include "aprs/config.h"
#include "aprs/ax25.h"
#include "aprs/afsk.h"

  typedef struct KissFrame{
  	char data[CONFIG_AX25_FRAME_BUF_LEN+5];
  	short lng;
  }KissFrame;

  /* C++ detection */
  #ifdef __cplusplus
  extern "C" {
  #endif

  short SendKISSToHost(KissFrame* in, char* frame, short int frm_len, char* output);
  short ParseReceivedKISS(char* SrlRXData, AX25Ctx* ax25, Afsk* a);

  #ifdef __cplusplus
  }
  #endif

#endif /* KISSCOMMUNICATION_H_ */
