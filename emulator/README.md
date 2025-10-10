# Pack Controller Emulator

Windows GUI application that emulates the Pack Controller functionality for direct battery module testing without requiring the STM32WB55 hardware.

## Purpose

This emulator allows you to:
- Test battery modules directly via CAN bus using a PCAN-USB adapter
- Replace the embedded pack controller during development and testing
- Manage module registration, state control, and monitoring
- Retrieve and export frame data from module SD cards
- Test Web4 key distribution protocols
- Debug module communication issues

## Hardware Requirements

- Windows PC (Windows 10/11 recommended)
- PCAN-USB adapter (or compatible PEAK CAN interface)
- CAN bus connection to battery modules
- 500 kBit/s CAN baudrate (standard for modules)

## Software Requirements

- Borland C++ Builder 6 (for building from source)
- PCAN-Basic drivers installed
- Visual C++ Runtime (for running pre-built executable)

## Building the Application

**IMPORTANT**: This project uses Borland C++ Builder 6 and must be built in Windows.

1. Open `PackEmulator.bpr` in Borland C++ Builder 6
2. Ensure PCAN-Basic API is installed (included in project)
3. Build configuration:
   - Debug: For development and testing
   - Release: For production use
4. Build the project (F9 or Project → Build)

**Note**: Cannot build from WSL or Linux. User must compile manually in Windows environment.

## User Interface Guide

### Main Window Layout

The emulator window is divided into several sections:

```
┌─────────────────────────────────────────────────────────┐
│ Connection Controls                                     │
├────────────────┬────────────────────────────────────────┤
│ Module List    │ Module Details Tabs                    │
│ & Controls     │ - Status                               │
│                │ - Cells                                 │
│                │ - Frames                                │
│                │ - History                               │
│                │ - Web4                                  │
├────────────────┴────────────────────────────────────────┤
│ Status Bar                                              │
└─────────────────────────────────────────────────────────┘
```

### Connection Panel (Top)

**CAN Channel Dropdown**
- Select PCAN-USB adapter channel (typically "PCAN-USB 1")
- Auto-detects available PEAK CAN interfaces

**Baudrate Dropdown**
- Set CAN bus speed (default: 500 kBit/s)
- Must match module configuration

**Connect Button**
- Establishes CAN bus connection
- Status indicator turns green when connected
- Enables module discovery and control

**Disconnect Button**
- Closes CAN bus connection
- Deregisters all modules
- Clears module list

**Connection Status Label**
- Shows current connection state
- Green "Connected" or gray "Disconnected"

**Heartbeat Indicator**
- Shows heartbeat activity when enabled
- Updates when heartbeat messages sent (1 Hz)

### Left Panel - Module Management

#### Modules Group

**Module List View**
- Shows all discovered modules
- Columns: ID, Unique ID, State, Voltage, Current, Temp
- Select a module to view details in center panel
- Highlights selected module

**Discover Button (Start/Stop)**
- **Start**: Begins sending discovery requests (every 10 seconds)
- **Stop**: Stops discovery broadcasts
- Unregistered modules will announce themselves when discovered

**Register Button**
- Registers the selected unregistered module
- Assigns module ID and establishes communication
- Module must be selected in list

**Deregister Button**
- Deregisters the selected module
- Removes module from active list
- Module returns to unregistered state

**Deregister All Button**
- Deregisters all currently registered modules
- Clears entire module list
- Use before disconnecting

**Heartbeat Button (Start/Stop)**
- **Start**: Sends periodic heartbeat at 1 Hz
- **Stop**: Stops heartbeat transmission
- Modules timeout after ~11 seconds without heartbeat
- Default: ON at startup

#### Control Group

**State Control Buttons**
- **Set Off**: Turns off both FET and relay (safe state)
- **Set Standby**: Mechanical relay ON, FET OFF (precharge ready)
- **Set Precharge**: Both relay and FET ON (full power)
- **Set On**: Same as Precharge (full operation)

**Set All States Button**
- Opens dialog to set all registered modules to same state
- Useful for batch operations
- Confirms before applying

**Enable Balancing Checkbox**
- Enables/disables cell balancing on selected module
- Only effective when module is in appropriate state

**Balancing Mask Edit**
- Enter hex mask for which cells to balance
- Format: 0xXXXXXXXX
- Each bit represents one cell

### Center Panel - Module Details

#### Status Tab

Shows real-time module status:

**Status Grid**
- Module state (Off/Standby/Precharge/On)
- Registration status
- Cell count
- Frame counter

