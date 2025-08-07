# Proposed Once-Only Debug Message System

## Overview
Add a mechanism to show certain debug messages only once until hard reset, preventing flood of repetitive messages while still capturing first occurrence.

## Implementation Design

### 1. Add Once-Only Configuration (debug.h)
```c
// Define which messages should only be shown once
#define DEBUG_ONCE_ONLY  (DBG_MSG_CAN_ERRORS | DBG_MSG_TX_FIFO_ERROR | \
                         DBG_MSG_TIMEOUT | DBG_MSG_DEREGISTER)

// Tracking variable for once-only messages (32-bit, one bit per flag)
extern uint32_t debugOnceShown;  // Tracks which once-only messages have been displayed
```

### 2. Add Tracking Variable (debug.c)
```c
// Global tracking for once-only messages
uint32_t debugOnceShown = 0;  // Reset to 0 on startup, bits set as messages are shown
```

### 3. Modify ShowDebugMessage Function (debug.c)
```c
void ShowDebugMessage(uint16_t messageId, ...) {
    const DebugMessageDef* def = FindDebugMessageDef(messageId);
    if(!def) return;
    
    // Step 1: Check if message is enabled
    if((debugLevel & def->requiredLevel) == 0) return;
    if((debugMessages & def->requiredFlag) == 0) return;
    
    // Step 2: Check if this is a once-only message that's already been shown
    if(DEBUG_ONCE_ONLY & def->requiredFlag) {  // Is this a once-only message type?
        if(debugOnceShown & def->requiredFlag) {  // Has it been shown already?
            return;  // Don't show again
        }
        // Mark as shown for future calls
        debugOnceShown |= def->requiredFlag;
    }
    
    // Step 3: Format and display the message
    // ... existing display code ...
}
```

### 4. Optional: Reset Function
```c
// Function to reset once-only tracking (could be called on specific events)
void ResetDebugOnceOnly(void) {
    debugOnceShown = 0;
}
```

## Messages to Make Once-Only

Recommended messages for once-only treatment:
- `DBG_MSG_CAN_ERRORS` - Unknown CAN ID errors
- `DBG_MSG_TX_FIFO_ERROR` - TX FIFO errors (if enabled)
- `DBG_MSG_TIMEOUT` - Module timeout warnings
- `DBG_MSG_DEREGISTER` - Module deregistration messages
- `DBG_MSG_REG_EVENTS` - Registration events (maybe)

## Benefits

1. **First occurrence captured** - Important to know something went wrong
2. **No flooding** - Subsequent occurrences suppressed
3. **Configurable** - Easy to add/remove messages from once-only list
4. **Per-flag granularity** - Each message type tracked independently
5. **Reset on power cycle** - Fresh start each session

## Example Scenario

1. Module 2 registers → Registration message shown (if once-only)
2. Module 2 deregisters → Deregister message shown (first time)
3. Module 2 registers again → Registration message NOT shown (already shown once)
4. Module 2 deregisters again → Deregister message NOT shown (already shown once)
5. Unknown CAN message → Error shown (first time)
6. More unknown CAN messages → Errors NOT shown (already shown once)

## Questions

1. Should registration events be once-only or always shown?
2. Should we have a way to reset the once-only tracking without a hard reset?
3. Should timeout messages be once-only or show first timeout per module?