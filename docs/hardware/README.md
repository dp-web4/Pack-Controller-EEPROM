# STM32WB55 Hardware Platform

## Table of Contents

1. [Overview](#overview)
2. [Microcontroller Specifications](#microcontroller-specifications)
3. [Dual-Core Architecture](#dual-core-architecture)
4. [Memory System](#memory-system)
5. [Communication Interfaces](#communication-interfaces)
6. [Power Management](#power-management)
7. [GPIO Configuration](#gpio-configuration)
8. [External Components](#external-components)

## Overview

The Pack Controller EEPROM firmware runs on the STM32WB55RGV6 microcontroller, a dual-core system designed for wireless and connectivity applications. This platform provides the computational power, memory capacity, and peripheral interfaces required for sophisticated battery management with EEPROM functionality.

### Key Hardware Advantages

**Dual-Core Processing**
- Dedicated application processor for real-time battery management
- Optional network processor for wireless communications
- Independent execution with shared memory communication

**Advanced Memory Architecture**
- 1MB Flash memory with EEPROM emulation capability
- 256KB SRAM with flexible allocation options
- Hardware security features and memory protection

**Rich Peripheral Set**
- Multiple CAN interfaces (integrated + external)
- SPI, UART, I2C communication options
- Real-time clock with battery backup capability
- Hardware timers and ADC for sensor interfaces

## Microcontroller Specifications

### STM32WB55RGV6 Core Features

```
STM32WB55RGV6 Technical Specifications:
┌─────────────────────────────────────────────────────────────────┐
│                        Core Architecture                       │
├─────────────────────────────────────────────────────────────────┤
│ Application Processor: ARM Cortex-M4 with FPU @ 64MHz         │
│ Network Processor:     ARM Cortex-M0+ @ 32MHz (optional)       │
│ Package:              VFQFPN68 (10mm x 10mm, 68-pin QFN)      │
│ Operating Voltage:     1.71V to 3.6V                          │
│ Operating Temperature: -40°C to +85°C (industrial grade)       │
├─────────────────────────────────────────────────────────────────┤
│                         Memory System                          │
├─────────────────────────────────────────────────────────────────┤
│ Flash Memory:         1024KB (1MB) total                       │
│ SRAM1:               192KB (main application memory)           │
│ SRAM2a:              32KB (shared with network processor)      │
│ SRAM2b:              32KB (secure memory area)                 │
│ Backup SRAM:         4KB (battery-backed for RTC)             │
├─────────────────────────────────────────────────────────────────┤
│                      Security Features                         │
├─────────────────────────────────────────────────────────────────┤
│ Hardware Security:    AES 256-bit encryption                   │
│ Public Key Crypto:    RSA, Elliptic Curve (ECDSA, ECDH)      │
│ True Random Number:   TRNG generator                           │
│ Secure Boot:          Immutable bootloader                     │
│ Memory Protection:    MPU, Firewall, PCROP                    │
└─────────────────────────────────────────────────────────────────┘
```

### Performance Characteristics

```c
// CPU Performance metrics
#define CPU_FREQUENCY_MAX       64000000UL  // 64MHz maximum
#define CPU_FREQUENCY_TYPICAL   64000000UL  // 64MHz typical operation
#define FLASH_ACCESS_TIME       3           // 3 wait states at 64MHz
#define CACHE_SIZE              0           // No instruction cache
#define FPU_SUPPORT             1           // Hardware floating point

// Memory access times (approximate cycles)
#define SRAM_ACCESS_CYCLES      1           // 1 cycle SRAM access
#define FLASH_ACCESS_CYCLES     4           // 4 cycles flash access (with wait states)
#define EXTERNAL_MEMORY_CYCLES  10          // External memory via SPI

// Interrupt response times
#define IRQ_LATENCY_MIN         12          // Minimum interrupt latency (cycles)
#define IRQ_LATENCY_MAX         32          // Maximum interrupt latency (cycles)
#define CONTEXT_SWITCH_CYCLES   50          // Task context switch overhead
```

### Clock System Configuration

```c
// Clock configuration for Pack Controller
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    // Configure the main internal regulator output voltage
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    // Configure MSI oscillator
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI | RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;                    // 32.768kHz for RTC
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;           // 4MHz MSI
    
    // Configure PLL for 64MHz system clock
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.PLL.PLLM = 1;     // Divide by 1: 4MHz
    RCC_OscInitStruct.PLL.PLLN = 32;    // Multiply by 32: 128MHz
    RCC_OscInitStruct.PLL.PLLP = 2;     // Divide by 2: 64MHz
    RCC_OscInitStruct.PLL.PLLQ = 2;     // Divide by 2: 64MHz  
    RCC_OscInitStruct.PLL.PLLR = 2;     // Divide by 2: 64MHz (system clock)
    
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    // Configure system clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;   // 64MHz from PLL
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;         // AHB: 64MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;          // APB1: 64MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;          // APB2: 64MHz
    
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3);

    // Configure peripheral clocks
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC | RCC_PERIPHCLK_USART1;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;     // RTC from 32.768kHz
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2; // UART from APB2
    
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}
```

## Dual-Core Architecture

### Core Allocation and Responsibilities

```
┌─────────────────────────────────────────────────────────────────┐
│                     STM32WB55 Dual Core                        │
├────────────────────────────────┬────────────────────────────────┤
│        Application Core        │         Network Core           │
│     (ARM Cortex-M4 @ 64MHz)    │    (ARM Cortex-M0+ @ 32MHz)   │
├────────────────────────────────┼────────────────────────────────┤
│ Primary Responsibilities:      │ Secondary Responsibilities:    │
│                               │                                │
│ • Battery pack state control  │ • Bluetooth LE stack (opt)    │
│ • Safety monitoring & faults  │ • Wireless diagnostics        │
│ • CAN communication protocols │ • OTA firmware updates        │
│ • EEPROM parameter management  │ • Security key management     │
│ • Module coordination         │ • Background data logging     │
│ • VCU interface handling      │ • Non-critical communications │
│ • Real-time control loops     │ • System health monitoring    │
│ • Interrupt service routines  │ • Power management assist     │
├────────────────────────────────┼────────────────────────────────┤
│ Memory Allocation:            │ Memory Allocation:             │
│ • Flash: 768KB application    │ • Flash: 128KB network stack  │
│ • SRAM1: 192KB primary       │ • SRAM2a: 32KB shared         │
│ • SRAM2b: 32KB secure        │ • Dedicated peripherals       │
├────────────────────────────────┼────────────────────────────────┤
│ Peripheral Access:            │ Peripheral Access:             │
│ • CAN controller              │ • Bluetooth LE radio          │
│ • SPI interfaces              │ • Security peripherals        │
│ • UART/LPUART                │ • RNG, AES, PKA               │
│ • GPIO, Timers, ADC          │ • Shared IPCC mailbox         │
└────────────────────────────────┴────────────────────────────────┘
```

### Inter-Processor Communication (IPCC)

```c
// IPCC channel definitions
#define IPCC_CHANNEL_1    0x01    // Command channel M4 → M0+
#define IPCC_CHANNEL_2    0x02    // Data channel M4 ↔ M0+
#define IPCC_CHANNEL_3    0x04    // Status channel M0+ → M4
#define IPCC_CHANNEL_4    0x08    // Interrupt channel

// Shared memory structure (located in SRAM2a)
typedef struct {
    volatile uint32_t m4_to_m0_command;     // Command from M4 to M0+
    volatile uint32_t m0_to_m4_status;      // Status from M0+ to M4
    volatile uint8_t shared_buffer[256];     // Data exchange buffer
    volatile uint32_t buffer_write_ptr;     // Write pointer
    volatile uint32_t buffer_read_ptr;      // Read pointer
    volatile uint32_t sequence_number;      // Message sequence
    volatile uint32_t checksum;            // Data integrity
} IPCC_SharedMemory;

// IPCC initialization
void IPCC_Init(void) {
    // Enable IPCC clock
    __HAL_RCC_IPCC_CLK_ENABLE();
    
    // Configure IPCC channels
    HAL_IPCC_ActivateNotification(&hipcc, IPCC_CHANNEL_1, IPCC_CHANNEL_DIR_TX);
    HAL_IPCC_ActivateNotification(&hipcc, IPCC_CHANNEL_2, IPCC_CHANNEL_DIR_RX);
    HAL_IPCC_ActivateNotification(&hipcc, IPCC_CHANNEL_3, IPCC_CHANNEL_DIR_RX);
    
    // Initialize shared memory
    memset((void*)&ipcc_shared_mem, 0, sizeof(IPCC_SharedMemory));
}

// Send command to M0+ core
void IPCC_SendCommand(uint32_t command, void* data, uint32_t length) {
    // Write data to shared buffer
    if (length <= sizeof(ipcc_shared_mem.shared_buffer)) {
        memcpy((void*)ipcc_shared_mem.shared_buffer, data, length);
        ipcc_shared_mem.buffer_write_ptr = length;
    }
    
    // Set command
    ipcc_shared_mem.m4_to_m0_command = command;
    ipcc_shared_mem.sequence_number++;
    
    // Calculate checksum
    ipcc_shared_mem.checksum = CalculateIPCCChecksum();
    
    // Notify M0+ core
    HAL_IPCC_NotifyCPU(&hipcc, IPCC_CHANNEL_1, IPCC_CHANNEL_DIR_TX);
}
```

## Memory System

### Flash Memory Organization

```
STM32WB55 Flash Layout (1MB Total):
┌─────────────────────────────────────────────────────────┐ 0x08000000
│                        Sector 0-15                     │
│                    Bootloader Area                      │
│                        (64KB)                          │
│                                                         │ 0x08010000
├─────────────────────────────────────────────────────────┤
│                      Sector 16-191                     │
│               Main Application Firmware                 │
│                       (704KB)                          │
│  ┌─────────────────────────────────────────────────────┐ │
│  │              Application Code                       │ │ 0x08020000
│  │  • main.c, vcu.c, mcu.c, safety systems           │ │
│  │  • HAL drivers and middleware                       │ │
│  │  • Protocol handlers and state machines            │ │
│  └─────────────────────────────────────────────────────┘ │ 0x08080000
├─────────────────────────────────────────────────────────┤
│                     Sector 192-199                     │
│                EEPROM Emulation Area                    │
│                        (32KB)                          │
│  ┌─────────────────────────────────────────────────────┐ │
│  │ Page 0: Active parameter storage     (4KB)          │ │ 0x08088000
│  │ Page 1: Backup parameter storage     (4KB)          │ │ 0x0808C000
│  │ Page 2-7: Wear leveling pool        (24KB)          │ │ 0x08090000
│  └─────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────┤
│                     Sector 200-223                     │
│              Application Data Storage                   │
│                        (96KB)                          │
│  • Factory calibration data                            │ 0x080A0000
│  • Manufacturing parameters                            │
│  • Diagnostic logs and fault history                   │
├─────────────────────────────────────────────────────────┤
│                     Sector 224-255                     │
│           Network Processor Firmware                   │
│                       (128KB)                          │
│  • Bluetooth LE stack (optional)                       │ 0x080E0000
│  • Wireless protocol handlers                          │
│  • Security and encryption services                    │
└─────────────────────────────────────────────────────────┘ 0x08100000
```

### SRAM Memory Allocation

```c
// Memory regions and allocation
#define SRAM1_BASE        0x20000000    // 192KB main SRAM
#define SRAM1_SIZE        0x30000       // 192KB
#define SRAM2A_BASE       0x20030000    // 32KB shared SRAM
#define SRAM2A_SIZE       0x8000        // 32KB
#define SRAM2B_BASE       0x20038000    // 32KB secure SRAM
#define SRAM2B_SIZE       0x8000        // 32KB

// Application memory layout
typedef struct {
    // Stack allocation (top of SRAM1)
    uint8_t main_stack[8192];           // 8KB main stack
    uint8_t process_stack[4096];        // 4KB process stack
    
    // Heap allocation
    uint8_t heap_area[16384];           // 16KB heap
    
    // Global variables
    SystemDataStructure system_data;    // Main system data
    PackData pack_data;                 // Pack telemetry data
    ModuleData module_data[32];         // Module data arrays
    
    // Communication buffers
    CANMessage can_tx_buffer[64];       // CAN TX message buffer
    CANMessage can_rx_buffer[64];       // CAN RX message buffer
    uint8_t uart_buffer[1024];          // UART communication buffer
    
    // Working memory
    uint8_t temp_buffer[4096];          // Temporary calculations
    uint8_t crypto_buffer[512];         // Cryptographic operations
    
} ApplicationMemoryLayout;

// Memory protection configuration
void ConfigureMemoryProtection(void) {
    // Configure MPU for memory protection
    MPU_Region_InitTypeDef MPU_InitStruct;
    
    // Disable MPU
    HAL_MPU_Disable();
    
    // Configure region 0: Application code (read-only, execute)
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress = 0x08020000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RO;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    // Configure region 1: Application data (read-write, no execute)
    MPU_InitStruct.Number = MPU_REGION_NUMBER1;
    MPU_InitStruct.BaseAddress = SRAM1_BASE;
    MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    // Configure region 2: EEPROM area (read-write, no execute)
    MPU_InitStruct.Number = MPU_REGION_NUMBER2;
    MPU_InitStruct.BaseAddress = 0x08080000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_32KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    // Enable MPU
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
```

## Communication Interfaces

### CAN Interface Configuration

```c
// CAN controller configuration
typedef struct {
    CAN_HandleTypeDef* hcan;            // HAL CAN handle
    uint32_t baudrate;                  // CAN baudrate
    uint32_t sample_point;              // Sample point percentage
    bool loopback_mode;                 // Loopback for testing
    bool silent_mode;                   // Silent mode for monitoring
} CAN_Config;

// Primary CAN interface (VCU communication)
void CAN1_Init(void) {
    // Configure CAN1 peripheral
    hcan1.Instance = CAN1;
    hcan1.Init.Prescaler = 8;           // 64MHz / 8 = 8MHz
    hcan1.Init.Mode = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1 = CAN_BS1_13TQ; // 87.5% sample point
    hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
    hcan1.Init.TimeTriggeredMode = DISABLE;
    hcan1.Init.AutoBusOff = ENABLE;     // Automatic bus-off recovery
    hcan1.Init.AutoWakeUp = ENABLE;
    hcan1.Init.AutoRetransmission = ENABLE;
    hcan1.Init.ReceiveFifoLocked = DISABLE;
    hcan1.Init.TransmitFifoPriority = ENABLE;
    
    // Resulting baudrate: 8MHz / (1 + 13 + 2) = 500kbps
    
    HAL_CAN_Init(&hcan1);
    
    // Configure message filters
    CAN_FilterTypeDef filter;
    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0x400 << 5;   // Accept 0x400-0x4FF
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x700 << 5; // Mask for range
    filter.FilterMaskIdLow = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    
    HAL_CAN_ConfigFilter(&hcan1, &filter);
    HAL_CAN_Start(&hcan1);
    
    // Enable interrupts
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_ERROR);
}
```

### SPI Interface for External CAN Controller

```c
// SPI configuration for MCP2517FD CAN controller
void SPI1_Init(void) {
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;      // CPOL = 0
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;          // CPHA = 0
    hspi1.Init.NSS = SPI_NSS_SOFT;                  // Software CS control
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; // 64MHz/8 = 8MHz
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    
    HAL_SPI_Init(&hspi1);
}

// MCP2517FD interface functions
typedef struct {
    SPI_HandleTypeDef* hspi;            // SPI handle
    GPIO_TypeDef* cs_port;              // Chip select port
    uint16_t cs_pin;                    // Chip select pin
    GPIO_TypeDef* int_port;             // Interrupt port  
    uint16_t int_pin;                   // Interrupt pin
    GPIO_TypeDef* reset_port;           // Reset port
    uint16_t reset_pin;                 // Reset pin
} MCP2517FD_Interface;

// SPI read/write functions for MCP2517FD
uint8_t MCP2517FD_ReadByte(MCP2517FD_Interface* intf, uint16_t address) {
    uint8_t tx_data[4], rx_data[4];
    
    // Prepare read command
    tx_data[0] = 0x03;                  // Read command
    tx_data[1] = (address >> 8) & 0xFF; // Address high
    tx_data[2] = address & 0xFF;        // Address low
    tx_data[3] = 0x00;                  // Dummy byte
    
    // Assert chip select
    HAL_GPIO_WritePin(intf->cs_port, intf->cs_pin, GPIO_PIN_RESET);
    
    // SPI transaction
    HAL_SPI_TransmitReceive(intf->hspi, tx_data, rx_data, 4, 100);
    
    // Deassert chip select
    HAL_GPIO_WritePin(intf->cs_port, intf->cs_pin, GPIO_PIN_SET);
    
    return rx_data[3];
}

void MCP2517FD_WriteByte(MCP2517FD_Interface* intf, uint16_t address, uint8_t data) {
    uint8_t tx_data[4];
    
    // Prepare write command
    tx_data[0] = 0x02;                  // Write command
    tx_data[1] = (address >> 8) & 0xFF; // Address high
    tx_data[2] = address & 0xFF;        // Address low
    tx_data[3] = data;                  // Data byte
    
    // Assert chip select
    HAL_GPIO_WritePin(intf->cs_port, intf->cs_pin, GPIO_PIN_RESET);
    
    // SPI transaction
    HAL_SPI_Transmit(intf->hspi, tx_data, 4, 100);
    
    // Deassert chip select
    HAL_GPIO_WritePin(intf->cs_port, intf->cs_pin, GPIO_PIN_SET);
}
```

### UART Module Communication

```c
// UART configuration for module communication
void UART1_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;      // 115.2kbps for modules
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    
    HAL_UART_Init(&huart1);
    
    // Enable UART interrupts
    HAL_UART_Receive_IT(&huart1, uart_rx_buffer, 1);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}
```

## Power Management

### Power System Architecture

```c
// Power management configuration
typedef enum {
    POWER_MODE_RUN,         // Normal operation
    POWER_MODE_SLEEP,       // CPU sleep, peripherals active  
    POWER_MODE_STOP,        // CPU and most peripherals stopped
    POWER_MODE_STANDBY,     // Minimal power, RAM lost
    POWER_MODE_SHUTDOWN     // Ultra-low power
} PowerMode;

// Power mode transition
void EnterPowerMode(PowerMode mode) {
    switch (mode) {
        case POWER_MODE_SLEEP:
            // CPU sleep, wake on interrupt
            HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
            break;
            
        case POWER_MODE_STOP:
            // Stop mode with retention
            HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
            SystemClock_Config(); // Restore clocks on wake
            break;
            
        case POWER_MODE_STANDBY:
            // Standby mode, minimal consumption
            HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
            HAL_PWR_EnterSTANDBYMode();
            break;
            
        case POWER_MODE_SHUTDOWN:
            // Shutdown mode, ultra-low power
            HAL_PWREx_EnterSHUTDOWNMode();
            break;
    }
}

// Power consumption estimates
typedef struct {
    PowerMode mode;
    float current_ma;       // Current consumption (mA)
    uint32_t wake_time_us;  // Wake-up time (microseconds)
    const char* description;
} PowerModeSpec;

static const PowerModeSpec power_modes[] = {
    {POWER_MODE_RUN,     25.0,    0,      "Normal operation @ 64MHz"},
    {POWER_MODE_SLEEP,   15.0,    10,     "CPU sleep, peripherals active"},
    {POWER_MODE_STOP,    2.0,     100,    "Stop mode, fast wake-up"},
    {POWER_MODE_STANDBY, 0.5,     1000,   "Standby, RTC active"},
    {POWER_MODE_SHUTDOWN, 0.01,   5000,   "Shutdown, minimal consumption"}
};
```

### Battery Backup System

```c
// RTC battery backup configuration
void RTC_Init(void) {
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 127;       // 32768 / (127+1) = 256Hz
    hrtc.Init.SynchPrediv = 255;        // 256 / (255+1) = 1Hz
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    
    HAL_RTC_Init(&hrtc);
    
    // Enable backup domain access
    HAL_PWR_EnableBkUpAccess();
    
    // Configure backup registers for critical data
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, BACKUP_MAGIC_NUMBER);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, system_config_checksum);
}

// Critical data backup
typedef struct {
    uint32_t magic_number;      // Validity marker
    uint32_t system_state;      // Last known system state
    uint32_t fault_flags;       // Active fault conditions
    uint32_t timestamp;         // Last update time
    uint32_t checksum;          // Data integrity
} BackupData;

void SaveCriticalData(void) {
    BackupData backup;
    
    backup.magic_number = BACKUP_MAGIC_NUMBER;
    backup.system_state = GetCurrentSystemState();
    backup.fault_flags = GetActiveFaults();
    backup.timestamp = HAL_GetTick();
    backup.checksum = CalculateBackupChecksum(&backup);
    
    // Write to backup registers
    uint32_t* data_ptr = (uint32_t*)&backup;
    for (int i = 0; i < sizeof(BackupData)/4; i++) {
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2 + i, data_ptr[i]);
    }
}
```

## GPIO Configuration

### Pin Assignment and Configuration

```c
// GPIO pin definitions based on hardware platform
#ifdef PLATFORM_NUCLEO
    // Nucleo board pin assignments
    #define CAN1_CS_Pin           GPIO_PIN_12
    #define CAN1_CS_GPIO_Port     GPIOB
    #define CAN1_INT_Pin          GPIO_PIN_13
    #define CAN1_INT_GPIO_Port    GPIOB
    #define CAN1_INT_EXTI_IRQn    EXTI15_10_IRQn
    
    #define LED1_Pin              GPIO_PIN_5
    #define LED1_GPIO_Port        GPIOA
    #define LED2_Pin              GPIO_PIN_0
    #define LED2_GPIO_Port        GPIOB
    
#elif defined(PLATFORM_MODBATT)
    // ModBatt hardware pin assignments  
    #define CAN1_CS_Pin           GPIO_PIN_4
    #define CAN1_CS_GPIO_Port     GPIOA
    #define CAN1_INT_Pin          GPIO_PIN_1
    #define CAN1_INT_GPIO_Port    GPIOB
    #define CAN1_INT_EXTI_IRQn    EXTI1_IRQn
    
    #define STATUS_LED_Pin        GPIO_PIN_8
    #define STATUS_LED_GPIO_Port  GPIOA
    #define FAULT_LED_Pin         GPIO_PIN_9
    #define FAULT_LED_GPIO_Port   GPIOA
    
    #define CONTACTOR_CTRL_Pin    GPIO_PIN_10
    #define CONTACTOR_CTRL_GPIO_Port GPIOA
#endif

// GPIO initialization function
void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Enable GPIO clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    // Configure CAN chip select pins (output, initially high)
    GPIO_InitStruct.Pin = CAN1_CS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(CAN1_CS_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(CAN1_CS_GPIO_Port, CAN1_CS_Pin, GPIO_PIN_SET);
    
    // Configure CAN interrupt pins (input with pull-up)
    GPIO_InitStruct.Pin = CAN1_INT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(CAN1_INT_GPIO_Port, &GPIO_InitStruct);
    
    // Configure LED pins (output, initially off)
    #ifdef PLATFORM_NUCLEO
        GPIO_InitStruct.Pin = LED1_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(LED1_GPIO_Port, &GPIO_InitStruct);
        
        GPIO_InitStruct.Pin = LED2_Pin;
        HAL_GPIO_Init(LED2_GPIO_Port, &GPIO_InitStruct);
    #endif
    
    // Configure contactor control (if available)
    #ifdef PLATFORM_MODBATT
        GPIO_InitStruct.Pin = CONTACTOR_CTRL_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(CONTACTOR_CTRL_GPIO_Port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(CONTACTOR_CTRL_GPIO_Port, CONTACTOR_CTRL_Pin, GPIO_PIN_RESET);
    #endif
    
    // Enable EXTI interrupts
    HAL_NVIC_SetPriority(CAN1_INT_EXTI_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN1_INT_EXTI_IRQn);
}
```

## External Components

### MCP2517FD CAN-FD Controller

```c
// MCP2517FD specifications and configuration
#define MCP2517FD_OSC_FREQ      40000000    // 40MHz crystal
#define MCP2517FD_SYSCLK_FREQ   40000000    // System clock = OSC
#define MCP2517FD_MAX_BAUDRATE  8000000     // Maximum CAN baudrate

// MCP2517FD CAN configuration for 500kbps
typedef struct {
    uint32_t sjw;           // Synchronization jump width
    uint32_t tseg1;         // Time segment 1
    uint32_t tseg2;         // Time segment 2
    uint32_t brp;           // Baudrate prescaler
} CAN_BitTiming;

// Calculate bit timing for 500kbps
// Target: 500kbps with 87.5% sample point
// Formula: Baudrate = SYSCLK / (BRP * (1 + TSEG1 + TSEG2))
// 500k = 40MHz / (BRP * 16) → BRP = 5
static const CAN_BitTiming can_500k_timing = {
    .sjw = 1,               // 1 time quantum
    .tseg1 = 13,            // 13 time quanta
    .tseg2 = 2,             // 2 time quanta
    .brp = 5                // Prescaler = 5
};
// Total: 1 + 13 + 2 = 16 TQ, Sample point = 14/16 = 87.5%

// MCP2517FD initialization sequence
bool MCP2517FD_Initialize(MCP2517FD_Interface* intf) {
    // Hardware reset
    HAL_GPIO_WritePin(intf->reset_port, intf->reset_pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(intf->reset_port, intf->reset_pin, GPIO_PIN_SET);
    HAL_Delay(10);
    
    // Verify device ID
    uint32_t device_id = MCP2517FD_ReadWord(intf, MCP2517FD_REG_DEVID);
    if ((device_id & 0xFF0) != 0x610) {
        return false;   // Wrong device ID
    }
    
    // Request configuration mode
    MCP2517FD_WriteByte(intf, MCP2517FD_REG_CON, 0x04);
    
    // Wait for configuration mode
    uint32_t timeout = 1000;
    while (timeout--) {
        uint8_t mode = MCP2517FD_ReadByte(intf, MCP2517FD_REG_CON + 1);
        if ((mode & 0x07) == 0x04) break;
        HAL_Delay(1);
    }
    
    if (timeout == 0) return false;
    
    // Configure bit timing
    MCP2517FD_WriteByte(intf, MCP2517FD_REG_NBTCFG, 
                       (can_500k_timing.sjw - 1) << 7 | 
                       (can_500k_timing.tseg2 - 1) << 4 |
                       (can_500k_timing.tseg1 - 1));
    MCP2517FD_WriteByte(intf, MCP2517FD_REG_NBTCFG + 1, can_500k_timing.brp - 1);
    
    // Configure FIFOs
    ConfigureMCP2517FD_FIFOs(intf);
    
    // Configure filters  
    ConfigureMCP2517FD_Filters(intf);
    
    // Request normal mode
    MCP2517FD_WriteByte(intf, MCP2517FD_REG_CON, 0x00);
    
    return true;
}
```

### Power Supply and Protection

```c
// Power supply monitoring and protection
typedef struct {
    float vin_voltage;          // Input voltage (V)
    float v3v3_voltage;         // 3.3V rail voltage (V)
    float v5v0_voltage;         // 5.0V rail voltage (V)
    float temperature;          // PCB temperature (°C)
    bool power_good;            // Overall power status
    uint32_t last_update;       // Last measurement time
} PowerStatus;

// ADC configuration for power monitoring
void ADC_Init(void) {
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ENABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.NbrOfConversion = 4;         // 4 channels
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    hadc1.Init.OversamplingMode = DISABLE;
    
    HAL_ADC_Init(&hadc1);
    
    // Configure ADC channels
    ADC_ChannelConfTypeDef sConfig = {0};
    
    // Channel 1: VIN voltage divider
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    // Channel 2: 3.3V rail monitor
    sConfig.Channel = ADC_CHANNEL_2;
    sConfig.Rank = ADC_REGULAR_RANK_2;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    // Channel 3: 5.0V rail monitor  
    sConfig.Channel = ADC_CHANNEL_3;
    sConfig.Rank = ADC_REGULAR_RANK_3;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    // Channel 4: Temperature sensor
    sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
    sConfig.Rank = ADC_REGULAR_RANK_4;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

// Power monitoring function
void UpdatePowerStatus(PowerStatus* status) {
    uint16_t adc_values[4];
    
    // Read ADC values
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_values, 4);
    
    // Convert to voltages
    status->vin_voltage = (adc_values[0] * 3.3f / 4095.0f) * VIN_SCALE_FACTOR;
    status->v3v3_voltage = adc_values[1] * 3.3f / 4095.0f;
    status->v5v0_voltage = (adc_values[2] * 3.3f / 4095.0f) * V5_SCALE_FACTOR;
    
    // Convert temperature (internal sensor)
    int32_t temp_cal1 = *((uint16_t*)0x1FFF75A8);  // 30°C calibration
    int32_t temp_cal2 = *((uint16_t*)0x1FFF75CA);  // 130°C calibration
    status->temperature = ((int32_t)adc_values[3] - temp_cal1) * 100.0f / 
                         (temp_cal2 - temp_cal1) + 30.0f;
    
    // Check power good status
    status->power_good = (status->vin_voltage > 8.0f && status->vin_voltage < 16.0f) &&
                        (status->v3v3_voltage > 3.0f && status->v3v3_voltage < 3.6f) &&
                        (status->v5v0_voltage > 4.5f && status->v5v0_voltage < 5.5f) &&
                        (status->temperature > -20.0f && status->temperature < 85.0f);
    
    status->last_update = HAL_GetTick();
}
```

This comprehensive hardware documentation provides the foundation for understanding the STM32WB55 platform capabilities and configuration for the Pack Controller EEPROM firmware.