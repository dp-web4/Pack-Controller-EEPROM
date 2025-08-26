# Pack Controller Emulator - Implementation Plan

## Overview
This document outlines the step-by-step implementation plan for the Windows-based pack controller emulator that will manage battery modules directly via CAN without requiring the embedded pack controller hardware.

## Architecture Summary

### System Layers
1. **GUI Layer** (C++ Builder VCL)
   - User interface for monitoring and control
   - Real-time data visualization
   - Configuration management

2. **Business Logic Layer** (C++)
   - Module management (registration, state control)
   - Pack calculations (voltage, current, SOC)
   - Fault detection and handling

3. **Communication Layer** (PCAN-Basic)
   - CAN message handling
   - Protocol implementation
   - Timing and synchronization

4. **Data Layer**
   - Module data structures
   - Configuration storage (INI/XML files)
   - Logging (CSV/database)

## Implementation Phases

### Phase 1: Foundation (Week 1)

#### Day 1-2: Project Setup
- [x] Create directory structure
- [x] Set up C++ Builder project
- [x] Define core header files
- [ ] Copy PCAN-Basic files from modbatt-CAN
- [ ] Copy relevant headers from Pack-Controller-EEPROM
- [ ] Configure build environment

#### Day 3-4: Basic CAN Communication
- [ ] Implement CANInterface class
- [ ] Test PCAN connection
- [ ] Implement message send/receive
- [ ] Add logging capabilities
- [ ] Verify with CAN analyzer

#### Day 5: Module Management Core
- [ ] Implement ModuleManager class
- [ ] Port module data structures
- [ ] Add registration logic
- [ ] Implement state machine
- [ ] Unit test core functions

### Phase 2: GUI Development (Week 2)

#### Day 6-7: Main Window Layout
- [ ] Design form in C++ Builder
- [ ] Create module list view
- [ ] Add connection controls
- [ ] Implement status displays
- [ ] Add menu structure

#### Day 8-9: Module Control Interface
- [ ] State control panel
- [ ] Module details tabs
- [ ] Cell voltage grid
- [ ] Temperature displays
- [ ] Real-time charts

#### Day 10: Integration
- [ ] Connect GUI to ModuleManager
- [ ] Wire up CAN callbacks
- [ ] Test with simulator
- [ ] Debug message flow

### Phase 3: Protocol Implementation (Week 3)

#### Day 11-12: Module Discovery
- [ ] Implement announcement request
- [ ] Process module responses
- [ ] Handle registration sequence
- [ ] Test with real modules

#### Day 13-14: State Management
- [ ] Port state machine from embedded
- [ ] Implement state commands
- [ ] Add transition validation
- [ ] Handle fault conditions

#### Day 15: Data Processing
- [ ] Cell voltage parsing
- [ ] Temperature processing
- [ ] SOC/SOH calculations
- [ ] Pack-level computations

### Phase 4: Advanced Features (Week 4)

#### Day 16-17: Web4 Integration
- [ ] Port web4_handler logic
- [ ] Add key distribution UI
- [ ] Implement chunked transfer
- [ ] Test encryption readiness

#### Day 18-19: Data Management
- [ ] Configuration save/load
- [ ] Data export (CSV)
- [ ] Logging system
- [ ] Performance metrics

#### Day 20: Testing & Polish
- [ ] System integration test
- [ ] Performance optimization
- [ ] UI polish
- [ ] Documentation

## Code Reuse Strategy

### From Pack-Controller-EEPROM (Embedded)
```
Core/Inc/
  ├── battery.h         → Copy data structures
  ├── can_id_module.h   → Copy CAN ID definitions
  ├── can_frm_mod.h     → Copy frame formats
  └── mcu.c            → Reference for logic

Core/Src/
  └── mcu.c            → Port module handling logic
```

### From modbatt-CAN (Windows Tool)
```
WEB4 Modbatt Configuration Utility/
  ├── PCANBasic.h          → Copy as-is
  ├── PCANBasicClass.cpp   → Copy and adapt
  ├── Unit1.cpp            → Reference for UI patterns
  └── web4.cpp             → Reference for Web4 client
```

## Key Differences from Embedded Implementation

| Aspect | Embedded Pack Controller | Windows Emulator |
|--------|-------------------------|------------------|
| Platform | STM32WB55 | Windows x86/x64 |
| Real-time | Hard real-time | Soft real-time |
| CAN | Hardware peripheral | USB adapter |
| Storage | Flash EEPROM | File system |
| UI | None (headless) | Rich GUI |
| Debugging | Limited (UART) | Full IDE |
| VCU Interface | CAN | None |
| Resources | Constrained | Abundant |

## Testing Strategy

### Unit Tests
- Module registration/deregistration
- State machine transitions
- Data parsing functions
- Fault detection logic

### Integration Tests
- CAN communication with modules
- Multi-module coordination
- Timing requirements
- Error recovery

### System Tests
- Full pack simulation
- Long-duration stability
- Performance under load
- Fault injection

### Acceptance Criteria
- [ ] Successfully discovers modules
- [ ] Maintains communication with 8+ modules
- [ ] State transitions work correctly
- [ ] Cell data displays accurately
- [ ] No memory leaks over 24hr run
- [ ] Sub-100ms response time
- [ ] Handles module disconnection gracefully

## Risk Mitigation

### Technical Risks
1. **CAN Timing**: Windows is not real-time
   - Mitigation: Use high-priority threads, accept some jitter

2. **PCAN Driver Issues**: Compatibility problems
   - Mitigation: Test with known-good modbatt-CAN code

3. **Module Compatibility**: Protocol differences
   - Mitigation: Implement protocol versioning

### Safety Risks
1. **Uncontrolled Module States**: Could damage batteries
   - Mitigation: Implement safety limits, emergency stop

2. **Communication Loss**: Modules enter unknown state
   - Mitigation: Watchdog timers, safe defaults

## Development Environment

### Required Tools
- **IDE**: RAD Studio / C++ Builder Community Edition
- **CAN Hardware**: PCAN-USB adapter
- **Test Equipment**: 
  - Oscilloscope for CAN debugging
  - Power supply for modules
  - Multimeter for verification

### Libraries and Dependencies
- PCAN-Basic API (included)
- VCL Framework (C++ Builder)
- Standard C++ libraries
- Optional: SQLite for logging

## Success Metrics

### Functional Requirements
- Manages up to 32 modules
- Updates at 10Hz minimum
- Logs all CAN traffic
- Exports data to CSV

### Performance Requirements
- CPU usage < 25%
- Memory usage < 500MB
- CAN bus utilization < 50%
- GUI remains responsive

### Quality Requirements  
- No crashes in 24hr operation
- Graceful error handling
- Comprehensive logging
- Intuitive UI

## Next Steps

1. **Immediate** (Today):
   - Copy PCAN files from modbatt-CAN
   - Set up development environment
   - Create main.cpp entry point

2. **Tomorrow**:
   - Implement basic CAN interface
   - Test PCAN connectivity
   - Create simple test program

3. **This Week**:
   - Complete Phase 1 foundation
   - Have basic module discovery working

## Questions to Resolve

1. Which version of C++ Builder to target?
2. Should we support both Win32 and Win64?
3. Do we need database logging or is CSV sufficient?
4. Should the emulator support firmware updates to modules?
5. How much of the Web4 functionality should be included?

## Contact

For questions about the embedded pack controller protocol:
- See `Core/Inc/*.h` headers in parent directory

For questions about Windows CAN implementation:
- Reference modbatt-CAN tool in `../../modbatt-CAN/`

For Web4 integration:
- See `web4_handler.c/h` in parent directory