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
#include "emc_config.h"

/* External function from dtm_cmd_core */
extern uint16_t dtm_cmd_put(uint16_t cmd);

int main(void)
{
	int err;
	uint16_t response;

	err = dtm_tr_init();
	if (err) {
#if !EMC_TEST_MODE
		printk("Error initializing DTM transport: %d\n", err);
#endif
		return err;
	}

#if !EMC_TEST_MODE
	printk("Starting Direct Test Mode with Shell Interface\n");
	printk("DTM Ready - Shell commands available\n");
	printk("Type 'dtm' to see available commands\n");
#endif

#if AUTO_START_RX_MODE
	/* Wait for system to stabilize */
	k_sleep(K_MSEC(100));
	
	/* Reset DTM first */
	response = dtm_cmd_put(0x0000);
#if !EMC_TEST_MODE
	printk("DTM Reset - Response: 0x%04X\n", response);
#endif
	
	k_sleep(K_MSEC(50));
	
	/* Start RX on configured channel
	 * RX command: bits 15-14 = 0x1 (RECEIVER_TEST)
	 * Channel is specified in bits 7-2
	 */
	uint16_t rx_cmd = 0x4000 | (AUTO_START_CHANNEL << 2);
	response = dtm_cmd_put(rx_cmd);
	
#if !EMC_TEST_MODE
	printk("Auto-starting RX on channel %d (%d MHz)\n", 
	       AUTO_START_CHANNEL, 2404 + AUTO_START_CHANNEL * 2);
	printk("RX started - Response: 0x%04X\n", response);
	if ((response & 0x8000) != 0x8000) {
		printk("Warning: Unexpected response from RX command\n");
	}
#endif
#endif

	/* Main loop - minimal activity during EMC testing */
	for (;;) {
#if EMC_TEST_MODE
		/* EMC Test Mode: Minimal CPU activity for clean measurements */
		k_sleep(K_SECONDS(60));  /* Sleep for 1 minute intervals */
#else
		/* Debug Mode: Periodic status updates */
		k_sleep(K_SECONDS(5));
#if AUTO_START_RX_MODE
		static uint32_t counter = 0;
		counter += 5;
		printk("\n[STATUS] RX Test Running - %u seconds elapsed\n", counter);
		printk("         Channel %d (%d MHz) - Waiting for packets...\n", 
		       AUTO_START_CHANNEL, 2404 + AUTO_START_CHANNEL * 2);
#endif
#endif
	}
}
