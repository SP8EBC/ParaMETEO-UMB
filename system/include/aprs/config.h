#ifndef CONFIG_H_
#define CONFIG_H_

#define CONFIG_AFSK_RX_BUFLEN 256
#define CONFIG_AFSK_TX_BUFLEN 256

#define CONFIG_AX25_FRAME_BUF_LEN 330


/**
 * AFSK RX timeout in ms, set to -1 to disable.
 * $WIZ$ type = "int"
 * $WIZ$ min = -1
 */
#define CONFIG_AFSK_RXTIMEOUT 0


/**
 * AFSK Preamble length in [ms], before starting transmissions.
 * $WIZ$ type = "int"
 * $WIZ$ min = 1
 */
#include <stdint.h>

//extern uint8_t kiss_txdelay;
//#define CONFIG_AFSK_PREAMBLE_LEN (kiss_txdelay*10UL)
#define CONFIG_AFSK_PREAMBLE_LEN 500UL			/// 300

/**
 * AFSK Trailer length in [ms], before stopping transmissions.
 * $WIZ$ type = "int"
 * $WIZ$ min = 1
 */
//extern uint8_t kiss_txtail;
//#define CONFIG_AFSK_TRAILER_LEN (kiss_txtail*10UL)
#define CONFIG_AFSK_TRAILER_LEN 50UL


#endif /* CONFIG_H_ */
