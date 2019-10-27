/*
 * umb-slave.c
 *
 *  Created on: 01.12.2016
 *      Author: mateusz
 */

#include <rte_umb.h>
#include <string.h>
#include "drivers/umb-slave.h"
#include "drivers/serial.h"
#include <stdint.h>
#include <stdio.h>
#include <umb-slave-config.h>


#define DEV_NAME "ParaMETEO"

#define SOH 0x01
#define STX 0x02
#define ETX 0x03
#define EOT 0x04

#define V10 0x10

#define MASTER_ID 0x01

#define UNSIGNED_CHAR 0x10
#define SIGNED_CHAR 0x11
#define UNSIGNED_SHORT 0x12
#define SIGNED_SHORT 0x13
#define UNSIGNED_LONG 0x14
#define SIGNED_LONG	0x15
#define FLOAT 0x16
#define DOUBLE 0x17

#define CURRENT 0x10
#define MINIMUM 0x11
#define MAXIMUM 0x12
#define AVERAGE 0x13
#define SUM 0x14
#define VECTOR_AVG 0x15

#define FLOAT_SUBLEN	8
#define SINT_SUBLEN		6
#define BYTE_SUBLEN		5

#define get_lsb(x) ((uint8_t)(x & 0xFF))
#define get_msb(x) ((uint8_t)((x & 0xFF00) >> 8))


umbMessage_t umb_message;

uint8_t umb_slave_state = 0;

static uint8_t umb_master_class = 0;
static uint8_t umb_master_id = 0;

volatile int ii = 0;

char umb_slave_listen() {
	if(srl_rx_state == SRL_RX_IDLE || srl_rx_state == SRL_RX_DONE || srl_rx_state == SRL_RX_ERROR) {			// EOT
		srl_receive_data(8, SOH, 0x00, 0, 6, 12);
		umb_clear_message_struct(0);
		umb_slave_state = 1;
		umb_master_class = 0;
		umb_master_id = 0;
		memset(umb_message.payload, 0x00, MESSAGE_PAYLOAD_MAX);
		umb_message.cmdId = 0;
	}
	else
		return -1;
	return 0;
}

char umb_slave_pooling() {
	if (srl_rx_state == SRL_RX_DONE && umb_slave_state == UMB_STATE_WAITING_FOR_MESSAGE) {
		if (srl_get_rx_buffer()[2] == DEV_ID && srl_get_rx_buffer()[3] == DEV_CLASS) {
			// sprawdzanie czy odebrana ramka jest zaadresownana do tego urządzenia
			int16_t ln = srl_get_rx_buffer()[6];
			uint16_t crc = 0xFFFF;

			for (int i = 0; i < 9 + ln; i++) {
				crc = calc_crc(crc, srl_get_rx_buffer()[i]);
			}

			uint8_t crc_l = crc & 0xFF;
			uint8_t crc_h = ((crc & 0xFF00) >> 8);

			if ( crc_l == srl_get_rx_buffer()[9 + ln] && crc_h == (srl_get_rx_buffer()[10 + ln]) ) {

				umb_message.masterId = srl_get_rx_buffer()[4];			// store master ID which requests something
				umb_message.masterClass = (srl_get_rx_buffer()[5] >> 4) & 0x0F;

				umb_message.cmdId = srl_get_rx_buffer()[8];		// command ID
				if (ln > 2)
					memcpy(umb_message.payload, srl_get_rx_buffer() + 10, ln - 2); // długość len - cmd - verc
				umb_message.payloadLn = ln - 2;
				umb_message.checksum = crc;

				rte_umb_correct_messages++;
				umb_slave_state = UMB_STATE_MESSAGE_RXED;
			}
			else {
				rte_umb_wrong_crc++;
				umb_slave_state = UMB_STATE_MESSAGE_RXED_WRONG_CRC;
				}
			}
		else {
			// jeżeli nie jest zaadresowana to zignoruj i słuchaj dalej
//			trace_printf("UmbSlave: Data addressed not to this device\n"), UmbSlaveListen();
			rte_umb_not_addresed++;
			for (ii = 0; ii <= 0x2FFFF; ii++);
			umb_slave_listen();
		}
	}
		else {
			;
		}


	return 0;
}

char umb_callback_status_request_0x26(void) {
//	UmbClearMessageStruct();
	umb_message.cmdId = 0x26;
	umb_message.payload[0] = 0x00;
	umb_message.payload[1] = 0x00;
	umb_message.payloadLn = 2;
	return 0;
}

char umb_return_status(char cmdId, char status) {
	umb_clear_message_struct(0);
	umb_message.payload[0] = status;
	umb_message.cmdId = cmdId;
	return 0;
}

