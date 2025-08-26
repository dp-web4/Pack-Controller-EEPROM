# STM32 Headers Used by Pack Controller Emulator

This document lists which STM32 firmware headers are included by the emulator to maintain consistency between the firmware and emulator codebases.

## Headers Included from ../Core/Inc/

### Primary Headers Used:

1. **bms.h** - Battery Management System definitions
   - `moduleState` enum (moduleOff, moduleStandby, modulePrecharge, moduleOn)
   - `packState` enum
   - `batteryCell` structure
   - `faultCode` structure
   - System constants (MAX_CELLS_PER_MODULE, MAX_MODULES_PER_PACK)

2. **can_id_module.h** - Module CAN message identifiers
   - Module → Pack IDs (0x500-0x509)
   - Pack → Module IDs (0x510-0x51F)
   - All message ID constants (ID_MODULE_ANNOUNCEMENT, etc.)

3. **can_id_bms_vcu.h** - VCU CAN message identifiers  
   - VCU → Pack IDs (0x400-0x40F)
   - Pack → VCU IDs (0x4A0-0x4AF)
   - WEB4 message IDs (0x407-0x40A, 0x4A7-0x4A9)

4. **can_frm_mod.h** - Module CAN frame structures
   - Detailed message format definitions
   - Bit field layouts for CAN messages
   - Data packing/unpacking structures

## Why This Matters

By using the actual STM32 headers:
- **Single source of truth** for all definitions
- **Automatic synchronization** when firmware is updated
- **No risk of divergence** between emulator and firmware
- **Consistent CAN protocol** implementation

## Files That Include STM32 Headers

### module_manager.h
```cpp
extern "C" {
    #include "../../Core/Inc/bms.h"              
    #include "../../Core/Inc/can_id_module.h"    
    #include "../../Core/Inc/can_id_bms_vcu.h"   
    #include "../../Core/Inc/can_frm_mod.h"      
}
```

### can_interface.h
```cpp
extern "C" {
    #include "../../Core/Inc/can_id_module.h"    
    #include "../../Core/Inc/can_id_bms_vcu.h"   
}
```

## Removed Files

- **battery.h** - Was incorrectly created as a placeholder. The actual definitions are in bms.h.

## Build Configuration

The project file (PackEmulator.cbproj) includes `..\Core\Inc\` in the include path, allowing direct access to STM32 headers with relative paths.

## Future Considerations

If the emulator needs additional STM32 definitions, include the appropriate headers from:
- `../Core/Inc/` for firmware definitions
- Avoid creating duplicate definitions in emulator code
- Use `extern "C"` blocks when including C headers in C++ code