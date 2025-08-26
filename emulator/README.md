# Pack Controller Emulator

## Overview
Windows GUI application that emulates pack controller functionality for direct module management via CAN bus. This allows a Windows PC to act as the pack controller without requiring the STM32WB55 hardware.

## Purpose
- **Development**: Test modules without full battery pack hardware
- **Diagnostics**: Advanced debugging and analysis capabilities  
- **Training**: Safe environment for learning battery management
- **Simulation**: Test edge cases and failure scenarios

## Architecture

### System Context
```
[Battery Modules] <--CAN--> [Windows PC + PCAN] <--GUI--> [User]
                                    |
                            [Pack Emulator App]
```

### Key Differences from Hardware Pack Controller
| Feature | Hardware Pack Controller | Windows Emulator |
|---------|-------------------------|------------------|
| Platform | STM32WB55 Embedded | Windows PC |
| CAN Interface | Built-in CAN peripheral | PCAN-USB adapter |
| VCU Communication | Yes (CAN) | No (acts standalone) |
| Web4 Integration | Yes | Module-level only |
| EEPROM Storage | Flash emulation | File system |
| Real-time | Hard real-time | Soft real-time |

## Features

### Phase 1: Core Module Management
- [ ] Module discovery and registration
- [ ] Module status monitoring
- [ ] Basic module control (on/off/standby)
- [ ] Cell voltage and temperature display
- [ ] SOC/SOH tracking

### Phase 2: Advanced Control
- [ ] Module balancing coordination
- [ ] Charge/discharge control
- [ ] Fault detection and isolation
- [ ] Data logging to CSV/database
- [ ] Real-time graphing

### Phase 3: Web4 Integration
- [ ] Module-level Web4 key distribution
- [ ] Encrypted CAN communication
- [ ] Module authentication
- [ ] Trust scoring

## Development Environment

### Requirements
- **IDE**: C++ Builder Community Edition (same as modbatt-CAN tool)
- **CAN Interface**: PCAN-USB adapter
- **Libraries**: 
  - PCAN-Basic API (already in modbatt-CAN)
  - Web4 client libraries (from modbatt-CAN)
  
### Build Instructions
1. Open `PackEmulator.cbproj` in C++ Builder
2. Configure PCAN-Basic library paths
3. Build for Win32 or Win64 platform
4. Run with PCAN-USB adapter connected

## File Structure
```
emulator/
├── README.md               # This file
├── docs/
│   ├── design.md          # Detailed design documentation
│   ├── protocol.md        # Module communication protocol
│   └── testing.md         # Test procedures
├── include/
│   ├── module_manager.h   # Module management logic
│   ├── can_interface.h    # PCAN wrapper
│   ├── pack_emulation.h   # Pack controller logic
│   └── ui_main.h          # GUI interface
├── src/
│   ├── main.cpp           # Application entry point
│   ├── module_manager.cpp # Module management
│   ├── can_interface.cpp  # CAN communication
│   ├── pack_emulation.cpp # Pack logic from embedded
│   └── ui_main.cpp        # GUI implementation
├── build/                  # Build outputs
└── PackEmulator.cbproj     # C++ Builder project

```

## Module Communication Protocol

The emulator implements the same CAN protocol as the hardware pack controller for module communication:

### Key Messages (Module → Pack)
- **0x100-0x11F**: Module status broadcasts
- **0x120-0x13F**: Cell voltage reports  
- **0x140-0x15F**: Temperature data
- **0x160-0x17F**: Fault reports

### Control Messages (Pack → Module)
- **0x200-0x21F**: State commands
- **0x220-0x23F**: Balancing commands
- **0x240-0x25F**: Configuration updates
- **0x260-0x27F**: Web4 key distribution

## Integration with Existing Code

### Reusable Components from Pack-Controller-EEPROM
- `can_id_*.h` - CAN ID definitions
- `battery.h` - Battery data structures
- Module management algorithms
- State machine logic

### Reusable Components from modbatt-CAN
- PCAN-Basic interface code
- Web4 client integration
- UI framework and styling
- Debug/logging infrastructure

## Testing Strategy

### Unit Testing
- Module registration/deregistration
- State transitions
- Fault handling
- Data validation

### Integration Testing  
- Physical module communication
- Multi-module coordination
- Timing and synchronization
- Error recovery

### System Testing
- Full pack simulation
- Performance under load
- Long-duration stability
- Edge case scenarios

## Safety Considerations

**WARNING**: This emulator controls real battery modules which can be dangerous if mishandled.

### Safety Features
- Voltage limits enforcement
- Current limits enforcement
- Temperature monitoring
- Emergency shutdown capability
- Watchdog timers

### Required Safeguards
- Always use current-limited power supplies
- Never exceed module specifications
- Monitor temperatures continuously
- Have emergency stop accessible
- Log all commands for audit

## Development Roadmap

### Milestone 1: Basic Communication (Week 1)
- PCAN interface working
- Module discovery functional
- Status display updating

### Milestone 2: Module Control (Week 2)
- State commands working
- Multiple modules supported
- Basic UI complete

### Milestone 3: Full Emulation (Week 3-4)
- All pack controller functions
- Data logging implemented
- Testing complete

### Future Enhancements
- Hardware-in-loop testing
- Automated test sequences
- Machine learning for fault prediction
- Integration with digital twin

## Contributing

This emulator is part of the larger Web4 battery management ecosystem. When contributing:

1. Follow existing code style from modbatt-CAN
2. Maintain compatibility with hardware pack controller protocols
3. Document all protocol changes
4. Test with real modules before committing
5. Update both code and documentation

## License

Copyright (C) 2025 Modular Battery Technologies, Inc.
US Patents 11,380,942; 11,469,470; 11,575,270; others pending.

This emulator is proprietary software for development and testing purposes.