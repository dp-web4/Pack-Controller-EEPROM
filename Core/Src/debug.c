/***************************************************************************************************************
*     D e b u g   M e s s a g e   S y s t e m                                      P A C K   C O N T R O L L E R
***************************************************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "main.h"
#include "debug.h"
#include "mcu.h"
#include "can_id_module.h"

// Debug message definitions table
static const DebugMessageDef debugMessageDefs[] = {
    // TX Messages
    {ID_MODULE_STATUS_REQUEST, DBG_COMMS, DBG_MSG_STATUS_REQ, 
     "MCU TX 0x512 Request Status: ID=%02x", ".%d-"},
     
    {ID_MODULE_ANNOUNCE_REQUEST, DBG_COMMS, DBG_MSG_ANNOUNCE_REQ,
     "MCU TX 0x51D Request module announcements", NULL},
     
    {ID_MODULE_REGISTRATION, DBG_COMMS, DBG_MSG_REGISTRATION,
     "MCU TX 0x510 Registration: ID=%02x, CTL=%02x, MFG=%02x, PN=%02x, UID=%08x", NULL},
     
    {ID_MODULE_STATE_CHANGE, DBG_COMMS, DBG_MSG_STATE_CHANGE,
     "MCU TX 0x514 State Change: ID=%02x, State=%d", NULL},
     
    {ID_MODULE_DEREGISTER, DBG_COMMS, DBG_MSG_DEREGISTER,
     "MCU TX 0x518 De-Register module ID=%02x", NULL},
     
    // RX Messages  
    {ID_MODULE_STATUS_1, DBG_COMMS, DBG_MSG_STATUS1,
     "MCU RX 0x502 Status #1: ID=%02x, State=%01x, Status=%01x, SOC=%d%%, SOH=%d%%, Cells=%d, Volt=%d, Curr=%d",
     "%d"},  // Minimal: just module ID
     
    {ID_MODULE_STATUS_2, DBG_COMMS, DBG_MSG_STATUS2,
     "MCU RX 0x503 Status #2: ID=%02x", NULL},
     
    {ID_MODULE_STATUS_3, DBG_COMMS, DBG_MSG_STATUS3,
     "MCU RX 0x504 Status #3: ID=%02x", NULL},
     
    {ID_MODULE_ANNOUNCEMENT, DBG_COMMS, DBG_MSG_ANNOUNCE,
     "MCU RX 0x500 Announcement: FW=%04x, MFG=%02x, PN=%02x, UID=%08x", NULL},
     
    {ID_MODULE_HARDWARE, DBG_COMMS, DBG_MSG_HARDWARE,
     "MCU RX 0x501 Hardware: ID=%02x", NULL},
     
    // Timeout/Error Messages (special IDs)
    {MSG_TIMEOUT_WARNING, DBG_ERRORS, DBG_MSG_TIMEOUT,
     "MCU TIMEOUT - Module ID=%02x (timeout %d of %d)", "%dT%d"},
     
    {MSG_DEREGISTER, DBG_ERRORS, DBG_MSG_DEREGISTER,
     "MCU INFO - Removing module from pack: ID=%02x, UID=%08x, Index=%d", "%dD"},
     
    // End marker
    {0, 0, 0, NULL, NULL}
};

// External variables
extern uint8_t debugLevel;
extern uint32_t debugMessages;
extern UART_HandleTypeDef huart1;
extern void serialOut(char *message);

// Find message definition by ID
static const DebugMessageDef* FindDebugMessageDef(uint16_t messageId) {
    for(int i = 0; debugMessageDefs[i].messageId != 0; i++) {
        if(debugMessageDefs[i].messageId == messageId) {
            return &debugMessageDefs[i];
        }
    }
    return NULL;
}

// Check if message should be shown
static bool ShouldShowDebugMessage(uint16_t messageId) {
    const DebugMessageDef* def = FindDebugMessageDef(messageId);
    if(!def) return false;
    
    // Check debug level
    if((debugLevel & def->requiredLevel) == 0) return false;
    
    // Check message flag
    if((debugMessages & def->requiredFlag) == 0) return false;
    
    return true;
}

// Show debug message with variable arguments
void ShowDebugMessage(uint16_t messageId, ...) {
    if(!ShouldShowDebugMessage(messageId)) return;
    
    const DebugMessageDef* def = FindDebugMessageDef(messageId);
    if(!def) return;
    
    // Determine which format to use
    const char* format = NULL;
    bool useMinimal = (debugMessages & DBG_MSG_MINIMAL) != 0;
    
    if(useMinimal && def->minFormat) {
        format = def->minFormat;
    } else if(def->fullFormat) {
        format = def->fullFormat;
    } else {
        return;  // No suitable format
    }
    
    // Format the message
    char tempBuffer[256];
    va_list args;
    va_start(args, messageId);
    vsnprintf(tempBuffer, sizeof(tempBuffer), format, args);
    va_end(args);
    
    // Output the message
    if(useMinimal && def->minFormat) {
        // Minimal mode - no newline, direct UART output
        HAL_UART_Transmit(&huart1, (uint8_t*)tempBuffer, strlen(tempBuffer), HAL_MAX_DELAY);
    } else {
        // Full mode - use serialOut with newline
        serialOut(tempBuffer);
    }
}