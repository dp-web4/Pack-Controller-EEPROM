# Pack Controller Emulator

Windows GUI application that emulates the Pack Controller functionality for direct battery module testing without requiring the STM32WB55 hardware.

## Purpose

This emulator allows you to:
- Test battery modules directly via CAN bus using a PCAN-USB adapter
- Replace the embedded pack controller during development and testing
- Manage module registration, state control, and monitoring
- Test Web4 key distribution protocols
- Debug module communication issues

## Hardware Requirements

- Windows PC (Windows 10/11 recommended)
- PCAN-USB adapter (or compatible PEAK CAN interface)
- CAN bus connection to battery modules
- 500 kBit/s CAN baudrate (configurable)

## Software Requirements

- RAD Studio / C++ Builder (for building from source)
- PCAN-Basic drivers installed
- Visual C++ Runtime (for running pre-built executable)

## Building the Application

1. Open `PackEmulator.cbproj` in RAD Studio
2. Ensure PCAN-Basic API is installed (included in lib/ directory)
3. Build configuration:
   - Debug: For development and testing
   - Release: For production use
4. Build the project (F9 or Project → Build)

## Running the Emulator

### Initial Setup

1. Install PCAN-Basic drivers from PEAK System
2. Connect PCAN-USB adapter to PC
3. Connect CAN bus to battery modules
4. Launch PackEmulator.exe

### Connection

1. Select PCAN channel (typically PCAN-USB 1)
2. Select baudrate (500 kBit/s for standard modules)
3. Click "Connect"
4. Status should show "Connected" in green

### Module Discovery

1. Click "Discover" to start module discovery
2. Modules will announce themselves via CAN
3. Discovered modules appear in the left panel list
4. Select a module and click "Register" to register it

### Module Control

1. Select a registered module from the list
2. Use the State Control panel to:
   - Set module state (Off/Standby/Precharge/On)
   - Enable cell balancing
   - Apply states to all modules
3. Monitor module status in real-time

### Module Details

The center panel shows detailed module information:
- **Status Tab**: Voltage, current, temperature, SOC, SOH
- **Cells Tab**: Individual cell voltages and temperatures
- **History Tab**: Communication log
- **Web4 Tab**: Key distribution management

## CAN Protocol

### Module → Pack Controller Messages
- `0x500`: Module announcement (discovery)
- `0x502-0x504`: Module status messages
- `0x505`: Module detail information
- `0x507-0x508`: Cell communication status

### Pack Controller → Module Commands
- `0x510`: Module registration acknowledgment
- `0x514`: State change command
- `0x516`: Time synchronization
- `0x518`: Module deregistration
- `0x51D`: Announce request (discovery trigger)

## Features

### Module Management
- Discovery of unregistered modules
- Registration/deregistration control
- Module state machine control
- Fault detection and reporting

### Data Monitoring
- Real-time voltage/current/temperature
- Individual cell monitoring
- Pack-level calculations
- Communication statistics

### Web4 Integration
- Device key distribution
- LCT key management
- Component ID storage
- Encryption enable/disable

### Logging
- CAN message logging to file
- History display in GUI
- Export data to CSV

## Troubleshooting

### Connection Issues
- Verify PCAN-Basic drivers installed
- Check CAN termination (120Ω)
- Confirm baudrate matches modules
- Check Windows Device Manager for adapter

### Module Not Responding
- Check CAN wiring and connections
- Verify module power supply
- Send announce request manually
- Check timeout settings (default 5s)

### Build Errors
- Ensure all dependencies in include/ and lib/
- Verify PCANBasic.h and library present
- Check project include paths
- Confirm C++ Builder version compatibility

## Testing Checklist

Before hardware testing:
1. ✓ CAN adapter recognized by Windows
2. ✓ Correct baudrate selected (500k typical)
3. ✓ CAN bus properly terminated
4. ✓ Module power supplies connected
5. ✓ Safety equipment ready
6. ✓ Emergency stop accessible

## Safety Notes

⚠️ **WARNING**: This emulator controls real battery modules
- Always follow proper safety procedures
- Never leave modules unattended while connected
- Implement emergency shutdown procedures
- Monitor temperature and voltage limits
- Use appropriate safety equipment

## File Structure

```
emulator/
├── include/           # Header files
│   ├── pack_emulator_main.h
│   ├── module_manager.h
│   ├── can_interface.h
│   ├── can_id_module.h
│   ├── can_id_bms_vcu.h
│   └── battery.h
├── src/              # Source files
│   ├── main.cpp
│   ├── pack_emulator_main.cpp
│   ├── module_manager.cpp
│   └── can_interface.cpp
├── lib/              # Libraries
│   ├── PCANBasic.h
│   └── PCANBasic.lib
├── PackEmulator.cbproj  # Project file
└── README.md         # This file
```

## License

Copyright (C) 2025 Modular Battery Technologies, Inc.  
Protected by US Patents 11,380,942; 11,469,470; 11,575,270; others.

## Support

For issues or questions, contact the Pack Controller development team.