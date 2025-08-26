/***************************************************************************************************************
 * @file           : can_pkt_bms_diag.h
 * @brief          : Modbatt CAN packet structures
 ***************************************************************************************************************
 * Copyright (C) 2023-2024 Modular Battery Technologies, Inc.
 * US Patents 11,380,942; 11,469,470; 11,575,270; others. All rights reserved
 **************************************************************************************************************/
#ifndef INC_CAN_FRM_BMS_DIAG_H_
#define INC_CAN_FRM_BMS_DIAG_H_
/*
                                          =====================
                                          CAN PACKET STRUCTURES
                                          =====================
*/
typedef struct {                                // 0x220 BMS_STATUS - 8 bytes
                                                // Bits   Factor  Offset   Min     Max       Unit
  uint32_t UNUSED_0_3                     : 4;  // 00-03
  uint32_t BMS_Battery_State              : 4;  // 04-07
  uint32_t BMS_Pack_Voltage               : 14; // 08-21  0.1     0        0       1638.3    Volts
  uint32_t UNUSED_22_23                   : 2;  // 22-23 
  uint32_t BMS_Balance_No_Cmd             : 1;  // 24
  uint32_t BMS_Balance_Cell_Voltage_Low   : 1;  // 25
  uint32_t BMS_Balance_Cell_Voltage_High  : 1;  // 26
  uint32_t BMS_Balance_Temp_Low           : 1;  // 27
  uint32_t BMS_Balance_Temp_High          : 1;  // 28 
  uint32_t BMS_Balance_Wrong_State        : 1;  // 29
  uint32_t UNUSED_30_39                   : 10; // 30-39
  uint32_t BMS_Balancing_Status           : 1;  // 40
  uint32_t BMS_Display_SOC                : 7;  // 41-47  1       0        0       100       Percent
  uint32_t UNUSED_48                      : 1;  // 48
  uint32_t BMS_Real_SOC                   : 7;  // 49-55  1       0        0       100       Percent
  uint32_t UNUSED_56                      : 1;  // 56
  uint32_t BMS_SOH                        : 7;  // 57-63  1       0        0       100       Percent
}CANPKT_0x220_BMS_STATUS;

typedef struct {                                // 0x221 BMS_FAULT - 8 bytes
  uint32_t BMS_Fault_Cell_Temp_High       : 1;  // 00
  uint32_t BMS_Fault_Cell_Temp_Low        : 1;   // 01
  uint32_t BMS_Fault_Cell_T_High_Chg      : 1;  // 02
  uint32_t BMS_Fault_Cell_T_Low_Chg       : 1;  // 03
  uint32_t BMS_Fault_Cell_Voltage_High    : 1;  // 04
  uint32_t BMS_Fault_Cell_Voltage_Low     : 1;  // 05
  uint32_t BMS_Fault_Current_Chg          : 1;  // 06
  uint32_t BMS_Fault_Module_Comm          : 1;  // 07
  uint32_t BMS_Fault_Current_DChg         : 1;  // 08
  uint32_t BMS_Fault_HVIL                 : 1;  // 09
  uint32_t BMS_Fault_PreCharge            : 1;  // 10
  uint32_t BMS_Fault_VCU_Comm             : 1;  // 11
  uint32_t UNUSED_12_23                   : 12; // 12-23
  uint32_t BMS_Warning_Cell_Temp_High     : 1;  // 24
  uint32_t BMS_Warning_Cell_Temp_Low      : 1;  // 25
  uint32_t BMS_Warn_Cell_T_High_Chg       : 1;  // 26
  uint32_t BMS_Warn_Cell_T_Low_Chg        : 1;  // 27
  uint32_t BMS_Warning_Cell_Voltage_High  : 1;  // 28
  uint32_t BMS_Warning_Cell_Voltage_Low   : 1;  // 29
  uint32_t BMS_Warning_Thermistor_Fail    : 1;  // 30
  uint32_t BMS_Warning_Current_Chg        : 1;  // 31
  uint32_t BMS_Warning_Current_DChg       : 1;  // 32
  uint32_t BMS_Warning_Module_Comm        : 1;  // 33
  uint32_t BMS_Warn_Cell_Delta            : 1;  // 34
  uint32_t UNUSED_35_63                   : 29; // 35-39
}CANPKT_0x221_BMS_FAULT;
    
