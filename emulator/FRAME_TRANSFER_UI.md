# Frame Transfer UI Implementation

## Overview
Added UI controls and protocol implementation to the Pack Controller Emulator to request and receive frame data from ModuleCPU via CAN bus.

## UI Changes (Cells Tab)

### New Controls
- **Get Frame** button: Initiates frame transfer request
- **Frame Number** edit box: Specifies which frame to request
  - Default value: `0xFFFFFFFF` (current frame in RAM)
  - Can enter specific frame counter in hex (0x...) or decimal format
  - Module retrieves from SD card if specific frame number provided

### Location
Controls added to Cells tab, below the "Export CSV" checkbox:
- Button: 376, 60 (100x25)
- Edit box: 482, 60 (100x21)
- Label: 482, 86

## Protocol Implementation

### CAN Message IDs
Added to `can_id_module.h`:
```c
#define ID_FRAME_TRANSFER_REQUEST   0x520  // Pack → Module
#define ID_FRAME_TRANSFER_START     0x521  // Module → Pack
#define ID_FRAME_TRANSFER_DATA      0x522  // Module → Pack
#define ID_FRAME_TRANSFER_END       0x523  // Module → Pack
```

### State Machine
```
FRAME_IDLE → FRAME_WAITING_START → FRAME_RECEIVING_DATA → FRAME_COMPLETE
```

### Transfer Sequence
1. User clicks "Get Frame" button
2. Pack sends `FRAME_TRANSFER_REQUEST` with module ID and frame number
3. Module responds with `FRAME_TRANSFER_START` (frame counter, total segments)
4. Module sends 128 `FRAME_TRANSFER_DATA` messages (8 bytes each)
5. Module sends `FRAME_TRANSFER_END` with CRC32
6. Pack validates CRC32 and prompts user to save CSV file

### CSV Output Format
```
Frame Counter,<value>
Module ID,<id>
Segments Received,<count>
CRC32,0x<hex>

Offset,Hex,ASCII
0x0000,<16 hex bytes>,<16 ascii chars>
0x0010,<16 hex bytes>,<16 ascii chars>
...
```

## Implementation Details

### New Class Members
- `frameTransferState`: Current transfer state
- `frameBuffer[1024]`: Receive buffer for frame data
- `frameSegmentCount`: Number of segments received
- `frameCounter`: Frame counter from START message
- `frameCRC`: CRC32 from END message

### New Methods
- `GetFrameButtonClick()`: UI button handler
- `SendFrameTransferRequest()`: Send request to module
- `ProcessFrameTransferStart()`: Handle START message
- `ProcessFrameTransferData()`: Handle DATA messages
- `ProcessFrameTransferEnd()`: Handle END message and validate CRC
- `WriteFrameToCSV()`: Export frame to CSV file
- `CalculateCRC32()`: Bit-by-bit CRC32 calculation (matches ModuleCPU)

### CRC32 Algorithm
Uses same polynomial as ModuleCPU (0xEDB88320) for compatibility:
```cpp
uint32_t crc = 0xFFFFFFFF;
for each byte:
    crc ^= byte
    for each bit:
        crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1))
return ~crc
```

## Testing Notes
- Requires module to be selected in list view
- Requires CAN connection to be established
- Frame transfer blocks other module operations (by design)
- Progress logged to History tab every 16 segments
- CRC validation ensures data integrity
- Transfer can fail if any message is lost (no retransmit yet)

## Future Enhancements
- Add progress bar for visual feedback
- Add automatic retransmit on CRC failure
- Parse frame structure and display decoded fields
- Support batch frame retrieval