char umb_callback_device_information_0x2d(void) {
  	uint16_t ch = 0;
  	int8_t temp = 0;
  	uint16_t i = 0;
	umb_message.payloadLn = 0;
	switch(umb_message.payload[0]) {
		case 0x10:
			umb_clear_message_struct(0);
			umb_message.cmdId = 0x2D;
			umb_message.payload[0] = 0x00;
			umb_message.payload[1] = 0x10;
			umb_message.payloadLn =  (sprintf(umb_message.payload + 2, DEV_NAME)) + 2;
			break;

		case 0x11:
			umb_clear_message_struct(0);
			umb_message.cmdId = 0x2D;
			umb_message.payload[0] = 0x00;
			umb_message.payload[1] = 0x11;
			umb_message.payloadLn =  (sprintf(umb_message.payload + 2, "Opis")) + 2;
			break;

		case 0x12:
			umb_clear_message_struct(0);
			umb_message.cmdId = 0x2D;
			umb_message.payload[0] = 0x00;
			umb_message.payload[1] = 0x12;
			umb_message.payload[2] = 0x0A;	// hardware	version
			umb_message.payload[3] = 0x0A;	// software	version
			umb_message.payloadLn = 4;
			break;

		case 0x13:
			umb_clear_message_struct(0);
			umb_message.cmdId = 0x2D;
			umb_message.payload[0] = 0x00;
			umb_message.payload[1] = 0x13;
			umb_message.payload[2] = 0x11; // Serial LSB
			umb_message.payload[3] = 0x11; // Serial MSB
			umb_message.payload[4] = 0x26; // MMYY LSB
			umb_message.payload[5] = 0x10; // MMYY MSB
			umb_message.payload[6] = 0x02;
			umb_message.payload[7] = 0x00;
			umb_message.payload[8] = 0x01; // PartsList
			umb_message.payload[9] = 0x01; // PartsPlan
			umb_message.payload[10] = 0x0A;	// hardware
			umb_message.payload[11] = SW_VERSION;	// software
			umb_message.payload[12] = 0x0A;	// e2version
			umb_message.payload[13] = 0x0A; // DeviceVersion LSB
			umb_message.payload[14] = 0x00;
			umb_message.payloadLn = 15;
			break;

		case 0x14:
			umb_clear_message_struct(0);
			umb_message.cmdId = 0x2D;
			umb_message.payload[0] = 0x00;
			umb_message.payload[1] = 0x14;
			umb_message.payload[2] = 0xFF; // LSB
			umb_message.payload[3] = 0xFF; // MSB
			umb_message.payloadLn = 4;
			break;

		case 0x15:
			umb_clear_message_struct(0);
			umb_message.cmdId = 0x2D;
			umb_message.payload[0] = 0x00;	// zawsze 0x00 -- satus
			umb_message.payload[1] = 0x15;	// zawsze info
			umb_message.payload[2] = CH_CHANNEL_CNT; // liczba kanalow LSB 0x01
			umb_message.payload[3] = 0x00; // liczba kanalow MSB
			umb_message.payload[4] = 0x01; // liczba blokow
			umb_message.payloadLn = 5;
			break;

		case 0x16:
			if (umb_message.payload[1] != 0x00) {
				// nie ma blokow kanalow powyzej 0
			}
			else {

				umb_clear_message_struct(0);
				umb_message.cmdId = 0x2D;
				umb_message.payload[i++] = 0x00; // zawsze 0x00
				umb_message.payload[i++] = 0x16; // zawsze info
				umb_message.payload[i++] = 0x00; // option -> block number
				umb_message.payload[i++] = CH_CHANNEL_CNT; // liczba kanalow 0x08
				umb_message.payload[i++] = get_lsb(CH_CURRENT_TEMPERATURE_INT); // kanal 100 LSB
				umb_message.payload[i++] = get_msb(CH_CURRENT_TEMPERATURE_INT); // kanal 100 MSB
				umb_message.payload[i++] = get_lsb(CH_CURRENT_TEMPERATURE_FLOAT); // kanal 101 LSB
				umb_message.payload[i++] = get_msb(CH_CURRENT_TEMPERATURE_FLOAT); // kanal 101 MSB
				umb_message.payload[i++] = get_lsb(CH_MIN_TEMPERATURE_INT); // kanal 120 LSB
				umb_message.payload[i++] = get_msb(CH_MIN_TEMPERATURE_INT); // kanal 120 MSB
				umb_message.payload[i++] = get_lsb(CH_MIN_TEMPERATURE_FLOAT); // kanal 121 LSB
				umb_message.payload[i++] = get_msb(CH_MIN_TEMPERATURE_FLOAT); // kanal 121 MSB
				umb_message.payload[i++] = get_lsb(CH_MAX_TEMPERATURE_INT); // kanal 140 LSB
				umb_message.payload[i++] = get_msb(CH_MAX_TEMPERATURE_INT); // kanal 140 MSB
				umb_message.payload[i++] = get_lsb(CH_MAX_TEMPERATURE_FLOAT); // kanal 141 LSB
				umb_message.payload[i++] = get_msb(CH_MAX_TEMPERATURE_FLOAT); // kanal 141 MSB
				umb_message.payload[i++] = get_lsb(CH_AVERAGE_TEMPERATURE_INT); // kanal 160 LSB
				umb_message.payload[i++] = get_msb(CH_AVERAGE_TEMPERATURE_INT); // kanal 160 MSB
				umb_message.payload[i++] = get_lsb(CH_AVERAGE_TEMPERATURE_FLOAT); // kanal 161 LSB
				umb_message.payload[i++] = get_msb(CH_AVERAGE_TEMPERATURE_FLOAT); // kanal 161 MSB
				umb_message.payload[i++] = get_lsb(CH_CURRENT_HUMIDITY); // kanal 200 LSB
				umb_message.payload[i++] = get_msb(CH_CURRENT_HUMIDITY); // kanal 200 MSB
				umb_message.payload[i++] = get_lsb(CH_CURRENT_QFE_INT); // kanal 300 LSB
				umb_message.payload[i++] = get_msb(CH_CURRENT_QFE_INT); // kanal 300 MSB
				umb_message.payload[i++] = get_lsb(CH_CURRENT_QFE_FLOAT); // kanal 301 LSB
				umb_message.payload[i++] = get_msb(CH_CURRENT_QFE_FLOAT); // kanal 301 MSB
				umb_message.payload[i++] = get_lsb(CH_CURRENT_QNH_INT); // kanal 305 LSB
				umb_message.payload[i++] = get_msb(CH_CURRENT_QNH_INT); // kanal 305 MSB
				umb_message.payload[i++] = get_lsb(CH_CURRENT_QNH_FLOAT); // kanal 306 LSB
				umb_message.payload[i++] = get_msb(CH_CURRENT_QNH_FLOAT); // kanal 306 MSB
				umb_message.payload[i++] = get_lsb(CH_MIN_WIND_MS); // kanal 420 LSB
				umb_message.payload[i++] = get_msb(CH_MIN_WIND_MS); // kanal 420 MSB
				umb_message.payload[i++] = get_lsb(CH_MAX_WIND_MS); // kanal 440 LSB
				umb_message.payload[i++] = get_msb(CH_MAX_WIND_MS); // kanal 440 MSB
				umb_message.payload[i++] = get_lsb(CH_AVERGE_WIND_MS); // kanal 460 LSB
				umb_message.payload[i++] = get_msb(CH_AVERGE_WIND_MS);; // kanal 460 MSB
				umb_message.payload[i++] = get_lsb(CH_AVERAGE_WIND_DIR); // kanal 560 LSB
				umb_message.payload[i++] = get_msb(CH_AVERAGE_WIND_DIR); // kanal 560 MSB
				umb_message.payload[i++] = get_lsb(CH_AVERAGE_WIND_VCT_DIR); // kanal 580 LSB
				umb_message.payload[i++] = get_msb(CH_AVERAGE_WIND_VCT_DIR); // kanal 580 MSB
				umb_message.payloadLn = i;
			}
			break;

		case 0x20:
			ch = umb_message.payload[1] | umb_message.payload[2] << 8;
			ch &= 0xFFFF;
			umb_clear_message_struct(0);
			umb_message.cmdId = 0x2D;
			umb_message.payload[0] = 0x00;	// zawsze 0x00
			umb_message.payload[1] = 0x20;	// zawsze info
			switch (ch) {
				default: break;
			}
			//umbMessage.payloadLn =  (sprintf(umbMessage.payload + 2, "ParaMETEO")) + 2;
			break;

		case 0x30:
			umb_message.payloadLn = 0;
			ch = 0;
			ch = umb_message.payload[1] | (umb_message.payload[2] << 8);
			umb_clear_message_struct(0);
			umb_message.cmdId = 0x2D;
			umb_message.payload[0] = 0x00;	// zawsze 0x00
			umb_message.payload[1] = 0x30;	// zawsze info
			switch (ch) {
				case CH_CURRENT_TEMPERATURE_FLOAT:
					umb_message.payload[2] = get_lsb(CH_CURRENT_TEMPERATURE_FLOAT);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_CURRENT_TEMPERATURE_FLOAT);	// kanal MSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Temperatura         ")) + 4; // 20
					temp = sprintf(umb_message.payload + umb_message.payloadLn, "StopnieC       "); // 15
					umb_message.payloadLn += temp;
					#ifdef CH_SWAP_TEMP_CURRENT_WITH_AVG
					umb_message.payload[umb_message.payloadLn++] = AVERAGE; // average value
					#else
					umb_message.payload[umb_message.payloadLn++] = CURRENT; // current value
					#endif
					umb_message.payload[umb_message.payloadLn++] = FLOAT;
					umb_message.payloadLn = umb_insert_float_to_buffer(-127.0f, umb_message.payload, umb_message.payloadLn);
					umb_message.payloadLn = umb_insert_float_to_buffer(127.0f, umb_message.payload, umb_message.payloadLn);
					break;

				case CH_CURRENT_TEMPERATURE_INT:
					umb_message.payload[2] = get_lsb(CH_CURRENT_TEMPERATURE_INT);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_CURRENT_TEMPERATURE_INT);	// kanal MSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Temperatura         ")) + 4; // 20
					temp = sprintf(umb_message.payload + umb_message.payloadLn, "StopnieC       "); // 15
					umb_message.payloadLn += temp;
					#ifdef CH_SWAP_TEMP_CURRENT_WITH_AVG
					umb_message.payload[umb_message.payloadLn++] = AVERAGE; // average value
					#else
					umb_message.payload[umb_message.payloadLn++] = CURRENT; // current value
					#endif
					umb_message.payload[umb_message.payloadLn++] = SIGNED_CHAR;
					umb_message.payload[umb_message.payloadLn++] = (uint8_t)-127;		// minimum
					umb_message.payload[umb_message.payloadLn++] = 127;
					break;

				case CH_MIN_TEMPERATURE_FLOAT:
					umb_message.payload[2] = get_lsb(CH_MIN_TEMPERATURE_FLOAT);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_MIN_TEMPERATURE_FLOAT);	// kanal MSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Temperatura         ")) + 4; // 20
					temp = sprintf(umb_message.payload + umb_message.payloadLn, "StopnieC       "); // 15
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = MINIMUM; // average value
					umb_message.payload[umb_message.payloadLn++] = FLOAT;
					umb_message.payloadLn = umb_insert_float_to_buffer(-127.0f, umb_message.payload, umb_message.payloadLn);
					umb_message.payloadLn = umb_insert_float_to_buffer(127.0f, umb_message.payload, umb_message.payloadLn);
					break;

				case CH_MIN_TEMPERATURE_INT:
					umb_message.payload[2] = get_lsb(CH_MIN_TEMPERATURE_INT);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_MIN_TEMPERATURE_INT);	// kanal MSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Temperatura         ")) + 4; // 20
					temp = sprintf(umb_message.payload + umb_message.payloadLn, "StopnieC       "); // 15
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = MINIMUM; // average value
					umb_message.payload[umb_message.payloadLn++] = SIGNED_CHAR;
					umb_message.payload[umb_message.payloadLn++] = (uint8_t)-127;		// minimum
					umb_message.payload[umb_message.payloadLn++] = 127;
					break;

				case CH_MAX_TEMPERATURE_FLOAT:
					umb_message.payload[2] = get_lsb(CH_MAX_TEMPERATURE_FLOAT);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_MAX_TEMPERATURE_FLOAT);	// kanal MSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Temperatura         ")) + 4; // 20
					temp = sprintf(umb_message.payload + umb_message.payloadLn, "StopnieC       "); // 15
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = MAXIMUM; // average value
					umb_message.payload[umb_message.payloadLn++] = FLOAT;
					umb_message.payloadLn = umb_insert_float_to_buffer(-127.0f, umb_message.payload, umb_message.payloadLn);
					umb_message.payloadLn = umb_insert_float_to_buffer(127.0f, umb_message.payload, umb_message.payloadLn);
					break;

				case CH_MAX_TEMPERATURE_INT:
					umb_message.payload[2] = get_lsb(CH_MAX_TEMPERATURE_INT);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_MAX_TEMPERATURE_INT);	// kanal MSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Temperatura         ")) + 4; // 20
					temp = sprintf(umb_message.payload + umb_message.payloadLn, "StopnieC       "); // 15
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = MAXIMUM; // average value
					umb_message.payload[umb_message.payloadLn++] = SIGNED_CHAR;
					umb_message.payload[umb_message.payloadLn++] = (uint8_t)-127;		// minimum
					umb_message.payload[umb_message.payloadLn++] = 127;
					break;

				case CH_AVERAGE_TEMPERATURE_FLOAT:
					umb_message.payload[2] = get_lsb(CH_AVERAGE_TEMPERATURE_FLOAT);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_AVERAGE_TEMPERATURE_FLOAT);	// kanal MSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Temperatura         ")) + 4; // 20
					temp = sprintf(umb_message.payload + umb_message.payloadLn, "StopnieC       "); // 15
					umb_message.payloadLn += temp;
					#ifndef CH_SWAP_TEMP_CURRENT_WITH_AVG
					umb_message.payload[umb_message.payloadLn++] = AVERAGE; // average value
					#else
					umb_message.payload[umb_message.payloadLn++] = CURRENT; // current value
					#endif
					umb_message.payload[umb_message.payloadLn++] = FLOAT;
					umb_message.payloadLn = umb_insert_float_to_buffer(-127.0f, umb_message.payload, umb_message.payloadLn);
					umb_message.payloadLn = umb_insert_float_to_buffer(127.0f, umb_message.payload, umb_message.payloadLn);
					break;

				case CH_AVERAGE_TEMPERATURE_INT:
					umb_message.payload[2] = get_lsb(CH_AVERAGE_TEMPERATURE_INT);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_AVERAGE_TEMPERATURE_INT);	// kanal MSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Temperatura         ")) + 4; // 20
					temp = sprintf(umb_message.payload + umb_message.payloadLn, "StopnieC       "); // 15
					umb_message.payloadLn += temp;
					#ifndef CH_SWAP_TEMP_CURRENT_WITH_AVG
					umb_message.payload[umb_message.payloadLn++] = AVERAGE; // average value
					#else
					umb_message.payload[umb_message.payloadLn++] = CURRENT; // current value
					#endif
					umb_message.payload[umb_message.payloadLn++] = SIGNED_CHAR;
					umb_message.payload[umb_message.payloadLn++] = (uint8_t)-127;		// minimum
					umb_message.payload[umb_message.payloadLn++] = 127;
					break;


				case CH_CURRENT_HUMIDITY:
					umb_message.payload[2] = get_lsb(CH_CURRENT_HUMIDITY);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_CURRENT_HUMIDITY);	// kanal MSB
					umb_message.payloadLn = (sprintf(umb_message.payload + 4, "Wilgotnosc          ")) + 4;
					temp = (sprintf(umb_message.payload + umb_message.payloadLn, "%%              "));
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = CURRENT; // current value
					umb_message.payload[umb_message.payloadLn++] = SIGNED_CHAR;
					umb_message.payload[umb_message.payloadLn++] = 1;		// minimum
					umb_message.payload[umb_message.payloadLn++] = 100;
					break;

				case CH_CURRENT_QFE_FLOAT:
					umb_message.payload[2] = get_lsb(CH_CURRENT_QFE_FLOAT);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_CURRENT_QFE_FLOAT);	// kanal MSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Cisnienie QFE       ")) + 4;
					temp =  (sprintf(umb_message.payload + umb_message.payloadLn, "hPa            "));
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = CURRENT; // current value
					umb_message.payload[umb_message.payloadLn++] = FLOAT;
					umb_message.payloadLn = umb_insert_float_to_buffer(500.0f, umb_message.payload, umb_message.payloadLn);
					umb_message.payloadLn = umb_insert_float_to_buffer(1300.0f, umb_message.payload, umb_message.payloadLn);
					break;

				case CH_CURRENT_QFE_INT:
					umb_message.payload[2] = get_lsb(CH_CURRENT_QFE_INT);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_CURRENT_QFE_INT);	// kanal MSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Cisnienie QFE       ")) + 4;
					temp =  (sprintf(umb_message.payload + umb_message.payloadLn, "hPa            "));
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = CURRENT; // current value
					umb_message.payload[umb_message.payloadLn++] = UNSIGNED_SHORT;
					umb_message.payloadLn = umb_insert_sint_to_buffer(500, umb_message.payload, umb_message.payloadLn);
					umb_message.payloadLn = umb_insert_sint_to_buffer(1300, umb_message.payload, umb_message.payloadLn);
					break;

				case CH_CURRENT_QNH_FLOAT:
					umb_message.payload[2] = get_lsb(CH_CURRENT_QNH_FLOAT);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_CURRENT_QNH_FLOAT);;	// kanal LSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Cisnienie QNH       ")) + 4;
					temp =  (sprintf(umb_message.payload + umb_message.payloadLn, "hPa            "));
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = CURRENT; // current value
					umb_message.payload[umb_message.payloadLn++] = FLOAT;
					umb_message.payloadLn = umb_insert_float_to_buffer(500.0f, umb_message.payload, umb_message.payloadLn);
					umb_message.payloadLn = umb_insert_float_to_buffer(1300.0f, umb_message.payload, umb_message.payloadLn);
					break;

				case CH_CURRENT_QNH_INT:
					umb_message.payload[2] = get_lsb(CH_CURRENT_QNH_INT);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_CURRENT_QNH_INT);	// kanal MSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Cisnienie QNH       ")) + 4;
					temp =  (sprintf(umb_message.payload + umb_message.payloadLn, "hPa            "));
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = CURRENT; // current value
					umb_message.payload[umb_message.payloadLn++] = UNSIGNED_SHORT;
					umb_message.payloadLn = umb_insert_sint_to_buffer(500, umb_message.payload, umb_message.payloadLn);
					umb_message.payloadLn = umb_insert_sint_to_buffer(1300, umb_message.payload, umb_message.payloadLn);
					break;

				case CH_AVERGE_WIND_MS:
					umb_message.payload[2] = get_lsb(CH_AVERGE_WIND_MS);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_AVERGE_WIND_MS);;	// kanal LSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Srednia predkosc    ")) + 4;
					temp =  (sprintf(umb_message.payload + umb_message.payloadLn, "m/s            "));
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = AVERAGE; // current value
					umb_message.payload[umb_message.payloadLn++] = FLOAT;
					umb_message.payloadLn = umb_insert_float_to_buffer(0.0f, umb_message.payload, umb_message.payloadLn);
					umb_message.payloadLn = umb_insert_float_to_buffer(40.0f, umb_message.payload, umb_message.payloadLn);
					break;

				case CH_MIN_WIND_MS:
					umb_message.payload[2] = get_lsb(CH_MIN_WIND_MS);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_MIN_WIND_MS);	// kanal LSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Minimalna predkosc  ")) + 4;
					temp =  (sprintf(umb_message.payload + umb_message.payloadLn, "m/s            "));
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = MINIMUM; // current value
					umb_message.payload[umb_message.payloadLn++] = FLOAT;
					umb_message.payloadLn = umb_insert_float_to_buffer(0.0f, umb_message.payload, umb_message.payloadLn);
					umb_message.payloadLn = umb_insert_float_to_buffer(40.0f, umb_message.payload, umb_message.payloadLn);
					break;

				case CH_MAX_WIND_MS:
					umb_message.payload[2] = get_lsb(CH_MAX_WIND_MS);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_MAX_WIND_MS);	// kanal LSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Porywy wiatru       ")) + 4;
					temp =  (sprintf(umb_message.payload + umb_message.payloadLn, "m/s            "));
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = MAXIMUM; // current value
					umb_message.payload[umb_message.payloadLn++] = FLOAT;
					umb_message.payloadLn = umb_insert_float_to_buffer(0.0f, umb_message.payload, umb_message.payloadLn);
					umb_message.payloadLn = umb_insert_float_to_buffer(40.0f, umb_message.payload, umb_message.payloadLn);
					break;

				case CH_AVERAGE_WIND_DIR:
					umb_message.payload[2] = get_lsb(CH_AVERAGE_WIND_DIR);	// kanal LSB
					umb_message.payload[3] = get_msb(CH_AVERAGE_WIND_DIR);	// kanal LSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Kierunek wiatru     ")) + 4;
