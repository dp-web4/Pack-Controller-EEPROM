# Proposed Debug System Overhaul

## Current Problems
1. Some messages bypass individual flags (raw CAN dumps)
2. Inconsistent handling of minimal vs full mode
3. Debug level and message flags interact confusingly
4. No clear hierarchy of control

## Proposed Architecture

### Three-Level Control System

```
Level 1: DEBUG_LEVEL (Category enable/disable)
    ↓
Level 2: DEBUG_MESSAGES (Individual message flags)
    ↓
Level 3: DEBUG_MODE (Minimal or Full format)
```

### 1. Debug Levels (Categories)
```c
#define DBG_NONE        0x00
#define DBG_ERRORS      0x01  // Error messages only
#define DBG_COMMS       0x02  // CAN communication
#define DBG_MCU         0x04  // MCU operations
#define DBG_VCU         0x08  // VCU operations
#define DBG_MODULES     0x10  // Module operations
#define DBG_TIMING      0x20  // Timing/performance
#define DBG_ALL         0xFF  // Everything
```

### 2. Message Flags (Individual Control)
Already defined correctly - one bit per message type

### 3. Display Modes
```c
#define DBG_MODE_FULL     0  // Full verbose messages
#define DBG_MODE_MINIMAL  1  // Compact messages only
```

## Proposed Message Handler Function

```c
typedef struct {
    uint16_t messageId;
    uint8_t  debugLevel;     // Required debug level
    uint32_t messageFlag;     // Required message flag
    const char* fullFormat;   // Full message format (NULL if no full format)
    const char* minFormat;    // Minimal format (NULL if no minimal format)
} DebugMessageDef;

// Define all messages in a table
const DebugMessageDef debugMessages[] = {
    // TX Messages
    {ID_MODULE_STATUS_REQUEST, DBG_COMMS, DBG_MSG_STATUS_REQ, 
     "MCU TX 0x512 Request Status: ID=%02x",
     ".%d-"},  // Minimal: .1-
     
    {ID_MODULE_ANNOUNCE_REQUEST, DBG_COMMS, DBG_MSG_ANNOUNCE_REQ,
     "MCU TX 0x51D Request module announcements",
     NULL},  // No minimal format
     
    // RX Messages  
    {ID_MODULE_STATUS_1, DBG_COMMS, DBG_MSG_STATUS1,
     "MCU RX 0x502 Status #1: ID=%02x, State=%01x, Status=%01x, SOC=%d%%",
     "%d"},  // Minimal: just module ID
     
    {ID_MODULE_ANNOUNCEMENT, DBG_COMMS, DBG_MSG_ANNOUNCE,
     "MCU RX 0x500 Announcement: FW=%04x, MFG=%02x, PN=%02x, UID=%08x",
     NULL},  // No minimal format
     
    // Timeout/Error Messages
    {MSG_TIMEOUT_WARNING, DBG_ERRORS, DBG_MSG_TIMEOUT,
     "MCU TIMEOUT - Module ID=%02x (timeout %d of %d)",
     "%dT%d"},  // Minimal: 2T1
     
    {MSG_DEREGISTER, DBG_ERRORS, DBG_MSG_DEREGISTER,
     "MCU INFO - Removing module: ID=%02x, UID=%08x",
     "%dD"},  // Minimal: 2D
};

bool ShouldShowMessage(uint16_t messageId, bool isTx) {
    // Find message definition
    DebugMessageDef* msg = FindMessageDef(messageId);
    if (!msg) return false;
    
    // Check debug level first
    if ((debugLevel & msg->debugLevel) == 0) return false;
    
    // Check message flag
    if ((debugMessages & msg->messageFlag) == 0) return false;
    
    // Message is enabled
    return true;
}

void ShowDebugMessage(uint16_t messageId, ...) {
    DebugMessageDef* msg = FindMessageDef(messageId);
    if (!ShouldShowMessage(messageId)) return;
    
    // Determine format to use
    const char* format = NULL;
    if (debugMessages & DBG_MSG_MINIMAL) {
        format = msg->minFormat;  // Use minimal if available
        if (!format) format = msg->fullFormat;  // Fall back to full
    } else {
        format = msg->fullFormat;  // Use full format
    }
    
    if (!format) return;  // No format available
    
    // Format and output message
    va_list args;
    va_start(args, messageId);
    vsprintf(tempBuffer, format, args);
    va_end(args);
    
    if (debugMessages & DBG_MSG_MINIMAL && msg->minFormat) {
        // Minimal mode - no newline for compact display
        HAL_UART_Transmit(&huart1, (uint8_t*)tempBuffer, strlen(tempBuffer), HAL_MAX_DELAY);
    } else {
        // Full mode - use serialOut with newline
        serialOut(tempBuffer);
    }
}
```

## Implementation Changes Needed

### 1. Remove Raw CAN Dumps
Replace lines like:
```c
// Remove this:
sprintf(tempBuffer,"MCU RX ID=0x%03x : EID=0x%08x : Byte[0..7]=...", ...);

// With message-specific handling
ShowDebugMessage(ID_MODULE_STATUS_1, moduleId, state, status, soc);
```

### 2. Update All Message Points
Every place that outputs debug needs to use the new system:
```c
// Old way:
if(debugMessages & DBG_MSG_STATUS_REQ){ 
    sprintf(tempBuffer,"MCU TX 0x512 Request Status : ID=%02x",moduleId); 
    serialOut(tempBuffer);
}

// New way:
ShowDebugMessage(ID_MODULE_STATUS_REQUEST, moduleId);
```

### 3. Module Side (ModuleCPU)
Apply same system to module firmware:
- Define module-specific debug categories
- Create message definition table
- Use consistent ShowDebugMessage function

## Benefits
1. **Clean Control**: Each message has clear on/off control
2. **Consistent Format**: All messages follow same rules
3. **No Flooding**: Raw dumps eliminated
4. **Easy Maintenance**: Add new messages to table
5. **Flexible Output**: Automatic minimal/full mode handling

## Example Configuration

### For Quiet Polling Debug:
```c
#define DEBUG_LEVEL    (DBG_ERRORS | DBG_COMMS)
#define DEBUG_MESSAGES (DBG_MSG_STATUS_REQ | DBG_MSG_STATUS1 | \
                       DBG_MSG_TIMEOUT | DBG_MSG_DEREGISTER | \
                       DBG_MSG_MINIMAL)
```
Output: `.1-1.2-2.1-1.2-2T1.2-2T2.2-2D`

### For Registration Debug:
```c
#define DEBUG_LEVEL    (DBG_COMMS)
#define DEBUG_MESSAGES (DBG_MSG_ANNOUNCE_REQ | DBG_MSG_ANNOUNCE | \
                       DBG_MSG_REGISTRATION)
```
Output: Full registration messages only

### For Error Tracking:
```c
#define DEBUG_LEVEL    (DBG_ERRORS)
#define DEBUG_MESSAGES (DBG_MSG_TIMEOUT | DBG_MSG_DEREGISTER | \
                       DBG_MSG_MINIMAL)
```
Output: `2T1 2T2 2T3 2D` (timeouts and deregistrations only)

## Questions
1. Should we implement this as a table-driven system or keep individual handlers?
2. Should minimal mode messages have newlines or stay compact?
3. Should we add timestamp prefix option for all messages?
4. Do we need a "critical only" mode that shows even less?