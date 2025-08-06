# Proposed Fix for TX FIFO Error Flooding

## Problem
TX FIFO errors are flooding the debug output even though they should be disabled.

## Root Cause
- `DBG_MSG_CAN_ERRORS` is enabled in the DEBUG_MESSAGES configuration
- This includes MSG_TX_FIFO_ERROR messages
- The TX FIFO error is logged every time there's a transmission issue, which can be very frequent

## Proposed Solutions

### Option 1: Disable CAN Errors in Default Config (Recommended)
**File**: Core/Inc/debug.h
**Change**: Remove `DBG_MSG_CAN_ERRORS` from DEBUG_MESSAGES
```c
#define DEBUG_MESSAGES  (DBG_MSG_REGISTRATION_GROUP | DBG_MSG_DEREGISTER | DBG_MSG_DEREGISTER_ALL | \
                        DBG_MSG_TIMEOUT | DBG_MSG_STATUS_REQ | DBG_MSG_STATUS1 | DBG_MSG_MINIMAL | \
                        DBG_MSG_REG_EVENTS)  // Removed DBG_MSG_CAN_ERRORS
```

### Option 2: Comment Out the TX FIFO Error Message
**File**: Core/Src/mcu.c
**Change**: Comment out the ShowDebugMessage call
```c
// ShowDebugMessage(MSG_TX_FIFO_ERROR, index, tec, rec, errorFlags);
```

### Option 3: Make it Conditional
**File**: Core/Src/mcu.c
**Change**: Only log if it's a new/different error
```c
static uint32_t lastErrorFlags = 0;
if(errorFlags != lastErrorFlags) {
    ShowDebugMessage(MSG_TX_FIFO_ERROR, index, tec, rec, errorFlags);
    lastErrorFlags = errorFlags;
}
```

## Recommendation
Go with Option 1 - simply remove `DBG_MSG_CAN_ERRORS` from the default configuration. This way:
- The capability remains in the code if needed later
- It can be easily re-enabled by adding the flag back
- No code changes needed in mcu.c

Would you like me to implement Option 1?