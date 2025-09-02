/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/sys/printk.h>

#include "transport/dtm_transport.h"

int main(void)
{
	int err;

	printk("Starting Direct Test Mode with Shell Interface\n");

	err = dtm_tr_init();
	if (err) {
		printk("Error initializing DTM transport: %d\n", err);
		return err;
	}

	printk("DTM Ready - Shell commands available\n");
	printk("Type 'dtm' to see available commands\n");

	/* Shell runs in background threads, main just sleeps */
	for (;;) {
		k_sleep(K_SECONDS(1));
	}
}
