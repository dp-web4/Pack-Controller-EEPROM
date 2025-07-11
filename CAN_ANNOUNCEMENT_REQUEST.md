# Module Announcement Request (0x51D)

## Overview
The Module Announcement Request is a broadcast message sent by the Pack Controller to request all unregistered modules on the CAN bus to send their announcement messages. This helps ensure all modules are discovered, especially after power cycles or when modules are added to the system.

## Message Details

### CAN ID
- **Standard ID**: 0x51D
- **Extended ID**: 0x00 (broadcast to all modules)
- **Direction**: Pack Controller → All Modules

### Data Format
| Byte | Bits | Field | Description |
|------|------|-------|-------------|
| 0 | 0-7 | controllerId | Pack Controller ID |

### Structure Definition
```c
typedef struct {                  // 0x51D ANNOUNCEMENT REQUEST - 1 bytes
  uint8_t controllerId  : 8;      // pack controller ID
}CANFRM_MODULE_ANNOUNCE_REQUEST;
```

## Behavior

### Pack Controller
1. Sends announcement request on startup after de-registering and isolating all modules
2. Sends periodic announcement requests every 1 second (configurable via `MCU_ANNOUNCE_REQUEST_INTERVAL`, currently set to 1 second for testing)
3. Uses broadcast Extended ID 0x00 to reach all modules

### Module Controller
When a module receives this message, it should:
1. Check if it is currently unregistered (no assigned module ID)
2. If unregistered, wait a pseudo-random delay (0-100ms recommended) to reduce bus contention
3. Send a Module Announcement message (0x500)

## Timing
- **Startup**: Sent once during pack controller initialization
- **Periodic**: Sent every 1 second (currently set for testing, typically 30 seconds)
- **Manual**: Can be triggered by user command or special events

## Implementation Notes

### Pack Controller Code
```c
void MCU_RequestModuleAnnouncement(void){
  CANFRM_MODULE_ANNOUNCE_REQUEST announceRequest;
  
  // configure the packet
  announceRequest.controllerId = pack.id;
  
  // ... CAN transmission setup ...
  
  txObj.bF.id.SID = ID_MODULE_ANNOUNCE_REQUEST;   // Standard ID
  txObj.bF.id.EID = 0;                            // Extended ID - broadcast to all
  
  // ... transmit message ...
}
```

### Module Controller Response
The module controller should implement:
- Reception handler for 0x51D messages
- Check registration status
- Random delay generation
- Announcement transmission if unregistered

## Benefits
1. **Reliable Discovery**: Ensures all modules are discovered even if initial announcements are missed
2. **Power Cycle Recovery**: Helps modules re-register after power interruptions
3. **Collision Reduction**: Random delays in module responses reduce CAN bus collisions
4. **Controlled Discovery**: Pack controller decides when to discover new modules

## Debug Messages
When debug level includes `DBG_MCU`, the pack controller will output:
```
MCU TX 0x51D Request module announcements
```