# Proposed Conversion of Old Debug Messages to New System

## Analysis of Current Messages

Looking at the old debug messages, they fall into distinct categories related to the polling and status request cycle:

### 1. Polling Cycle Messages (High Frequency)
- **Line 326**: "Checking %d modules" - Start of polling cycle
- **Line 333**: "module[%d] ID=%02x elapsed=%lu pending=%d" - Per-module check
- **Line 398**: "Module ID=%02x elapsed=%lu pending=%d commsErr=%d" - **THE FLOODER**
- **Line 423**: "Round-robin polling module ID=%02x" - Round-robin status

### 2. Status Request Messages
- **Line 382**: "Requesting status from module ID=%02x" - Status request sent

### 3. State Machine Messages  
- **Line 473**: "Module ID=%02x current=%d next=%d cmd=%d cmdStatus=%d" - State transitions

## Proposed New Special Message IDs

### Add to debug.h:
```c
// Polling and status monitoring messages (0xF00C - 0xF00F range)
#define MSG_POLLING_CYCLE      0xF00C  // Start of polling cycle
#define MSG_MODULE_CHECK       0xF00D  // Per-module status check
#define MSG_STATUS_REQUEST     0xF00E  // Status request sent (different from ID_MODULE_STATUS_REQUEST which is the CAN message)
#define MSG_STATE_TRANSITION   0xF00F  // Module state machine transition
```

## Proposed New Debug Flags

### Add to debug.h:
```c
#define DBG_MSG_POLLING_DETAIL    0x08000000  // Detailed polling information
#define DBG_MSG_STATE_MACHINE     0x10000000  // State machine transitions
```

## Message Conversions

### 1. Line 326 - Polling cycle start
```c
// OLD:
sprintf(tempBuffer,"MCU DEBUG - Checking %d modules", pack.moduleCount);
serialOut(tempBuffer);

// NEW:
ShowDebugMessage(MSG_POLLING_CYCLE, pack.moduleCount);
```

### 2. Line 333 & 398 - Module check (THE FLOODER)
These are essentially the same message at different points. Consolidate to one:
```c
// OLD (line 398 - the flooding one):
sprintf(tempBuffer,"MCU DEBUG - Module ID=%02x elapsed=%lu pending=%d commsErr=%d", 
        module[index].moduleId, elapsedTicks, module[index].statusPending,
        module[index].faultCode.commsError);
serialOut(tempBuffer);

// NEW:
ShowDebugMessage(MSG_MODULE_CHECK, module[index].moduleId, elapsedTicks, 
                module[index].statusPending, module[index].faultCode.commsError);
```

### 3. Line 382 - Status request
```c
// OLD:
sprintf(tempBuffer,"MCU DEBUG - Requesting status from module ID=%02x (index=%d)", 
        module[index].moduleId, index);
serialOut(tempBuffer);

// NEW:
ShowDebugMessage(MSG_STATUS_REQUEST, module[index].moduleId, index);
```

### 4. Line 423 - Round-robin polling
```c
// OLD:
sprintf(tempBuffer,"MCU DEBUG - Round-robin polling module ID=%02x (index=%d)", 
        module[MCU_GetNextModuleIndex()].moduleId, MCU_GetNextModuleIndex());
serialOut(tempBuffer);

// NEW:
uint8_t nextIndex = MCU_GetNextModuleIndex();
ShowDebugMessage(MSG_STATUS_REQUEST, module[nextIndex].moduleId, nextIndex);  // Reuse MSG_STATUS_REQUEST
```

### 5. Line 473 - State machine
```c
// OLD:
sprintf(tempBuffer,"MCU DEBUG - Module ID=%02x current=%d next=%d cmd=%d cmdStatus=%d",
        module[index].moduleId, module[index].currentState, module[index].nextState,
        module[index].command.commandedState, module[index].command.commandStatus);
serialOut(tempBuffer);

// NEW:
ShowDebugMessage(MSG_STATE_TRANSITION, module[index].moduleId, 
                module[index].currentState, module[index].nextState,
                module[index].command.commandedState, module[index].command.commandStatus);
```

## Debug Table Entries (debug.c)

```c
// Polling and monitoring messages
{MSG_POLLING_CYCLE, DBG_MCU, DBG_MSG_POLLING_DETAIL,
 "MCU DEBUG - Checking %d modules", NULL},

{MSG_MODULE_CHECK, DBG_MCU, DBG_MSG_POLLING_DETAIL,
 "MCU DEBUG - Module ID=%02x elapsed=%lu pending=%d commsErr=%d", 
 ".%d"},  // Minimal format: just module ID

{MSG_STATUS_REQUEST, DBG_MCU, DBG_MSG_POLLING_DETAIL,
 "MCU DEBUG - Requesting status from module ID=%02x (index=%d)", NULL},

{MSG_STATE_TRANSITION, DBG_MCU, DBG_MSG_STATE_MACHINE,
 "MCU DEBUG - Module ID=%02x current=%d next=%d cmd=%d cmdStatus=%d", NULL},
```

## Default Configuration

### Enable by default with once-only:
```c
// Include polling and state machine messages by default
#define DEBUG_MESSAGES  (DBG_MSG_REGISTRATION_GROUP | DBG_MSG_DEREGISTER | \
                        DBG_MSG_DEREGISTER_ALL | DBG_MSG_TIMEOUT | \
                        DBG_MSG_STATUS_REQ | DBG_MSG_STATUS1 | DBG_MSG_MINIMAL | \
                        DBG_MSG_REG_EVENTS | DBG_MSG_CAN_ERRORS | \
                        DBG_MSG_POLLING_DETAIL | DBG_MSG_STATE_MACHINE)

// Add to once-only list to prevent flooding
#define DEBUG_ONCE_ONLY (DBG_MSG_CAN_ERRORS | DBG_MSG_TX_FIFO_ERROR | \
                        DBG_MSG_POLLING_DETAIL | DBG_MSG_STATE_MACHINE)
```

## Once-Only Behavior

With these as once-only:
- **First polling cycle** → Shows "Checking X modules" 
- **First module check** → Shows "Module ID=XX elapsed=XXX pending=X commsErr=X"
- **First status request** → Shows "Requesting status from module ID=XX"
- **First state transition** → Shows state machine info
- **Subsequent occurrences** → All suppressed (no flooding)

This gives us the diagnostic info we need without the flood!

## Benefits

1. **No more flooding** - Polling details disabled by default
2. **Available when needed** - Can enable DBG_MSG_POLLING_DETAIL for diagnostics  
3. **Proper categorization** - Messages grouped logically
4. **Preserves information** - The old messages showed useful data (elapsed, pending, commsErr)
5. **Minimal format option** - MSG_MODULE_CHECK has minimal format for compact display