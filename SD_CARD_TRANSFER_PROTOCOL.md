# SD Card Sector Transfer Protocol

## Overview
This document defines the CAN protocol for transferring 512-byte SD card sectors from battery modules to the pack controller. The protocol uses a windowed approach to efficiently transfer large data blocks over CAN while maintaining flow control and error recovery.

## Design Requirements
- Transfer complete 512-byte SD card sectors
- Support multiple concurrent transfers from different modules
- Provide error detection and recovery
- Minimize CAN bus utilization
- Compatible with existing ModBatt CAN architecture

## Protocol Architecture

### Message Types

1. **SD_SECTOR_REQUEST** (Pack → Module)
   - Initiates sector read request
   - Standard CAN message (not extended)

2. **SD_DATA_CHUNK** (Module → Pack)
   - Carries sector data in 8-byte chunks
   - Extended CAN ID for metadata

3. **SD_WINDOW_ACK** (Pack → Module)
   - Acknowledges received window
   - Requests retransmission if needed

4. **SD_TRANSFER_STATUS** (Module → Pack)
   - Reports transfer completion or errors
   - Provides sector metadata

## Detailed Message Formats

### 1. SD_SECTOR_REQUEST (CAN ID: 0x3F0 + Module ID)
**Direction:** Pack Controller → Module
**Payload (8 bytes):**
```
Byte 0:     Command (0x01 = Read Sector)
Byte 1:     Transfer ID (unique per request)
Bytes 2-5:  Sector Number (32-bit, little endian)
Byte 6:     Options (bit 0: priority, bits 1-7: reserved)
Byte 7:     Checksum (XOR of bytes 0-6)
```

### 2. SD_DATA_CHUNK (Extended CAN ID)
**Direction:** Module → Pack Controller
**Extended ID Structure (29 bits):**
```
Bits 28-18: Base ID (0x3F1)           [11 bits]
Bit  17:    Mode (1 = Data Transfer)   [1 bit]
Bit  16:    Last Chunk Flag            [1 bit]
Bits 15-14: Window ID (0-3)            [2 bits]
Bits 13-10: Chunk in Window (0-15)     [4 bits]
Bits 9-8:   Transfer ID (0-3)          [2 bits]
Bits 7-0:   Module ID                  [8 bits]
```

**Payload (8 bytes):**
- Raw sector data (8 bytes per chunk)
- 64 chunks total for 512-byte sector
- No additional headers in payload

### 3. SD_WINDOW_ACK (CAN ID: 0x3F2 + Module ID)
**Direction:** Pack Controller → Module
**Payload (8 bytes):**
```
Byte 0:     Command (0x02 = Window ACK)
Byte 1:     Transfer ID
Byte 2:     Window ID (0-3)
Bytes 3-4:  Received Bitmap (16 bits, bit n = chunk n received)
Byte 5:     Status (0x00 = OK, 0x01 = Retry, 0xFF = Abort)
Bytes 6-7:  Running CRC16 of received data
```

### 4. SD_TRANSFER_STATUS (CAN ID: 0x3F3 + Module ID)
**Direction:** Module → Pack Controller
**Payload (8 bytes):**
```
Byte 0:     Command (0x03 = Transfer Status)
Byte 1:     Transfer ID
Byte 2:     Status Code:
            0x00 = Transfer Complete
            0x01 = In Progress
            0x10 = SD Card Error
            0x11 = Sector Out of Range
            0x12 = Busy (retry later)
            0x20 = CRC Error
            0xFF = Unknown Error
Byte 3:     Windows Completed (0-4)
Bytes 4-5:  Final CRC16
Bytes 6-7:  Time Elapsed (ms, little endian)
```

## Transfer Sequence

### Successful Transfer
```
1. Pack → Module: SD_SECTOR_REQUEST (sector 1000, transfer ID 0x42)
2. Module → Pack: SD_TRANSFER_STATUS (In Progress)
3. Module → Pack: SD_DATA_CHUNK (Window 0, Chunks 0-15)
4. Pack → Module: SD_WINDOW_ACK (Window 0, all received)
5. Module → Pack: SD_DATA_CHUNK (Window 1, Chunks 0-15)
6. Pack → Module: SD_WINDOW_ACK (Window 1, all received)
7. Module → Pack: SD_DATA_CHUNK (Window 2, Chunks 0-15)
8. Pack → Module: SD_WINDOW_ACK (Window 2, all received)
9. Module → Pack: SD_DATA_CHUNK (Window 3, Chunks 0-15)
10. Pack → Module: SD_WINDOW_ACK (Window 3, all received)
11. Module → Pack: SD_TRANSFER_STATUS (Complete, CRC16)
```