**Summary Labels**
- **Voltage**: Pack voltage (V)
- **Current**: Pack current (A)
- **Temperature**: Temperature range (°C)
- **SOC**: State of charge (%)
- **SOH**: State of health (%)

#### Cells Tab

Displays individual cell data:

**Cell Grid**
- Row for each cell
- Columns: Cell #, Voltage (V), Temperature (°C)
- Color coding for out-of-range values
- Updates in real-time

**Export Cells Checkbox**
- Enables automatic CSV export of cell data
- Creates timestamped CSV file
- Updates with each cell data message

**Export Filename Label**
- Shows current export file path
- Format: `cells_[ModuleID]_[timestamp].csv`

#### Frames Tab

Retrieve and view frames from module SD card:

**Frame Number Controls**
- **Frame Number Edit**: Enter frame number to retrieve
- **Dec Button (-)**: Decrement frame number
- **Inc Button (+)**: Increment frame number
- **Current Button**: Jump to current/latest frame

**Get Frame Button**
- Requests frame from module via CAN transfer protocol
- Retrieves 1024-byte frame in segments
- Shows transfer progress

**Frame Metadata Display**
- **Frame Bytes Label**: Total frame size
- **Frame CRC Label**: CRC32 checksum for verification
- **Metadata Memo**: Formatted frame metadata
  - Timestamp
  - Cell count
  - Voltage/current/temperature statistics
  - Frame counter
  - Module unique ID

**Frame Hex Memo**
- Shows complete frame in hex dump format
- Useful for debugging frame structure

**CSV Export Controls**
- **Export Filename Edit**: Set output CSV filename
- **Export Append Button**: Add frame cell data to existing CSV
- **Export Overwrite Button**: Create new CSV with frame cell data

**How Frame Transfer Works**
1. Click "Get Frame" → Sends FRAME_TRANSFER_REQUEST
2. Module responds with FRAME_TRANSFER_START (includes frame counter, CRC)
3. Module sends 16 FRAME_TRANSFER_DATA messages (64 bytes each)
4. Module sends FRAME_TRANSFER_END
5. Emulator verifies CRC and displays frame

**Frame Data Format**
- Each frame is 1024 bytes (2 SD card sectors)
- Contains metadata + circular buffer of cell readings
- Multiple string readings per frame (depends on cell count)
- Cell data in RAW format (requires conversion for display)

#### History Tab

Communication event log:

**History Memo**
- Scrolling text view of CAN messages
- Timestamped entries
- Shows sent and received messages
- Color-coded by message type

**Clear History Button**
- Clears the history memo
- Does not affect stored data

**Export History Button**
- Saves history to text file
- Includes timestamps
- Useful for debugging communication issues

#### Web4 Tab

Web4 blockchain key distribution (future feature):

**Web4 Group**
- **Device Key Edit**: Enter 256-bit device key (hex)
- **LCT Key Edit**: Enter Lithium Cell Token key (hex)
- **Component ID Edit**: Enter component identifier

**Distribute Keys Button**
- Sends keys to selected module
- Stores in module EEPROM
- Used for blockchain authentication

**Encryption Enabled Checkbox**
- Enables/disables message encryption
- Requires valid keys to be distributed first

**Note**: Web4 integration is hardware-ready but not fully implemented.

### Bottom Status Bar

Shows:
- Connection status
- Last received message timestamp
- Error messages and warnings
- Module count (registered / total discovered)

## Running the Emulator

### Initial Setup

1. Install PCAN-Basic drivers from PEAK System
2. Connect PCAN-USB adapter to PC
3. Connect CAN bus to battery modules (verify termination)
4. Launch `PackEmulator.exe`

### Basic Workflow

**1. Connect to CAN Bus**
   - Select PCAN channel
   - Select 500 kBit/s baudrate
   - Click "Connect"
   - Verify green "Connected" status

**2. Discover Modules**
   - Click "Discover" to start
   - Wait for module announcements
   - Modules appear in list with Unique ID

**3. Register Modules**
   - Select module in list
   - Click "Register"
   - Module assigned ID (1-32)
   - Can now control module

**4. Control Module State**
   - Select registered module
   - Click state button (Off/Standby/Precharge/On)
   - Monitor status in Status tab
   - Watch for state transition completion

**5. Monitor Cell Data**
   - Go to Cells tab
   - View real-time cell voltages/temperatures
   - Enable "Export Cells" for CSV logging

**6. Retrieve Frame Data**
   - Go to Frames tab
   - Enter frame number (or use Current)
   - Click "Get Frame"
   - Wait for transfer to complete (~2-3 seconds)
   - View metadata and export to CSV if needed

