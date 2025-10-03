# Claude Context for Pack-Controller-EEPROM

## Recent Updates (September 17, 2025)

### Pack Emulator Improvements
1. **Heartbeat Control**: Added Start/Stop heartbeat button with visual indicator
   - Heartbeat rate reduced to 1 Hz (was 2 Hz)
   - Discovery requests reduced to every 10 seconds (was 5 seconds)
   - Indicator shows gray when disabled or no recent heartbeat

2. **Module Slot Management Fix**: 
   - Pre-initialize all 32 module slots with uniqueId = 0
   - Check uniqueId == 0 to determine slot availability (not isRegistered)
   - Deregistration keeps uniqueId intact to prevent slot reuse
   - New modules won't take slots of deregistered modules
   - GetAllModules only returns modules with non-zero unique IDs

3. **UI Improvements**:
   - Button panel width increased to 700px to accommodate new heartbeat button
   - Heartbeat button functions like discovery Start/Stop
   - Default heartbeat state is ON

### Known Issues
- Need to test changes before pushing (reminder to self!)
- Module display properly preserves deregistered modules in list now

## Build Environment
**IMPORTANT**: This project runs in WSL but uses Windows-based build tools:
- **Pack Controller firmware**: Uses STM32CubeIDE (Windows/Linux) - build process varies
- **Pack Emulator**: Uses Borland C++ Builder 6 (Windows) - **cannot build from WSL**
- **DO NOT** attempt to build the emulator from WSL using make, gcc, or bcc32
- User must compile manually in Windows environment
- Focus on code correctness, not attempting builds

## Git Authentication
**Universal Push Command**:
```bash
grep GITHUB_PAT ../.env | cut -d= -f2 | xargs -I {} git push https://dp-web4:{}@github.com/dp-web4/Pack-Controller-EEPROM.git
```
See `../private-context/GIT_COMMANDS_CLAUDE.md` for details.

## Project Context System

**IMPORTANT**: A comprehensive context system exists at `../misc/context-system/` (relative to project home)

Quick access from project home:
```bash
# Get overview of battery hierarchy projects
cd ../misc/context-system
python3 query_context.py project pack-controller

# See complete battery hierarchy
cat projects/battery-hierarchy.md

# Find integration points
python3 query_context.py search "blockchain"
```

## This Project's Role

Pack-Controller is the top layer of the three-tier battery management hierarchy:
- CellCPU → ModuleCPU → **Pack-Controller** (this)

System-level management (STM32WB55) featuring:
- Dual-core ARM Cortex-M4/M0+
- Flash-based EEPROM emulation with wear leveling
- Triple CAN architecture (VCU, Modules, Reserved)
- Manages up to 32 battery modules
- Bluetooth LE capability (hardware ready, not implemented)

## Key Relationships
- **Integrates With**: ModuleCPU (via CAN), future Web4 blockchain
- **Bridges To**: web4-modbatt-demo (planned digital twin)
- **Configured By**: modbatt-CAN tool
- **Protected By**: US Patent 11,575,270

## Design Philosophy
Pack-Controller represents the "conscious" level of the battery system - aware of system-wide state, making strategic decisions, and interfacing with the external world. Yet it respects the autonomy of lower levels, embodying Synchronism's hierarchical intelligence principles.