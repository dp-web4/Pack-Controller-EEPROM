 /**************************************************************************************************************
 * @file           : can_frm_vcu.h                                                 P A C K   C O N T R O L L E R
 * @brief          : Modbatt CAN frame structures for VCU <-> Pack Controller
 ***************************************************************************************************************
 * Copyright (C) 2023-2024 Modular Battery Technologies, Inc.
 * US Patents 11,380,942; 11,469,470; 11,575,270; others. All rights reserved
 **************************************************************************************************************/
#ifndef INC_CAN_FRM_VCU_H_
#define INC_CAN_FRM_VCU_H_
/*
                                          =====================
                                          CAN FRAME STRUCTURES
                                          =====================
*/


typedef struct {                                // 0x400 VCU_COMMAND - 8 bytes
                                                // Bits   Factor     Offset   Min     Max           Unit
  uint32_t vcu_contactor_ctrl             : 2;  // 00-01  1          0        0       3
  uint32_t vcu_cell_balance_ctrl          : 2;  // 02-03  1          0        0       3
  uint32_t vcu_hv_bus_actv_iso_en         : 2;  // 04-05  1          0        0       3
  uint32_t UNUSED_06_15                   : 10; // 06-15
  uint32_t vcu_hv_bus_voltage             : 16; // 16-31  0.015      0        0       983.025       Volts             Inverter hv bus voltage
  uint32_t UNUSED_32_63                   : 32; // 32-63
}CANFRM_0x400_VCU_COMMAND;

typedef struct {                                // 0x401 VCU_TIME - 8 bytes
  uint64_t vcu_time                           : 64; // 64 bit time_t
}CANFRM_0x401_VCU_TIME;


typedef struct {                               // 0x402 VCU_READ_EEPROM - 8 bytes
  uint32_t bms_eeprom_data_register      : 8;  // eeprom data register
  uint32_t UNUSED_8_31                   : 24; // UNUSED bits 08-31
  uint32_t bms_eeprom_data               : 32; // eeprom data
}CANFRM_0x402_VCU_READ_EEPROM;

typedef struct {                               // 0x403 VCU_WRITE_EEPROM - 8 bytes
  uint32_t bms_eeprom_data_register      : 8;  // eeprom data register
  uint32_t UNUSED_8_31                   : 24; // UNUSED bits 08-31
  uint32_t bms_eeprom_data               : 32; // eeprom data
}CANFRM_0x403_VCU_WRITE_EEPROM;



typedef struct {                                // 0x410 BMS_STATE - 8 bytes
                                                // Bits   Factor     Offset   Min     Max           Unit
  uint32_t bms_state                      : 2;  // 00-01                                                              BMS State 00=OFF (command all module relays (both mechanical & FET) off, 01=STDBY (command mechanical ON, FET OFF on all modules), 10=PRCHG ( command mechanical ON for all modules, FET on for one module), 11=ON (command both relays ON for all modules)
  uint32_t bms_soh                        : 8;  // 02-09
  uint32_t bms_status                     : 2;  // 10-11                                                              BMS Status 00=off, 01=pack full, 10 = pack empty, 11=pack normal
  uint32_t bms_cell_balance_status        : 1;  // 12                                                                 Flag to indicate whether cells have been balanced
  uint32_t bms_cell_balance_active        : 1;  // 13                                                                 Flag to indicate that cell balancing is in progress
  uint32_t bms_module_off                 : 1;  // 14                                                                 Flag to indicate one or more modules in a fault condition
  uint32_t UNUSED_16                      : 1;  // 15
  uint32_t bms_total_mod_cnt              : 8;  // 16-23                                                              Total number of modules in the pack
  uint32_t bms_active_mod_cnt             : 8;  // 24-31                                                              Number of active modules in the pack
  uint32_t UNUSED_31_63                   : 32; // 31-63
 }CANFRM_0x410_BMS_STATE;



 /*
    VCU relay command to BMS are as follows:
    0: OFF (command all module relays (both mechanical & FET) off)
    1: STDBY (command mechanical ON, FET OFF on all modules)
    2: PRCHG ( command mechanical ON for all modules, FET on for one module)
    3: ON (command both relays ON for all modules).

    Status can be one of 4:
    0: charge prohibited/discharge prohibited - pack not in a condition to source or receive current, for any reason, in any state, but always the case in any state except ON
    1: charge allowed/discharge prohibited - pack is empty, state is ON
    2: charge allowed/discharge allowed - pack in a normal state, state is ON
    3: charge prohibited/discharge allowed - pack is full, state is ON
  */


typedef struct {                                // 0x421 (was 0xCF090F3) BMS_DATA_1 - 8 bytes
                                                // Bits   Factor     Offset   Min     Max           Unit
  uint32_t UNUSED_00_31                   : 32; // 00-31
  uint32_t bms_pack_voltage               : 16; // 32-47  0.05       0        0       3276.75       Volts             The voltage level of the pack
  uint32_t bms_pack_current               : 16; // 48-63  0.05       -1600    -1600   1676.75       Amps              The current in or out of the pack. A positive value represents current into (charging) the energy storage system.  A negative value represents current out of (discharging) the energy storage system.
 }CANFRM_0x421_BMS_DATA_1;


