# Frame-Based SD Card Data Architecture

## Overview
This document defines a frame-based architecture for efficiently storing and retrieving battery cell data on SD cards. The system uses 1KB frames as the fundamental storage unit, with each frame containing multiple cell string readings and metadata.

## Frame Structure (1024 bytes total)

### Frame Layout
```
typedef struct {
    // Frame Header (32 bytes)
    uint32_t frame_number;      // Sequential frame ID (0 to ~16M for 32GB card)
    uint32_t timestamp;         // RTC timestamp when frame started
    uint16_t granularity;       // Number of string readings in this frame
    uint16_t current_index;     // Current write position (0 to granularity-1)
    uint8_t  cells_expected;    // Number of cells per string
    uint8_t  module_id;         // Module that owns this frame
    uint16_t frame_crc;         // CRC16 of entire frame
    uint8_t  status_flags;      // Bit flags for frame status
    uint8_t  reserved[15];      // Future expansion

    // Frame Data (992 bytes)
    uint8_t  data[992];         // Circular buffer of string readings
} sd_frame_t;
```

### Frame Granularity Calculation
```
bytes_per_reading = cells_expected * 4  // 2 bytes voltage + 2 bytes temp per cell
granularity = 992 / bytes_per_reading

Examples:
- 14 cells: 56 bytes/reading → 17 readings per frame
- 16 cells: 64 bytes/reading → 15 readings per frame
- 20 cells: 80 bytes/reading → 12 readings per frame
- 24 cells: 96 bytes/reading → 10 readings per frame
```

### String Reading Format (within frame data)
```
typedef struct {
    uint16_t voltage[cells_expected];    // Cell voltages in mV
    uint16_t temperature[cells_expected]; // Cell temps in 0.1°C
} string_reading_t;
```

## SD Card Organization

### Sector Allocation
- Each frame occupies 2 consecutive sectors (512 bytes each)
- Frame N is stored at:
  - Sector (N * 2): First 512 bytes of frame
  - Sector (N * 2 + 1): Second 512 bytes of frame

### Address Space
- 32GB SD card = 62,914,560 sectors
- Maximum frames = 31,457,280 (using all sectors)
- Practical limit: 16,777,216 frames (24-bit frame number)
- At 1 frame/minute: ~31 years of data

## EEPROM Frame Counter Management

### Storage Strategy
```
EEPROM Layout for Frame Counter:
Address 0x00-0x02: Current frame number (24 bits)
Address 0x03: Checksum

Wear Leveling:
- Use 8 consecutive locations (32 bytes total)
- Rotate through locations every 10,000 writes
- Each location can handle 100,000 writes
- Total: 800,000 frame counter updates
```

### Byte-Wise Update Algorithm
```c
// Only update bytes that changed to minimize EEPROM writes
void update_frame_counter(uint32_t new_frame) {
    uint32_t old_frame = read_frame_counter();

    // Check each byte
    if ((old_frame & 0xFF) != (new_frame & 0xFF)) {
        eeprom_write_byte(ADDR_FRAME_L, new_frame & 0xFF);
    }
    if (((old_frame >> 8) & 0xFF) != ((new_frame >> 8) & 0xFF)) {
        eeprom_write_byte(ADDR_FRAME_M, (new_frame >> 8) & 0xFF);
    }
    if (((old_frame >> 16) & 0xFF) != ((new_frame >> 16) & 0xFF)) {
        eeprom_write_byte(ADDR_FRAME_H, (new_frame >> 16) & 0xFF);
    }

    // Update checksum
    uint8_t checksum = calculate_checksum(new_frame);
    eeprom_write_byte(ADDR_CHECKSUM, checksum);
}
```

## Module Operation Flow

### Initialization (Power-Up)
```
1. Read frame_number from EEPROM
2. Load frame from SD card sectors (frame_number * 2)
3. Validate frame CRC
4. If invalid, create new frame:
   - Clear frame structure
   - Set frame_number from EEPROM
   - Initialize timestamp
   - Set granularity based on cells_expected
5. Set current_index to continue where left off
```

