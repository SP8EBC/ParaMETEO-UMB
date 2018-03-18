/*
 * KissCommunication.c
 *
 *  Created on: 29.04.2017
 *      Author: mateusz
 */

#include "KissCommunication.h"

short SendKISSToHost(KissFrame* in, char* frame, short int frm_len, char* output) {
	#define FEND	(char)0xC0
	#define FESC	(char)0xDB
	#define TFEND	(char)0xDC
	#define TFESC	(char)0xDD
	short int i /* Zmienna do poruszania siê po frame */, j /* zmienna do poruszani siê po data*/;

	char* data;
	if (output != 0x00)
		data = output;
	else
		data = in->data;

	data[0] = FEND;
	data[1] = 0x00;
//	KissFrm.data[2] = HDLC_FLAG;
	for (j = 2, i = 0; i < frm_len; j++, i++) {
		if (*(frame+i) != FEND && *(frame+i) != FESC)
			data[j] = frame[i];
		else if (*(frame+i) == FEND) {
			data[j] = FESC;
			*(data + (j++)) = TFEND;
		}
		else if(*(frame+i) == FESC) {
			data[j] = FESC;
			*(data + (j++)) = TFESC;
		}
		else;
	}
//	*(KissFrm.data + (j++)) = HDLC_FLAG;
	*(data + (j++)) = FEND;
	in->lng = j;
	return j;
}

short ParseReceivedKISS(char* SrlRXData, AX25Ctx* ax25, Afsk* a) {
	int i/* zmienna do poruszania sie po buforze odbiorczym usart */;
	int j/* zmienna do poruszania sie po lokalnej tablicy do przepisywania*/;
	char FrameBuff[100];
	if (*(SrlRXData+1) != 0x00)
		return 1;
	for (i=2, j=0; (i<100 && *(SrlRXData+i) != FEND); i++, j++) {
		if (*(SrlRXData+i) == FESC) {
			if(*(SrlRXData+i+1) == TFEND)
				FrameBuff[j]=FEND;
			else if(*(SrlRXData+i+1) == TFESC)
				FrameBuff[j]=FESC;
			else;
			i++;
		}
		else
			FrameBuff[j] = *(SrlRXData+i);
	}
	ax25_sendRaw(&ax25,FrameBuff,j);
 	afsk_txStart(&a);
	return 0;
}

