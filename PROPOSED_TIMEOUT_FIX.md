# Proposed Fix for Module Timeout Detection

## Problem
When a module is powered off, the pack controller doesn't detect the timeout and continues reporting it as active. No timeout messages (`2T1`) or deregistration messages (`2D`) appear.

## Root Cause Analysis

### Hypothesis 1: statusPending Flag Issue
After announcement request, when we restore polling, the module might not have `statusPending` set properly.

**Investigation needed**:
- Check if `statusPending` is actually being set for the missing module
- Verify the timeout check is looking at the right condition

### Hypothesis 2: Timeout Check Not Running
The timeout check in `MCU_CheckTimeouts()` might not be executing for the module.

**Current code (line 336)**:
```c
if(elapsedTicks > MCU_ET_TIMEOUT && (module[index].statusPending == true))
```

This only times out if `statusPending` is true. If it's false, no timeout occurs.

### Hypothesis 3: lastContact Being Updated Incorrectly
The module's `lastContact` might be getting updated even when it's not responding.

## Proposed Diagnostic Changes

### 1. Add Debug to Show statusPending State
```c
// In minimal mode, periodically show which modules have statusPending
// Add to MCU_CheckTimeouts or main loop
if(debugMessages & DBG_MSG_MINIMAL){
  static uint32_t lastDebugTime = 0;
  uint32_t now = HAL_GetTick();
  if(now - lastDebugTime > 5000){ // Every 5 seconds
    for(int i = 0; i < MAX_MODULES_PER_PACK; i++){
      if(module[i].isRegistered){
        sprintf(tempBuffer, "%d:%c ", module[i].moduleId, 
                module[i].statusPending ? 'P' : '-');
        serialOut(tempBuffer);
      }
    }
    lastDebugTime = now;
  }
}
```
Output: `1:P 2:P` means both modules have statusPending set

### 2. Add Debug for Timeout Check Entry
```c
// At the start of timeout check for each module
if(debugMessages & DBG_MSG_MINIMAL && module[index].isRegistered){
  if(elapsedTicks > MCU_ET_TIMEOUT){
    sprintf(tempBuffer, "%d>T ", module[index].moduleId); // Module exceeded timeout threshold
    serialOut(tempBuffer);
  }
}
```

### 3. Fix Potential statusPending Issue
The real issue might be that after a module fails to respond, its `statusPending` gets cleared somewhere.

**Check these locations**:
1. When Status1/2/3 received - does it clear statusPending even for wrong module?
2. After announcement request - is statusPending being restored correctly?

## Proposed Fix

### Option A: Always Check Timeouts
```c
// Change timeout condition from:
if(elapsedTicks > MCU_ET_TIMEOUT && (module[index].statusPending == true))

// To:
if(elapsedTicks > MCU_ET_TIMEOUT && module[index].isRegistered)
```
This ensures registered modules are always checked for timeout.

### Option B: Never Clear statusPending Until Response
Ensure `statusPending` stays true until we actually get a response from THAT specific module.

### Option C: Add Heartbeat Requirement
Even if not polling for status, require periodic communication from each module.

## Questions
1. Should modules timeout even if we're not actively polling them?
2. Is 4 seconds (MCU_ET_TIMEOUT) appropriate or too short?
3. Should we track "last heard from" separately from "last status request"?

## Test Plan
1. Start with 2 modules
2. Power off module 2
3. Watch for timeout messages (should see `2T1`, `2T2`, `2T3`, then `2D`)
4. Verify active count decreases from 2 to 1