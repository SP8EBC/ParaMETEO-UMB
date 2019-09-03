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

// 3.9.8 Channel Assignment Device Class 7 Compact Weather Station
#define CH_CURRENT_TEMPERATURE_FLOAT 	100U
#define CH_CURRENT_TEMPERATURE_INT		101U
#define CH_CURRENT_HUMIDITY				200U
#define CH_CURRENT_QFE_FLOAT			300U
#define CH_CURRENT_QFE_INT				301U
#define CH_CURRENT_QNH_FLOAT			305U
#define CH_CURRENT_QNH_INT				306U
#define CH_AVERGE_WIND_MS				460U
#define CH_MAX_WIND_MS					440U
#define CH_AVERAGE_WIND_DIR				560U //580
#define CH_AVERAGE_WIND_VCT_DIR			580U

#define CH_CHANNEL_CNT					11

#endif /* UMB_SLAVE_CONFIG_H_ */
