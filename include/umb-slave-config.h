/*
 * umb-slave-config.h
 *
 *  Created on: 03.09.2019
 *      Author: mateusz
 */

#ifndef UMB_SLAVE_CONFIG_H_
#define UMB_SLAVE_CONFIG_H_

#define DEV_CLASS (uint8_t)(0x0C << 4)
#define DEV_ID (uint8_t)2

#define SW_VERSION 0xA2

#define CH_SWAP_TEMP_CURRENT_WITH_AVG

// 3.9.8 Channel Assignment Device Class 7 Compact Weather Station
#define CH_CURRENT_TEMPERATURE_INT		100U
#define CH_CURRENT_TEMPERATURE_FLOAT 	101U
#define CH_MIN_TEMPERATURE_INT			120U
#define CH_MIN_TEMPERATURE_FLOAT 		121U
#define CH_MAX_TEMPERATURE_INT			140U
#define CH_MAX_TEMPERATURE_FLOAT 		141U
#define CH_AVERAGE_TEMPERATURE_INT		160U
#define CH_AVERAGE_TEMPERATURE_FLOAT 	161U
#define CH_CURRENT_HUMIDITY				200U
#define CH_CURRENT_QFE_INT				300U
#define CH_CURRENT_QFE_FLOAT			301U
#define CH_CURRENT_QNH_INT				305U
#define CH_CURRENT_QNH_FLOAT			306U
#define CH_AVERGE_WIND_MS				460U
#define CH_MAX_WIND_MS					440U
#define CH_MIN_WIND_MS					420U
#define CH_AVERAGE_WIND_DIR				560U //580
#define CH_AVERAGE_WIND_VCT_DIR			580U

#define CH_CHANNEL_CNT					18

#endif /* UMB_SLAVE_CONFIG_H_ */
