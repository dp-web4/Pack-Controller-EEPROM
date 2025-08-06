# Debug System Documentation

## Overview

The Pack Controller debug system provides configurable, hierarchical logging for CAN messages and internal events. It uses a table-driven approach for maintainability and minimal runtime overhead.

## Architecture

### Message ID Ranges

The system uses two distinct ID ranges:

1. **CAN Message IDs (0x500-0x51F)**: Actual CAN bus messages
   - Example: `ID_MODULE_STATUS_REQUEST` (0x512)
   - These represent real messages transmitted or received on the CAN bus

2. **Special Message IDs (0xF000+)**: Internal Pack Controller events  
   - Example: `MSG_TIMEOUT_WARNING` (0xF001)
   - These represent events occurring within the Pack Controller code
   - Not actual CAN messages, but important system events

### Components

1. **debug.h**: Configuration and definitions
   - Debug levels (DBG_ERRORS, DBG_COMMS, DBG_MCU, etc.)
   - Message flags for individual message control
   - Special message ID definitions

2. **debug.c**: Core implementation
   - Message definition table with formats
   - `ShowDebugMessage()` function for output
   - Support for both full and minimal output modes

3. **mcu.c**: Integration points
   - Calls to `ShowDebugMessage()` throughout the code
   - Replaces previous sprintf/serialOut pattern

## Configuration

### Debug Levels

Control which categories of messages are displayed:

```c
#define DEBUG_LEVEL (DBG_ERRORS | DBG_COMMS | DBG_MCU)
```

- `DBG_DISABLED` (0x00): No debug output
- `DBG_ERRORS` (0x01): Error messages only
- `DBG_COMMS` (0x02): CAN communication messages
- `DBG_MCU` (0x08): Module control unit events
- `DBG_VCU` (0x10): Vehicle control unit events
- `DBG_VERBOSE` (0x80): Detailed verbose output
- `DBG_ALL` (0xFF): Everything

### Message Flags

Fine-grained control over individual message types:

```c
#define DEBUG_MESSAGES (DBG_MSG_STATUS_REQ | DBG_MSG_STATUS1 | DBG_MSG_MINIMAL)
```

Key flags include:
- `DBG_MSG_ANNOUNCE_REQ`: Module announcement requests
- `DBG_MSG_STATUS_REQ`: Status polling messages
- `DBG_MSG_STATUS1/2/3`: Status response messages
- `DBG_MSG_TIMEOUT`: Timeout events
- `DBG_MSG_MINIMAL`: Enable minimal output mode
- `DBG_MSG_REG_EVENTS`: Registration/deregistration events
- `DBG_MSG_CAN_ERRORS`: CAN bus errors

## Message Formats

### Full Format
Standard detailed output with complete information:
```
MCU TX 0x512 Request Status: ID=02
MCU RX 0x502 Status #1: ID=02, State=3, Status=0, SOC=95%, SOH=100%, Cells=14, Volt=3750, Curr=0
```

### Minimal Format
Compact output for high-frequency messages:
```
.1-1.2-2
```
- `.1-`: Status request to module 1
- `1`: Response from module 1
- `.2-`: Status request to module 2  
- `2`: Response from module 2

## Special Messages

Internal events use the 0xF000+ range:

| ID | Name | Purpose |
|----|------|---------|
| 0xF001 | MSG_TIMEOUT_WARNING | Module timeout detected |
| 0xF002 | MSG_DEREGISTER | Module being removed |
| 0xF004 | MSG_VOLTAGE_SELECTION | Module selected by voltage |
| 0xF005 | MSG_UNKNOWN_CAN_ID | Unknown CAN message received |
| 0xF006 | MSG_TX_FIFO_ERROR | CAN transmission error |
| 0xF007 | MSG_MODULE_REREGISTER | Module re-registration |
| 0xF008 | MSG_NEW_MODULE_REG | New module registered |
| 0xF009 | MSG_UNREGISTERED_MOD | Message from unregistered module |
| 0xF00A | MSG_TIMEOUT_RESET | Timeout counter cleared |
| 0xF00B | MSG_CELL_DETAIL_REQ | Cell detail request |

## Usage Examples

### Basic Message
```c
ShowDebugMessage(ID_MODULE_STATUS_REQUEST, moduleId);
```

### Message with Multiple Parameters
```c
ShowDebugMessage(ID_MODULE_STATUS_1, moduleId, 
    status1->moduleState, status1->moduleStatus,
    status1->stateOfCharge, status1->stateOfHealth,
    status1->numberOfCells, status1->voltage, status1->current);
```

### Conditional Message
```c
if(module[moduleIndex].consecutiveTimeouts > 0) {
    ShowDebugMessage(MSG_TIMEOUT_RESET, moduleId, module[moduleIndex].consecutiveTimeouts);
}
```

## Adding New Messages

1. **Define the message ID** (in debug.h):
   - For CAN messages: Use the actual CAN ID
   - For internal events: Use 0xF0xx range

2. **Add a message flag** (in debug.h):
   ```c
   #define DBG_MSG_YOUR_MESSAGE 0x04000000
   ```

3. **Add to message table** (in debug.c):
   ```c
   {MSG_YOUR_MESSAGE, DBG_MCU, DBG_MSG_YOUR_MESSAGE,
    "MCU INFO - Your message: param1=%d, param2=%d", NULL},
   ```

4. **Call ShowDebugMessage** (in your code):
   ```c
   ShowDebugMessage(MSG_YOUR_MESSAGE, param1, param2);
   ```

## Performance Considerations

- Messages are filtered at multiple levels (debug level, then message flag)
- Table lookup is linear but typically fast (< 50 entries)
- Minimal mode reduces UART traffic for high-frequency messages
- No dynamic memory allocation

## Debugging Module Issues

The debug system has been instrumental in identifying module communication issues:

1. **Module 2 Going Offline**: The minimal output clearly showed:
   - Before announcement: `.1-1.2-2` (both responding)
   - After announcement: `.2-.1-1` (module 2 stops)

2. **Timeout Tracking**: MSG_TIMEOUT_WARNING and MSG_TIMEOUT_RESET help track:
   - When modules miss responses
   - When communication is restored

3. **Registration Events**: Track the full lifecycle:
   - MSG_NEW_MODULE_REG: New module joins
   - MSG_MODULE_REREGISTER: Existing module re-announces
   - MSG_DEREGISTER: Module removed from pack

## Default Configuration

The current default configuration (debug.h) enables:
- Error messages (DBG_ERRORS)
- Communication messages (DBG_COMMS)  
- MCU events (DBG_MCU)
- Registration events (DBG_MSG_REG_EVENTS)
- CAN errors (DBG_MSG_CAN_ERRORS)
- Minimal output mode (DBG_MSG_MINIMAL)

This provides good visibility into system operation while keeping output manageable.