# Pack Controller Console Test

This is a minimal console application to test the WEB4 key distribution functionality without requiring RAD Studio or the full GUI.

## Requirements

### Option 1: MinGW-w64 (Free, Windows)
1. Download MinGW-w64 from: https://www.mingw-w64.org/downloads/
2. Add to PATH
3. Build with: `mingw32-make`

### Option 2: Visual Studio Community (Free, Windows)
1. Download VS Community from: https://visualstudio.microsoft.com/vs/community/
2. Open Developer Command Prompt
3. Build with: `nmake -f Makefile.msvc` (needs Makefile.msvc created)

### Option 3: WSL2 with g++ (Free, Windows via Linux)
1. In WSL2 terminal: `sudo apt install build-essential`
2. Build with: `make`

### Option 4: MSYS2 (Free, Windows)
1. Download MSYS2 from: https://www.msys2.org/
2. Install toolchain: `pacman -S mingw-w64-x86_64-toolchain`
3. Build with: `make`

## Building

```bash
cd /mnt/c/projects/ai-agents/Pack-Controller-EEPROM/emulator/console_test
make
```

## Running

```bash
./pack_console_test.exe
```

## What It Tests

1. WEB4 handler initialization
2. Pack device key reception (8 chunks)
3. App device key reception (8 chunks)  
4. Component ID reception (8 chunks)
5. Checksum validation
6. ACK/NACK generation
7. Key storage simulation

## Output

The test will show:
- Each CAN message being processed
- Chunk reception status
- ACK messages that would be sent
- Final validation status

## Limitations

This console test:
- Doesn't use real CAN hardware
- Doesn't have GUI
- Uses mock EEPROM (in-memory)
- Is for functionality testing only

For full integration testing, you'll need the complete emulator built with RAD Studio.