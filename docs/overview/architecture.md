# System Architecture

## Table of Contents

1. [Overview](#overview)
2. [Dual-Core Architecture](#dual-core-architecture)
3. [Memory Architecture](#memory-architecture)
4. [Software Architecture](#software-architecture)
5. [Communication Architecture](#communication-architecture)
6. [Data Flow](#data-flow)
7. [State Management](#state-management)
8. [Error Handling Architecture](#error-handling-architecture)

## Overview

The Pack Controller EEPROM firmware implements a sophisticated architecture that leverages the STM32WB55's dual-core capabilities to provide robust battery management with advanced parameter storage, real-time communication, and comprehensive safety systems.

### Architectural Principles

**Modularity**: Clear separation of concerns with well-defined interfaces between components
**Reliability**: Multiple redundancy layers and fault tolerance mechanisms
**Scalability**: Support for 1-32 modules with up to 192 cells per module
**Performance**: Real-time operation with deterministic response times
**Safety**: Multiple protection layers with fail-safe operation modes

## Dual-Core Architecture

### Core Allocation Strategy

```
┌─────────────────────────────────────────────────────────────────┐
│                   STM32WB55 Dual-Core System                   │
├─────────────────────────────────┬───────────────────────────────┤
│        Application Core         │        Network Core           │
│     (ARM Cortex-M4 @ 64MHz)     │    (ARM Cortex-M0+ @ 32MHz)  │
├─────────────────────────────────┼───────────────────────────────┤
│ Primary Functions:              │ Secondary Functions:          │
│ • Battery management logic      │ • Bluetooth LE stack         │
│ • CAN communication            │ • Wireless diagnostics       │
│ • Safety monitoring            │ • OTA updates (future)       │
│ • EEPROM management            │ • Security services          │
│ • VCU interface                │ • Auxiliary communication    │
│ • Module coordination          │ • Background processing      │
│ • State machine control        │ • Data logging               │
│ • Real-time tasks              │ • Non-critical tasks         │
└─────────────────────────────────┴───────────────────────────────┘
```

### Inter-Core Communication

```c
// Shared memory structure for core communication
typedef struct {
    volatile uint32_t m4_to_m0_flags;      // Command flags M4 → M0+
    volatile uint32_t m0_to_m4_flags;      // Status flags M0+ → M4
    volatile uint8_t shared_buffer[256];    // Data exchange buffer
    volatile uint32_t buffer_head;          // Buffer write pointer
    volatile uint32_t buffer_tail;          // Buffer read pointer
    volatile uint32_t sequence_number;      // Message sequencing
} InterCoreComm;

// Inter-core mailbox system
#define IPCC_CHANNEL_1    0x01  // Command channel
#define IPCC_CHANNEL_2    0x02  // Data channel  
#define IPCC_CHANNEL_3    0x04  // Status channel

void SendCommandToM0(uint32_t command, void* data, uint32_t length) {
    // Write data to shared memory
    WriteSharedBuffer(data, length);
    
    // Set command flag
    SET_BIT(shared_comm.m4_to_m0_flags, command);
    
    // Trigger M0+ interrupt via IPCC
    HAL_IPCC_NotifyCPU(&hipcc, IPCC_CHANNEL_1, IPCC_CHANNEL_DIR_TX);
}
```

## Memory Architecture

### Flash Memory Layout

```
STM32WB55 Flash Memory Map (1MB Total):
┌─────────────────────────────────────────────────────────┐ 0x08000000
│                    Bootloader                           │
│                     (64KB)                              │ 0x08010000
├─────────────────────────────────────────────────────────┤
│                  Main Application                       │
│                    (768KB)                              │
│  ┌─────────────────────────────────────────────────────┐ │ 0x08020000
│  │              Core Application Code                  │ │
│  │  • main.c, vcu.c, mcu.c                           │ │
│  │  • CAN protocol handlers                           │ │
│  │  • Safety systems                                  │ │
│  ├─────────────────────────────────────────────────────┤ │ 0x08080000
│  │              EEPROM Emulation Area                 │ │
│  │                   (32KB)                           │ │
│  │  • Page 0: Active parameter storage                │ │
│  │  • Page 1: Backup parameter storage                │ │
│  │  • Pages 2-7: Wear leveling area                   │ │
│  ├─────────────────────────────────────────────────────┤ │ 0x08088000
│  │              Application Data                       │ │
│  │  • Calibration tables                              │ │
│  │  • Factory defaults                                │ │
│  │  • Diagnostic logs                                 │ │
│  └─────────────────────────────────────────────────────┘ │ 0x080C0000
├─────────────────────────────────────────────────────────┤
│                Network Processor Code                   │
│                     (128KB)                             │ 0x080E0000
├─────────────────────────────────────────────────────────┤
│                     Reserved                            │
│                     (128KB)                             │
└─────────────────────────────────────────────────────────┘ 0x08100000
```

### SRAM Memory Organization

```
STM32WB55 SRAM Layout (256KB Total):
┌─────────────────────────────────────────────────────────┐ 0x20000000
│                     SRAM1 (192KB)                      │
│  ┌─────────────────────────────────────────────────────┐ │
│  │                 Application Stack                   │ │ 0x20000000
│  │                     (8KB)                           │ │ 0x20002000
│  ├─────────────────────────────────────────────────────┤ │
│  │                Application Heap                     │ │ 0x20002000
│  │                    (16KB)                           │ │ 0x20006000
│  ├─────────────────────────────────────────────────────┤ │
│  │              Global Variables & BSS                 │ │ 0x20006000
│  │  • Pack data structures                             │ │
│  │  • Module configuration arrays                      │ │
│  │  • Communication buffers                            │ │ 0x20010000
│  ├─────────────────────────────────────────────────────┤ │
│  │               Working Memory                        │ │ 0x20010000
│  │  • CAN message buffers                              │ │
│  │  • Temporary calculations                           │ │
│  │  • Function local variables                         │ │
│  └─────────────────────────────────────────────────────┘ │ 0x20030000
├─────────────────────────────────────────────────────────┤
│              SRAM2a (32KB) - Shared                    │ 0x20030000
│  • Inter-core communication                            │
│  • Bluetooth LE stack (if used)                        │ 0x20038000
├─────────────────────────────────────────────────────────┤
│              SRAM2b (32KB) - Secure                    │ 0x20038000
│  • Security keys and certificates                      │
│  • Protected configuration data                        │
└─────────────────────────────────────────────────────────┘ 0x20040000
```

### EEPROM Emulation Memory Management

```c
// EEPROM page structure
#define EEPROM_PAGE_SIZE        0x1000      // 4KB per page
#define EEPROM_PAGE_COUNT       8           // 8 pages total (32KB)
#define EEPROM_START_ADDRESS    0x08080000  // Start of EEPROM area

typedef enum {
    PAGE_STATUS_ERASED = 0xFFFFFFFF,    // Page is erased and ready
    PAGE_STATUS_RECEIVE = 0xAAAAAAAA,   // Page is receiving data
    PAGE_STATUS_ACTIVE = 0x55555555,    // Page is active for read/write
    PAGE_STATUS_VALID = 0x00000000      // Page contains valid data
} PageStatus;

typedef struct {
    PageStatus status;          // Page status marker
    uint32_t sequence;          // Page sequence number
    uint32_t checksum;          // Page integrity checksum
    uint32_t reserved;          // Reserved for future use
} PageHeader;

// Variable storage format within page
typedef struct {
    uint16_t virtualAddress;    // Parameter ID
    uint16_t data;             // Parameter value
    uint32_t timestamp;        // Write timestamp
} VariableRecord;
```

## Software Architecture

### Layered Software Design

```
┌─────────────────────────────────────────────────────────────────┐
│                     Application Layer                          │
│  ┌─────────────┬─────────────┬─────────────┬─────────────────┐  │
│  │ Pack State  │ Module      │ Safety      │ Diagnostic      │  │
│  │ Controller  │ Manager     │ Monitor     │ Service         │  │
│  │             │             │             │                 │  │
│  │ • State     │ • Module    │ • Fault     │ • Health        │  │
│  │   machine   │   discovery │   detection │   monitoring    │  │
│  │ • Precharge │ • Data      │ • Protection│ • Performance   │  │
│  │   control   │   collection│   actions   │   metrics       │  │
│  │ • Contactor │ • Balance   │ • Recovery  │ • Log           │  │
│  │   management│   control   │   procedures│   management    │  │
│  └─────────────┴─────────────┴─────────────┴─────────────────┘  │
└─────────────────┬───────────────────────────────────────────────┘
                  │ Service APIs
┌─────────────────┼───────────────────────────────────────────────┐
│                 Service Layer                                   │
│  ┌─────────────┴─────────────┬─────────────────────────────────┐  │
│  │ Communication Services    │ Storage Services                │  │
│  │                          │                                 │  │
│  │ • VCU Protocol Handler    │ • EEPROM Parameter Manager     │  │
│  │ • Module Protocol Handler │ • Configuration Service        │  │
│  │ • Message Router          │ • Backup/Restore Service       │  │
│  │ • Error Recovery          │ • Factory Default Service      │  │
│  └───────────────────────────┴─────────────────────────────────┘  │
└─────────────────┬───────────────────────────────────────────────┘
                  │ System APIs
┌─────────────────┼───────────────────────────────────────────────┐
│                System Services Layer                            │
│  ┌─────────────┴─────────────┬─────────────────────────────────┐  │
│  │ Real-Time Services        │ System Management               │  │
│  │                          │                                 │  │
│  │ • Timer Management        │ • Watchdog Service              │  │
│  │ • Event Scheduler         │ • Power Management              │  │
│  │ • Interrupt Handlers      │ • Clock Management              │  │
│  │ • Critical Sections       │ • Reset/Recovery                │  │
│  └───────────────────────────┴─────────────────────────────────┘  │
└─────────────────┬───────────────────────────────────────────────┘
                  │ HAL APIs
┌─────────────────┼───────────────────────────────────────────────┐
│            Hardware Abstraction Layer                          │
│  ┌─────────────┴─────────────┬─────────────────────────────────┐  │
│  │ Communication HAL         │ Storage HAL                     │  │
│  │                          │                                 │  │
│  │ • CAN Driver              │ • Flash Driver                  │  │
│  │ • SPI Driver              │ • EEPROM Emulation Driver       │  │
│  │ • UART Driver             │ • DMA Controller                │  │
│  │ • GPIO Driver             │ • Memory Protection             │  │
│  └───────────────────────────┴─────────────────────────────────┘  │
└─────────────────┬───────────────────────────────────────────────┘
                  │ Register Access
┌─────────────────┼───────────────────────────────────────────────┐
│                    Hardware Layer                              │
│  STM32WB55 Peripherals, External ICs, Power Management         │
└─────────────────────────────────────────────────────────────────┘
```

### Module Interface Design

```c
// Module interface structure
typedef struct {
    const char* name;           // Module name
    uint32_t version;          // Module version
    bool (*init)(void);        // Initialization function
    void (*main)(void);        // Main execution function
    void (*shutdown)(void);    // Shutdown function
    uint32_t (*getStatus)(void); // Status query function
} ModuleInterface;

// Application modules registry
static const ModuleInterface appModules[] = {
    {
        .name = "VCU_Interface",
        .version = 0x00010001,
        .init = vcuInit,
        .main = vcuMain,
        .shutdown = vcuShutdown,
        .getStatus = vcuGetStatus
    },
    {
        .name = "Module_Manager", 
        .version = 0x00010001,
        .init = moduleInit,
        .main = moduleMain,
        .shutdown = moduleShutdown,
        .getStatus = moduleGetStatus
    },
    {
        .name = "Safety_Monitor",
        .version = 0x00010001,
        .init = safetyInit,
        .main = safetyMain,
        .shutdown = safetyShutdown,
        .getStatus = safetyGetStatus
    }
};
```

## Communication Architecture

### Multi-Protocol Communication Stack

```
┌─────────────────────────────────────────────────────────────────┐
│                    External Systems                            │
│  ┌─────────────┬─────────────┬─────────────┬─────────────────┐  │
│  │     VCU     │  Module 1   │  Module 2   │   Module N      │  │
│  │  (Vehicle   │ (Battery    │ (Battery    │   (Battery      │  │
│  │   Control   │  Module)    │  Module)    │    Module)      │  │
│  │    Unit)    │             │             │                 │  │
│  └─────────────┴─────────────┴─────────────┴─────────────────┘  │
└─────────────────┬───────────────────────────────────────────────┘
                  │ CAN Bus Networks
┌─────────────────┼───────────────────────────────────────────────┐
│              Physical Interface Layer                          │
│  ┌─────────────┴─────────────┬─────────────────────────────────┐  │
│  │ VCU CAN Interface         │ Module CAN Interface            │  │
│  │                          │                                 │  │
│  │ • STM32WB55 CAN-FD        │ • MCP2517FD SPI-CAN             │  │
│  │ • 500kbps/1Mbps           │ • 250kbps/500kbps               │  │
│  │ • 11-bit standard IDs     │ • 11-bit standard IDs           │  │
│  │ • Hardware filtering      │ • Software filtering            │  │
│  └───────────────────────────┴─────────────────────────────────┘  │
└─────────────────┬───────────────────────────────────────────────┘
                  │ Hardware Drivers
┌─────────────────┼───────────────────────────────────────────────┐
│                Protocol Layer                                   │
│  ┌─────────────┴─────────────┬─────────────────────────────────┐  │
│  │ ModBatt VCU Protocol      │ ModBatt Module Protocol         │  │
│  │                          │                                 │  │
│  │ • Command/Response        │ • Broadcast/Poll                │  │
│  │ • Parameter Read/Write    │ • Status Reporting              │  │
│  │ • State Control           │ • Data Collection               │  │
│  │ • Fault Reporting         │ • Configuration Sync            │  │
│  └───────────────────────────┴─────────────────────────────────┘  │
└─────────────────┬───────────────────────────────────────────────┘
                  │ Message APIs
┌─────────────────┼───────────────────────────────────────────────┐
│               Application Layer                                 │
│  Pack Controller Business Logic                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Message Flow Architecture

```c
// Message processing pipeline
typedef enum {
    MSG_DIR_RX,     // Received message
    MSG_DIR_TX      // Transmitted message
} MessageDirection;

typedef struct {
    uint32_t id;                // CAN message ID
    uint8_t data[8];           // Message data
    uint8_t length;            // Data length
    uint32_t timestamp;        // Message timestamp
    MessageDirection direction; // Message direction
} CANMessage;

// Message handler function type
typedef bool (*MessageHandler)(const CANMessage* msg);

// Message routing table
typedef struct {
    uint32_t messageId;         // CAN ID to handle
    uint32_t maskId;           // ID mask for filtering
    MessageHandler handler;     // Handler function
    uint32_t priority;         // Processing priority
} MessageRoute;

// Message routing configuration
static const MessageRoute messageRoutes[] = {
    // VCU interface messages
    {0x400, 0x7F0, handleVCUCommand, 1},       // VCU commands
    {0x401, 0x7FF, handleVCUTime, 2},          // Time sync
    {0x402, 0x7FF, handleVCUReadEEPROM, 3},    // Parameter read
    {0x403, 0x7FF, handleVCUWriteEEPROM, 3},   // Parameter write
    
    // Module interface messages
    {0x500, 0x700, handleModuleStatus, 4},     // Module status
    {0x510, 0x7F0, handleModuleData, 5},       // Module data
    {0x520, 0x7F0, handleModuleFault, 2},      // Module faults
};
```

## Data Flow

### Real-Time Data Processing Pipeline

```
Data Sources → Collection → Processing → Storage → Transmission
     ↓              ↓           ↓          ↓           ↓
┌──────────┐ ┌─────────────┐ ┌────────┐ ┌─────────┐ ┌─────────┐
│ Hardware │ │ Interrupt   │ │ Filter │ │ EEPROM  │ │ CAN TX  │
│ Sensors  │→│ Handlers    │→│ & Calc │→│ Updates │→│ Queue   │
│          │ │             │ │        │ │         │ │         │
│ • ADC    │ │ • CAN RX    │ │ • Range│ │ • Param │ │ • VCU   │
│ • GPIO   │ │ • Timer     │ │   check│ │   store │ │ • Module│
│ • Timers │ │ • UART      │ │ • Unit │ │ • Config│ │ • Diag  │
│ • CAN RX │ │ • SPI       │ │   conv │ │   sync  │ │         │
└──────────┘ └─────────────┘ └────────┘ └─────────┘ └─────────┘
     ↑                                                    ↓
     └─────────────── Feedback Control ←──────────────────┘
```

### Data Structure Hierarchy

```c
// System-wide data organization
typedef struct {
    // System identification
    uint32_t systemId;
    uint32_t firmwareVersion;
    uint32_t hardwareVersion;
    
    // Pack-level data
    PackData pack;
    
    // Module data array
    ModuleData modules[MAX_MODULES];
    uint8_t activeModuleCount;
    
    // Configuration data
    SystemConfig config;
    
    // Status and diagnostics
    SystemStatus status;
    FaultData faults;
    
    // Communication state
    CommunicationState comm;
    
    // Timing and synchronization
    uint32_t systemTime;
    uint32_t lastUpdateTime;
    
} SystemDataStructure;

// Data access control
typedef enum {
    ACCESS_READ_ONLY,
    ACCESS_READ_WRITE,
    ACCESS_WRITE_PROTECTED
} DataAccess;

typedef struct {
    void* dataPtr;              // Pointer to data
    uint32_t dataSize;          // Size of data
    DataAccess accessLevel;     // Access permissions
    uint32_t lastUpdateTime;    // Last modification time
    uint32_t crc;              // Data integrity check
} DataDescriptor;
```

## State Management

### Pack State Machine

```c
// Pack operating states
typedef enum {
    PACK_STATE_OFF = 0,         // Pack disconnected, no power
    PACK_STATE_STANDBY = 1,     // Pack ready, contactors open
    PACK_STATE_PRECHARGE = 2,   // Contactors closing, voltage equalizing
    PACK_STATE_ON = 3,          // Pack operational, full power
    PACK_STATE_FAULT = 4,       // Fault condition, protection active
    PACK_STATE_EMERGENCY = 5    // Emergency shutdown
} PackState;

// State transition validation
typedef struct {
    PackState fromState;        // Source state
    PackState toState;          // Target state
    bool (*validator)(void);    // Validation function
    void (*action)(void);       // Transition action
    uint32_t timeout;          // Maximum transition time
} StateTransition;

// State machine configuration
static const StateTransition validTransitions[] = {
    // From OFF
    {PACK_STATE_OFF, PACK_STATE_STANDBY, validateOffToStandby, actionOffToStandby, 1000},
    
    // From STANDBY
    {PACK_STATE_STANDBY, PACK_STATE_OFF, validateStandbyToOff, actionStandbyToOff, 500},
    {PACK_STATE_STANDBY, PACK_STATE_PRECHARGE, validateStandbyToPrecharge, actionStandbyToPrecharge, 2000},
    
    // From PRECHARGE
    {PACK_STATE_PRECHARGE, PACK_STATE_STANDBY, validatePrechargeToStandby, actionPrechargeToStandby, 500},
    {PACK_STATE_PRECHARGE, PACK_STATE_ON, validatePrechargeToOn, actionPrechargeToOn, 1000},
    
    // From ON
    {PACK_STATE_ON, PACK_STATE_STANDBY, validateOnToStandby, actionOnToStandby, 500},
    
    // Emergency transitions (from any state)
    {PACK_STATE_OFF, PACK_STATE_EMERGENCY, NULL, actionEmergencyShutdown, 100},
    {PACK_STATE_STANDBY, PACK_STATE_EMERGENCY, NULL, actionEmergencyShutdown, 100},
    {PACK_STATE_PRECHARGE, PACK_STATE_EMERGENCY, NULL, actionEmergencyShutdown, 100},
    {PACK_STATE_ON, PACK_STATE_EMERGENCY, NULL, actionEmergencyShutdown, 100},
};

// State machine execution
bool ExecuteStateTransition(PackState targetState) {
    PackState currentState = GetCurrentPackState();
    
    // Find valid transition
    const StateTransition* transition = FindTransition(currentState, targetState);
    if (transition == NULL) {
        return false;  // Invalid transition
    }
    
    // Validate transition conditions
    if (transition->validator != NULL && !transition->validator()) {
        return false;  // Validation failed
    }
    
    // Execute transition action
    if (transition->action != NULL) {
        transition->action();
    }
    
    // Update state
    SetPackState(targetState);
    
    // Start timeout timer
    StartTransitionTimeout(transition->timeout);
    
    return true;
}
```

## Error Handling Architecture

### Multi-Level Error Detection

```c
// Error severity levels
typedef enum {
    ERROR_SEVERITY_INFO = 0,        // Informational message
    ERROR_SEVERITY_WARNING = 1,     // Warning condition
    ERROR_SEVERITY_ERROR = 2,       // Error condition
    ERROR_SEVERITY_CRITICAL = 3,    // Critical error
    ERROR_SEVERITY_FATAL = 4        // Fatal system error
} ErrorSeverity;

// Error categories
typedef enum {
    ERROR_CAT_HARDWARE = 0x01,      // Hardware failures
    ERROR_CAT_COMMUNICATION = 0x02,  // Communication errors
    ERROR_CAT_PARAMETER = 0x04,     // Parameter/configuration errors
    ERROR_CAT_SAFETY = 0x08,        // Safety system errors
    ERROR_CAT_SYSTEM = 0x10         // System errors
} ErrorCategory;

// Error handling structure
typedef struct {
    uint16_t errorCode;             // Unique error code
    ErrorSeverity severity;         // Error severity level
    ErrorCategory category;         // Error category
    uint32_t timestamp;            // Error occurrence time
    uint32_t context;              // Additional context data
    const char* description;        // Error description
    void (*handler)(uint32_t);     // Error handler function
} ErrorDescriptor;

// Error recovery strategies
typedef enum {
    RECOVERY_IGNORE,                // Ignore error and continue
    RECOVERY_RETRY,                 // Retry failed operation
    RECOVERY_RESET_SUBSYSTEM,       // Reset affected subsystem
    RECOVERY_SAFE_MODE,             // Enter safe operation mode
    RECOVERY_EMERGENCY_SHUTDOWN     // Emergency system shutdown
} RecoveryStrategy;

// Error handling function
void HandleSystemError(uint16_t errorCode, uint32_t context) {
    const ErrorDescriptor* error = FindErrorDescriptor(errorCode);
    if (error == NULL) {
        // Unknown error - treat as critical
        error = &unknownErrorDescriptor;
    }
    
    // Log error
    LogError(error, context);
    
    // Determine recovery strategy
    RecoveryStrategy strategy = DetermineRecoveryStrategy(error);
    
    // Execute recovery action
    switch (strategy) {
        case RECOVERY_IGNORE:
            // No action required
            break;
            
        case RECOVERY_RETRY:
            // Retry the failed operation
            ScheduleOperationRetry(context);
            break;
            
        case RECOVERY_RESET_SUBSYSTEM:
            // Reset the affected subsystem
            ResetSubsystem(error->category);
            break;
            
        case RECOVERY_SAFE_MODE:
            // Enter safe operation mode
            EnterSafeMode();
            break;
            
        case RECOVERY_EMERGENCY_SHUTDOWN:
            // Emergency shutdown
            ExecuteEmergencyShutdown();
            break;
    }
    
    // Notify external systems if required
    if (error->severity >= ERROR_SEVERITY_ERROR) {
        NotifyVCUOfError(errorCode, context);
    }
}
```

This comprehensive architecture provides the foundation for a robust, scalable, and safety-critical battery management system with advanced parameter storage and communication capabilities.