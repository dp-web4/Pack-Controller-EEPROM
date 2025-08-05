# Proposed Debug Improvements for Module Timeouts

## Current Issue
Module 2 disappears after announcement requests but we're not seeing timeout/deregistration messages in minimal debug mode.

## Analysis
The timeout detection code checks for `(debugLevel & (DBG_MCU + DBG_ERRORS))` which isn't set in minimal mode, so timeout messages are silent. We need visibility into what's happening.

## Proposed Changes

### 1. Add Minimal Mode Timeout Messages
**Current**: Timeout messages only show if full debug is enabled
**Proposed**: Add compact timeout indicators in minimal mode

```c
// In timeout detection (around line 375), after existing error message:
else if(debugMessages & DBG_MSG_TIMEOUT){
  if(debugMessages & DBG_MSG_MINIMAL){
    // Minimal format: moduleID + T + timeout count
    sprintf(tempBuffer,"%dT%d", module[index].moduleId, module[index].consecutiveTimeouts);
  } else {
    sprintf(tempBuffer,"MCU TIMEOUT - Module ID=%02x (timeout %d of %d)",
            module[index].moduleId, module[index].consecutiveTimeouts, MCU_MAX_CONSECUTIVE_TIMEOUTS);
  }
  serialOut(tempBuffer);
}
```

**Output format**: `2T1` = Module 2, timeout 1 of 3

### 2. Add Minimal Mode Deregistration Messages  
**Current**: Deregistration messages check full format only
**Proposed**: Add compact deregistration indicator

```c
// In deregistration logging (around line 353):
if(debugMessages & DBG_MSG_DEREGISTER){
  if(debugMessages & DBG_MSG_MINIMAL){
    // Minimal format: moduleID + D
    sprintf(tempBuffer,"%dD", module[index].moduleId);
  } else {
    sprintf(tempBuffer,"MCU INFO - Removing module from pack: ID=%02x, UID=%08x, Index=%d", 
            module[index].moduleId, (int)module[index].uniqueId, index);
  }
  serialOut(tempBuffer);
}
```

**Output format**: `2D` = Module 2 deregistered

### 3. Remove Redundant uniqueId Checks (OPTIONAL)
**Current**: Code checks both `isRegistered` and `uniqueId != 0`
**Proposed**: Remove `uniqueId != 0` checks as they're redundant
**Rationale**: A registered module should never have uniqueId of 0

```c
// Instead of:
if(module[i].isRegistered && module[i].uniqueId != 0)

// Just use:
if(module[i].isRegistered)
```

### 4. Clear statusPendingSaved Flag
**Current**: statusPendingSaved might not be cleared after restore
**Proposed**: Clear it after restoring to prevent state confusion

```c
module[i].statusPendingSaved = false;  // Clear after restore
```

## Benefits
1. **Visibility**: See exactly when modules timeout in minimal mode
2. **Compact**: Minimal output format maintains quiet debug style
3. **Diagnostic**: Can track timeout progression (T1, T2, T3, then D)

## Questions
1. Should we keep the redundant `uniqueId != 0` checks for extra safety?
2. Is the format `2T1` and `2D` clear enough or prefer something else?
3. Should timeout messages show elapsed time too? (e.g., `2T1[4500]` for 4.5 seconds)

## Testing
With these changes, we should see:
- Normal: `2[10]` (module 2, 10ms response)
- Timeout: `2T1` (module 2, first timeout)
- Deregister: `2D` (module 2 deregistered)

This will clearly show what's happening to module 2 after announcement requests.