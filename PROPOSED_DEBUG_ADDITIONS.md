# Proposed Debug Message Additions

## Overview
We need to add debug messages for the remaining TODOs in mcu.c. These include various system messages and error conditions.

## New Message IDs to Add (debug.h)

### Special Message IDs (0xF000 range)
```c
#define MSG_VOLTAGE_SELECTION  0xF004  // Module voltage selection info
#define MSG_UNKNOWN_CAN_ID     0xF005  // Unknown CAN message received
#define MSG_TX_FIFO_ERROR      0xF006  // CAN TX FIFO error
#define MSG_MODULE_REREGISTER  0xF007  // Module re-registration
#define MSG_NEW_MODULE_REG     0xF008  // New module registration
#define MSG_UNREGISTERED_MOD   0xF009  // Unregistered module error
#define MSG_TIMEOUT_RESET      0xF00A  // Timeout counter reset
#define MSG_CELL_DETAIL_REQ    0xF00B  // Cell detail request
```

## New Debug Flags to Add (debug.h)

```c
#define DBG_MSG_VOLTAGE_SEL       0x00400000  // Voltage selection messages
#define DBG_MSG_CAN_ERRORS        0x00800000  // CAN error messages  
#define DBG_MSG_REG_EVENTS        0x01000000  // Registration events
#define DBG_MSG_CELL_DETAIL_REQ   0x02000000  // Cell detail request
```

## New Message Definitions (debug.c)

```c
// Module selection and errors
{MSG_VOLTAGE_SELECTION, DBG_MCU, DBG_MSG_VOLTAGE_SEL,
 "MCU INFO - Selected module ID=%02x with voltage=%dmV", NULL},

{MSG_UNKNOWN_CAN_ID, DBG_ERRORS, DBG_MSG_CAN_ERRORS,
 "MCU ERROR - Unknown CAN ID: 0x%03x", NULL},

{MSG_TX_FIFO_ERROR, DBG_ERRORS, DBG_MSG_CAN_ERRORS,
 "MCU ERROR - TX FIFO error on CAN%d, TEC=%d, REC=%d, Flags=0x%08x", NULL},

// Registration events
{MSG_MODULE_REREGISTER, DBG_MCU, DBG_MSG_REG_EVENTS,
 "MCU INFO - Module re-registered: ID=%02x, UID=%08x", NULL},

{MSG_NEW_MODULE_REG, DBG_MCU, DBG_MSG_REG_EVENTS,
 "MCU INFO - New module registered: ID=%02x, UID=%08x, Index=%d", NULL},

{MSG_UNREGISTERED_MOD, DBG_ERRORS, DBG_MSG_REG_EVENTS,
 "MCU ERROR - Status from unregistered module: ID=%02x", NULL},

{MSG_TIMEOUT_RESET, DBG_MCU, DBG_MSG_TIMEOUT,
 "MCU INFO - Module ID=%02x timeout counter reset (was %d)", NULL},

// Already have IDs for these, just need to add to table:
{ID_MODULE_ISOLATE_ALL, DBG_COMMS, DBG_MSG_ISOLATE_ALL,
 "MCU TX 0x51F Isolate All Modules", NULL},

{ID_MODULE_DEREGISTER_ALL, DBG_COMMS, DBG_MSG_DEREGISTER_ALL,
 "MCU TX 0x51E De-Register All Modules", NULL},

{ID_MODULE_TIME_REQUEST, DBG_COMMS, DBG_MSG_TIME_REQ,
 "MCU RX 0x506 Time Request from Module ID=%02x", NULL},

{ID_MODULE_SET_TIME, DBG_COMMS, DBG_MSG_SET_TIME,
 "MCU TX 0x516 Set Time: %04d-%02d-%02d %02d:%02d:%02d", NULL},

{ID_MODULE_HARDWARE_REQUEST, DBG_COMMS, DBG_MSG_HARDWARE_REQ,
 "MCU TX 0x511 Hardware Request: ID=%02x", NULL},

{MSG_CELL_DETAIL_REQ, DBG_COMMS, DBG_MSG_CELL_DETAIL_REQ,
 "MCU TX 0x515 Cell Detail Request: ID=%02x, Cell=%d", NULL},

{ID_MODULE_CELL_DETAIL, DBG_COMMS, DBG_MSG_CELL_DETAIL,
 "MCU RX 0x505 Cell Detail: ID=%02x, Cell=%d, V=%dmV", NULL},
```