typedef struct {                                // 0x222 BMS_CELL_DATA - 8 bytes
                                                // Bits   Factor  Offset   Min     Max       Unit
  uint32_t BMS_Min_Cell_Voltage           : 13; // 00-12  0.001   0        0      8.191      Volts
  uint32_t BMS_Max_Cell_Voltage           : 13; // 13-25  0.001   0        0      8.191      Volts
  uint32_t BMS_Avg_Cell_Voltage           : 13; // 26-38  0.001   0        0      8.191      Volts
  uint32_t UNUSED_39                      : 1;  // 39
  uint32_t BMS_Min_Cell_Temp              : 8;  // 40-47  1      -40       -40    215        Degrees Celcius
  uint32_t BMS_Max_Cell_Temp              : 8;  // 48-55  1      -40       -40    215        Degrees Celcius
  uint32_t BMS_Avg_Cell_Temp              : 8;  // 56-63  1      -40       -40    215        Degrees Celcius
}CANPKT_0x222_BMS_CELL_DATA;
    
typedef struct {                                // 0x223 BMS_IO - 8 bytes
                                                // Bits   Factor  Offset   Min     Max       Unit
  uint32_t BMS_Input_1_State              : 1;  // 00
  uint32_t BMS_Input_2_State              : 1;  // 01
  uint32_t BMS_Input_3_State              : 1;  // 02
  uint32_t BMS_Input_4_State              : 1;  // 03
  uint32_t BMS_Input_5_State              : 1;  // 04
  uint32_t BMS_Input_6_State              : 1;  // 05
  uint32_t BMS_Input_7_State              : 1;  // 06
  uint32_t BMS_Input_8_State              : 1;  // 07
  uint32_t BMS_Contactor_HS_State         : 1;  // 08
  uint32_t BMS_Contactor_LS_State         : 1;  // 09
  uint32_t BMS_Contactor_PC_State         : 1;  // 10
  uint32_t UNUSED_11                      : 1;  // 11
  uint32_t BMS_HVIL_State                 : 1;  // 12
  uint32_t UNUSED13_15                    : 3;  // 13-15
  uint32_t BMS_Output_1_State             : 1;  // 16
  uint32_t BMS_Output_2_State             : 1;  // 17
  uint32_t BMS_Output_3_State             : 1;  // 18
  uint32_t BMS_Output_4_State             : 1;  // 19
  uint32_t UNUSED_20_31                   : 12; // 20-31
  uint32_t UNUSED_32_63                   : 32; // 32-63
}CANPKT_0x223_BMS_IO;

typedef struct {                                // 0x224 BMS_LIMITS - 8 bytes
                                                // Bits   Factor  Offset   Min     Max       Unit
  uint32_t BMS_Max_Chg_Current            : 11; // 00-10  1A      0        0       2047      Amps
  uint32_t UNUSED_11_15                   : 5;   // 11-15
  uint32_t BMS_Max_Dchg_Current           : 11; // 16-26  1A      0        0       2047      Amps
  uint32_t UNUSED_27_31                   : 5;  // 27-31
  uint32_t UNUSED_32_63                   : 32; // 32-63
}CANPKT_0x224_BMS_LIMITS;