**7. Disconnect**
   - Click "Deregister All" to cleanly disconnect modules
   - Click "Disconnect" to close CAN bus
   - Exit application

## CAN Protocol Details

### Module → Pack Controller Messages

| ID    | Name | Description |
|-------|------|-------------|
| 0x500 | MODULE_ANNOUNCE | Module discovery announcement |
| 0x502 | MODULE_STATUS_1 | Voltage, current, state |
| 0x503 | MODULE_STATUS_2 | Cell statistics, SOC, SOH |
| 0x504 | MODULE_STATUS_3 | Temperature, fault flags |
| 0x505 | MODULE_DETAIL | Individual cell data |
| 0x506 | MODULE_HARDWARE | Hardware version, capabilities |
| 0x507 | MODULE_CELL_COMM_STATUS | Cell communication statistics |
| 0x51A | FRAME_TRANSFER_START | Frame transfer begin |
| 0x51B | FRAME_TRANSFER_DATA | Frame data segment |
| 0x51C | FRAME_TRANSFER_END | Frame transfer complete |

### Pack Controller → Module Commands

| ID    | Name | Description |
|-------|------|-------------|
| 0x510 | MODULE_REGISTER_ACK | Register module, assign ID |
| 0x514 | MODULE_STATE_CHANGE | Set module state |
| 0x516 | TIME_SYNC | Synchronize RTC |
| 0x518 | MODULE_DEREGISTER | Deregister module |
| 0x519 | FRAME_TRANSFER_REQUEST | Request frame from SD |
| 0x51D | MODULE_ANNOUNCE_REQUEST | Trigger discovery |
| 0x51E | HEARTBEAT | Keep-alive message (1 Hz) |

### Message Timing

- **Heartbeat**: 1 Hz (every 1 second)
- **Discovery Request**: Every 10 seconds
- **Module Timeout**: 11.1 seconds without heartbeat
- **Status Polling**: On-demand (not periodic)
- **Frame Transfer**: ~2-3 seconds for 1024-byte frame

## Features

### Module State Machine

Modules follow a strict state machine:

```
INIT → OFF ⇄ STANDBY ⇄ PRECHARGE → ON
         ↑                            ↓
         └────────────────────────────┘
```

- **OFF**: All contactors open, safe state
- **STANDBY**: Mechanical relay closed, FET open (ready for precharge)
- **PRECHARGE**: Both closed (in-rush current limited)
- **ON**: Same as precharge, full operation

**Safety Notes**:
- Module will automatically transition to OFF on timeout
- Overcurrent protection triggers transition to STANDBY
- Loss of 5V triggers transition to OFF
- State transitions are mutex-protected with SD writes

### Frame Data Retrieval

Frames contain:
- Timestamp (64-bit microseconds)
- Module unique ID
- Cell count and string reading count
- Voltage/current/temperature statistics (converted values)
- Circular buffer of cell readings (RAW values)
- Watchdog and error counters

**RAW Cell Data Format** (requires conversion):
- **Voltage**: 10-bit ADC value (0-1023)
  - Formula: `(adc/1024) * 1.1V / voltage_scale * calibration`
  - Voltage scale: 30100/(90900+30100)
  - Calibration: 1.032
- **Temperature**: MCP9843 format (signed 8.4 fixed point)
  - Bits 0-3: Fractional (16ths of degree)
  - Bits 4-11: Whole degrees
  - Bit 12: Sign (0=positive, 1=negative)
  - Bit 15: I2C status (0x8000 = valid)

### CSV Export Formats

**Cell Export** (real-time streaming):
```
Timestamp,Cell0_V,Cell0_T,Cell1_V,Cell1_T,...
2025-01-10 14:30:00.123,3.456,25.3,3.457,25.1,...
```

**Frame Export** (from SD card frames):
```
Frame,Timestamp,Cell0_V,Cell0_T,Cell1_V,Cell1_T,...
12345,1736526600000000,3.456,25.3,3.457,25.1,...
```

## Troubleshooting

### Connection Issues

**"Cannot connect to CAN bus"**
- Verify PCAN-Basic drivers installed
- Check Windows Device Manager for adapter
- Try different USB port
- Restart PCAN driver service

**"No modules discovered"**
- Check CAN termination (120Ω at both ends)
- Verify module power supply (5V, sufficient current)
- Confirm baudrate matches (500 kBit/s)
- Send announce request manually
- Check CAN wiring (CAN_H, CAN_L, GND)

### Module Communication Issues

