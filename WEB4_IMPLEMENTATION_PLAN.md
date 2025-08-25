# WEB4 Key Distribution Implementation Plan for Pack Controller

## Overview
This document outlines the implementation plan for adding WEB4 key distribution and encryption support to the Pack Controller firmware. The GUI tool (modbatt-CAN) already supports sending keys - this plan covers the firmware side to receive and process them.

## Current Status

### ✅ Completed
1. **CAN ID Definitions** - Added to `Core/Inc/can_id_bms_vcu.h`:
   - 0x407-0x40A for VCU→Pack messages
   - 0x4A7-0x4A9 for Pack→VCU acknowledgments

2. **Stub Implementation** - Created modular files:
   - `Core/Inc/web4_handler.h` - Header with data structures
   - `Core/Src/web4_handler.c` - Stub implementation with all functions
   - Debug messages added to `Core/Inc/debug.h` (0xF010-0xF021)

3. **Documentation** - Copied from GUI project:
   - `docs/Robust_Key_Distribution_Protocol.md` - Complete protocol spec

### ❌ To Be Implemented

## Phase 1: Basic Integration (Minimal Disruption)

### Step 1.1: Hook into MCU Message Handler
**File**: `Core/Src/mcu.c`
**Function**: Main CAN receive handler (likely `ProcessCANMessage()` or similar)
**Changes**:
```c
// Add to includes
#include "web4_handler.h"

// In CAN message processing function
if (WEB4_HandleCANMessage(canId, data, length)) {
    return;  // Message handled by WEB4
}
```

### Step 1.2: Add Timeout Check to Main Loop
**File**: `Core/Src/main.c` or wherever main loop exists
**Changes**:
```c
// In main loop
WEB4_CheckTimeouts();  // Call every iteration or every 100ms
```

### Step 1.3: Initialize WEB4 Handler
**File**: `Core/Src/main.c`
**Changes**:
```c
// In initialization section
WEB4_Init();  // Call after EEPROM and CAN initialization
```

## Phase 2: CAN Transmission Integration

### Step 2.1: Implement ACK Transmission
**File**: `Core/Src/web4_handler.c`
**Function**: `WEB4_SendAcknowledgment()`
**TODO**: 
- Find existing CAN transmit function (likely `CAN_Transmit()` or `SendCANMessage()`)
- Replace TODO comment with actual transmission code

### Step 2.2: Extended CAN ID Support
**Requirement**: Support chunk numbers in bits 8-10 of CAN ID
**Files**: CAN driver files
**Changes**:
- Ensure CAN driver supports extended IDs (29-bit)
- Modify receive handler to pass full 29-bit ID to WEB4 handler

## Phase 3: EEPROM Integration

### Step 3.1: Define EEPROM Addresses
**File**: Create `Core/Inc/web4_eeprom.h`
```c
#define EEPROM_WEB4_BASE_ADDR     0x1000  // Adjust based on available space
#define EEPROM_WEB4_PACK_KEY      (EEPROM_WEB4_BASE_ADDR + 0x00)
#define EEPROM_WEB4_APP_KEY       (EEPROM_WEB4_BASE_ADDR + 0x40)
#define EEPROM_WEB4_PACK_ID       (EEPROM_WEB4_BASE_ADDR + 0x80)
#define EEPROM_WEB4_APP_ID        (EEPROM_WEB4_BASE_ADDR + 0xC0)
#define EEPROM_WEB4_VALID_FLAG    (EEPROM_WEB4_BASE_ADDR + 0x100)
```

### Step 3.2: Implement Store/Load Functions
**File**: `Core/Src/web4_handler.c`
**Functions**: `WEB4_StoreKeysToEEPROM()` and `WEB4_LoadKeysFromEEPROM()`
**TODO**: 
- Use existing EEPROM emulation functions
- Store keys with validity flag
- Implement wear leveling if needed

## Phase 4: Testing & Verification

### Step 4.1: Basic Reception Test
1. Use modbatt-CAN GUI to send test keys
2. Verify chunks are received (debug messages)
3. Verify ACKs are sent back
4. Check complete key assembly

### Step 4.2: EEPROM Persistence Test
1. Send keys and verify storage
2. Reset Pack Controller
3. Verify keys are loaded on startup

### Step 4.3: Error Handling Test
1. Test timeout scenarios
2. Test checksum errors
3. Test out-of-order chunks
4. Test duplicate chunks

## Phase 5: Future Enhancements

### Step 5.1: Encryption Implementation
- Add AES-128 or ChaCha20 encryption library
- Use stored keys for message encryption/decryption
- Set bit 17 in CAN ID for encrypted messages

### Step 5.2: Module Key Distribution
- Forward keys from Pack to Modules
- Implement secure module-to-module communication

### Step 5.3: Key Rotation
- Implement periodic key updates
- Support multiple key versions
- Graceful key transition

## Build Configuration

### Compiler Flags
No special flags needed for stub implementation.

### Linker Configuration
Ensure sufficient RAM for key buffers (minimum 256 bytes).

## Integration Checklist

- [ ] Add `web4_handler.c` to build system (Makefile or IDE project)
- [ ] Include `web4_handler.h` in mcu.c
- [ ] Call `WEB4_Init()` in initialization
- [ ] Hook `WEB4_HandleCANMessage()` into CAN receive
- [ ] Add `WEB4_CheckTimeouts()` to main loop
- [ ] Find and integrate with CAN transmit function
- [ ] Find and integrate with EEPROM functions
- [ ] Test with modbatt-CAN GUI tool

## Risk Assessment

### Low Risk
- Stub implementation is isolated in separate files
- No changes to existing functionality when keys not present
- Can be disabled with single #define if needed

### Medium Risk
- EEPROM space allocation (need to verify available space)
- CAN bandwidth for ACK messages (minimal - only during key distribution)

### Mitigation
- All WEB4 code is in separate module
- Can be compiled out with preprocessor flag
- Graceful degradation - system works without keys

## Notes

1. **Minimal Disruption Approach**: The implementation is designed as a drop-in module requiring only 3-4 hook points in existing code.

2. **Extended ID Support**: The protocol uses extended CAN IDs for chunking. Verify the CAN peripheral and driver support 29-bit IDs.

3. **Timing**: Key distribution happens once at system configuration, not during normal operation, so performance impact is negligible.

4. **Compatibility**: System remains fully backward compatible - operates normally if keys are never sent.

## Contact

For questions about the WEB4 protocol or GUI tool integration, refer to:
- `/mnt/c/projects/ai-agents/modbatt-CAN/WEB4 Modbatt Configuration Utility/`
- Protocol documentation in `docs/Robust_Key_Distribution_Protocol.md`