### Normal Operation (Cell Data Collection)
```
1. Receive cell string data via vUART
2. Store in frame.data at current_index position
3. Increment current_index
4. If current_index >= granularity:
   - Calculate and store frame CRC
   - Write frame to SD card (2 sectors)
   - Increment frame_number
   - Update EEPROM (only changed bytes)
   - Initialize new frame
   - Reset current_index to 0
```

### Frame Retrieval (Pack Request)
```
1. Receive frame request from Pack Controller
2. If requesting current frame:
   - Send current frame data (even if partially filled)
3. If requesting previous frame:
   - Pause cell data collection
   - Save current frame to temporary buffer
   - Read requested frame from SD card
   - Send requested frame via CAN
   - Restore current frame from buffer
   - Resume cell data collection
```

## CAN Protocol for Frame Transfer

### New Message Types

#### FRAME_INFO_REQUEST (Pack → Module)
```
CAN ID: 0x3E0 + module_id
Payload:
  Byte 0: Command (0x10 = Get Info)
  Bytes 1-7: Reserved
```

#### FRAME_INFO_RESPONSE (Module → Pack)
```
CAN ID: 0x3E1 + module_id
Payload:
  Bytes 0-2: Current frame number (24 bits)
  Byte 3: Current index in frame
  Byte 4: Frame granularity
  Byte 5: Cells expected
  Bytes 6-7: Reserved
```

#### FRAME_REQUEST (Pack → Module)
```
CAN ID: 0x3E2 + module_id
Payload:
  Byte 0: Command (0x11 = Get Frame)
  Bytes 1-3: Frame number (24 bits)
  Byte 4: Transfer ID
  Bytes 5-7: Reserved
```

#### FRAME_DATA (Module → Pack)
```
Uses existing windowed transfer protocol
- 1024 bytes = 128 chunks of 8 bytes
- 8 windows of 16 chunks each
- Extended CAN ID encodes window/chunk/module
```

## Advantages of Frame-Based Approach

### Efficiency
- **Write Efficiency**: Only writes to SD when frame is full
- **Read Efficiency**: Retrieves meaningful data blocks (not raw sectors)
- **EEPROM Efficiency**: Byte-wise updates minimize wear

### Data Coherency
- Each frame is a complete time-series snapshot
- Granularity ensures optimal use of 1KB blocks
- Metadata travels with data

### Flexibility
- Adapts to different cell counts automatically
- Supports both current and historical data retrieval
- Allows for future metadata expansion

### Reliability
- CRC validation per frame
- Power-loss recovery via EEPROM
- Sequential frame numbering prevents gaps

## Implementation Considerations

### Module Side
1. **Buffer Management**
   - Single 1KB frame buffer in RAM
   - Temporary buffer for frame swapping during retrieval

2. **Timing**
   - Cell data collection continues during SD writes
   - Use DMA for SD card operations to minimize blocking

3. **Error Handling**
   - Retry SD writes up to 3 times
   - Mark frame as corrupt if write fails
   - Continue with next frame number

### Pack Controller Side
1. **Frame Tracking**
   - Maintain last known frame number per module
   - Request current frame periodically for real-time data
   - Request historical frames for analysis

2. **Data Processing**
   - Parse frame structure to extract cell readings
   - Validate CRC before processing
   - Handle partial frames (current_index < granularity)

## Testing Requirements

### Unit Tests
1. Frame granularity calculation for various cell counts
2. EEPROM byte-wise update logic
3. Frame CRC calculation and validation
4. Circular buffer wraparound

### Integration Tests
1. Power-cycle recovery with EEPROM persistence
2. Frame retrieval during active data collection
3. SD card write performance under load
4. CAN transfer of complete frames

### Stress Tests
1. Continuous operation for 24+ hours
2. Rapid frame requests while collecting data
3. SD card near capacity behavior
4. EEPROM wear leveling over time