### Transfer with Retransmission
```
1-3. [Same as above]
4. Pack → Module: SD_WINDOW_ACK (Window 0, missing chunk 5)
5. Module → Pack: SD_DATA_CHUNK (Window 0, Chunk 5 only)
6. Pack → Module: SD_WINDOW_ACK (Window 0, all received)
7. [Continue with Window 1...]
```

## Window Management

### Window Size
- Each window: 16 chunks × 8 bytes = 128 bytes
- Total windows: 4 (for 512 bytes)
- Window 0: Bytes 0-127 (chunks 0-15)
- Window 1: Bytes 128-255 (chunks 16-31)
- Window 2: Bytes 256-383 (chunks 32-47)
- Window 3: Bytes 384-511 (chunks 48-63)

### Flow Control
- Module waits for ACK before sending next window
- Pack controller can request retransmission via bitmap
- Maximum 3 retransmission attempts per window
- Timeout: 100ms per window

## Error Handling

### Module Errors
- **SD Card Not Ready:** Return STATUS with code 0x10
- **Invalid Sector:** Return STATUS with code 0x11
- **Read Error:** Abort transfer, send STATUS with code 0x10

### Communication Errors
- **Missing Chunks:** Pack sets bits in ACK bitmap for retransmission
- **Window Timeout:** Pack sends ACK with retry request
- **Transfer Abort:** Pack sends ACK with status 0xFF

### CRC Verification
- CRC16-CCITT polynomial (0x1021)
- Calculated over entire 512-byte sector
- Verified at both window and transfer completion

## Implementation Considerations

### Module Side
1. Buffer Management
   - Need 512-byte buffer for sector data
   - Window transmission buffer (128 bytes)
   - Retransmission tracking per window

2. State Machine
   - IDLE → READING → TRANSMITTING → WAITING_ACK → COMPLETE
   - Handle concurrent requests via Transfer ID

3. SD Card Interface
   - Use existing FatFS integration
   - Support both FAT and raw sector access

### Pack Controller Side
1. Reception Buffer
   - 512-byte buffer per active transfer
   - Bitmap tracking for received chunks
   - Multiple concurrent transfers (up to 4)

2. Reassembly Logic
   - Place chunks at correct offset: `offset = (window * 128) + (chunk * 8)`
   - Verify CRC after each window
   - Final CRC verification on complete sector

3. Timeout Management
   - Per-window timeout (100ms)
   - Overall transfer timeout (2 seconds)
   - Exponential backoff on retries

## Performance Metrics

### Bandwidth Utilization
- Sector size: 512 bytes
- CAN frames needed: 64 (data) + 4 (ACK) + 2 (status) = 70 frames
- At 500kbps: ~14ms transmission time (theoretical)
- With overhead and ACKs: ~50-100ms typical

### Concurrent Transfers
- Support up to 4 concurrent transfers (2-bit Transfer ID)
- Different modules can transfer simultaneously
- Same module can queue multiple requests

## Future Enhancements

### Version 2 Considerations
1. **Compression:** Add simple RLE for sparse sectors
2. **Encryption:** Use session keys for sensitive data
3. **Multi-sector:** Support for continuous multi-sector reads
4. **Priority Levels:** Emergency vs normal transfer queues
5. **Broadcast Mode:** Single sector to multiple pack controllers

## Testing Requirements

### Unit Tests
1. Single sector transfer
2. Multiple concurrent transfers
3. Error injection (missing chunks)
4. CRC error detection
5. Timeout handling

### Integration Tests
1. Real SD card reading
2. CAN bus stress testing
3. Module power cycling during transfer
4. Pack controller reset handling

## Compliance Notes
- Compatible with CAN 2.0B (Extended IDs)
- Works with existing ModBatt module protocol
- No changes required to bootloader
- Backward compatible with modules without SD cards