typedef struct {                                // 0x422 (was 0xCF091F3) BMS_DATA_2 - 8 bytes
                                                // Bits   Factor     Offset   Min     Max           Unit
  uint32_t bms_soc                        : 16; // 00-15  0.0015625  0        0       102.3984375   %                 State of charge
  uint32_t bms_high_cell_volt             : 16; // 16-31  0.001      0        0       65.535        Volts             Highest cell voltage reported by any cell
  uint32_t bms_low_cell_volt              : 16; // 32-47  0.001      0        0       65.535        Volts             Lowest cell voltage reported by any cell
  uint32_t bms_avg_cell_volt              : 16; // 32-39  0.001      0        0       65.535        Volts             The average cell voltage
 }CANFRM_0x422_BMS_DATA_2;

typedef struct {                                // 0x423 (was 0xCF092F3) BMS_DATA_3 - 8 bytes
                                                // Bits   Factor     Offset   Min     Max           Unit
  uint32_t bms_high_cell_temp             : 16; // 00-15  0.03125    -273     0       1774.96875    Degrees Celcius   Highest cell temperature reported by any cell
  uint32_t bms_low_cell_temp              : 16; // 16-31  0.03125    -273     0       1774.96875    Degrees Celcius   Lowest cell temperature reported by any cell
  uint32_t bms_avg_cell_temp              : 16; // 32-47  0.03125    -273     0       1774.96875    Degrees Celcius   The average temperature level of all cells
  uint32_t UNUSED_48_63                   : 16; // 48-63
 }CANFRM_0x423_BMS_DATA_3;


 typedef struct {                                // 0x425 (was 0xCF094F3) BMS_DATA_5 - 8 bytes
                                                 // Bits   Factor     Offset   Min     Max           Unit
   uint32_t bms_dischage_limit             : 16; // 00-15  0.05       -1600    -1600   1676.75       Amps             The maximum permissible current flow out of the High Voltage Energy Storage System
   uint32_t bms_charge_limit               : 16; // 16-31  0.05       -1600    -1600   1676.75       Amps             The maximum permissible current flow in to the High Voltage Energy Storage System
   uint32_t bms_charge_end_voltage_limit   : 16; // 32-47  0.05       0        0       3276.75       Volts            The maximum permissable voltage at end of charge
   uint32_t UNUSED_48_63                   : 16; // 48-63
  }CANFRM_0x425_BMS_DATA_5;


typedef struct {                                // 0x428 (was 0xCF10AF3) BMS_DATA_8 - 8 bytes
                                                // Bits   Factor     Offset   Min     Max           Unit
  uint32_t bms_max_volt_mod               : 8;  // 00-07  1          0        0       255                             Module number with highest cell voltage
  uint32_t bms_max_volt_cell              : 8;  // 08-15  1          0        0       255                             The number of the cell with highest voltage, within the module
  uint32_t bms_min_volt_mod               : 8;  // 16-23  1          0        0       255                             Module number with lowest cell voltage
  uint32_t bms_min_volt_cell              : 8;  // 24-31  1          0        0       255                             The number of the cell with lowest voltage,  within the module
  uint32_t UNUSED_32_63                   : 32; // 48-63
 }CANFRM_0x428_BMS_DATA_8;


typedef struct {                                // 0x429 (was 0xCF10BF3) BMS_DATA_9 - 8 bytes
                                                // Bits   Factor     Offset   Min     Max           Unit
  uint32_t bms_max_temp_mod               : 8;  // 00-07  1          0        0       255                             Module number with highest cell temperature
  uint32_t bms_max_temp_cell              : 8;  // 08-15  1          0        0       255                             The number of the cell with highest temperature, within the module
  uint32_t bms_min_temp_mod               : 8;  // 16-23  1          0        0       255                             Module number with lowest cell temperature
  uint32_t bms_min_temp_cell              : 8;  // 24-31  1          0        0       255                             The number of the cell with lowest temperature, within the module
  uint32_t UNUSED_32_63                   : 32; // 32-63
 }CANFRM_0x429_BMS_DATA_9;


typedef struct {                                // 0x430 (was 0xCF10CF3) BMS_DATA_10 - 8 bytes
                                                // Bits   Factor     Offset   Min     Max           Unit
  uint32_t bms_hv_bus_actv_iso            : 16; // 00-15  0.01       0        0       6553.5        Ohm/V             High-Voltage Bus Active Isolation Test Results
  uint32_t UNUSED_16_31                   : 16; // 16-31
  uint32_t UNUSED_32_63                   : 32; // 32-63
 }CANFRM_0x430_BMS_DATA_10;

 typedef struct {                               // 0x440 BMS_TIME_REQUEST - 8 bytes
   uint32_t UNUSED_00_31                  : 32; // UNUSED bits 00-31
   uint32_t UNUSED_32_63                  : 32; // UNUSED bits 32-63
 }CANFRM_0x440_BMS_TIME_REQUEST;


 typedef struct {                               // 0x441 BMS_EEPROM_DATA - 8 bytes
   uint32_t bms_eeprom_data_register      : 8;  // eeprom data register
   uint32_t UNUSED_8_31                   : 24; // UNUSED bits 08-31
   uint32_t bms_eeprom_data               : 32; // eeprom data
 }CANFRM_0x441_BMS_EEPROM_DATA;



#endif /* INC_CAN_FRM_VCU_H_ */
