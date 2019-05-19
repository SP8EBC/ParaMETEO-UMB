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

#define DEV_CLASS (char)(0x0C << 4)
#define DEV_ID (char)2

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


UmbMessage umbMessage;

uint8_t umbSlaveState = 0;

static uint8_t umbMasterClass = 0;
static uint8_t umbMasterId = 0;

volatile int ii = 0;

char UmbSlaveListen() {
	if(srl_rx_state == SRL_RX_IDLE || srl_rx_state == SRL_RX_DONE || srl_rx_state == SRL_RX_ERROR) {			// EOT
		SrlReceiveData(8, SOH, 0x00, 0, 6, 12);
		UmbClearMessageStruct(0);
		umbSlaveState = 1;
		umbMasterClass = 0;
		umbMasterId = 0;
		memset(umbMessage.payload, 0x00, MESSAGE_PAYLOAD_MAX);
		umbMessage.cmdId = 0;
	}
	else
		return -1;
	return 0;
}

char UmbSlavePool() {
	if (srl_rx_state == SRL_RX_DONE && umbSlaveState == 1) {
		if (srl_get_rx_buffer()[2] == DEV_ID && srl_get_rx_buffer()[3] == DEV_CLASS) {
			// sprawdzanie czy odebrana ramka jest zaadresownana do tego urządzenia
			int16_t ln = srl_get_rx_buffer()[6];
			uint16_t crc = 0xFFFF;

			for (int i = 0; i < 9 + ln; i++) {
				crc = calc_crc(crc, srl_get_rx_buffer()[i]);
			}

			char crc_l = crc & 0xFF;
			char crc_h = ((crc & 0xFF00) >> 8);

			if ( crc_l == srl_get_rx_buffer()[9 + ln] && crc_h == (srl_get_rx_buffer()[10 + ln]) ) {

				umbMessage.masterId = srl_get_rx_buffer()[4];
				umbMessage.masterClass = (srl_get_rx_buffer()[5] >> 4) & 0x0F;

				umbMessage.cmdId = srl_get_rx_buffer()[8];
				if (ln > 2)
					memcpy(umbMessage.payload, srl_get_rx_buffer() + 10, ln - 2); // długość len - cmd - verc
				umbMessage.payloadLn = ln - 2;
				umbMessage.checksum = crc;

				umbSlaveState = 2;
			}
			else {
//				trace_printf("UmbSlave: Wrong CRC in data %d bytes long with cmdId 0x%02x\n", ln, srlRXData[8]);
//				trace_printf("UmbSlave: CRC should be 0x%04X but was 0x%04X\n", crc, (srlRXData[9 + ln] | srlRXData[10 + ln] << 8) );
				umbSlaveState = 3;
				}
			}
		else {
			// jeżeli nie jest zaadresowana to zignoruj i słuchaj dalej
//			trace_printf("UmbSlave: Data addressed not to this device\n"), UmbSlaveListen();
			for (ii = 0; ii <= 0x2FFFF; ii++);
			UmbSlaveListen();
		}
	}
		else;


	return 0;
}

char UmbStatusRequestResponse(void) {
//	UmbClearMessageStruct();
	umbMessage.cmdId = 0x26;
	umbMessage.payload[0] = 0x00;
	umbMessage.payload[1] = 0x00;
	umbMessage.payloadLn = 2;
	return 0;
}

char UmbReturnStatus(char cmdId, char status) {
	UmbClearMessageStruct(0);
	umbMessage.payload[0] = status;
	umbMessage.cmdId = cmdId;
	return 0;
}

