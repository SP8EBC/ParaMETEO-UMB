/*
 * umb-slave.h
 *
 *  Created on: 01.12.2016
 *      Author: mateusz
 */

#ifndef INCLUDE_DRIVERS_UMB_SLAVE_H_
#define INCLUDE_DRIVERS_UMB_SLAVE_H_

#include <stdint.h>

extern uint8_t umbSlaveState;

#define MESSAGE_PAYLOAD_MAX 100

typedef struct UmbMessage {
	int8_t cmdId;
	int8_t masterClass;
	uint8_t masterId;
	int32_t checksum;
	uint8_t payload[MESSAGE_PAYLOAD_MAX];
	int8_t payloadLn;

} UmbMessage;

typedef struct UmbMeteoData {
	float fTemperature;
	char temperature;
	char humidity;
	unsigned short qfe, qnh;
	float windspeed, windgusts;
	short winddirection;
}UmbMeteoData;

extern UmbMessage umbMessage;

#ifdef __cplusplus
extern "C" {
#endif

char UmbSlaveListen();
char UmbSlavePool();
char UmbStatusRequestResponse(void);
char UmbDeviceInformationRequestResponse(void);
char UmbOnlineDataRequestResponse(UmbMeteoData *pMeteo, char status);
char UmbMultiOnlineDataRequestResponse(UmbMeteoData *pMeteo, char status);

void UmbClearMessageStruct(char b);
short UmbPrepareFrameToSend(UmbMessage* pMessage, char* pUsartBuffer);

unsigned short calc_crc(unsigned short crc_buff, unsigned char input);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_UMB_SLAVE_H_ */
