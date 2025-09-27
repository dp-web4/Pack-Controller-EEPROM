/*
 * eeprom_data.h
 *
 *  Created on: 16 May 2024
 *      Author: Dean
 */

#ifndef INC_EEPROM_DATA_H_
#define INC_EEPROM_DATA_H_


#define MAGIC1                  0x50616C61
#define MAGIC2                  0x746F7621

#define EE_MAGIC1               1
#define EE_MAGIC2               2
#define EE_PACK_CONTROLLER_ID   3

// SD Card Frame Counter EEPROM addresses
#define EE_FRAME_COUNTER_BASE   4      // Base address for frame counter
#define EE_FRAME_COUNTER_L      4      // Low byte of frame counter
#define EE_FRAME_COUNTER_M      5      // Middle byte of frame counter
#define EE_FRAME_COUNTER_H      6      // High byte of frame counter
#define EE_FRAME_CHECKSUM       7      // Checksum for frame counter

// Frame counter wear leveling - use 8 slots
#define EE_FRAME_SLOTS          8      // Number of wear leveling slots
#define EE_FRAME_SLOT_SIZE      4      // Bytes per slot (3 bytes + checksum)
#define EE_FRAME_SLOT_BASE      8      // Start of wear leveling area
#define EE_FRAME_WRITES_PER_SLOT 10000 // Writes before rotating slot

/*
 * Pack Controller Platform ID  8   //nucleo =0 , modbatt =1
Pack Unique ID  32
Pack Part Number  8
Pack Manufacturer ID    8                                                                       Pack 16-Bit Firmware Build Number 16
Pack Controller ID  8
Pack Controller Debug Level 8

Module UID 32
Module Part ID  8
Module Manufacturer ID 8
Module 16-Bit Firmware Build Number 16
Module Hardware Capability  16
Module Max End of Charge Voltage  16
Module Discharge threshold  16
Module Charge threshold   16

#define PACK_ID    0
#define HW_VER     0
#define FW_VER     0

#define MAX_CELLS_PER_MODULE   192
#define MAX_MODULES_PER_PACK   32

#define MCU_STATUS_INTERVAL       2000      // Module status request interval - 2 seconds
#define MCU_STATE_TX_INTERVAL     1000      // Module state retry interval - 1 seconds
#define MCU_ET_TIMEOUT            4000      // Module timeout 4 seconds
#define VCU_ET_WARNING            600       // VCU warning 0.6 seconds
#define VCU_ET_TIMEOUT            1200      // VCU timeout 1.2 seconds

#define PACK_CURRENT_BASE         -1600     // amps
#define PACK_CURRENT_FACTOR       0.05      // amps
#define MODULE_VOLTAGE_BASE       0         // Volts
#define MODULE_VOLTAGE_FACTOR     0.015     // Volts
#define CELL_VOLTAGE_BASE         0         // Volts
#define CELL_VOLTAGE_FACTOR       0.001     // Volts
#define CELL_TOTAL_VOLTAGE_FACTOR 0.015     // Volts
#define MODULE_CURRENT_BASE       -655.36   // Amps
#define MODULE_CURRENT_FACTOR     0.02      // Amps
#define TEMPERATURE_BASE          -55.35    // Degrees C
#define TEMPERATURE_FACTOR        0.01      // Degrees C
#define PERCENTAGE_BASE           0         // Percent
#define PERCENTAGE_FACTOR         0.5       // Percent

#define MODULE_CURRENT_TOLERANCE  0.3       // +/- tolerance

#define PACK_EMPTY_SOC_THRESHOLD  5         // Percent
#define PACK_FULL_SOC_THRESHOLD   95        // Percent

#define MODULE_MAX_CHARGE_A       10.0      // Amps
#define MODULE_MAX_DISCHARGE_A   -42.0      // Amps

#define VCU_CURRENT_BASE            -1600       // amps
#define VCU_CURRENT_FACTOR          0.05        // amps
#define VCU_VOLTAGE_BASE            0           // volts
#define VCU_VOLTAGE_FACTOR          0.05        // volts
#define VCU_TEMPERATURE_BASE        -273        // degrees C
#define VCU_TEMPERATURE_FACTOR      0.03125     // degrees C
#define VCU_CELL_VOLTAGE_BASE       0           // volts
#define VCU_CELL_VOLTAGE_FACTOR     0.001       // volts
#define VCU_SOC_PERCENTAGE_BASE     0           // %
#define VCU_SOC_PERCENTAGE_FACTOR   0.0015625   // %
#define VCU_SOH_PERCENTAGE_BASE     0           // %
#define VCU_SOH_PERCENTAGE_FACTOR   0.4         // %
#define VCU_ISOLATION_FACTOR        0.001       // Ohms/Volt
 */


#endif /* INC_EEPROM_DATA_H_ */
