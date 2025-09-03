#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <dtm.h>
#include "dtm_cmd_core.h"

LOG_MODULE_REGISTER(dtm_cmd_core, CONFIG_DTM_TRANSPORT_LOG_LEVEL);

/* ---------------- Definitions (moved from UART implementation) ---------------- */

/* Mask/enum definitions */
#define LE_CTE_TYPE_MASK 0x03
#define LE_CTE_TYPE_POS 0x06
#define LE_CTE_CTETIME_MASK 0x1F
#define LE_ANTENNA_NUMBER_MASK 0x7F
#define LE_ANTENNA_SWITCH_PATTERN_POS 0x07
#define LE_ANTENNA_SWITCH_PATTERN_MASK 0x80
#define LE_TRANSMIT_POWER_RESPONSE_LVL_POS (0x01)
#define LE_TRANSMIT_POWER_RESPONSE_LVL_MASK (0x1FE)
#define LE_TRANSMIT_POWER_MAX_LVL_BIT BIT(0x0A)
#define LE_TRANSMIT_POWER_MIN_LVL_BIT BIT(0x09)
#define DTM_RESPONSE_EVENT_SHIFT 0x01
#define LE_UPPER_BITS_MASK 0xC0
#define LE_UPPER_BITS_POS 0x04
#define LE_TEST_END_MAX_RANGE 10

/* Event opcodes */
#define LE_TEST_STATUS_EVENT_ERROR   0x0000
#define LE_TEST_STATUS_EVENT_SUCCESS 0x0001
#define LE_PACKET_REPORTING_EVENT    0x8000

/* Upper bits of packet length (shared) */
uint8_t upper_len;

/* DTM command codes */
enum dtm_cmd_code {
    LE_TEST_SETUP = 0x0,
    LE_RECEIVER_TEST = 0x1,
    LE_TRANSMITTER_TEST = 0x2,
    LE_TEST_END  = 0x3,
};

/* DTM Test Setup Control codes */
enum dtm_ctrl_code {
    LE_TEST_SETUP_RESET = 0x00,
    LE_TEST_SETUP_SET_UPPER = 0x01,
    LE_TEST_SETUP_SET_PHY = 0x02,
    LE_TEST_SETUP_SELECT_MODULATION = 0x03,
    LE_TEST_SETUP_READ_SUPPORTED = 0x04,
    LE_TEST_SETUP_READ_MAX = 0x05,
    LE_TEST_SETUP_CONSTANT_TONE_EXTENSION = 0x06,
    LE_TEST_SETUP_CONSTANT_TONE_EXTENSION_SLOT = 0x07,
    LE_TEST_SETUP_ANTENNA_ARRAY = 0x08,
    LE_TEST_SETUP_TRANSMIT_POWER = 0x09,
};

enum dtm_pkt_type {
    DTM_PKT_PRBS9 = 0x0,
    DTM_PKT_0X0F = 0x1,
    DTM_PKT_0X55 = 0x2,
    DTM_PKT_0XFF_OR_VS = 0x3,
};

/* Flags for LE_TEST_SETUP_READ_SUPPORTED response */
enum dtm_supported_feature_flag {
    LE_TEST_SETUP_DLE_SUPPORTED         = BIT(1),
    LE_TEST_SETUP_2M_PHY_SUPPORTED      = BIT(2),
    LE_TEST_STABLE_MODULATION_SUPPORTED = BIT(3),
    LE_TEST_CODED_PHY_SUPPORTED         = BIT(4),
    LE_TEST_CTE_SUPPORTED               = BIT(5),
    DTM_LE_ANTENNA_SWITCH               = BIT(6),
    DTM_LE_AOD_1US_TANSMISSION          = BIT(7),
    DTM_LE_AOD_1US_RECEPTION            = BIT(8),
    DTM_LE_AOA_1US_RECEPTION            = BIT(9),
};