typedef struct {                                // 0x225 BMS_MOD_DATA_1 - 8 bytes
                                                // Bits   Factor  Offset   Min     Max       Unit
  uint32_t BMS_Module_Index               : 8;  // 00-07
  uint32_t BMS_Cell_Voltage_1             : 8;  // 08-15  0.01    2        0      4.55      Volts
  uint32_t BMS_Cell_Voltage_2             : 8;  // 16-23  0.01    2        0      4.55      Volts
  uint32_t BMS_Cell_Voltage_3             : 8;  // 24-31  0.01    2        0      4.55      Volts
  uint32_t BMS_Cell_Voltage_4             : 8;  // 32-39  0.01    2        0      4.55      Volts
  uint32_t BMS_Cell_Voltage_5             : 8;  // 40-47  0.01    2        0      4.55      Volts
  uint32_t BMS_Cell_Voltage_6             : 8;  // 48-55  0.01    2        0      4.55      Volts
  uint32_t BMS_Cell_Voltage_7             : 8;  // 56-63  0.01    2        0      4.55      Volts
}CANPKT_0x225_BMS_MOD_DATA_1;

typedef struct {                                // 0x226 BMS_MOD_DATA_2 - 8 bytes
                                                // Bits   Factor  Offset   Min     Max       Unit
  uint32_t BMS_Module_Index               : 8;  // 00-07
  uint32_t BMS_Cell_Voltage_8             : 8;  // 08-15  0.01    2        0      4.55      Volts
  uint32_t BMS_Cell_Voltage_9             : 8;  // 16-23  0.01    2        0      4.55      Volts
  uint32_t BMS_Cell_Voltage_10            : 8;  // 24-31  0.01    2        0      4.55      Volts
  uint32_t BMS_Cell_Voltage_11            : 8;  // 32-39  0.01    2        0      4.55      Volts
  uint32_t BMS_Cell_Voltage_12            : 8;  // 40-47  0.01    2        0      4.55      Volts
  uint32_t BMS_Cell_Voltage_13            : 8;  // 48-55  0.01    2        0      4.55      Volts
  uint32_t BMS_Cell_Voltage_14            : 8;  // 56-63  0.01    2        0      4.55      Volts
}CANPKT_0x226_BMS_MOD_DATA_2;

typedef struct {                                // 0x227 BMS_MOD_DATA_3 - 8 bytes
                                                // Bits   Factor  Offset   Min     Max       Unit
  uint32_t BMS_Module_Index               : 8;  // 00-07
  uint32_t BMS_Mod_Temp_1                 : 8;  // 08-15  1      -40       -40    215        Degrees Celcius
  uint32_t BMS_Mod_Temp_2                 : 8;  // 16-23  1      -40       -40    215        Degrees Celcius
  uint32_t BMS_Mod_Temp_3                 : 8;  // 24-31  1      -40       -40    215        Degrees Celcius
  uint32_t BMS_Mod_Temp_4                 : 8;  // 32-39  1      -40       -40    215        Degrees Celcius
  uint32_t BMS_Mod_Temp_5                 : 8;  // 40-47  1      -40       -40    215        Degrees Celcius
  uint32_t BMS_Mod_Temp_6                 : 8;  // 48-55  1      -40       -40    215        Degrees Celcius
  uint32_t UNUSED_56_63                   : 8;  // 56-63
}CANPKT_0x227_BMS_MOD_DATA_3;

typedef struct {                                // 0x228 BMS_MOD_DATA_4 - 8 bytes
                                                // Bits   Factor  Offset   Min     Max       Unit
  uint32_t BMS_Module_Index               : 8;  // 00-07
  uint32_t BMS_Cell_Balancing_1           : 1;  // 08
  uint32_t BMS_Cell_Balancing_2           : 1;  // 09
  uint32_t BMS_Cell_Balancing_3           : 1;  // 10
  uint32_t BMS_Cell_Balancing_4           : 1;  // 11
  uint32_t BMS_Cell_Balancing_5           : 1;  // 12
  uint32_t BMS_Cell_Balancing_6           : 1;  // 13
  uint32_t BMS_Cell_Balancing_7           : 1;  // 14
  uint32_t BMS_Cell_Balancing_8           : 1;  // 15
  uint32_t BMS_Cell_Balancing_9           : 1;  // 16
  uint32_t BMS_Cell_Balancing_10          : 1;  // 17
  uint32_t BMS_Cell_Balancing_11          : 1;  // 18
  uint32_t BMS_Cell_Balancing_12          : 1;  // 19
  uint32_t BMS_Cell_Balancing_13          : 1;  // 20
  uint32_t BMS_Cell_Balancing_14          : 1;  // 21
  uint32_t UNUSED_22_31                   : 10; // 22-31
  uint32_t UNUSED_32_63                   : 32; // 32-63
}CANPKT_0x228_BMS_MOD_DATA_4;


