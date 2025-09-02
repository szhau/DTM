/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <SEGGER_RTT.h>

#include "transport/dtm_transport.h"

/* External function from dtm_cmd_core */
extern uint16_t dtm_cmd_put(uint16_t cmd);

/* Set to 1 to enable auto-start RX mode on channel 5 */
#define AUTO_START_RX_MODE 1
#define AUTO_START_CHANNEL 5  /* Channel 5 = 2410 MHz */

int main(void)
{
	int err;
	uint16_t response;

	printk("Starting Direct Test Mode with Shell Interface\n");

#if AUTO_START_RX_MODE
	SEGGER_RTT_WriteString(0, "Auto-start RX mode enabled on channel 5\n");
#endif

	err = dtm_tr_init();
	if (err) {
		printk("Error initializing DTM transport: %d\n", err);
		return err;
	}

	printk("DTM Ready - Shell commands available\n");
	printk("Type 'dtm' to see available commands\n");

#if AUTO_START_RX_MODE
	/* Auto-start RX mode */
	printk("Auto-starting RX on channel %d (%.0f MHz)\n", 
	       AUTO_START_CHANNEL, 2402.0 + AUTO_START_CHANNEL * 2);
	
	/* Wait for system to stabilize */
	k_sleep(K_MSEC(100));
	
	/* Reset DTM first */
	response = dtm_cmd_put(0x0000);
	printk("DTM Reset - Response: 0x%04X\n", response);
	SEGGER_RTT_WriteString(0, "DTM Reset complete\n");
	
	k_sleep(K_MSEC(50));
	
	/* Start RX on configured channel
	 * RX command: bits 15-14 = 0x1 (RECEIVER_TEST)
	 * Channel is specified in bits 7-2
	 */
	uint16_t rx_cmd = 0x4000 | (AUTO_START_CHANNEL << 2);
	response = dtm_cmd_put(rx_cmd);
	printk("RX started on channel %d (%.0f MHz) - Response: 0x%04X\n", 
	       AUTO_START_CHANNEL, 2402.0 + AUTO_START_CHANNEL * 2, response);
	
	SEGGER_RTT_WriteString(0, "===========================================\n");
	SEGGER_RTT_WriteString(0, "RX MODE ACTIVE: Channel 5 (2410 MHz)\n");
	SEGGER_RTT_WriteString(0, "Receiver is listening...\n");
	SEGGER_RTT_WriteString(0, "To stop: Use shell command 'dtm end'\n");
	SEGGER_RTT_WriteString(0, "===========================================\n");
	
	if ((response & 0x8000) == 0x8000) {
		printk("RX successfully started!\n");
		SEGGER_RTT_WriteString(0, "SUCCESS: Receiver is running\n");
	} else {
		printk("Warning: Unexpected response from RX command\n");
		SEGGER_RTT_WriteString(0, "WARNING: Check response code\n");
	}
#endif

	/* Shell runs in background threads, main just sleeps */
	uint32_t counter = 0;
	for (;;) {
		k_sleep(K_SECONDS(10));
#if AUTO_START_RX_MODE
		counter += 10;
		printk("RX active for %u seconds\n", counter);
		SEGGER_RTT_WriteString(0, "Receiver still listening on channel 5\n");
#endif
	}
}
