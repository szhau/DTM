/*
 * EMC Test Configuration
 * 
 * This file centralizes EMC test mode configuration to ensure consistency
 * across all source files.
 */

#ifndef EMC_CONFIG_H
#define EMC_CONFIG_H

/* EMC Compliance Test Mode */
#define EMC_TEST_MODE 1          /* Set to 1 for clean EMC testing, 0 for debug mode */

/* Auto-start configuration */
#define AUTO_START_TX_MODE 1
#define AUTO_START_CHANNEL 19    /* Channel 19 = 2440 MHz (2402 + 19*2 = 2440) */

#endif /* EMC_CONFIG_H */