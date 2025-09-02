/*
 * Direct Test Mode transport over SEGGER RTT (two-wire 2-octet protocol)
 *
 * This is a trimmed-down version of the original UART two-wire transport
 * (dtm_uart_twowire.c).  It replaces UART polling with SEGGER RTT functions
 * so that boards without an exposed UART can still run DTM using the RTT
 * backend.
 */

#include <SEGGER_RTT.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <dtm.h>

#include "dtm_transport.h"
#include "dtm_cmd_core.h"

LOG_MODULE_REGISTER(dtm_rtt_tr, CONFIG_DTM_TRANSPORT_LOG_LEVEL);

/* Mask/enum definitions copied from original UART implementation ------------------ */

/* Mask of the CTE type in the CTEInfo. */
#define LE_CTE_TYPE_MASK 0x03
/* Position of the CTE type in the CTEInfo. */
#define LE_CTE_TYPE_POS 0x06
/* Mask of the CTE Time in the CTEInfo. */
#define LE_CTE_CTETIME_MASK 0x1F

/* DTM command parameter: Mask of the Antenna Number. */
#define LE_ANTENNA_NUMBER_MASK 0x7F
/* DTM command parameter: Position of the Antenna switch pattern. */
#define LE_ANTENNA_SWITCH_PATTERN_POS 0x07
/* DTM command parameter: Mask of the Antenna switch pattern. */
#define LE_ANTENNA_SWITCH_PATTERN_MASK 0x80

/* Position of power level in the DTM power level set response. */
#define LE_TRANSMIT_POWER_RESPONSE_LVL_POS (0x01)
/* Mask of the power level in the DTM power level set response. */
#define LE_TRANSMIT_POWER_RESPONSE_LVL_MASK (0x1FE)
/* Maximum power level bit in the power level set response. */
#define LE_TRANSMIT_POWER_MAX_LVL_BIT BIT(0x0A)
/* Minimum power level bit in the power level set response. */
#define LE_TRANSMIT_POWER_MIN_LVL_BIT BIT(0x09)

/* Response event data shift. */
#define DTM_RESPONSE_EVENT_SHIFT 0x01

/* DTM command parameter: Upper bits mask. */
#define LE_UPPER_BITS_MASK 0xC0
/* DTM command parameter: Upper bits position. */
#define LE_UPPER_BITS_POS 0x04

/* The DTM maximum wait time in milliseconds for the RTT command second byte. */
#define DTM_RTT_SECOND_BYTE_MAX_DELAY 5

/* ---- Declarations reused from original implementation ------------------------- */
/* Many enums / helpers are needed from the original file.  Instead of duplicating
 * everything, include the header of the original implementation.  Because those
 * definitions are all in dtm_uart_twowire.c (not a header) we have to replicate
 * only the small subset used in the code below (upper_len & dtm_cmd_put).  To
 * avoid excessive duplication, just declare dtm_cmd_put here as an extern coming
 * from the original module, then call it.  This keeps the command decoding logic
 * in one place.
 */
extern uint16_t dtm_cmd_put(uint16_t cmd);

/* Store upper bits (replicated symbol from original) */
extern uint8_t upper_len;
/* ------------------------------------------------------------------------------ */

int dtm_tr_init(void)
{
	int err;
	
	/* Note: Shell is using RTT channel 0, so we coexist with it */
	/* The shell commands in dtm_shell_commands.c will call dtm_cmd_put directly */
	
	err = dtm_init(NULL);
	if (err) {
		LOG_ERR("Error during DTM initialization: %d", err);
		return err;
	}

	LOG_INF("DTM RTT transport initialised");
	LOG_INF("Use shell commands: dtm reset, dtm rx_test <ch>, dtm tx_carrier <ch>, dtm end");
	return 0;
}

union dtm_tr_packet dtm_tr_get(void)
{
	bool is_msb_read = false;
	union dtm_tr_packet tmp;
	uint8_t rx_byte;
	uint16_t dtm_cmd = 0;
	int64_t msb_time = 0;
	uint8_t buffer[2];
	unsigned num_bytes;

	for (;;) {
		if (!is_msb_read) {
			/* Try to read both bytes at once for better performance */
			num_bytes = SEGGER_RTT_Read(0, buffer, 2);
			if (num_bytes == 2) {
				/* Got both bytes at once */
				dtm_cmd = (buffer[0] << 8) | buffer[1];
				LOG_INF("Received 0x%04x command via RTT", dtm_cmd);
				tmp.twowire = dtm_cmd;
				return tmp;
			} else if (num_bytes == 1) {
				/* Got first byte only */
				is_msb_read = true;
				dtm_cmd = buffer[0] << 8;
				msb_time = k_uptime_get();
			} else {
				/* No data, yield CPU but don't sleep */
				k_yield();
				continue;
			}
		} else {
			/* Waiting for second byte */
			num_bytes = SEGGER_RTT_Read(0, &rx_byte, 1);
			if (num_bytes == 0) {
				if ((k_uptime_get() - msb_time) > DTM_RTT_SECOND_BYTE_MAX_DELAY) {
					/* Timeout - reset and wait for new command */
					is_msb_read = false;
				} else {
					/* Still waiting, just yield */
					k_yield();
				}
				continue;
			}

			/* Got second byte */
			dtm_cmd |= rx_byte;
			LOG_INF("Received 0x%04x command via RTT", dtm_cmd);
			tmp.twowire = dtm_cmd;
			return tmp;
		}
	}
}

int dtm_tr_process(union dtm_tr_packet cmd)
{
	uint16_t request = cmd.twowire;
	uint16_t response;

	LOG_INF("Processing 0x%04x command", request);

	response = dtm_cmd_put(request);
	LOG_INF("Sending 0x%04x response", response);

	uint8_t buf[2] = { (uint8_t)((response >> 8) & 0xFF), (uint8_t)(response & 0xFF) };
	SEGGER_RTT_Write(0, buf, 2);

	return 0;
}
