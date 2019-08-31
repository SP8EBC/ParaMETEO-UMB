/*
 * umb-slave.c
 *
 *  Created on: 01.12.2016
 *      Author: mateusz
 */

#include <string.h>
#include "drivers/umb-slave.h"
#include "drivers/serial.h"

#include <stdint.h>

#define DEV_CLASS (uint8_t)(0x0C << 4)
#define DEV_ID (uint8_t)2

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

				umb_slave_state = UMB_STATE_MESSAGE_RXED;
			}
			else {
//				trace_printf("UmbSlave: Wrong CRC in data %d bytes long with cmdId 0x%02x\n", ln, srlRXData[8]);
//				trace_printf("UmbSlave: CRC should be 0x%04X but was 0x%04X\n", crc, (srlRXData[9 + ln] | srlRXData[10 + ln] << 8) );
				umb_slave_state = UMB_STATE_MESSAGE_RXED_WRONG_CRC;
				}
			}
		else {
			// jeżeli nie jest zaadresowana to zignoruj i słuchaj dalej
//			trace_printf("UmbSlave: Data addressed not to this device\n"), UmbSlaveListen();
			for (ii = 0; ii <= 0x2FFFF; ii++);
			umb_slave_listen();
		}
	}
		else;


	return 0;
}

char umb_callback_status_request(void) {
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

char umb_callback_device_information_request(void) {
//  	trace_printf("UmbSlave: cmd[DEVICE_INFO] option 0x%02x\n", umbMessage.payload[0]);
  	uint16_t ch = 0;
  	int8_t temp = 0;
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
			umb_message.payload[4] = 0x02; // MMYY LSB
			umb_message.payload[5] = 0x10; // MMYY MSB
			umb_message.payload[6] = 0x02;
			umb_message.payload[7] = 0x00;
			umb_message.payload[8] = 0x01; // PartsList
			umb_message.payload[9] = 0x01; // PartsPlan
			umb_message.payload[10] = 0x0A;	// hardware
			umb_message.payload[11] = 0x0A;	// software
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
			umb_message.payload[2] = 0x08; // liczba kanalow LSB 0x01
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
				umb_message.payload[0] = 0x00; // zawsze 0x00
				umb_message.payload[1] = 0x16; // zawsze info
				umb_message.payload[2] = 0x00; // option -> block number
				umb_message.payload[3] = 0x08; // liczba kanalow 0x08
				umb_message.payload[4] = 0x64; // kanal 100 LSB
				umb_message.payload[5] = 0x00; // kanal 100 MSB
				umb_message.payload[6] = 0xC8; // kanal 200 LSB
				umb_message.payload[7] = 0x00; // kanal 200 MSB
				umb_message.payload[8] = 0x2C; // kanal 300 LSB
				umb_message.payload[9] = 0x01; // kanal 300 MSB
				umb_message.payload[10] = 0x31; // kanal 305 LSB
				umb_message.payload[11] = 0x01; // kanal 305 MSB
				umb_message.payload[12] = 0xCC; // kanal 440 LSB
				umb_message.payload[13] = 0x01; // kanal 440 MSB
				umb_message.payload[14] = 0xB8; // kanal 460 LSB
				umb_message.payload[15] = 0x01; // kanal 460 MSB
				umb_message.payload[16] = 0x30; // kanal 560 LSB
				umb_message.payload[17] = 0x02; // kanal 560 MSB
				umb_message.payload[16] = 0x44; // kanal 580 LSB
				umb_message.payload[17] = 0x02; // kanal 580 MSB
				umb_message.payloadLn = 18;
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
				case 100U:
					umb_message.payload[2] = 0x64;	// kanal LSB
					umb_message.payload[3] = 0x00;	// kanal LSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Temperatura         ")) + 4; // 20
					temp = sprintf(umb_message.payload + umb_message.payloadLn, "StopnieC       "); // 15
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = CURRENT; // current value
					umb_message.payload[umb_message.payloadLn++] = SIGNED_CHAR;
					umb_message.payload[umb_message.payloadLn++] = (uint8_t)-127;
					umb_message.payload[umb_message.payloadLn++] = (uint8_t)127;
					break;

				case 200U:
					umb_message.payload[2] = 0xC8;	// kanal LSB
					umb_message.payload[3] = 0x00;	// kanal LSB
					umb_message.payloadLn = (sprintf(umb_message.payload + 4, "Wilgotnosc          ")) + 4;
					temp = (sprintf(umb_message.payload + umb_message.payloadLn, "%%              "));
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = CURRENT; // current value
					umb_message.payload[umb_message.payloadLn++] = SIGNED_CHAR;
					umb_message.payload[umb_message.payloadLn++] = (uint8_t)1;
					umb_message.payload[umb_message.payloadLn++] = 100;
					break;

				case 300U:
					umb_message.payload[2] = 0x2C;	// kanal LSB
					umb_message.payload[3] = 0x01;	// kanal MSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Cisnienie QFE       ")) + 4;
					temp =  (sprintf(umb_message.payload + umb_message.payloadLn, "hPa            "));
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = CURRENT; // current value
					umb_message.payload[umb_message.payloadLn++] = UNSIGNED_SHORT;
					umb_message.payload[umb_message.payloadLn++] = 0xF4;
					umb_message.payload[umb_message.payloadLn++] = 0x01; /// 0x1F4 -> 500d
					umb_message.payload[umb_message.payloadLn++] = 0x4C;
					umb_message.payload[umb_message.payloadLn++] = 0x04; /// 0x44C -> 1100d
					break;

				case 305U:
					umb_message.payload[2] = 0x31;	// kanal LSB
					umb_message.payload[3] = 0x01;	// kanal LSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Cisnienie QNH       ")) + 4;
					temp =  (sprintf(umb_message.payload + umb_message.payloadLn, "hPa            "));
					umb_message.payloadLn += temp;
					umb_message.payload[umb_message.payloadLn++] = CURRENT; // current value
					umb_message.payload[umb_message.payloadLn++] = UNSIGNED_SHORT;
					umb_message.payload[umb_message.payloadLn++] = 0xF4;
					umb_message.payload[umb_message.payloadLn++] = 0x01;
					umb_message.payload[umb_message.payloadLn++] = 0x4C;
					umb_message.payload[umb_message.payloadLn++] = 0x04;
					break;

				case 460U:
					umb_message.payload[2] = 0xB8;	// kanal LSB
					umb_message.payload[3] = 0x01;	// kanal LSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Srednia predkosc    ")) + 4;
//					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					temp =  (sprintf(umb_message.payload + umb_message.payloadLn, "m/s            "));
					umb_message.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umb_message.payload[umb_message.payloadLn++] = AVERAGE; // current value
					umb_message.payload[umb_message.payloadLn++] = FLOAT;
					umb_message.payload[umb_message.payloadLn++] = 0x00;	// minimum
					umb_message.payload[umb_message.payloadLn++] = 0x00;
					umb_message.payload[umb_message.payloadLn++] = 0x00;
					umb_message.payload[umb_message.payloadLn++] = 0x00;
					umb_message.payload[umb_message.payloadLn++] = 0x00; // maximum 40.0m/s
					umb_message.payload[umb_message.payloadLn++] = 0x00; // maximum 40.0m/s
					umb_message.payload[umb_message.payloadLn++] = 0x20; // maximum 40.0m/s
					umb_message.payload[umb_message.payloadLn++] = 0x42; // maximum 40.0m/s
					break;

				case 440U:
					umb_message.payload[2] = 0xCC;	// kanal LSB
					umb_message.payload[3] = 0x01;	// kanal LSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Porywy wiatru       ")) + 4;
//					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					temp =  (sprintf(umb_message.payload + umb_message.payloadLn, "m/s            "));
					umb_message.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umb_message.payload[umb_message.payloadLn++] = MAXIMUM; // current value
					umb_message.payload[umb_message.payloadLn++] = FLOAT;
					umb_message.payload[umb_message.payloadLn++] = 0x00;	// minimum
					umb_message.payload[umb_message.payloadLn++] = 0x00;
					umb_message.payload[umb_message.payloadLn++] = 0x00;
					umb_message.payload[umb_message.payloadLn++] = 0x00;
					umb_message.payload[umb_message.payloadLn++] = 0x00; // maximum 40.0m/s
					umb_message.payload[umb_message.payloadLn++] = 0x00; // maximum 40.0m/s
					umb_message.payload[umb_message.payloadLn++] = 0x20; // maximum 40.0m/s
					umb_message.payload[umb_message.payloadLn++] = 0x42; // maximum 40.0m/s
					break;

				case 560U:
					umb_message.payload[2] = 0x30;	// kanal LSB
					umb_message.payload[3] = 0x02;	// kanal LSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Kierunek wiatru     ")) + 4;
//					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					temp =  (sprintf(umb_message.payload + umb_message.payloadLn, "stopnie        "));
					umb_message.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umb_message.payload[umb_message.payloadLn++] = VECTOR_AVG; // current value
					umb_message.payload[umb_message.payloadLn++] = UNSIGNED_SHORT;
					umb_message.payload[umb_message.payloadLn++] = 0x00;	// minimum
					umb_message.payload[umb_message.payloadLn++] = 0x00;
					umb_message.payload[umb_message.payloadLn++] = 0x67;  // maximum 360
					umb_message.payload[umb_message.payloadLn++] = 0x01;
					break;

				case 580U:
					umb_message.payload[2] = 0x44;	// kanal LSB
					umb_message.payload[3] = 0x02;	// kanal LSB
					umb_message.payloadLn =  (sprintf(umb_message.payload + 4, "Kierunek wiatru     ")) + 4;
//					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					temp =  (sprintf(umb_message.payload + umb_message.payloadLn, "s              "));
					umb_message.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umb_message.payload[umb_message.payloadLn++] = VECTOR_AVG; // current value
					umb_message.payload[umb_message.payloadLn++] = UNSIGNED_SHORT;
					umb_message.payload[umb_message.payloadLn++] = 0x00;	// minimum
					umb_message.payload[umb_message.payloadLn++] = 0x00;
					umb_message.payload[umb_message.payloadLn++] = 0x67;  // maximum 360
					umb_message.payload[umb_message.payloadLn++] = 0x01;
					break;

					default:
//						trace_printf("UmbSlave: cmd[DEVICE_INFO] Fail!\n");

						break;



			}
			//trace_printf("UmbSlave: cmd[DEVICE_INFO] Complete channel info for number %d ln %d\n", ch, umbMessage.payloadLn);
			break;

		default:
			break;
	}

	return 0;
}