//					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					temp =  (sprintf(umb_message.payload + umb_message.payloadLn, "stopnie        "));
					umb_message.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umb_message.payload[umb_message.payloadLn++] = VECTOR_AVG; // current value
					umb_message.payload[umb_message.payloadLn++] = UNSIGNED_SHORT;
					umb_message.payloadLn = umb_insert_sint_to_buffer(0, umb_message.payload, umb_message.payloadLn);
					umb_message.payloadLn = umb_insert_sint_to_buffer(360, umb_message.payload, umb_message.payloadLn);
					break;

				case CH_AVERAGE_WIND_VCT_DIR:
					umb_message.payload[2] = 0x44;	// kanal LSB
					umb_message.payload[3] = 0x02;	// kanal LSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Kierunek wiatru     ")) + 4;
					temp =  (sprintf(umb_message.payload + umb_message.payloadLn, "s              "));
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = VECTOR_AVG; // current value
					umb_message.payload[umb_message.payloadLn++] = UNSIGNED_SHORT;
					umb_message.payloadLn = umb_insert_sint_to_buffer(0, umb_message.payload, umb_message.payloadLn);
					umb_message.payloadLn = umb_insert_sint_to_buffer(360, umb_message.payload, umb_message.payloadLn);
					break;

					default:
						break;



			}
			break;

		default:
			break;
	}

	return 0;
}

