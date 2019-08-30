/*
 * umb-slave.h
 *
 *  Created on: 01.12.2016
 *      Author: mateusz
 */

#ifndef INCLUDE_DRIVERS_UMB_SLAVE_H_
#define INCLUDE_DRIVERS_UMB_SLAVE_H_

#include <stdint.h>

extern uint8_t umb_slave_state;

#define MESSAGE_PAYLOAD_MAX 100

#define UMB_STATE_UNINITIALIZED 			0
#define UMB_STATE_WAITING_FOR_MESSAGE 		1
#define UMB_STATE_MESSAGE_RXED 				2
#define UMB_STATE_MESSAGE_RXED_WRONG_CRC 	3
#define UMB_STATE_PROCESSING_DONE			4

typedef struct umbMessage_t {
	int8_t cmdId;
	int8_t masterClass;
	uint8_t masterId;
	int32_t checksum;
	uint8_t payload[MESSAGE_PAYLOAD_MAX];
	int8_t payloadLn;

} umbMessage_t;

typedef struct umbMeteoData_t {
	float fTemperature;
	char temperature;
	char humidity;
	unsigned short qfe, qnh;
	float windspeed, windgusts;
	short winddirection;
}umbMeteoData_t;

extern umbMessage_t umb_message;

#ifdef __cplusplus
extern "C" {
#endif

char umb_slave_listen();
char umb_slave_pooling();
char umb_callback_status_request(void);
char umb_callback_device_information_request(void);
char umb_callback_online_data_request(umbMeteoData_t *pMeteo, char status);
char umb_callback_multi_online_data_request(umbMeteoData_t *pMeteo, char status);

void umb_clear_message_struct(char b);
short umb_prepare_frame_to_send(umbMessage_t* pMessage, uint8_t* pUsartBuffer);

unsigned short calc_crc(unsigned short crc_buff, unsigned char input);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_UMB_SLAVE_H_ */