char umb_callback_online_data_request(umbMeteoData_t *pMeteo, char status) {
	uint16_t ch = (umb_message.payload[0] | umb_message.payload[1] << 8);
	uint8_t ch_lsb = umb_message.payload[0];
	uint8_t ch_msb = umb_message.payload[1];
	char ln;
	umb_clear_message_struct(0);
	umb_message.cmdId = 0x23;
	void* val;
	////
	umb_message.payload[0] = status;
	umb_message.payload[1] = ch_lsb;
	umb_message.payload[2] = ch_msb;
	switch (ch) { // switch for <value>
	case 100:
		val = &pMeteo->temperature; break;
	case 101:
		val = &pMeteo->fTemperature; break;
	case 200:
		val = &pMeteo->humidity; break;
	case 300:
		val = &pMeteo->qfe; break;
	case 350:
		val = &pMeteo->qnh; break;
	case 440:
		val = &pMeteo->windgusts; break;
	case 460:
		val = &pMeteo->windspeed; break;
	case 560:
		val = &pMeteo->winddirection; break;
	case 580:
		val = &pMeteo->winddirection; break;
	default: return -1;

	}
	switch (ch) {	// switch for <type>
		case 100:
		case 200:
			umb_message.payload[3] = SIGNED_CHAR;
			umb_message.payload[4] = *(char*)val;
			umb_message.payloadLn = 5;
			break;
		case 101:
		case 440:
		case 460:
			umb_message.payload[3] = FLOAT;
			umb_message.payload[4] = *(int*)val & 0xFF;
			umb_message.payload[5] = (*(int*)val & 0xFF00) >> 8;
			umb_message.payload[6] = (*(int*)val & 0xFF0000) >> 16;
			umb_message.payload[7] = (*(int*)val & 0xFF000000) >> 24;
			umb_message.payloadLn = 8;
			break;
		case 300:
		case 350:
		case 560:
		case 580:
			umb_message.payload[3] = UNSIGNED_SHORT;
			umb_message.payload[4] = *(unsigned short*)val & 0xFF;
			umb_message.payload[5] = (*(unsigned short*)val & 0xFF00) >> 8;
			umb_message.payloadLn = 6;
			break;

	default: return -1;
	}

	return 0;
}

