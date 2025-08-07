# Proposed Cleanup of Old Debug Messages

## Problem
Old sprintf/serialOut debug messages are flooding the output, particularly in the polling loop.

## Old Debug Messages Found

### Line 326: Module count check
```c
sprintf(tempBuffer,"MCU DEBUG - Checking %d modules", pack.moduleCount);
```

### Line 333: Module check details
```c
sprintf(tempBuffer,"MCU DEBUG - module[%d] ID=%02x elapsed=%lu pending=%d",
```

### Line 382: Status request
```c
sprintf(tempBuffer,"MCU DEBUG - Requesting status from module ID=%02x (index=%d)",
```

### Line 398: Module elapsed/pending/commsErr (THE FLOOD SOURCE)
```c
sprintf(tempBuffer,"MCU DEBUG - Module ID=%02x elapsed=%lu pending=%d commsErr=%d",
```

### Line 423: Round-robin polling
```c
sprintf(tempBuffer,"MCU DEBUG - Round-robin polling module ID=%02x (index=%d)",
```

### Line 473: State machine debug
```c
sprintf(tempBuffer,"MCU DEBUG - Module ID=%02x current=%d next=%d cmd=%d cmdStatus=%d",
```

## Proposed Solution

### Option 1: Comment them all out (Quick fix)
Simply comment out all these old debug messages since we have the new debug system.

### Option 2: Convert critical ones to ShowDebugMessage
Convert only the important ones to use the new system with appropriate flags.

### Option 3: Create new debug message IDs for polling
Add MSG_POLLING_STATUS or similar for these repetitive messages, make them minimal format.

## Recommendation

**Go with Option 1 for now** - Comment them all out. These are diagnostic messages that were useful during development but are now just noise. The new debug system captures the important events (registration, timeouts, etc.).

If we need polling diagnostics later, we can add proper ShowDebugMessage calls with appropriate flags and minimal format.