/*

  uint32_t BMS_Cell_Temp_9                : 8;  // 16-23  1      -40       -40    215        Degrees Celcius
  uint32_t BMS_Cell_Temp_10               : 8;  // 24-31  1      -40       -40    215        Degrees Celcius
  uint32_t BMS_Cell_Temp_11               : 8;  // 32-39  1      -40       -40    215        Degrees Celcius
  uint32_t BMS_Cell_Temp_12               : 8;  // 40-47  1      -40       -40    215        Degrees Celcius



typedef struct {                                // 0x200 BMS_TEMPLATE - 8 bytes
                                                // Bits   Factor  Offset   Min     Max       Unit
  uint32_t  : 1;  // 00
  uint32_t  : 1   // 01
  uint32_t  : 1;  // 02
  uint32_t  : 1;  // 03
  uint32_t  : 1;  // 04
  uint32_t  : 1;  // 05
  uint32_t  : 1;  // 06
  uint32_t  : 1;  // 07
  uint32_t  : 1;  // 08
  uint32_t  : 1;  // 09
  uint32_t  : 1;  // 10
  uint32_t  : 1;  // 11
  uint32_t  : 1;  // 12
  uint32_t  : 1;  // 13
  uint32_t  : 1;  // 14
  uint32_t  : 1;  // 15
  uint32_t  : 1;  // 16
  uint32_t  : 1;  // 17
  uint32_t  : 1;  // 18
  uint32_t  : 1;  // 19
  uint32_t  : 1;  // 20
  uint32_t  : 1;  // 21
  uint32_t  : 1;  // 22
  uint32_t  : 1;  // 23
  uint32_t  : 1;  // 24
  uint32_t  : 1;  // 25
  uint32_t  : 1;  // 26
  uint32_t  : 1;  // 27
  uint32_t  : 1;  // 28
  uint32_t  : 1;  // 29
  uint32_t  : 1;  // 30
  uint32_t  : 1;  // 31
  uint32_t  : 1;  // 32
  uint32_t  : 1;  // 33
  uint32_t  : 1;  // 34
  uint32_t  : 1;  // 35
  uint32_t  : 1;  // 36
  uint32_t  : 1;  // 37
  uint32_t  : 1;  // 38
  uint32_t  : 1;  // 39
  uint32_t  : 1;  // 40
  uint32_t  : 1;  // 41
  uint32_t  : 1;  // 42
  uint32_t  : 1;  // 43
  uint32_t  : 1;  // 44
  uint32_t  : 1;  // 45
  uint32_t  : 1;  // 46
  uint32_t  : 1;  // 47
  uint32_t  : 1;  // 48
  uint32_t  : 1;  // 49
  uint32_t  : 1;  // 50
  uint32_t  : 1;  // 51
  uint32_t  : 1;  // 52
  uint32_t  : 1;  // 53
  uint32_t  : 1;  // 54
  uint32_t  : 1;  // 55
  uint32_t  : 1;  // 56
  uint32_t  : 1;  // 57
  uint32_t  : 1;  // 58
  uint32_t  : 1;  // 59
  uint32_t  : 1;  // 60
  uint32_t  : 1;  // 60
  uint32_t  : 1;  // 61
  uint32_t  : 1;  // 62
  uint32_t  : 1;  // 63
}CANPKT_0x_BMS_TEMPLATE;
*/


#endif /* INC_CAN_FRM_BMS_DIAG_H_ */