char umb_callback_online_data_request_0x23(umbMeteoData_t *pMeteo, char status) {
	uint16_t ch = (umb_message.payload[0] | umb_message.payload[1] << 8);
	uint8_t ch_lsb = umb_message.payload[0];
	uint8_t ch_msb = umb_message.payload[1];
	umb_clear_message_struct(0);
	umb_message.cmdId = 0x23;
	void* val;
	////
	umb_message.payload[0] = status;
	umb_message.payload[1] = ch_lsb;
	umb_message.payload[2] = ch_msb;
	switch (ch) { // switch for <value>
	case CH_CURRENT_TEMPERATURE_FLOAT:
		#ifdef CH_SWAP_TEMP_CURRENT_WITH_AVG
			val = &pMeteo->float_avg_temperature; break;
		#else
			val = &pMeteo->float_temperature; break;
		#endif
	case CH_CURRENT_TEMPERATURE_INT:
		#ifdef CH_SWAP_TEMP_CURRENT_WITH_AVG
			val = &pMeteo->avg_temperature; break;
		#else
			val = &pMeteo->temperature; break;
		#endif
	case CH_MIN_TEMPERATURE_FLOAT:
		val = &pMeteo->float_min_temperature; break;
	case CH_MAX_TEMPERATURE_FLOAT:
		val = &pMeteo->float_max_temperature; break;
	case CH_AVERAGE_TEMPERATURE_FLOAT:
		#ifdef CH_SWAP_TEMP_CURRENT_WITH_AVG
			val = &pMeteo->float_temperature; break;
		#else
			val = &pMeteo->float_avg_temperature; break;
		#endif
	case CH_AVERAGE_TEMPERATURE_INT:
		#ifdef CH_SWAP_TEMP_CURRENT_WITH_AVG
			val = &pMeteo->temperature; break;
		#else
			val = &pMeteo->avg_temperature; break;
		#endif
	case CH_MIN_TEMPERATURE_INT:
		val = &pMeteo->min_temperature; break;
	case CH_MAX_TEMPERATURE_INT:
		val = &pMeteo->max_temperature; break;
	case CH_CURRENT_HUMIDITY:
		val = &pMeteo->humidity; break;
	case CH_CURRENT_QFE_FLOAT:
		val = &pMeteo->qfe; break;
	case CH_CURRENT_QNH_FLOAT:
		val = &pMeteo->qnh; break;
	case CH_MAX_WIND_MS:
		val = &pMeteo->windgusts; break;
	case CH_AVERGE_WIND_MS:
		val = &pMeteo->windspeed; break;
	case CH_AVERAGE_WIND_DIR:
		val = &pMeteo->winddirection; break;
	case CH_AVERAGE_WIND_VCT_DIR:
		val = &pMeteo->winddirection; break;
	case CH_CURRENT_QFE_INT:
		val = &pMeteo->sqfe; break;
	case CH_CURRENT_QNH_INT:
		val = &pMeteo->sqfe; break;
	default: return -1;

	}
	switch (ch) {	// switch for <type>
		case CH_CURRENT_HUMIDITY:
			umb_message.payload[3] = SIGNED_CHAR;
			umb_message.payload[4] = *(int8_t*)val;
			umb_message.payloadLn = 5;
			break;
		case CH_CURRENT_QFE_FLOAT:
		case CH_CURRENT_QNH_FLOAT:
		case CH_CURRENT_TEMPERATURE_FLOAT:
		case CH_AVERAGE_TEMPERATURE_FLOAT:
		case CH_MIN_TEMPERATURE_FLOAT:
		case CH_MAX_TEMPERATURE_FLOAT:
		case CH_MAX_WIND_MS:
		case CH_AVERGE_WIND_MS:
			umb_message.payload[3] = FLOAT;
			umb_message.payload[4] = *(int32_t*)val & 0xFF;
			umb_message.payload[5] = (*(int32_t*)val & 0xFF00) >> 8;
			umb_message.payload[6] = (*(int32_t*)val & 0xFF0000) >> 16;
			umb_message.payload[7] = (*(int32_t*)val & 0xFF000000) >> 24;
			umb_message.payloadLn = 8;
			break;
		case CH_AVERAGE_WIND_DIR:
		case CH_AVERAGE_WIND_VCT_DIR:
		case CH_CURRENT_QFE_INT:
		case CH_CURRENT_QNH_INT:
			umb_message.payload[3] = UNSIGNED_SHORT;
			umb_message.payload[4] = *(uint16_t*)val & 0xFF;
			umb_message.payload[5] = (*(uint16_t*)val & 0xFF00) >> 8;
			umb_message.payloadLn = 6;
			break;
		case CH_CURRENT_TEMPERATURE_INT:
		case CH_AVERAGE_TEMPERATURE_INT:
		case CH_MIN_TEMPERATURE_INT:
		case CH_MAX_TEMPERATURE_INT:
			umb_message.payload[3] = UNSIGNED_CHAR;
			umb_message.payload[4] = *(uint8_t*)val & 0xFF;
			umb_message.payloadLn = 5;
			break;

	default: return -1;
	}

	return 0;
}