/* Parameter ranges used by READ_SUPPORTED / READ_MAX operations */
enum dtm_read_supported_code {
    LE_TEST_SUPPORTED_TX_OCTETS_MIN_RANGE = 0x00,
    LE_TEST_SUPPORTED_TX_OCTETS_MAX_RANGE = 0x03,
    LE_TEST_SUPPORTED_TX_TIME_MIN_RANGE   = 0x04,
    LE_TEST_SUPPORTED_TX_TIME_MAX_RANGE   = 0x07,
    LE_TEST_SUPPORTED_RX_OCTETS_MIN_RANGE = 0x08,
    LE_TEST_SUPPORTED_RX_OCTETS_MAX_RANGE = 0x0B,
    LE_TEST_SUPPORTED_RX_TIME_MIN_RANGE   = 0x0C,
    LE_TEST_SUPPORTED_RX_TIME_MAX_RANGE   = 0x0F,
    LE_TEST_SUPPORTED_CTE_LENGTH          = 0x10,
};

enum dtm_feature_read_code {
    LE_TEST_FEATURE_READ_MIN_RANGE = 0x00,
    LE_TEST_FEATURE_READ_MAX_RANGE = 0x03,
};

/* Static helper prototypes */
static uint16_t on_test_setup_cmd(enum dtm_ctrl_code control, uint8_t parameter);
static uint16_t on_test_end_cmd(enum dtm_ctrl_code control, uint8_t parameter);
static uint16_t on_test_rx_cmd(uint8_t chan);
static uint16_t on_test_tx_cmd(uint8_t chan, uint8_t length, enum dtm_pkt_type type);

/* Local helpers copied from UART transport to satisfy feature/max queries */
static int features_read(uint8_t parameter, uint16_t *ret)
{
    struct dtm_supp_features features;

    if (parameter > LE_TEST_FEATURE_READ_MAX_RANGE) {
        return -EINVAL;
    }

    features = dtm_setup_read_features();

    *ret = 0;
    *ret |= (features.data_len_ext ? LE_TEST_SETUP_DLE_SUPPORTED : 0);
    *ret |= (features.phy_2m ? LE_TEST_SETUP_2M_PHY_SUPPORTED : 0);
    *ret |= (features.stable_mod ? LE_TEST_STABLE_MODULATION_SUPPORTED : 0);
    *ret |= (features.coded_phy ? LE_TEST_CODED_PHY_SUPPORTED : 0);
    *ret |= (features.cte ? LE_TEST_CTE_SUPPORTED : 0);
    *ret |= (features.ant_switching ? DTM_LE_ANTENNA_SWITCH : 0);
    *ret |= (features.aod_1us_tx ? DTM_LE_AOD_1US_TANSMISSION : 0);
    *ret |= (features.aod_1us_rx ? DTM_LE_AOD_1US_RECEPTION : 0);
    *ret |= (features.aoa_1us_rx ? DTM_LE_AOA_1US_RECEPTION : 0);

    return 0;
}

static int read_max(uint8_t parameter, uint16_t *ret)
{
    int err;

    switch (parameter) {
    case LE_TEST_SUPPORTED_TX_OCTETS_MIN_RANGE ... LE_TEST_SUPPORTED_TX_OCTETS_MAX_RANGE:
        err = dtm_setup_read_max_supported_value(DTM_MAX_SUPPORTED_TX_OCTETS, ret);
        break;
    case LE_TEST_SUPPORTED_TX_TIME_MIN_RANGE ... LE_TEST_SUPPORTED_TX_TIME_MAX_RANGE:
        err = dtm_setup_read_max_supported_value(DTM_MAX_SUPPORTED_TX_TIME, ret);
        break;
    case LE_TEST_SUPPORTED_RX_OCTETS_MIN_RANGE ... LE_TEST_SUPPORTED_RX_OCTETS_MAX_RANGE:
        err = dtm_setup_read_max_supported_value(DTM_MAX_SUPPORTED_RX_OCTETS, ret);
        break;
    case LE_TEST_SUPPORTED_RX_TIME_MIN_RANGE ... LE_TEST_SUPPORTED_RX_TIME_MAX_RANGE:
        err = dtm_setup_read_max_supported_value(DTM_MAX_SUPPORTED_RX_TIME, ret);
        break;
    case LE_TEST_SUPPORTED_CTE_LENGTH:
        err = dtm_setup_read_max_supported_value(DTM_MAX_SUPPORTED_CTE_LENGTH, ret);
        break;
    default:
        return -EINVAL;
    }

    *ret = *ret << DTM_RESPONSE_EVENT_SHIFT;
    return err;
}

