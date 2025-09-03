/*
 * DTM Shell Commands for easy testing via RTT
 * Provides text-based commands that translate to DTM 2-byte protocol
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include "transport/dtm_transport.h"

/* External function from dtm_cmd_core */
extern uint16_t dtm_cmd_put(uint16_t cmd);

static int cmd_dtm_reset(const struct shell *sh, size_t argc, char **argv)
{
	uint16_t response = dtm_cmd_put(0x0000);
	shell_print(sh, "DTM Reset - Response: 0x%04X", response);
	return 0;
}

static int cmd_dtm_rx_test(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(sh, "Usage: rx_test <channel>");
		shell_print(sh, "  channel: 0-39 (2404-2482 MHz)");
		return -EINVAL;
	}
	
	uint8_t channel = atoi(argv[1]);
	if (channel > 39) {
		shell_print(sh, "Error: Channel must be 0-39");
		return -EINVAL;
	}
	
	/* RX Test command: bits 15-14 = 0x1 (RECEIVER_TEST)
	 * bits 7-2 = channel, other bits = 0
	 */
	uint16_t cmd = 0x4000 | (channel << 2);  /* 0x4000 = LE_RECEIVER_TEST */
	uint16_t response = dtm_cmd_put(cmd);
	shell_print(sh, "RX Test on channel %d (%.0f MHz) - Response: 0x%04X", 
	            channel, 2404.0 + channel * 2, response);
	return 0;
}

static int cmd_dtm_tx_carrier(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(sh, "Usage: tx_carrier <channel>");
		shell_print(sh, "  channel: 0-39 (2404-2482 MHz)");
		return -EINVAL;
	}
	
	uint8_t channel = atoi(argv[1]);
	if (channel > 39) {
		shell_print(sh, "Error: Channel must be 0-39");
		return -EINVAL;
	}
	
	/* TX Test command: bits 15-14 = 0x2 (TRANSMITTER_TEST)
	 * For carrier test, length = 0 (vendor specific for continuous carrier)
	 * bits 13-8 = length (0), bits 7-2 = channel, bits 1-0 = packet type (0)
	 */
	uint16_t cmd = 0x8000 | (channel << 2);  /* 0x8000 = LE_TRANSMITTER_TEST */
	uint16_t response = dtm_cmd_put(cmd);
	shell_print(sh, "TX Carrier on channel %d (%.0f MHz) - Response: 0x%04X", 
	            channel, 2404.0 + channel * 2, response);
	return 0;
}

static int cmd_dtm_tx_test(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2 && argc != 3) {
		shell_print(sh, "Usage: tx_test <channel> [length]");
		shell_print(sh, "  channel: 0-39 (2404-2482 MHz)");
		shell_print(sh, "  length: packet length 0-37 (default 37)");
		return -EINVAL;
	}
	
	uint8_t channel = atoi(argv[1]);
	uint8_t length = (argc == 3) ? atoi(argv[2]) : 37;
	
	if (channel > 39) {
		shell_print(sh, "Error: Channel must be 0-39");
		return -EINVAL;
	}
	if (length > 37) {
		shell_print(sh, "Error: Length must be 0-37");
		return -EINVAL;
	}
	
	/* TX Test command: bits 15-14 = 0x2 (TRANSMITTER_TEST)
	 * bits 13-8 = length, bits 7-2 = channel, bits 1-0 = packet type (PRBS9)
	 */
	uint16_t cmd = 0x8000 | (length << 8) | (channel << 2) | 0x00;  /* 0x00 = PRBS9 */
	uint16_t response = dtm_cmd_put(cmd);
	shell_print(sh, "TX Test on channel %d, length %d - Response: 0x%04X", 
	            channel, length, response);
	return 0;
}

static int cmd_dtm_tx_power(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(sh, "Usage: tx_power <power_dbm>");
		shell_print(sh, "  power_dbm: TX power in dBm (e.g., 0, 4, 8)");
		return -EINVAL;
	}
	
	int8_t power = atoi(argv[1]);
	
	/* Test Setup command with Set TX Power parameter
	 * bits 15-14 = 0x0 (TEST_SETUP), bits 13-8 = 0x02 (TX_POWER)
	 * bits 7-2 = power level
	 */
	uint16_t cmd = 0x0200 | ((power & 0x3F) << 2);
	uint16_t response = dtm_cmd_put(cmd);
	shell_print(sh, "Set TX Power to %d dBm - Response: 0x%04X", power, response);
	return 0;
}

static int cmd_dtm_end_test(const struct shell *sh, size_t argc, char **argv)
{
	uint16_t response = dtm_cmd_put(0xC000);
	shell_print(sh, "End Test - Response: 0x%04X", response);
	
	/* Extract packet count from response if it's a packet reporting event */
	if (response & 0x8000) {
		uint16_t packet_count = response & 0x7FFF;
		shell_print(sh, "  Packets received: %d", packet_count);
	}
	
	return 0;
}

static int cmd_dtm_raw(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(sh, "Usage: dtm_raw <hex_value>");
		shell_print(sh, "  hex_value: 4-digit hex (e.g., 8014 for RX ch 20)");
		return -EINVAL;
	}
	
	uint16_t cmd = strtol(argv[1], NULL, 16);
	uint16_t response = dtm_cmd_put(cmd);
	shell_print(sh, "Sent 0x%04X - Response: 0x%04X", cmd, response);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(dtm_cmds,
	SHELL_CMD(reset, NULL, "Reset DTM", cmd_dtm_reset),
	SHELL_CMD(rx_test, NULL, "Start RX test", cmd_dtm_rx_test),
	SHELL_CMD(tx_carrier, NULL, "Start TX carrier (continuous)", cmd_dtm_tx_carrier),
	SHELL_CMD(tx_test, NULL, "Start TX test (modulated)", cmd_dtm_tx_test),
	SHELL_CMD(tx_power, NULL, "Set TX power", cmd_dtm_tx_power),
	SHELL_CMD(end, NULL, "End test", cmd_dtm_end_test),
	SHELL_CMD(raw, NULL, "Send raw DTM command", cmd_dtm_raw),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(dtm, &dtm_cmds, "DTM test commands", NULL);