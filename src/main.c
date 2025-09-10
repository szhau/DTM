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

#if AUTO_START_TX_MODE
	/* Wait for system to stabilize */
	k_sleep(K_MSEC(100));
	
	/* Reset DTM first */
	response = dtm_cmd_put(0x0000);
#if !EMC_TEST_MODE
	printk("DTM Reset - Response: 0x%04X\n", response);
#endif
	
	k_sleep(K_MSEC(50));
	
	/* Start TX carrier on configured channel
	 * TX command: bits 15-14 = 0x2 (TRANSMITTER_TEST)
	 * Channel is specified in bits 13-8
	 * Length = 0x25 (37 bytes) in bits 7-2 for carrier test
	 * Packet type = 0x0 (PRBS9) in bits 1-0
	 */
	uint16_t tx_cmd = 0x8000 | (AUTO_START_CHANNEL << 8) | (0x25 << 2) | 0x0;
	response = dtm_cmd_put(tx_cmd);
	
#if !EMC_TEST_MODE
	printk("Auto-starting TX carrier on channel %d (%d MHz)\n", 
	       AUTO_START_CHANNEL, 2402 + AUTO_START_CHANNEL * 2);
	printk("TX started - Response: 0x%04X\n", response);
	if ((response & 0x8000) != 0x8000) {
		printk("Warning: Unexpected response from TX command\n");
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
#if AUTO_START_TX_MODE
		static uint32_t counter = 0;
		counter += 5;
		printk("\n[STATUS] TX Test Running - %u seconds elapsed\n", counter);
		printk("         Channel %d (%d MHz) - Transmitting carrier...\n", 
		       AUTO_START_CHANNEL, 2404 + AUTO_START_CHANNEL * 2);
#endif
#endif
	}
}