char umb_callback_multi_online_data_request_0x2f(umbMeteoData_t *pMeteo, char status) {
	// The UMB Config Tool uses this service to query for measuremenets even if only one value
	// is monitored
	uint8_t chann = umb_message.payload[0];
	uint8_t total_payload_len = 0;
	uint16_t channels[4] = {0, 0, 0, 0};
	uint8_t j = 0;
	void* val;

	if (chann > 4)
	{
		umb_clear_message_struct(0);
		return -1;
	}

	for (int i = 0; i < chann; i++)
	{
		// rewriting channels requested by the tester
		channels[i] = (umb_message.payload[j+1] | umb_message.payload[j+2] << 8);
		j += 2;
	}

	umb_clear_message_struct(0);
	j = 0;

	umb_message.cmdId = 0x2F;
	umb_message.payload[0] = status;
	umb_message.payload[1] = chann;
	umb_message.payloadLn = 2;
	total_payload_len = 2;

	for (int i = 0; i < chann; i++)
	{
		switch (channels[i]) { // switch for <value>
		case CH_CURRENT_TEMPERATURE_FLOAT:
			#ifdef CH_SWAP_TEMP_CURRENT_WITH_AVG
				val = &pMeteo->float_avg_temperature; break;
			#else
				val = &pMeteo->float_temperature; break;
			#endif
		case CH_CURRENT_TEMPERATURE_INT:
			#ifdef CH_SWAP_TEMP_CURRENT_WITH_AVG
				val = &pMeteo->avg_temperature; break;
			#else
				val = &pMeteo->temperature; break;
			#endif
		case CH_MIN_TEMPERATURE_FLOAT:
			val = &pMeteo->float_min_temperature; break;
		case CH_MAX_TEMPERATURE_FLOAT:
			val = &pMeteo->float_max_temperature; break;
		case CH_AVERAGE_TEMPERATURE_FLOAT:
			#ifdef CH_SWAP_TEMP_CURRENT_WITH_AVG
				val = &pMeteo->float_temperature; break;
			#else
				val = &pMeteo->float_avg_temperature; break;
			#endif
		case CH_AVERAGE_TEMPERATURE_INT:
			#ifdef CH_SWAP_TEMP_CURRENT_WITH_AVG
				val = &pMeteo->temperature; break;
			#else
				val = &pMeteo->avg_temperature; break;
			#endif
		case CH_MIN_TEMPERATURE_INT:
			val = &pMeteo->min_temperature; break;
		case CH_MAX_TEMPERATURE_INT:
			val = &pMeteo->max_temperature; break;
		case CH_CURRENT_HUMIDITY:
			val = &pMeteo->humidity; break;
		case CH_CURRENT_QFE_FLOAT:
			val = &pMeteo->qfe; break;
		case CH_CURRENT_QNH_FLOAT:
			val = &pMeteo->qnh; break;
		case CH_MAX_WIND_MS:
			val = &pMeteo->windgusts; break;
		case CH_AVERGE_WIND_MS:
			val = &pMeteo->windspeed; break;
		case CH_AVERAGE_WIND_DIR:
			val = &pMeteo->winddirection; break;
		case CH_AVERAGE_WIND_VCT_DIR:
			val = &pMeteo->winddirection; break;
		case CH_CURRENT_QFE_INT:
			val = &pMeteo->sqfe; break;
		case CH_CURRENT_QNH_INT:
			val = &pMeteo->sqfe; break;
		default: return -1;

		}

		switch (channels[i]) {	// switch for <type>
		case CH_CURRENT_HUMIDITY:
			total_payload_len = umb_insert_byte_to_buffer(BYTE_SUBLEN, umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_byte_to_buffer(0x00, umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_sint_to_buffer(channels[i], umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_byte_to_buffer(SIGNED_CHAR, umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_byte_to_buffer(*(int8_t*)val, umb_message.payload, total_payload_len);
			break;
		case CH_CURRENT_QFE_FLOAT:
		case CH_CURRENT_QNH_FLOAT:
		case CH_CURRENT_TEMPERATURE_FLOAT:
		case CH_AVERAGE_TEMPERATURE_FLOAT:
		case CH_MIN_TEMPERATURE_FLOAT:
		case CH_MAX_TEMPERATURE_FLOAT:
		case CH_MAX_WIND_MS:
		case CH_AVERGE_WIND_MS:
			total_payload_len = umb_insert_byte_to_buffer(FLOAT_SUBLEN, umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_byte_to_buffer(0x00, umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_sint_to_buffer(channels[i], umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_byte_to_buffer(FLOAT, umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_float_to_buffer(*(float*)val, umb_message.payload, total_payload_len);
			break;
		case CH_AVERAGE_WIND_DIR:
		case CH_AVERAGE_WIND_VCT_DIR:
		case CH_CURRENT_QFE_INT:
		case CH_CURRENT_QNH_INT:
			total_payload_len = umb_insert_byte_to_buffer(SINT_SUBLEN, umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_byte_to_buffer(0x00, umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_sint_to_buffer(channels[i], umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_byte_to_buffer(UNSIGNED_SHORT, umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_sint_to_buffer(*(int16_t*)val, umb_message.payload, total_payload_len);
			break;
		case CH_CURRENT_TEMPERATURE_INT:
		case CH_AVERAGE_TEMPERATURE_INT:
		case CH_MIN_TEMPERATURE_INT:
		case CH_MAX_TEMPERATURE_INT:			total_payload_len = umb_insert_byte_to_buffer(BYTE_SUBLEN, umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_byte_to_buffer(0x00, umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_sint_to_buffer(channels[i], umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_byte_to_buffer(SIGNED_CHAR, umb_message.payload, total_payload_len);
			total_payload_len = umb_insert_byte_to_buffer(*(int8_t*)val, umb_message.payload, total_payload_len);
			break;


		default: return -1;
		}

	}
	umb_message.payloadLn = total_payload_len;
	return 0;


}

void umb_clear_message_struct(char b) {
	if (b) {
		umb_message.masterClass = 0;
		umb_message.masterId = 0;
	}
	umb_message.checksum = 0;
	umb_message.cmdId = 0;
	memset(umb_message.payload, 0x00, MESSAGE_PAYLOAD_MAX);
}

short umb_prepare_frame_to_send(umbMessage_t* pMessage, uint8_t* pUsartBuffer) {
	unsigned short crc = 0xFFFF;
	short j = 0;

	memset(pUsartBuffer, 0x00, 128);
	pUsartBuffer[j++] = SOH;
	pUsartBuffer[j++] = V10;
	pUsartBuffer[j++] = pMessage->masterId;
	pUsartBuffer[j++] = pMessage->masterClass << 4;
	pUsartBuffer[j++] = DEV_ID;
	pUsartBuffer[j++] = DEV_CLASS;
	pUsartBuffer[j++] = pMessage->payloadLn + 0x02;
	pUsartBuffer[j++] = STX;
	pUsartBuffer[j++] = pMessage->cmdId;
	pUsartBuffer[j++] = V10;
	for( int i = 0; i < pMessage->payloadLn; i++)
		pUsartBuffer[j++] = pMessage->payload[i];
	pUsartBuffer[j++] = ETX;
	for( int i = 0; i < j; i++)
		crc = calc_crc(crc, pUsartBuffer[i]);
	pUsartBuffer[j++] = crc & 0xFF;
	pUsartBuffer[j++] = ((crc & 0xFF00) >> 8);
	pUsartBuffer[j++] = EOT;

	pMessage->checksum = crc;

	return j;
}

unsigned short calc_crc(unsigned short crc_buff, unsigned char input) {
	unsigned char i;
	unsigned short x16;
	for	(i=0; i<8; i++)
	{
		// XOR current D0 and next input bit to determine x16 value
		if		( (crc_buff & 0x0001) ^ (input & 0x01) )
			x16 = 0x8408;
		else
			x16 = 0x0000;
		// shift crc buffer
		crc_buff = crc_buff >> 1;
		// XOR in the x16 value
		crc_buff ^= x16;
		// shift input for next iteration
		input = input >> 1;
	}
	return (crc_buff);
}

uint8_t umb_insert_float_to_buffer(float value, uint8_t* output,
		uint8_t output_ptr) {

	uint8_t ptr = output_ptr;
	uint32_t val = *((uint32_t*)&value);

	*(output+ (ptr++)) = (uint8_t)( val & 0xFF);
	*(output+ (ptr++)) = (uint8_t)((val & 0xFF00) >> 8);
	*(output+ (ptr++)) = (uint8_t)((val & 0xFF0000) >> 16);
	*(output+ (ptr++)) = (uint8_t)((val & 0xFF000000) >> 24);

	return ptr;
}

uint8_t umb_insert_int_to_buffer(int32_t value, uint8_t* output,
		uint8_t output_ptr) {

	uint8_t ptr = output_ptr;
	uint32_t val = *(uint32_t*)&value;

	*(output+ (ptr++)) = (uint8_t)( val & 0xFF);
	*(output+ (ptr++)) = (uint8_t)((val & 0xFF00) >> 8);
	*(output+ (ptr++)) = (uint8_t)((val & 0xFF0000) >> 16);
	*(output+ (ptr++)) = (uint8_t)((val & 0xFF000000) >> 24);

	return ptr;
}

uint8_t umb_insert_sint_to_buffer(int16_t value, uint8_t* output,
		uint8_t output_ptr) {

	uint8_t ptr = output_ptr;
	uint16_t val = *(uint16_t*)&value;

	*(output+ (ptr++)) = (uint8_t)( val & 0xFF);
	*(output+ (ptr++)) = (uint8_t)((val & 0xFF00) >> 8);

	return ptr;

}

uint8_t umb_insert_byte_to_buffer(int8_t value, uint8_t* output,
		uint8_t output_ptr) {

	uint8_t ptr = output_ptr;
	uint8_t val = *(uint8_t*)&value;

	*(output+ (ptr++)) = val;

	return ptr;

}
