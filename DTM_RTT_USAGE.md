# DTM RTT Usage Guide

## Overview
Direct Test Mode (DTM) with RTT transport provides both:
1. Shell commands for easy testing
2. Real-time packet reception output via RTT

## RTT Channel Layout
- **Channel 0**: DTM shell commands and packet reception output
- **Channel 1**: Log output (if needed)

## Real-Time Packet Reception Output

When running RX tests, the system provides dynamic, real-time packet statistics via RTT, similar to nRF Connect for Desktop's Direct Test Mode interface:

### RX Test Output Features
- **Test Start Notification**: Shows channel and frequency
- **Dynamic Updates**: Statistics update every 10 packets or 2 seconds
- **Real-Time Statistics**: 
  - Channel number
  - Total packet count
  - Packet rate (packets/second)
  - RSSI value in dBm
  - CRC error count
- **Test Summary**: Final statistics when test ends

### Example RX Test Output
```
===== RX Test Started =====
Channel: 20 (2440 MHz)
Monitoring packets... Updates every 10 packets or 2 seconds

[RX] Ch:20 | Total:   10 | RSSI:-45 dBm | Errors:0
[RX] Ch:20 | Total:   20 | RSSI:-44 dBm | Errors:1
[RX] Ch:20 | Total:   42 | Rate: 625 pkt/s | RSSI:-46 dBm | Errors:2
[RX] Ch:20 | Total:   87 | Rate: 620 pkt/s | RSSI:-45 dBm | Errors:2
[RX] Ch:20 | Total:  156 | Rate: 625 pkt/s | RSSI:-44 dBm | Errors:3

===== RX Test Ended =====
Total packets received: 156
Total CRC errors: 3
Channel: 20 (2440 MHz)
```

## Shell Commands via RTT

### Basic Commands
```bash
dtm reset                    # Reset DTM to initial state
dtm rx_test <channel>        # Start RX test (0-39)
dtm tx_carrier <channel>     # Start continuous TX carrier
dtm tx_test <ch> [len]       # Start modulated TX test
dtm tx_power <dBm>          # Set TX power
dtm end                      # End test and show packet count
dtm raw <hex>               # Send raw 2-byte DTM command
```

### Usage Examples

#### Start RX Test and Monitor Packets
```bash
# Start receiving on channel 20 (2440 MHz)
dtm rx_test 20
# Watch RTT output for real-time packet info
# ...
dtm end
# Shows: End Test - Response: 0x8XXX
#        Packets received: XXX
```

#### EMC Receiver Spurious Emissions Test
```bash
# Set desired channel for emissions testing
dtm rx_test 20
# Receiver and LO run continuously
# Measure with spectrum analyzer
# ...
dtm end
```

#### TX Carrier for EMC Testing
```bash
dtm tx_power 8      # Set to 8 dBm
dtm tx_carrier 20   # Start carrier on 2440 MHz
# Measure with spectrum analyzer
dtm end
```

## Connecting via J-Link RTT

### Option 1: RTT Viewer (GUI)
```
JLinkRTTViewer.exe
```
- Select target device (nRF5340_xxAA_NET)
- Connect to board
- Type commands in Terminal 0

### Option 2: RTT Client (CLI)
```bash
JLinkRTTClient
```
- Connects automatically if J-Link is running
- Type commands directly

## Channel Frequency Mapping
| Channel | Frequency | Channel | Frequency |
|---------|-----------|---------|-----------|
| 0       | 2402 MHz  | 20      | 2440 MHz  |
| 10      | 2420 MHz  | 30      | 2460 MHz  |
| 19      | 2438 MHz  | 39      | 2480 MHz  |

Formula: Frequency (MHz) = 2402 + (channel × 2)

## Benefits Over UART
- No physical UART pins needed
- Real-time packet reception monitoring
- Works with custom boards without UART
- Shell commands for easier testing
- Compatible with DTM protocol specifications

## Note for nRF Connect Desktop
While nRF Connect for Desktop's Direct Test Mode app expects UART, this RTT implementation provides equivalent functionality with enhanced debugging via real-time packet output. The packet count reporting follows standard DTM protocol (0x8XXX response format).