/* ---------------- Public API ---------------- */
uint16_t dtm_cmd_put(uint16_t cmd)
{
    enum dtm_cmd_code cmd_code = (cmd >> 14) & 0x03;
    uint8_t chan = (cmd >> 8) & 0x3F;
    uint8_t length = (cmd >> 2) & 0x3F;
    enum dtm_pkt_type type = (enum dtm_pkt_type)(cmd & 0x03);
    enum dtm_ctrl_code control = (cmd >> 8) & 0x3F;
    uint8_t parameter = (uint8_t)cmd;

    switch (cmd_code) {
    case LE_TEST_SETUP:
        return on_test_setup_cmd(control, parameter);
    case LE_TEST_END:
        return on_test_end_cmd(control, parameter);
    case LE_RECEIVER_TEST:
        return on_test_rx_cmd(chan);
    case LE_TRANSMITTER_TEST:
        return on_test_tx_cmd(chan, length, type);
    default:
        LOG_ERR("Unknown cmd code %d", cmd_code);
        return LE_TEST_STATUS_EVENT_ERROR;
    }
}

/* ---------------- Internal helpers ---------------- */
static int reset_dtm(uint8_t parameter)
{
    ARG_UNUSED(parameter);
    upper_len = 0;
    return dtm_setup_reset();
}

static int upper_set(uint8_t parameter)
{
    upper_len = (parameter << LE_UPPER_BITS_POS) & LE_UPPER_BITS_MASK;
    return 0;
}

static uint16_t on_test_setup_cmd(enum dtm_ctrl_code control, uint8_t parameter)
{
    uint16_t ret = 0;
    int err = 0;

    switch (control) {
    case LE_TEST_SETUP_RESET:
        err = reset_dtm(parameter);
        break;
    case LE_TEST_SETUP_SET_UPPER:
        err = upper_set(parameter);
        break;
    case LE_TEST_SETUP_READ_SUPPORTED:
        err = features_read(parameter, &ret);
        break;
    case LE_TEST_SETUP_READ_MAX:
        err = read_max(parameter, &ret);
        break;
    default:
        err = -EINVAL;
        break;
    }

    return err ? LE_TEST_STATUS_EVENT_ERROR : (LE_TEST_STATUS_EVENT_SUCCESS | ret);
}

static uint16_t on_test_end_cmd(enum dtm_ctrl_code control, uint8_t parameter)
{
    uint16_t cnt;
    int err;

    if (control || (parameter > LE_TEST_END_MAX_RANGE)) {
        return LE_TEST_STATUS_EVENT_ERROR;
    }

    err = dtm_test_end(&cnt);

    return err ? LE_TEST_STATUS_EVENT_ERROR : (LE_PACKET_REPORTING_EVENT | cnt);
}

static uint16_t on_test_rx_cmd(uint8_t chan)
{
    printk("[DEBUG] DTM RX command: channel %d\n", chan);
    return dtm_test_receive(chan) ? LE_TEST_STATUS_EVENT_ERROR : LE_TEST_STATUS_EVENT_SUCCESS;
}

static uint16_t on_test_tx_cmd(uint8_t chan, uint8_t length, enum dtm_pkt_type type)
{
    enum dtm_packet pkt;

    switch (type) {
    case DTM_PKT_PRBS9: pkt = DTM_PACKET_PRBS9; break;
    case DTM_PKT_0X0F: pkt = DTM_PACKET_0F; break;
    case DTM_PKT_0X55: pkt = DTM_PACKET_55; break;
    case DTM_PKT_0XFF_OR_VS: pkt = DTM_PACKET_FF_OR_VENDOR; break;
    default: return LE_TEST_STATUS_EVENT_ERROR;
    }

    length = (length & ~LE_UPPER_BITS_MASK) | upper_len;

    return dtm_test_transmit(chan, length, pkt) ? LE_TEST_STATUS_EVENT_ERROR : LE_TEST_STATUS_EVENT_SUCCESS;
}
