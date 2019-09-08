/*
 * wx_handler.h
 *
 *  Created on: 26.01.2019
 *      Author: mateusz
 */

#ifndef WX_HANDLER_H_
#define WX_HANDLER_H_

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

void wx_get_all_measurements(void);
void wx_pool_dht22(void);
void wx_copy_to_rte_meteodata(void);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* WX_HANDLER_H_ */
