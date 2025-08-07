# Proposed Fix for Module Polling After Registration

## Problem Analysis

After module 2 registers, the pack controller shows "Checking 2 modules" but only polls module 1:
- The output shows `.1-1` repeatedly (poll module 1, get response from 1)
- Module 2 is never polled after registration
- Cell Status messages (0x507) from module 2 don't affect statusPending

## Root Cause Hypothesis

There are two potential issues:

### 1. Module ID Confusion
When module 2 announces with UID=babe0004 (which was module 1's UID earlier), the system might be:
- Re-registering module 1 in slot 2
- Creating confusion about which physical module has which ID

### 2. StatusPending Flag Issue
When a new module registers:
- `statusPending` is set to `true` (line 1058)
- The module needs to send Status1/2/3 messages to clear this flag
- But if module sends Cell Status (0x507) instead, the flag never clears
- Module remains in "statusPending=true" state forever
- Polling logic at line 373 requires `statusPending==false` to poll

## Proposed Solutions

### Option 1: Add More Debug Output
Add debug messages to understand the registration and polling state:

```c
// In MCU_RegisterModule, after assigning moduleId:
ShowDebugMessage(MSG_MODULE_REGISTRATION_DETAIL, 
    moduleIndex, module[moduleIndex].moduleId, 
    announcement.moduleUniqueId, module[moduleIndex].statusPending);

// In polling loop when skipping a module:
if(module[index].statusPending == true) {
    ShowDebugMessage(MSG_MODULE_SKIP_PENDING, 
        module[index].moduleId, module[index].statusPending);
}
```

### Option 2: Fix StatusPending Logic
The statusPending flag should be cleared after initial registration:

```c
// In MCU_RegisterModule, for new modules:
module[moduleIndex].statusPending = false;  // Start with false, not true
module[moduleIndex].hardwarePending = true;  // Still request hardware info
```

### Option 3: Add Timeout for StatusPending
If a module doesn't send all 3 status messages within a timeout, clear the flag anyway:

```c
// In main polling loop:
if(module[index].statusPending == true && 
   elapsedTicks > MCU_STATUS_TIMEOUT) {
    // Module isn't sending proper status messages
    module[index].statusPending = false;
    module[index].statusMessagesReceived = 0;
}
```

## Recommended Approach

Start with **Option 1** (more debug output) to understand what's happening, then apply **Option 2** or **Option 3** based on findings.

## Key Code Locations

- Registration: `mcu.c:1003-1095` (MCU_RegisterModule)
- Polling decision: `mcu.c:373` (statusPending check)
- Status message handling: `mcu.c:1538, 1645, 1722` (clearing statusPending)
- Round-robin polling: `mcu.c:406` (also checks statusPending)