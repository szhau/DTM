# DTM RTT Usage Guide

## Overview
Direct Test Mode (DTM) uses raw 2-byte commands, not shell commands. It communicates over RTT channel 0.

## RTT Channel Layout
- **Channel 0**: DTM command/response (2-byte protocol)
- **Channel 1**: Log output

## How to Use DTM over RTT

### 1. Connect with J-Link RTT Viewer
```
JLinkRTTViewer.exe
```
- Select your target device
- Connect to the board

### 2. Send DTM Commands
DTM expects 2-byte commands in the format:
- Byte 1 (MSB): Command type and upper bits
- Byte 2 (LSB): Parameters

### Common DTM Commands (2-byte hex values)

#### Reset/Stop Test
- `0x00 0x00` - Reset DTM

#### Receiver Test
- `0x80 0x00` - Start RX test on channel 0 (2402 MHz)
- `0x80 0x27` - Start RX test on channel 39 (2480 MHz)

#### Transmitter Test  
- `0xC0 0x00` - Start TX carrier on channel 0
- `0xC0 0x27` - Start TX carrier on channel 39

#### Set TX Power
- `0x02 0x09` - Set TX power (vendor specific)

#### End Test
- `0x00 0x00` - End test

### 3. View Responses
DTM sends 2-byte responses back on RTT channel 0:
- `0x00 0x00` - Success/ACK
- `0x00 0x01` - Error/NACK

### 4. View Logs
Switch to RTT channel 1 to see debug logs

## Testing Receiver Spurious Emissions
For EMC testing (EN 300 328 / EN 301 893):
1. Send RX start command: `0x80 0x14` (channel 20, 2440 MHz)
2. Receiver and LO will run continuously
3. Measure spurious emissions with spectrum analyzer
4. Send reset command: `0x00 0x00` to stop

## Note
This is NOT a shell interface. You need to send raw hex bytes to RTT channel 0.
Some RTT viewers allow sending hex data directly.