char umb_callback_multi_online_data_request(umbMeteoData_t *pMeteo, char status) {
	uint8_t chann = umb_message.payload[0];
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
		channels[i] = (umb_message.payload[j+1] | umb_message.payload[j+2] << 8);
		j += 2;
	}

	umb_clear_message_struct(0);
	j = 0;

	umb_message.cmdId = 0x2F;
	umb_message.payload[0] = status;
	umb_message.payload[1] = chann;

	for (int i = 0; i < chann; i++)
	{
		switch (channels[i]) { // switch for <value>
		case 100:
			val = &pMeteo->temperature; break;
		case 200:
			val = &pMeteo->humidity; break;
		case 300:
			val = &pMeteo->qfe; break;
		case 350:
			val = &pMeteo->qnh; break;
		case 440:
			val = &pMeteo->windgusts; break;
		case 460:
			val = &pMeteo->windspeed; break;
		case 560:
			val = &pMeteo->winddirection; break;
		case 580:
			val = &pMeteo->winddirection; break;
		default: return -1;

		}
		switch (channels[i]) {	// switch for <type>
			case 100:
			case 200:
				umb_message.payload[2 + j++] = 5;			// subtelegram lenght
				umb_message.payload[2 + j++] = 0;	// substatus
				umb_message.payload[2 + j++] = channels[i] & 0xFF;
				umb_message.payload[2 + j++] = (channels[i] >> 8) & 0xFF;
				umb_message.payload[2 + j++] = SIGNED_CHAR;
				umb_message.payload[2 + j++] = *(char*)val;
				break;
			case 440:
			case 460:
				umb_message.payload[2 + j++] = 8;			// subtelegram lenght
				umb_message.payload[2 + j++] = 0;	// substatus
				umb_message.payload[2 + j++] = channels[i] & 0xFF;
				umb_message.payload[2 + j++] = (channels[i] >> 8) & 0xFF;
				umb_message.payload[2 + j++] = FLOAT;
				umb_message.payload[2 + j++] = *(int*)val & 0xFF;
				umb_message.payload[2 + j++] = (*(int*)val & 0xFF00) >> 8;
				umb_message.payload[2 + j++] = (*(int*)val & 0xFF0000) >> 16;
				umb_message.payload[2 + j++] = (*(int*)val & 0xFF000000) >> 24;

				break;
			case 300:
			case 305:
			case 560:
			case 580:
				umb_message.payload[2 + j++] = 6;			// subtelegram lenght
				umb_message.payload[2 + j++] = 0;	// substatus
				umb_message.payload[2 + j++] = channels[i] & 0xFF;
				umb_message.payload[2 + j++] = (channels[i] >> 8) & 0xFF;
				umb_message.payload[2 + j++] = UNSIGNED_SHORT;
				umb_message.payload[2 + j++] = *(unsigned short*)val & 0xFF;
				umb_message.payload[2 + j++] = (*(unsigned short*)val & 0xFF00) >> 8;
				break;

		default: return -1;
		}

	}
	umb_message.payloadLn = 2 + j;
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