**"Module timeout / deregistered"**
- Heartbeat must be running (Start Heartbeat button)
- Check CAN bus integrity
- Verify module hasn't crashed (check LEDs)
- Network may be overloaded (reduce message rate)

**"Module not responding to commands"**
- Ensure module is registered first
- Check module state allows command
- Verify no CAN errors in History tab
- Module may be busy (wait for state transition)

### Frame Transfer Issues

**"Frame transfer timeout"**
- Module SD card may not be ready
- Frame number may not exist yet
- Module may be writing to SD (wait and retry)
- CAN bus errors during transfer

**"CRC mismatch on frame"**
- Bus noise corrupted transfer
- Retry frame retrieval
- Check CAN termination and wiring
- SD card may have corruption

**"Frame metadata shows wrong values"**
- Frame may be incomplete (buffer not full)
- Check frame counter vs requested frame
- Module may have reset during frame write

### Build Issues

**"Cannot find PCANBasic.h"**
- Install PCAN-Basic API from PEAK System
- Copy headers to include/ directory
- Update project include paths

**"Linker error: Unresolved external"**
- Ensure PCANBasic.lib in lib/ directory
- Check library search paths in project settings
- Verify correct library for architecture (x86/x64)

## Safety Warnings

⚠️ **DANGER: HIGH VOLTAGE**
- Battery modules contain dangerous voltages and currents
- Always follow proper electrical safety procedures
- Use appropriate PPE (insulated tools, safety glasses)
- Never work on live circuits

⚠️ **FIRE HAZARD**
- Lithium cells can catch fire if mishandled
- Have fire suppression equipment ready
- Monitor temperature continuously
- Never leave modules unattended while connected

⚠️ **SOFTWARE SAFETY**
- This emulator directly controls real hardware
- Incorrect commands can damage modules
- Test all operations in safe environment first
- Implement emergency stop procedures
- Keep documentation and schematics available

## Known Issues

- **PD2/PD3/PD4 pullups only reach 0.8V**: Hardware pullup resistor fixes behavior (investigation ongoing)
- **Web4 key distribution**: Protocol defined but blockchain integration incomplete
- **Cell polling**: Polls one cell at a time (slow for large packs)

## Testing Checklist

Before connecting to modules:
- [ ] PCAN-USB recognized in Device Manager
- [ ] CAN bus properly terminated (120Ω both ends)
- [ ] Module power supplies connected and verified
- [ ] Correct baudrate selected (500 kBit/s)
- [ ] Safety equipment ready (fire extinguisher, PPE)
- [ ] Emergency stop accessible
- [ ] Workspace clear of flammable materials
- [ ] Ventilation adequate
- [ ] Observer present (never work alone on high voltage)

## File Structure

```
emulator/
├── include/              # Header files
│   ├── pack_emulator_main.h    # Main form
│   ├── module_manager.h        # Module state management
│   ├── can_interface.h         # CAN bus abstraction
│   └── battery.h               # Battery definitions
├── src/                 # Source files
│   ├── pack_emulator_main.cpp  # UI implementation
│   ├── pack_emulator_main.dfm  # Form layout
│   ├── module_manager.cpp      # Module logic
│   └── can_interface.cpp       # CAN communication
├── lib/                 # Libraries
│   ├── PCANBasic.h
│   └── PCANBasic.lib
├── PackEmulator.bpr     # Borland C++ Builder project
└── README.md           # This file
```

## Development Notes

### Recent Updates

**September 2025**:
- Added heartbeat Start/Stop control with visual indicator
- Reduced heartbeat rate to 1 Hz (was 2 Hz)
- Reduced discovery interval to 10 seconds (was 5 seconds)
- Fixed module slot management (pre-initialize all 32 slots)
- Deregistration preserves unique IDs to prevent slot reuse

**October 2025**:
- Fixed CSV export cell data conversion (voltage and temperature)
- Implemented proper ADC voltage conversion with voltage divider
- Implemented proper MCP9843 temperature conversion
- Documented RAW vs CONVERTED data formats
- Fixed address read exception in frame parsing

### Future Enhancements

- Web4 blockchain integration
- Multi-module CSV export
- Real-time graphing of cell voltages
- Automatic state transition sequencing
- Load profile playback
- CAN bus analyzer view

## License

Copyright (C) 2025 Modular Battery Technologies, Inc.
Protected by US Patents 11,380,942; 11,469,470; 11,575,270; others pending.

## Support

For issues, questions, or to report bugs:
- Create issue at: https://github.com/dp-web4/Pack-Controller-EEPROM/issues
- Contact: Pack Controller development team
- Include: Log files, CAN trace, error messages, hardware configuration
