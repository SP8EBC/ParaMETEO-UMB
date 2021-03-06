#ifndef AX25_H_
#define AX25_H_

#ifndef CCC
#include "stm32f10x_conf.h"
#endif
#include <stdbool.h>

#include <aprs/fifo.h>
#include <aprs/afsk.h>

#include <aprs/macros.h>
#include <aprs/config.h>

/**
 * Maximum size of a AX25 frame.
 */
#define AX25_MIN_FRAME_LEN 18

/**
 * CRC computation on correct AX25 packets should
 * give this result (don't ask why).
 */
#define AX25_CRC_CORRECT 0xF0B8


#define AX25_CTRL_UI      0x03
#define AX25_PID_NOLAYER3 0xF0

#define HDLC_FLAG  0x7E
#define HDLC_RESET 0x7F
#define AX25_ESC   0x1B


struct AX25Msg; // fwd declaration

/**
 * Type for AX25 messages callback.
 */
typedef void (*ax25_callback_t)(struct AX25Msg *msg);


typedef struct AX25Ctx
{

//	uint8_t buf[CONFIG_AX25_FRAME_BUF_LEN]; ///< buffer for received chars
	Afsk *afsk;

	uint16_t frm_len;   ///< received frame length.
	uint16_t crc_in;  ///< CRC for current received frame
	uint16_t crc_out; ///< CRC of current sent frame

	ax25_callback_t hook; ///< Hook function to be called when a message is received

	bool raw;
	bool sync;   ///< True if we have received a HDLC flag.
	bool escape; ///< True when we have to escape the following char.
	uint8_t dcd_state;
	bool dcd;

} AX25Ctx;


/**
 * AX25 Call sign.
 */
typedef struct AX25Call
{
	char call[6]; ///< Call string, max 6 character
	uint8_t ssid; ///< SSID (secondary station ID) for the call
} AX25Call;


/**
 * Create an AX25Call structure on the fly.
 * \param str callsign, can be 6 characters or shorter.
 * \param id  ssid associated with the callsign.
 */
#define AX25_CALL(str, id) {.call = (str), .ssid = (id) }
#define AX25_PATH(dst, src, ...) { dst, src, ## __VA_ARGS__ }


/**
 * Maximum number of Repeaters in a AX25 message.
 */
#define AX25_MAX_RPT 8


/**
 * AX25 Message.
 * Used to handle AX25 sent/received messages.
 */
typedef struct AX25Msg
{

	AX25Call src;  ///< Source adress
	AX25Call dst;  ///< Destination address
	AX25Call rpt_lst[AX25_MAX_RPT]; ///< List of repeaters
	uint8_t rpt_cnt; ///< Number of repeaters in this message
	uint8_t rpt_flags; ///< Has-been-repeated flags for each repeater (bit-mapped)
	#define AX25_REPEATED(msg, idx) ((msg)->rpt_flags & BV(idx))
	uint16_t ctrl; ///< AX25 control field
	uint8_t pid;   ///< AX25 PID field
	const uint8_t *info; ///< Pointer to the info field (payload) of the message
	uint16_t len;    ///< Payload length
	char raw_data[CONFIG_AX25_FRAME_BUF_LEN];   /// Surowa zawarto�� ramki przekopiowana z Ctx->buff
	short int raw_msg_len;				// wielkosc surowej ramki

} AX25Msg;

extern AX25Msg msg;
extern char new_msg_rx;

#ifdef __cplusplus
extern "C"
{
#endif
	
/*********************************************************************************************************************/
void ax25_poll(AX25Ctx *ctx);
/*********************************************************************************************************************/

/*********************************************************************************************************************/
void ax25_sendVia(AX25Ctx *ctx, const AX25Call *path, uint16_t path_len, const void *_buf, uint16_t len);
/*********************************************************************************************************************/

/*********************************************************************************************************************/
void ax25_sendRaw(AX25Ctx *ctx, const void *_buf, uint16_t len);
/*********************************************************************************************************************/

/*********************************************************************************************************************/
void ax25_init(AX25Ctx *ctx, Afsk *afsk, bool raw, ax25_callback_t hook);
/*********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* AX25_H_ */
