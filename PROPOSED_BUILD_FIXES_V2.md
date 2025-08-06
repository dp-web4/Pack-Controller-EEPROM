# Proposed Build Fixes V2 - Based on Actual Errors

## Build Errors to Fix

### 1. Line 508: `module[].status.voltage` - status is not a struct
**Error**: `request for member 'voltage' in something not a structure or union`
**Fix**: Just use `module[moduleId-1].voltage` directly

### 2. Line 1058, 1086: `announcement.moduleId` doesn't exist
**Error**: `'CANFRM_MODULE_ANNOUNCEMENT' has no member named 'moduleId'`
**Analysis**: The announcement structure doesn't have moduleId. We need to get it from elsewhere.
**Fix**: The moduleId is passed as a parameter to MCU_RegisterModule, use that

### 3. Line 1547, 1560: `status1->moduleId` - status1 is not a pointer
**Error**: `invalid type argument of '->' (have 'CANFRM_MODULE_STATUS_1')`
**Analysis**: status1 is a structure, not a pointer, and it doesn't have moduleId
**Fix**: Extract moduleId from rxd[0] or use the moduleId we already have

### 4. Line 1667: `status1` undeclared in MCU_ProcessModuleStatus2
**Error**: `'status1' undeclared (first use in this function)`
**Fix**: Should be status2, but status2 also doesn't have moduleId. Use rxd[0]

### 5. Line 1744: `status1` undeclared in MCU_ProcessModuleStatus3
**Error**: `'status1' undeclared (first use in this function)`
**Fix**: Should be status3, but status3 also doesn't have moduleId. Use rxd[0]

## Detailed Fixes

### Fix 1: Line 508
```c
// Change from:
ShowDebugMessage(MSG_VOLTAGE_SELECTION, moduleId, module[moduleId-1].status.voltage);
// To:
ShowDebugMessage(MSG_VOLTAGE_SELECTION, moduleId, module[moduleId-1].voltage);
```

### Fix 2: Lines 1058, 1086
The function signature is: `MCU_RegisterModule(uint8_t moduleId, CANFRM_MODULE_ANNOUNCEMENT announcement)`
```c
// Change from:
ShowDebugMessage(MSG_MODULE_REREGISTER, announcement.moduleId);
ShowDebugMessage(MSG_NEW_MODULE_REG, announcement.moduleId);
// To:
ShowDebugMessage(MSG_MODULE_REREGISTER, moduleId);
ShowDebugMessage(MSG_NEW_MODULE_REG, moduleId);
```

### Fix 3: Lines 1547, 1560
In MCU_ProcessModuleStatus1, we need to extract moduleId:
```c
// Change from:
ShowDebugMessage(MSG_UNREGISTERED_MOD, status1->moduleId);
ShowDebugMessage(MSG_TIMEOUT_RESET, status1->moduleId, module[moduleIndex].consecutiveTimeouts);
// To:
uint8_t moduleId = rxd[0] & 0x1F;  // Module ID is in lower 5 bits of first byte
ShowDebugMessage(MSG_UNREGISTERED_MOD, moduleId);
ShowDebugMessage(MSG_TIMEOUT_RESET, moduleId, module[moduleIndex].consecutiveTimeouts);
```

### Fix 4: Line 1667 (MCU_ProcessModuleStatus2)
```c
// Change from:
ShowDebugMessage(MSG_TIMEOUT_RESET, status1->moduleId, module[moduleIndex].consecutiveTimeouts);
// To:
uint8_t moduleId = rxd[0] & 0x1F;  // Module ID is in lower 5 bits of first byte
ShowDebugMessage(MSG_TIMEOUT_RESET, moduleId, module[moduleIndex].consecutiveTimeouts);
```

### Fix 5: Line 1744 (MCU_ProcessModuleStatus3)
```c
// Change from:
ShowDebugMessage(MSG_TIMEOUT_RESET, status1->moduleId, module[moduleIndex].consecutiveTimeouts);
// To:
uint8_t moduleId = rxd[0] & 0x1F;  // Module ID is in lower 5 bits of first byte
ShowDebugMessage(MSG_TIMEOUT_RESET, moduleId, module[moduleIndex].consecutiveTimeouts);
```

## Note on Module ID Extraction
The module ID is transmitted in the first byte of status messages, typically in the lower 5 bits (0x1F mask). This is consistent with the CAN protocol where the module ID is part of the message header.