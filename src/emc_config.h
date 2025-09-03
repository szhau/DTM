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
#define AUTO_START_RX_MODE 1
#define AUTO_START_CHANNEL 3     /* Channel 3 = 2410 MHz */

#endif /* EMC_CONFIG_H */