char UmbDeviceInformationRequestResponse(void) {
//  	trace_printf("UmbSlave: cmd[DEVICE_INFO] option 0x%02x\n", umbMessage.payload[0]);
  	uint16_t ch = 0;
  	int8_t temp = 0;
	umbMessage.payloadLn = 0;
	switch(umbMessage.payload[0]) {
		case 0x10:
			UmbClearMessageStruct(0);
			umbMessage.cmdId = 0x2D;
			umbMessage.payload[0] = 0x00;
			umbMessage.payload[1] = 0x10;
			umbMessage.payloadLn =  (sprintf(umbMessage.payload + 2, DEV_NAME)) + 2;
			break;

		case 0x11:
			UmbClearMessageStruct(0);
			umbMessage.cmdId = 0x2D;
			umbMessage.payload[0] = 0x00;
			umbMessage.payload[1] = 0x11;
			umbMessage.payloadLn =  (sprintf(umbMessage.payload + 2, "Opis")) + 2;
			break;

		case 0x12:
			UmbClearMessageStruct(0);
			umbMessage.cmdId = 0x2D;
			umbMessage.payload[0] = 0x00;
			umbMessage.payload[1] = 0x12;
			umbMessage.payload[2] = 0x0A;	// hardware
			umbMessage.payload[3] = 0x0A;	// software
			umbMessage.payloadLn = 4;
			break;

		case 0x13:
			UmbClearMessageStruct(0);
			umbMessage.cmdId = 0x2D;
			umbMessage.payload[0] = 0x00;
			umbMessage.payload[1] = 0x13;
			umbMessage.payload[2] = 0x11; // Serial LSB
			umbMessage.payload[3] = 0x11; // Serial MSB
			umbMessage.payload[4] = 0x02; // MMYY LSB
			umbMessage.payload[5] = 0x10; // MMYY MSB
			umbMessage.payload[6] = 0x02;
			umbMessage.payload[7] = 0x00;
			umbMessage.payload[8] = 0x01; // PartsList
			umbMessage.payload[9] = 0x01; // PartsPlan
			umbMessage.payload[10] = 0x0A;	// hardware
			umbMessage.payload[11] = 0x0A;	// software
			umbMessage.payload[12] = 0x0A;	// e2version
			umbMessage.payload[13] = 0x0A; // DeviceVersion LSB
			umbMessage.payload[14] = 0x00;
			umbMessage.payloadLn = 15;
			break;

		case 0x14:
			UmbClearMessageStruct(0);
			umbMessage.cmdId = 0x2D;
			umbMessage.payload[0] = 0x00;
			umbMessage.payload[1] = 0x14;
			umbMessage.payload[2] = 0xFF; // LSB
			umbMessage.payload[3] = 0xFF; // MSB
			umbMessage.payloadLn = 4;
			break;

		case 0x15:
			UmbClearMessageStruct(0);
			umbMessage.cmdId = 0x2D;
			umbMessage.payload[0] = 0x00;	// zawsze 0x00 -- satus
			umbMessage.payload[1] = 0x15;	// zawsze info
			umbMessage.payload[2] = 0x08; // liczba kanalow LSB 0x01
			umbMessage.payload[3] = 0x00; // liczba kanalow MSB
			umbMessage.payload[4] = 0x01; // liczba blokow
			umbMessage.payloadLn = 5;
			break;

		case 0x16:
			if (umbMessage.payload[1] != 0x00) {
				// nie ma blokow kanalow powyzej 0
			}
			else {

				UmbClearMessageStruct(0);
				umbMessage.cmdId = 0x2D;
				umbMessage.payload[0] = 0x00; // zawsze 0x00
				umbMessage.payload[1] = 0x16; // zawsze info
				umbMessage.payload[2] = 0x00; // option -> block number
				umbMessage.payload[3] = 0x08; // liczba kanalow 0x08
				umbMessage.payload[4] = 0x64; // kanal 100 LSB
				umbMessage.payload[5] = 0x00; // kanal 100 MSB
				umbMessage.payload[6] = 0xC8; // kanal 200 LSB
				umbMessage.payload[7] = 0x00; // kanal 200 MSB
				umbMessage.payload[8] = 0x2C; // kanal 300 LSB
				umbMessage.payload[9] = 0x01; // kanal 300 MSB
				umbMessage.payload[10] = 0x31; // kanal 305 LSB
				umbMessage.payload[11] = 0x01; // kanal 305 MSB
				umbMessage.payload[12] = 0xCC; // kanal 440 LSB
				umbMessage.payload[13] = 0x01; // kanal 440 MSB
				umbMessage.payload[14] = 0xB8; // kanal 460 LSB
				umbMessage.payload[15] = 0x01; // kanal 460 MSB
				umbMessage.payload[16] = 0x30; // kanal 560 LSB
				umbMessage.payload[17] = 0x02; // kanal 560 MSB
				umbMessage.payload[16] = 0x44; // kanal 580 LSB
				umbMessage.payload[17] = 0x02; // kanal 580 MSB
				umbMessage.payloadLn = 18;
			}
			break;

		case 0x20:
			ch = umbMessage.payload[1] | umbMessage.payload[2] << 8;
			ch &= 0xFFFF;
			UmbClearMessageStruct(0);
			umbMessage.cmdId = 0x2D;
			umbMessage.payload[0] = 0x00;	// zawsze 0x00
			umbMessage.payload[1] = 0x20;	// zawsze info
			switch (ch) {
				default: break;
			}
			//umbMessage.payloadLn =  (sprintf(umbMessage.payload + 2, "ParaMETEO")) + 2;
			break;

		case 0x30:
			umbMessage.payloadLn = 0;
			ch = 0;
			ch = umbMessage.payload[1] | (umbMessage.payload[2] << 8);
			UmbClearMessageStruct(0);
			umbMessage.cmdId = 0x2D;
			umbMessage.payload[0] = 0x00;	// zawsze 0x00
			umbMessage.payload[1] = 0x30;	// zawsze info
			switch (ch) {
				case 100U:
					umbMessage.payload[2] = 0x64;	// kanal LSB
					umbMessage.payload[3] = 0x00;	// kanal LSB
					umbMessage.payloadLn =  (sprintf(umbMessage.payload + 4, "Temperatura         ")) + 4; // 20
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					temp = sprintf(umbMessage.payload + umbMessage.payloadLn, "StopnieC       "); // 15
					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = CURRENT; // current value
					umbMessage.payload[umbMessage.payloadLn++] = SIGNED_CHAR;
					umbMessage.payload[umbMessage.payloadLn++] = (uint8_t)-127;
					umbMessage.payload[umbMessage.payloadLn++] = (uint8_t)127;
					break;

				case 200U:
					umbMessage.payload[2] = 0xC8;	// kanal LSB
					umbMessage.payload[3] = 0x00;	// kanal LSB
					umbMessage.payloadLn = (sprintf(umbMessage.payload + 4, "Wilgotnosc          ")) + 4;
//					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					temp = (sprintf(umbMessage.payload + umbMessage.payloadLn, "%%              "));
					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = CURRENT; // current value
					umbMessage.payload[umbMessage.payloadLn++] = SIGNED_CHAR;
					umbMessage.payload[umbMessage.payloadLn++] = (uint8_t)1;
					umbMessage.payload[umbMessage.payloadLn++] = 100;
					break;

				case 300U:
					umbMessage.payload[2] = 0x2C;	// kanal LSB
					umbMessage.payload[3] = 0x01;	// kanal MSB
					umbMessage.payloadLn =  (sprintf(umbMessage.payload + 4, "Cisnienie QFE       ")) + 4;
					//umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					temp =  (sprintf(umbMessage.payload + umbMessage.payloadLn, "hPa            "));
					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = CURRENT; // current value
					umbMessage.payload[umbMessage.payloadLn++] = UNSIGNED_SHORT;
					umbMessage.payload[umbMessage.payloadLn++] = 0xF4;
					umbMessage.payload[umbMessage.payloadLn++] = 0x01; /// 0x1F4 -> 500d
					umbMessage.payload[umbMessage.payloadLn++] = 0x4C;
					umbMessage.payload[umbMessage.payloadLn++] = 0x04; /// 0x44C -> 1100d
					break;

				case 305U:
					umbMessage.payload[2] = 0x31;	// kanal LSB
					umbMessage.payload[3] = 0x01;	// kanal LSB
					umbMessage.payloadLn =  (sprintf(umbMessage.payload + 4, "Cisnienie QNH       ")) + 4;
//					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					temp =  (sprintf(umbMessage.payload + umbMessage.payloadLn, "hPa            "));
					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = CURRENT; // current value
					umbMessage.payload[umbMessage.payloadLn++] = UNSIGNED_SHORT;
					umbMessage.payload[umbMessage.payloadLn++] = 0xF4;
					umbMessage.payload[umbMessage.payloadLn++] = 0x01;
					umbMessage.payload[umbMessage.payloadLn++] = 0x4C;
					umbMessage.payload[umbMessage.payloadLn++] = 0x04;
					break;

				case 460U:
					umbMessage.payload[2] = 0xB8;	// kanal LSB
					umbMessage.payload[3] = 0x01;	// kanal LSB
					umbMessage.payloadLn =  (sprintf(umbMessage.payload + 4, "Srednia predkosc    ")) + 4;
//					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					temp =  (sprintf(umbMessage.payload + umbMessage.payloadLn, "m/s            "));
					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = AVERAGE; // current value
					umbMessage.payload[umbMessage.payloadLn++] = FLOAT;
					umbMessage.payload[umbMessage.payloadLn++] = 0x00;	// minimum
					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = 0x00; // maximum 40.0m/s
					umbMessage.payload[umbMessage.payloadLn++] = 0x00; // maximum 40.0m/s
					umbMessage.payload[umbMessage.payloadLn++] = 0x20; // maximum 40.0m/s
					umbMessage.payload[umbMessage.payloadLn++] = 0x42; // maximum 40.0m/s
					break;

				case 440U:
					umbMessage.payload[2] = 0xCC;	// kanal LSB
					umbMessage.payload[3] = 0x01;	// kanal LSB
					umbMessage.payloadLn =  (sprintf(umbMessage.payload + 4, "Porywy wiatru       ")) + 4;
//					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					temp =  (sprintf(umbMessage.payload + umbMessage.payloadLn, "m/s            "));
					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = MAXIMUM; // current value
					umbMessage.payload[umbMessage.payloadLn++] = FLOAT;
					umbMessage.payload[umbMessage.payloadLn++] = 0x00;	// minimum
					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = 0x00; // maximum 40.0m/s
					umbMessage.payload[umbMessage.payloadLn++] = 0x00; // maximum 40.0m/s
					umbMessage.payload[umbMessage.payloadLn++] = 0x20; // maximum 40.0m/s
					umbMessage.payload[umbMessage.payloadLn++] = 0x42; // maximum 40.0m/s
					break;

				case 560U:
					umbMessage.payload[2] = 0x30;	// kanal LSB
					umbMessage.payload[3] = 0x02;	// kanal LSB
					umbMessage.payloadLn =  (sprintf(umbMessage.payload + 4, "Kierunek wiatru     ")) + 4;
//					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					temp =  (sprintf(umbMessage.payload + umbMessage.payloadLn, "stopnie        "));
					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = VECTOR_AVG; // current value
					umbMessage.payload[umbMessage.payloadLn++] = UNSIGNED_SHORT;
					umbMessage.payload[umbMessage.payloadLn++] = 0x00;	// minimum
					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = 0x67;  // maximum 360
					umbMessage.payload[umbMessage.payloadLn++] = 0x01;
					break;

				case 580U:
					umbMessage.payload[2] = 0x44;	// kanal LSB
					umbMessage.payload[3] = 0x02;	// kanal LSB
					umbMessage.payloadLn =  (sprintf(umbMessage.payload + 4, "Kierunek wiatru     ")) + 4;
//					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					temp =  (sprintf(umbMessage.payload + umbMessage.payloadLn, "s              "));
					umbMessage.payloadLn += temp;
//					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = VECTOR_AVG; // current value
					umbMessage.payload[umbMessage.payloadLn++] = UNSIGNED_SHORT;
					umbMessage.payload[umbMessage.payloadLn++] = 0x00;	// minimum
					umbMessage.payload[umbMessage.payloadLn++] = 0x00;
					umbMessage.payload[umbMessage.payloadLn++] = 0x67;  // maximum 360
					umbMessage.payload[umbMessage.payloadLn++] = 0x01;
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

char UmbOnlineDataRequestResponse(UmbMeteoData *pMeteo, char status) {
	uint16_t ch = (umbMessage.payload[0] | umbMessage.payload[1] << 8);
	uint8_t ch_lsb = umbMessage.payload[0];
	uint8_t ch_msb = umbMessage.payload[1];
	char ln;
	UmbClearMessageStruct(0);
	umbMessage.cmdId = 0x23;
	void* val;
	////
	umbMessage.payload[0] = status;
	umbMessage.payload[1] = ch_lsb;
	umbMessage.payload[2] = ch_msb;
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
			umbMessage.payload[3] = SIGNED_CHAR;
			umbMessage.payload[4] = *(char*)val;
			umbMessage.payloadLn = 5;
			break;
		case 101:
		case 440:
		case 460:
			umbMessage.payload[3] = FLOAT;
			umbMessage.payload[4] = *(int*)val & 0xFF;
			umbMessage.payload[5] = (*(int*)val & 0xFF00) >> 8;
			umbMessage.payload[6] = (*(int*)val & 0xFF0000) >> 16;
			umbMessage.payload[7] = (*(int*)val & 0xFF000000) >> 24;
			umbMessage.payloadLn = 8;
			break;
		case 300:
		case 350:
		case 560:
		case 580:
			umbMessage.payload[3] = UNSIGNED_SHORT;
			umbMessage.payload[4] = *(unsigned short*)val & 0xFF;
			umbMessage.payload[5] = (*(unsigned short*)val & 0xFF00) >> 8;
			umbMessage.payloadLn = 6;
			break;

	default: return -1;
	}

	return 0;
}

char UmbMultiOnlineDataRequestResponse(UmbMeteoData *pMeteo, char status) {
	uint8_t chann = umbMessage.payload[0];
	uint16_t channels[4] = {0, 0, 0, 0};
	uint8_t j = 0;
	void* val;

	if (chann > 4)
	{
		UmbClearMessageStruct(0);
		return -1;
	}

	for (int i = 0; i < chann; i++)
	{
		channels[i] = (umbMessage.payload[j+1] | umbMessage.payload[j+2] << 8);
		j += 2;
	}

	UmbClearMessageStruct(0);
	j = 0;

	umbMessage.cmdId = 0x2F;
	umbMessage.payload[0] = status;
	umbMessage.payload[1] = chann;

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
				umbMessage.payload[2 + j++] = 5;			// subtelegram lenght
				umbMessage.payload[2 + j++] = 0;	// substatus
				umbMessage.payload[2 + j++] = channels[i] & 0xFF;
				umbMessage.payload[2 + j++] = (channels[i] >> 8) & 0xFF;
				umbMessage.payload[2 + j++] = SIGNED_CHAR;
				umbMessage.payload[2 + j++] = *(char*)val;
				break;
			case 440:
			case 460:
				umbMessage.payload[2 + j++] = 8;			// subtelegram lenght
				umbMessage.payload[2 + j++] = 0;	// substatus
				umbMessage.payload[2 + j++] = channels[i] & 0xFF;
				umbMessage.payload[2 + j++] = (channels[i] >> 8) & 0xFF;
				umbMessage.payload[2 + j++] = FLOAT;
				umbMessage.payload[2 + j++] = *(int*)val & 0xFF;
				umbMessage.payload[2 + j++] = (*(int*)val & 0xFF00) >> 8;
				umbMessage.payload[2 + j++] = (*(int*)val & 0xFF0000) >> 16;
				umbMessage.payload[2 + j++] = (*(int*)val & 0xFF000000) >> 24;

				break;
			case 300:
			case 305:
			case 560:
			case 580:
				umbMessage.payload[2 + j++] = 6;			// subtelegram lenght
				umbMessage.payload[2 + j++] = 0;	// substatus
				umbMessage.payload[2 + j++] = channels[i] & 0xFF;
				umbMessage.payload[2 + j++] = (channels[i] >> 8) & 0xFF;
				umbMessage.payload[2 + j++] = UNSIGNED_SHORT;
				umbMessage.payload[2 + j++] = *(unsigned short*)val & 0xFF;
				umbMessage.payload[2 + j++] = (*(unsigned short*)val & 0xFF00) >> 8;
				break;

		default: return -1;
		}

	}
	umbMessage.payloadLn = 2 + j;
	return 0;


}

void UmbClearMessageStruct(char b) {
	if (b) {
		umbMessage.masterClass = 0;
		umbMessage.masterId = 0;
	}
	umbMessage.checksum = 0;
	umbMessage.cmdId = 0;
	memset(umbMessage.payload, 0x00, MESSAGE_PAYLOAD_MAX);
}

short UmbPrepareFrameToSend(UmbMessage* pMessage, uint8_t* pUsartBuffer) {
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
