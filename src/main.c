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

/* Set to 1 to enable auto-start RX mode on channel 4 */
#define AUTO_START_RX_MODE 1
#define AUTO_START_CHANNEL 3  /* Channel 3 = 2410 MHz  */

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
	       AUTO_START_CHANNEL, 2404.0 + AUTO_START_CHANNEL * 2);
	
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
	printk("[DEBUG] Sending RX command: 0x%04X for channel %d\n", rx_cmd, AUTO_START_CHANNEL);
	response = dtm_cmd_put(rx_cmd);
	printk("RX started on channel %d (%.0f MHz) - Response: 0x%04X\n", 
	       AUTO_START_CHANNEL, 2404.0 + AUTO_START_CHANNEL * 2, response);
	
	SEGGER_RTT_WriteString(0, "===========================================\n");
	SEGGER_RTT_WriteString(0, "RX MODE ACTIVE: Channel 3 (2410 MHz)\n");
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

	/* Shell runs in background threads, main provides periodic status */
	uint32_t counter = 0;
	uint32_t last_status_time = k_uptime_get_32();
	
	for (;;) {
		k_sleep(K_SECONDS(5));
#if AUTO_START_RX_MODE
		counter += 5;
		uint32_t now = k_uptime_get_32();
		
		/* Print periodic status every 5 seconds */
		printk("\n[STATUS] RX Test Running - %u seconds elapsed\n", counter);
		printk("         Channel 3 (2410 MHz) - Waiting for packets...\n");
		
		/* You could query DTM status here if needed */
		SEGGER_RTT_WriteString(0, "=== Receiver Active ===\n");
#endif
	}
}
