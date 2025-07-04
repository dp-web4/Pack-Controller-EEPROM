# Pack Controller EEPROM Firmware Overview

## Table of Contents

1. [System Overview](#system-overview)
2. [Architecture](#architecture)
3. [Key Features](#key-features)
4. [Technology Stack](#technology-stack)
5. [Hardware Platform](#hardware-platform)
6. [Firmware Components](#firmware-components)
7. [Development Status](#development-status)
8. [Quick Start](#quick-start)

## System Overview

The **Pack Controller EEPROM** firmware is a comprehensive battery management system (BMS) running on the STM32WB55 dual-core microcontroller. It serves as the central intelligence for modular battery packs, providing advanced parameter storage, safety management, and communication capabilities for vehicle-integrated energy storage systems.

### Primary Functions

**Central Pack Management**
- Coordinates overall battery pack operation and state control
- Manages pack-level safety systems and protection mechanisms  
- Provides unified interface to Vehicle Control Unit (VCU)
- Implements hierarchical control over individual battery modules

**Advanced Parameter Storage**
- Flash-based EEPROM emulation with wear leveling
- Comprehensive configuration parameter management
- Factory default restoration and backup capabilities
- Secure parameter validation and integrity checking

**Multi-Protocol Communication**
- Dual CAN interface for VCU and module communication
- External MCP2517FD CAN-FD controller integration
- Custom ModBatt communication protocol implementation
- Real-time telemetry and diagnostic data exchange

**Safety-Critical Operation**
- Multi-layer fault detection and protection systems
- State machine-based safe operation control
- Watchdog monitoring and system health checks
- Emergency shutdown and recovery procedures

## Architecture

### System Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     Vehicle Control Unit                       │
│                        (VCU)                                   │
└────────────────────┬────────────────────────────────────────────┘
                     │ CAN Bus (500kbps/1Mbps)
┌────────────────────┼────────────────────────────────────────────┐
│              Pack Controller STM32WB55                         │
│  ┌─────────────────┴─────────────────────────────────────────┐  │
│  │              Application Processor                       │  │
│  │              (ARM Cortex-M4 @ 64MHz)                    │  │
│  │  ┌─────────────┬─────────────┬─────────────────────────┐  │  │
│  │  │ VCU Interface│ EEPROM Mgr  │ Safety Controller       │  │  │
│  │  ├─────────────┼─────────────┼─────────────────────────┤  │  │
│  │  │ Module Mgr  │ CAN Protocol│ State Machine           │  │  │
│  │  └─────────────┴─────────────┴─────────────────────────┘  │  │
│  └─────────────────┬─────────────────────────────────────────┘  │
│  ┌─────────────────┴─────────────────────────────────────────┐  │
│  │            Network Processor (Optional)                  │  │
│  │            (ARM Cortex-M0+ @ 32MHz)                     │  │
│  │            • Bluetooth LE                               │  │
│  │            • Wireless diagnostics                       │  │
│  └─────────────────┬─────────────────────────────────────────┘  │
└────────────────────┼────────────────────────────────────────────┘
                     │ Module CAN Network
    ┌────────────────┼────────────────┬───────────────────────────┐
    │                │                │                           │
┌───▼────────┐ ┌─────▼──────┐ ┌──────▼──────┐ ┌─────────────────▼──┐
│  Module 1  │ │  Module 2  │ │  Module 3   │ │     Module N       │
│  Controller│ │  Controller│ │  Controller │ │     Controller     │
│  (1-32     │ │  (1-32     │ │  (1-32      │ │     (1-32          │
│   modules) │ │   modules) │ │   modules)  │ │      modules)      │
└────────────┘ └────────────┘ └─────────────┘ └────────────────────┘
```

### Firmware Architecture Layers

```
┌─────────────────────────────────────────────────────────────────┐
│                     Application Layer                          │
│  ┌─────────────┬─────────────┬─────────────┬─────────────────┐  │
│  │ Pack State  │ Module      │ Safety      │ Diagnostic      │  │
│  │ Management  │ Coordinator │ Monitor     │ Services        │  │
│  └─────────────┴─────────────┴─────────────┴─────────────────┘  │
└─────────────────┬───────────────────────────────────────────────┘
                  │ Application Programming Interface (API)
┌─────────────────┼───────────────────────────────────────────────┐
│                 Middleware Layer                               │
│  ┌─────────────┴─────────────┬─────────────────────────────────┐  │
│  │ Protocol Stack            │ System Services                 │  │
│  │ • CAN message handling    │ • EEPROM parameter management   │  │
│  │ • State machine logic     │ • Timer and event services      │  │
│  │ • Error handling          │ • Watchdog management           │  │
│  └───────────────────────────┴─────────────────────────────────┘  │
└─────────────────┬───────────────────────────────────────────────┘
                  │ Hardware Abstraction Layer (HAL)
┌─────────────────┼───────────────────────────────────────────────┐
│              STM32WB55 Hardware Platform                       │
│  ┌─────────────┴─────────────┬─────────────────────────────────┐  │
│  │ Dual-Core MCU             │ Peripheral Controllers          │  │
│  │ • Cortex-M4 App Processor │ • CAN-FD controller             │  │
│  │ • Cortex-M0+ Network Proc │ • SPI for external CAN          │  │
│  │ • 1MB Flash, 256KB SRAM   │ • UART, GPIO, Timers            │  │
│  │ • Hardware security       │ • RTC with battery backup       │  │
│  └───────────────────────────┴─────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Key Features

### Enhanced EEPROM Capability

**Flash-Based Parameter Storage**
- Emulated EEPROM using STM32 internal flash memory
- Automatic wear leveling to extend memory lifespan
- Data integrity protection with CRC validation
- Atomic write operations with rollback capability

**Comprehensive Parameter Set**
- Pack configuration parameters (voltage, current, temperature limits)
- Module-specific settings and calibration data
- Safety thresholds and protection settings
- Communication timing and protocol parameters
- Factory defaults with restoration capability

**Backup and Recovery**
- Automatic parameter backup before critical updates
- Multiple backup slots with timestamp tracking
- Factory reset capability with default parameter loading
- Configuration validation and error recovery

### Advanced Communication Systems

**Multi-Channel CAN Architecture**
- Integrated STM32WB55 CAN-FD controller for VCU communication
- External MCP2517FD controller for additional module channels
- Configurable baudrates: 125k/250k/500k/1M bps
- Message filtering and prioritization

**Protocol Implementation**
- Custom ModBatt CAN protocol with defined message structures
- Real-time telemetry streaming (100ms pack data, 200ms module data)
- Parameter read/write operations with acknowledgment
- Diagnostic and fault reporting with detailed error codes

**Message Types**
- VCU command and control messages (0x400-0x40F)
- Pack status and telemetry data (0x410-0x43F)
- Module data collection (0x411-0x417)
- Configuration and parameter messages (0x440-0x44F)

### Safety and Protection Systems

**Multi-Layer Protection**
- Hardware-level overvoltage/undervoltage protection
- Current limiting with configurable thresholds
- Temperature monitoring and thermal management
- Isolation resistance monitoring
- Communication timeout detection

**State Machine Control**
- Safe state transitions: Off → Standby → Precharge → On
- Validation of all state change requests
- Emergency shutdown capability
- Fault handling with automatic recovery

**Fault Management**
- Comprehensive fault detection and classification
- Fault logging with timestamp and context information
- Automatic protection actions based on fault severity
- Fault clearing and system recovery procedures

## Technology Stack

### Development Platform

**STM32CubeIDE**
- Eclipse-based integrated development environment
- STM32CubeMX for hardware configuration
- GNU toolchain for ARM Cortex-M processors
- ST-Link debugger integration

**STM32 HAL Library**
- Hardware abstraction layer for peripheral access
- CMSIS (Cortex Microcontroller Software Interface Standard)
- FreeRTOS support (optional)
- USB, CAN, SPI, UART, GPIO drivers

**Build System**
- Auto-generated makefiles from STM32CubeIDE
- GCC compiler with optimization settings
- Linker scripts for memory layout management
- Debug and release build configurations

### Programming Languages and Standards

**C Programming Language**
- C99 standard compliance
- Structured programming with modular design
- Real-time embedded programming patterns
- Safety-critical coding standards (MISRA-C compatible)

**Assembly Language**
- Startup code and low-level initialization
- Interrupt service routines
- Critical timing-sensitive functions
- Memory management operations

## Hardware Platform

### STM32WB55RGV6 Specifications

**Dual-Core Architecture**
- **Application Processor**: ARM Cortex-M4 @ 64MHz with FPU
- **Network Processor**: ARM Cortex-M0+ @ 32MHz (optional Bluetooth LE)
- **Package**: VFQFPN68 (68-pin QFN)
- **Operating Voltage**: 1.71V to 3.6V

**Memory Configuration**
- **Flash Memory**: 1MB total (application + data storage)
- **SRAM1**: 192KB main application memory
- **SRAM2a**: 32KB shared with radio stack
- **SRAM2b**: 32KB secure area
- **EEPROM Emulation**: 32KB dedicated flash area

**Communication Interfaces**
- **CAN-FD Controller**: Integrated with flexible data rate support
- **SPI**: 2 channels for external CAN controllers
- **UART/LPUART**: 2 channels for module communication
- **USB 2.0 FS**: Device interface for programming/debugging
- **Bluetooth LE 5.0**: Wireless diagnostics (optional)

### External Components

**CAN Interface Hardware**
- **MCP2517FD**: External SPI-to-CAN-FD controller
- **CAN Transceivers**: Automotive-grade TJA1043/TJA1463
- **Termination**: Configurable 120Ω termination resistors
- **Protection**: ESD protection and EMI filtering

**Power Management**
- **Vehicle Interface**: 12V automotive power input
- **Regulation**: 3.3V and 5V regulated supplies
- **Protection**: Reverse polarity, overvoltage, undervoltage
- **Backup Power**: Supercapacitor for RTC and critical data

## Firmware Components

### Core Application Modules

#### Main Application (main.c)
```c
// Central control loop and system initialization
int main(void) {
    // Hardware initialization
    SystemInit();
    
    // Application module initialization
    vcuInit();
    mcuInit();
    EE_Init();  // EEPROM emulation
    
    // Main control loop
    while (1) {
        vcuMain();           // VCU communication
        moduleMain();        // Module management
        safetyCheck();       // Safety monitoring
        backgroundTasks();   // Housekeeping
    }
}
```

#### VCU Interface Module (vcu.c)
- Vehicle Control Unit communication protocol implementation
- CAN message processing and response generation
- State change command handling
- Telemetry data transmission to VCU

#### MCU Hardware Abstraction (mcu.c)  
- STM32WB55 peripheral initialization and control
- GPIO management for status LEDs and control signals
- Timer configuration for periodic tasks
- Interrupt service routine implementations

#### EEPROM Manager (eeprom_emul.c)
- Flash-based EEPROM emulation with wear leveling
- Parameter storage and retrieval operations
- Data integrity validation and error recovery
- Factory default restoration capability

### Communication Protocol Stack

#### CAN Protocol Handler
- Message ID recognition and routing
- Data encoding/decoding for ModBatt protocol
- Message prioritization and transmission scheduling
- Error detection and recovery mechanisms

#### External CAN Controller (canfdspi_api.c)
- MCP2517FD SPI interface driver
- CAN-FD message transmission and reception
- Configuration and status monitoring
- Interrupt handling for real-time operation

### Safety and Monitoring Systems

#### Safety Controller
- Continuous monitoring of pack operational parameters
- Fault detection algorithms and threshold checking
- Protection action implementation (current limiting, disconnection)
- Emergency shutdown procedures

#### State Machine
- Pack state management (Off/Standby/Precharge/On)
- State transition validation and control
- Interlock and safety condition checking
- Recovery and fault handling procedures

## Development Status

### Current Implementation

**Core Functionality** ✅ Complete
- STM32WB55 hardware initialization and configuration
- Basic CAN communication with VCU and modules
- EEPROM emulation with parameter storage
- Safety monitoring and protection systems

**EEPROM Management** ✅ Complete
- Flash-based parameter storage implementation
- Wear leveling and data integrity protection
- Factory default parameter loading
- Configuration backup and restore

**Communication Protocol** ✅ Complete
- ModBatt CAN protocol implementation
- VCU interface with command/response handling
- Module communication with data collection
- Message filtering and error handling

**Safety Systems** ✅ Complete
- Multi-layer protection mechanisms
- State machine implementation
- Fault detection and reporting
- Emergency shutdown capability

### Future Enhancements

**Bluetooth LE Integration** 🔄 Planned
- Wireless diagnostic interface using Cortex-M0+ processor
- Remote configuration and monitoring capability
- OTA firmware update support
- Mobile app integration

**Advanced Analytics** 🔄 Planned
- Predictive maintenance algorithms
- Performance optimization features
- Data logging and trend analysis
- Machine learning integration

**Security Enhancements** 🔄 Planned
- Secure boot and firmware verification
- Encrypted communication channels
- Hardware security module utilization
- Certificate-based authentication

## Quick Start

### Prerequisites

**Development Environment**
1. STM32CubeIDE (version 1.8.0 or later)
2. STM32CubeMX (integrated with IDE)
3. ST-Link debugger/programmer
4. PEAK PCAN interface for testing

**Hardware Requirements**
1. STM32WB55 development board or custom hardware
2. CAN transceiver circuits
3. 12V power supply
4. Battery pack or pack simulator

### Build and Flash

```bash
# Clone repository
git clone <repository-url>
cd "Pack Controller EEPROM"

# Open in STM32CubeIDE
# File → Open Projects from File System
# Select the project directory

# Build project
# Project → Build Project (Ctrl+B)

# Flash to target
# Run → Debug (F11) or Run → Run (Ctrl+F11)
```

### Configuration

1. **Hardware Setup**: Configure GPIO pins and peripheral assignments in STM32CubeMX
2. **CAN Configuration**: Set baudrates and message filtering in CAN initialization
3. **EEPROM Parameters**: Load factory defaults or custom parameter set
4. **Safety Limits**: Configure protection thresholds for specific application

### Testing

1. **Hardware Validation**: Verify all peripherals and communication interfaces
2. **Parameter Storage**: Test EEPROM read/write operations
3. **CAN Communication**: Validate message exchange with VCU and modules
4. **Safety Functions**: Test protection mechanisms and fault handling

This firmware provides a robust foundation for advanced battery management systems with comprehensive parameter storage, safety features, and communication capabilities suitable for automotive and energy storage applications.