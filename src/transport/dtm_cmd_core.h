#ifndef DTM_CMD_CORE_H_
#define DTM_CMD_CORE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DTM_CMD_CORE_PRESENT

/* Upper bits of packet length – shared state between transports */
extern uint8_t upper_len;

/* Decode a 16-bit DTM command and execute it via nrfx_dtm library.
 * Returns the 16-bit response that shall be sent back to the tester.
 */
uint16_t dtm_cmd_put(uint16_t cmd);

#ifdef __cplusplus
}
#endif

#endif /* DTM_CMD_CORE_H_ */
