# Pack-Controller-EEPROM

**ModBatt Battery Management System - Pack Controller**

## Overview

The Pack-Controller is the top-tier controller in ModBatt's hierarchical battery management system, coordinating up to 32 battery modules while interfacing with Vehicle Control Units (VCUs) and providing system-level management. This repository demonstrates how distributed intelligence principles scale to the pack level in production embedded hardware.

**Implementation Status**: ✅ **Production-ready** - Complete firmware with dual CAN bus architecture, EEPROM emulation, and comprehensive module coordination

### Quick Facts

- **Target Platform**: STM32WB55 microcontroller (Dual-core ARM Cortex-M4/M0+)
- **Manages**: Up to 32 battery modules via CAN bus (4096 cells total)
- **Interfaces**: Dual CAN (VCU + Modules), Bluetooth LE capable (hardware ready)
- **Storage**: Flash-based EEPROM emulation with wear leveling
- **Features**: Real-time pack state estimation, fault detection, module health tracking

## Architecture

### System Hierarchy

```
Vehicle Control Unit (VCU)
    ↓ CAN Bus (500kbps)
Pack Controller (STM32WB55) ──── This Repository
    ↓ CAN Bus (250kbps, extended frames)
Up to 32 × ModuleCPU (ATmega64M1)
    ↓ Virtual UART (20kbps)
Up to 128 × CellCPU per module (ATtiny45)
    ↓
4096 Li-ion Cells Total
```

### Key Features

#### 🔋 Module Management
- **Capacity**: Coordinates up to 32 independent battery modules
- **Communication**: Extended CAN 2.0B protocol with module addressing
- **Monitoring**: Aggregates voltage, temperature, and status from all modules
- **Control**: Module registration/deregistration, balancing coordination, fault isolation

#### 🚗 VCU Interface
- **Protocol**: Standard CAN bus to Vehicle Control Unit
- **Data**: Pack voltage, current, SOC, temperature, fault status
- **Commands**: Charge/discharge requests, balancing control, diagnostics

#### 💾 Data Persistence
- **EEPROM Emulation**: Flash-based storage with wear leveling
- **Configuration**: Module registry, calibration data, fault history
- **Resilience**: Power-loss safe writes, data integrity verification

#### 📡 Future Capabilities
- **Bluetooth LE**: Hardware integrated (STM32WB55), firmware not yet implemented
- **Wireless Diagnostics**: OTA updates, remote monitoring, cloud integration

## Repository Contents

### Core Firmware
- STM32WB55 dual-core firmware (main Cortex-M4 application)
- CAN bus communication drivers (VCU and Module interfaces)
- EEPROM emulation with wear leveling
- Module registration and health monitoring
- Pack-level state estimation and control

### Emulator Tools (Illustrative Utilities)
- **Pack Emulator**: C++ Builder 6 application for hardware-in-loop testing
- **VCU Emulator**: Simulates vehicle control unit for development
- Note: Emulators require Borland licensing appropriate to intended use; migration to open platforms encouraged

### Documentation
- `docs/` - Architecture, protocols, implementation guides
- `emulator/` - Pack Emulator source and documentation
- Technical documentation for CAN protocols, module communication, EEPROM layout

## License and Patents

### Software License

This software is licensed under the **GNU Affero General Public License v3.0 or later** (AGPL-3.0-or-later).

See [LICENSE](LICENSE) for full license text.
See [LICENSE_SUMMARY.md](LICENSE_SUMMARY.md) for a summary of software vs. hardware licensing.

### Patents

Protected by 14 US Patents including:
- **11,380,942**: Distributed battery management with autonomous cell controllers
- **11,469,470**: Hierarchical battery management architecture
- **11,575,270**: Pack control systems and methods
- **11,699,817**: System, pack and module controllers, blockchain integration
- And 10 additional patents (see [PATENTS.md](PATENTS.md) for complete list)

**Patent Grant**: A limited patent grant for the software is provided under the AGPL-3.0 license scope. See [PATENTS.md](PATENTS.md) for details.

**Hardware Licensing**: Separate from software. See `docs/legal/ModBatt_Product_License.pdf` for hardware manufacturing licensing.

### Copyright

© 2023-2025 Modular Battery Technologies, Inc.

**Open Source Release**: October 2025 - Part of the Web4 trust-native ecosystem demonstration.

## Development

### Build Environment

**Important**: This project uses STM32CubeIDE (available for Windows/Linux). The Pack Emulator uses Borland C++ Builder 6 (Windows only) and cannot be built from WSL/Linux.

### Getting Started

1. **Firmware**: Open project in STM32CubeIDE
2. **Emulator**: Build in Windows using C++ Builder 6
3. **Testing**: Use emulator for hardware-in-loop validation

See individual documentation in `docs/` for detailed setup instructions.

## Integration with ModBatt Ecosystem

This controller works in concert with:
- **[ModuleCPU](https://github.com/dp-web4/ModuleCPU)** - Module-level coordination (downstream)
- **[CellCPU](https://github.com/dp-web4/CellCPU)** - Individual cell monitoring (via ModuleCPU)
- **[modbatt-CAN](https://github.com/dp-web4/modbatt-CAN)** - Windows configuration utility

## Contributing

By contributing, you agree your changes are licensed under AGPL-3.0-or-later.
- Use conventional commits
- Include SPDX headers in new files
- Add tests or validation notes
- Discuss large changes in an issue before opening a PR

See [CONTRIBUTING.md](CONTRIBUTING.md) for details.

---

## Related Projects

This repository can be viewed as demonstrating patterns of distributed trust and verifiable provenance in embedded systems.

**Related**: [Web4](https://github.com/dp-web4/web4) (trust architecture) • [Synchronism](https://github.com/dp-web4/Synchronism) (conceptual framing)

---

**Note**: This firmware demonstrates system-level coordination patterns in hierarchical battery management. The design patterns are production-tested across thousands of cells in real-world electric vehicle applications.