## ShowDebugMessage Calls to Add (mcu.c)

### 1. Voltage Selection (line ~505)
```c
ShowDebugMessage(MSG_VOLTAGE_SELECTION, moduleId, module[moduleId-1].status1.voltage);
```

### 2. Unknown CAN ID (line ~976)
```c
ShowDebugMessage(MSG_UNKNOWN_CAN_ID, rxHeader.Identifier);
```

### 3. TX FIFO Error (line ~999)
```c
ShowDebugMessage(MSG_TX_FIFO_ERROR, index, tec, rec, errorFlags);
```

### 4. Module Re-registration (line ~1055)
```c
ShowDebugMessage(MSG_MODULE_REREGISTER, moduleId, announcement.serialNumber);
```

### 5. New Module Registration (line ~1083)
```c
ShowDebugMessage(MSG_NEW_MODULE_REG, moduleId, announcement.serialNumber, pack.numberOfActiveModules-1);
```

### 6. Deregister All (line ~1182)
```c
ShowDebugMessage(ID_MODULE_DEREGISTER_ALL);
```

### 7. Isolate All (line ~1222)
```c
ShowDebugMessage(ID_MODULE_ISOLATE_ALL);
```

### 8. Time Request (line ~1263)
```c
ShowDebugMessage(ID_MODULE_TIME_REQUEST, moduleId);
```

### 9. Set Time (line ~1287)
```c
ShowDebugMessage(ID_MODULE_SET_TIME, 
    moduleTime.year, moduleTime.month, moduleTime.day,
    moduleTime.hour, moduleTime.minute, moduleTime.second);
```

### 10. Hardware Request (line ~1335)
```c
ShowDebugMessage(ID_MODULE_HARDWARE_REQUEST, moduleId);
```

### 11. Unregistered Module (line ~1544)
```c
ShowDebugMessage(MSG_UNREGISTERED_MOD, moduleId);
```

### 12. Timeout Reset (lines ~1556, 1661, 1736)
```c
if(module[moduleIndex].consecutiveTimeouts > 0) {
    ShowDebugMessage(MSG_TIMEOUT_RESET, moduleId, module[moduleIndex].consecutiveTimeouts);
}
```

### 13. Cell Detail Request (line ~1836)
```c
ShowDebugMessage(MSG_CELL_DETAIL_REQ, moduleId, cellId);
```

### 14. Cell Detail RX (line ~1945)
```c
ShowDebugMessage(ID_MODULE_CELL_DETAIL, moduleId, 
    cellDetail.cellNumber, cellDetail.voltage);
```

## Configuration Updates

In debug.h, update the default configuration to include the new messages:
```c
#define DEBUG_MESSAGES  (DBG_MSG_REGISTRATION_GROUP | DBG_MSG_DEREGISTER | \
                        DBG_MSG_DEREGISTER_ALL | DBG_MSG_TIMEOUT | \
                        DBG_MSG_STATUS_REQ | DBG_MSG_STATUS1 | DBG_MSG_MINIMAL | \
                        DBG_MSG_REG_EVENTS | DBG_MSG_CAN_ERRORS)
```

## Testing Impact

These additions will provide visibility into:
- Module registration/re-registration events
- CAN communication errors
- Module selection logic
- Time synchronization
- Cell detail requests
- Timeout recovery

The messages are categorized appropriately (DBG_ERRORS for errors, DBG_MCU for system events, DBG_COMMS for CAN messages).

## Questions

1. Should we add minimal format strings for any of these?
2. Do you want the voltage selection message to show all module voltages or just the selected one?
3. Should timeout reset only be logged if the counter was non-zero (as proposed)?