 /**************************************************************************************************************
 * @file           : bms.h                                                         P A C K   C O N T R O L L E R
 * @brief          : Structures for BMS System
 ***************************************************************************************************************
 * Copyright (C) 2023-2024 Modular Battery Technologies, Inc.
 * US Patents 11,380,942; 11,469,470; 11,575,270; others. All rights reserved
 **************************************************************************************************************/

#ifndef INC_BMS_H_
#define INC_BMS_H_

#include "stdbool.h"

#define MAX_CELLS_PER_MODULE   192
#define MAX_MODULES_PER_PACK   32


typedef enum {
  moduleOff       = 0,   // both relays off
  moduleStandby   = 1,   // mechanical on, FET off on all modules (connected to HV bus via 33K resistor)
  modulePrecharge = 2,   // Pulse FET relay
  moduleOn        = 3    // both relays on for all modules.
}moduleState;

typedef enum {
  packOff         = 0,   // both relays off
  packStandby     = 1,   // mechanical on, FET off on all modules
  packPrecharge   = 2,   // mechanical on for all modules, FET on for one module (you select the highest voltage one, we can discuss further refinements)
  packOn          = 3    // both relays on for all modules.
}packState;


typedef enum {
  packStatusOff    = 0,  // charge prohibited/discharge prohibited - pack not in a condition to source or receive current, for any reason, in any state, but always the case in any state except ON
  packStatusEmpty  = 1,  // charge allowed/discharge prohibited - pack is empty, state is ON
  packStatusNormal = 2,  // charge allowed/discharge allowed - pack in a normal state, state is ON
  packStatusFull   = 3   // charge prohibited/discharge allowed - pack is full, state is ON
}bmsStatus;

typedef enum {
  commandIssued   = 0,
  commandActive   = 1,
  commandError    = 2
}commandStatus;

typedef enum {
  stageSelectModule   = 0,
  stagePowerOnModule  = 1,
  stageIdle           = 2
}powerUpStage;

enum  ledMode{
  ledOn,
  ledOff,
  ledBlinkX,
  ledBlink1,
  ledBlink2
};

typedef enum {
  packMode = 0,
  dmcMode = 1
}controlMode;


typedef struct {
  uint32_t    ticks;
  uint32_t    overflows;
}lastContact;


typedef struct {
  uint16_t    voltage;
  uint16_t    temp;       // cell 16 bit temperature
  uint8_t     soc;
  uint8_t     soh;
}batteryCell;

typedef struct {
  uint16_t     firstModule;
}errorCounts;

typedef struct {
  moduleState commandedState;
  commandStatus commandStatus;
}command;

typedef struct {
  uint8_t     commsError      : 1;    // comms timeout
  uint8_t     hwIncompatible  : 1;
  uint8_t     overCurrent     : 1;
  uint8_t     overTemperature : 1;
  uint8_t     overVoltage     : 1;
  uint8_t     unused_bit5     : 1;
  uint8_t     unused_bit6     : 1;
  uint8_t     unused_bit7     : 1;
}faultCode;


typedef struct {
  uint8_t       firstModuleId;
  powerUpStage  powerStage;
}powerStatus;

typedef struct {
  uint8_t     mfgId;
  uint8_t     partId;  // module part ID
  uint32_t    uniqueId;
  uint8_t     moduleId;
  uint16_t    fwVersion;
  uint16_t    hwVersion;
  uint16_t    maxChargeA;
  uint16_t    maxDischargeA;
  uint16_t    maxChargeEndV;
  uint16_t    mmv;
  uint16_t    mmc;
  uint16_t    cellHiTemp;     // highest temperature
  uint16_t    cellLoTemp;     // lowest temperature
  uint16_t    cellAvgTemp;    // average temperature
  uint16_t    cellHiVolt;     // highest voltage
  uint16_t    cellLoVolt;     // lowest voltage
  uint16_t    cellAvgVolt;    // average voltage
  uint16_t    cellTotalVolt;  // sum of the cell voltages
  uint8_t     status;
  moduleState currentState;
  moduleState nextState;
  command     command;
  uint8_t     soc;
  uint8_t     soh;
  uint8_t     cellCount;
  batteryCell cell[MAX_CELLS_PER_MODULE];
  lastContact lastContact;
  lastContact lastTransmit;
  bool        hardwarePending;
  bool        statusPending;
  faultCode   faultCode;
  uint8_t     consecutiveTimeouts;
  uint8_t     statusMessagesReceived;  // Bitmask: bit0=Status1, bit1=Status2, bit2=Status3
  bool        isRegistered;            // Module currently registered (vs just known)
}batteryModule;


typedef struct {
  uint8_t     id;
  uint8_t     mfgId;
  uint8_t     partId;  // module part ID
  uint32_t    uniqueId;
  uint8_t     hwVersion;
  uint8_t     fwVersion;
  uint16_t    vcuCanOffset;
  uint16_t    voltage;
  uint32_t    current;
  uint8_t     moduleCount;        // Can be deprecated - kept for compatibility
  uint8_t     cellBalanceActive;
  uint8_t     cellBalanceStatus;
  uint8_t     activeModules;      // Will be repurposed for currently registered count
  uint8_t     faultedModules;
  uint8_t     totalModules;       // Count of all modules ever seen (non-empty slots)
  powerStatus powerStatus;
  uint16_t    totalCells;
  uint16_t    cellHiTemp;     // highest temperature
  uint8_t     modCellHiTemp;  // module with the highest cell temp
  uint16_t    cellLoTemp;     // lowest temperature
  uint8_t     modCellLoTemp;  // module with the lowest cell temp
  uint16_t    cellAvgTemp;    // average temperature
  uint16_t    cellHiVolt;     // highest voltage
  uint8_t     modCellHiVolt;  // module with the highest cell voltage
  uint16_t    cellLoVolt;     // lowest voltage
  uint8_t     modCellLoVolt;  // module with the lowest cell voltage
  uint16_t    cellAvgVolt;    // average voltage
  uint32_t    maxChargeA;
  uint32_t    maxDischargeA;
  uint16_t    maxChargeEndV;
  bmsStatus   status;
  bool        vcuStateChange;
  packState   state;
  packState   vcuRequestedState;
  uint8_t     soc;
  uint8_t     soh;
  lastContact vcuLastContact;
  errorCounts errorCounts;
  bool        rtcValid;
  uint16_t    vcuHvBusVoltage;
  controlMode controlMode;
  uint8_t     dmcModuleId;
}batteryPack;




#endif /* INC_BMS_H_ */

