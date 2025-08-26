/***************************************************************************************************************
 * @file           : app.c                                                         P A C K   C O N T R O L L E R
 * @brief          : Functions for communication with battery modules.
 ***************************************************************************************************************
 * Copyright (C) 2023-2024 Modular Battery Technologies, Inc.
 * US Patents 11,380,942; 11,469,470; 11,575,270; others. All rights reserved
 **************************************************************************************************************/
// Include files
#include "main.h"
#include "mcu.h"
#include "bms.h"
#include <can_id_module.h>
#include "canfdspi_api.h"
#include "string.h"
#include "stdio.h"
#include "can_frm_mod.h"
#include "vcu.h"
#include "time.h"
#include "eeprom_data.h"
#include "debug.h"

/***************************************************************************************************************
*
*                               Section: Global Data Definitions                   P A C K   C O N T R O L L E R
*
***************************************************************************************************************/
MCU_DATA appData;

lastContact_t vcuLastContact;
static lastContact_t lastAnnounceRequest = {0, 0};  // Track last announcement request time

CAN_CONFIG config;
CAN_OPERATION_MODE opMode;

// Transmit objects
CAN_TX_FIFO_CONFIG txConfig;
CAN_TX_FIFO_EVENT txFlags;
CAN_TX_MSGOBJ txObj;
uint8_t txd[MAX_DATA_BYTES];

// Receive objects
CAN_RX_FIFO_CONFIG rxConfig;
REG_CiFLTOBJ fObj;
REG_CiMASK mObj;
CAN_RX_FIFO_EVENT rxFlags;
CAN_RX_MSGOBJ rxObj;
uint8_t rxd[MAX_DATA_BYTES];


REG_t reg;

uint8_t i;

CAN_BUS_DIAGNOSTIC busDiagnostics;
uint8_t tec;
uint8_t rec;
CAN_ERROR_STATE errorFlags;

batteryModule module[MAX_MODULES_PER_PACK];
batteryPack pack;

uint32_t MCU_TicksSinceLastMessage(uint8_t moduleId);
uint32_t MCU_TicksSinceLastStateTx(uint8_t moduleId);
uint32_t MCU_ElapsedTicks(lastContact_t* pLastContact);
void MCU_UpdateModuleCounts(void);
void MCU_TransmitState(uint8_t moduleId, moduleState state);
uint8_t MCU_FindMaxVoltageModule(void);
void MCU_UpdateStats(void);
void MCU_RequestHardware(uint8_t moduleId);
void MCU_ProcessModuleHardware(void);
void MCU_ProcessModuleTime(void);
void MCU_ProcessCellCommStatus1(void);
static bool MCU_ShouldLogMessage(uint16_t messageId, bool isTx);


/***************************************************************************************************************
*
*                   Section: Application Local Functions                           P A C K   C O N T R O L L E R
*
***************************************************************************************************************/

/***************************************************************************************************************
*     M C U _ S h o u l d L o g M e s s a g e                                      P A C K   C O N T R O L L E R
***************************************************************************************************************/
static bool MCU_ShouldLogMessage(uint16_t messageId, bool isTx)
{
  // Check if general COMMS debug is enabled
  if((debugLevel & DBG_COMMS) == 0) return false;
  
  // Map message IDs to debug flags
  switch(messageId){
    case ID_MODULE_ANNOUNCE_REQUEST:  return (debugMessages & DBG_MSG_ANNOUNCE_REQ) != 0;
    case ID_MODULE_ANNOUNCEMENT:      return (debugMessages & DBG_MSG_ANNOUNCE) != 0;
    case ID_MODULE_REGISTRATION:      return (debugMessages & DBG_MSG_REGISTRATION) != 0;
    case ID_MODULE_STATUS_REQUEST:    return (debugMessages & DBG_MSG_STATUS_REQ) != 0;
    case ID_MODULE_STATUS_1:          return (debugMessages & DBG_MSG_STATUS1) != 0;
    case ID_MODULE_STATUS_2:          return (debugMessages & DBG_MSG_STATUS2) != 0;
    case ID_MODULE_STATUS_3:          return (debugMessages & DBG_MSG_STATUS3) != 0;
    case ID_MODULE_STATE_CHANGE:      return (debugMessages & DBG_MSG_STATE_CHANGE) != 0;
    case ID_MODULE_HARDWARE_REQUEST:  return (debugMessages & DBG_MSG_HARDWARE_REQ) != 0;
    case ID_MODULE_HARDWARE:          return (debugMessages & DBG_MSG_HARDWARE) != 0;
    case ID_MODULE_DETAIL:            return (debugMessages & DBG_MSG_CELL_DETAIL) != 0;
    case ID_MODULE_CELL_COMM_STATUS1: return (debugMessages & DBG_MSG_CELL_STATUS1) != 0;
    case ID_MODULE_CELL_COMM_STATUS2: return (debugMessages & DBG_MSG_CELL_STATUS2) != 0;
    case ID_MODULE_TIME_REQUEST:      return (debugMessages & DBG_MSG_TIME_REQ) != 0;
    case ID_MODULE_SET_TIME:          return (debugMessages & DBG_MSG_SET_TIME) != 0;
    case ID_MODULE_MAX_STATE:         return false; // Never log 0x517 to prevent flooding
    case ID_MODULE_DEREGISTER:        return (debugMessages & DBG_MSG_DEREGISTER) != 0;
    case ID_MODULE_ALL_ISOLATE:       return (debugMessages & DBG_MSG_ISOLATE_ALL) != 0;
    case ID_MODULE_ALL_DEREGISTER:    return (debugMessages & DBG_MSG_DEREGISTER_ALL) != 0;
    default:                          return false; // Unknown messages not logged unless DBG_MSG_ALL
  }
}

/***************************************************************************************************************
*
*       Section: Application Initialization and State Machine Functions            P A C K   C O N T R O L L E R
*
***************************************************************************************************************/


/***************************************************************************************************************
*     P C U _ I n i t i a l i z e                                                  P A C K   C O N T R O L L E R
***************************************************************************************************************/
void PCU_Initialize(void)
{


  // Retrieve parameters from emulated EEPROM
  // - PACK_ID            1 Byte
  // - HARDWARE_VERSION   16 Bytes
  // - MANUFACTURER_ID    16 Bytes
  // - PART_NUMBER        16 Bytes
  // - SERIAL_NUMBER      16 Bytes


  LoadAllEEPROM();

  memset(&pack,0,sizeof(pack));

  //pack.id = EE_PACK_ID;
  pack.id = eeVarDataTab[EE_PACK_CONTROLLER_ID];
  pack.mfgId=0;
  pack.partId=0;
  pack.uniqueId=0;
  if (pack.id == 0)
    pack.vcuCanOffset = 0;
  else if (pack.id == 1)
    pack.vcuCanOffset =0x100;
  pack.hwVersion=HW_VER;
  pack.fwVersion=FW_VER;
  pack.voltage=0;
  pack.current=0;
  pack.moduleCount=0;
  pack.activeModules=0;
  pack.totalCells=0;
  pack.cellHiTemp=0;
  pack.cellLoTemp=0;
  pack.cellAvgTemp=0;
  pack.cellHiVolt=0;
  pack.cellLoVolt=0;
  pack.cellAvgVolt=0;
  pack.status=0;
  pack.vcuStateChange=0;
  pack.state=0;
  pack.vcuRequestedState=0;
  pack.soc=0;
  pack.soh=0;
  pack.vcuLastContact.overflows=0;
  pack.vcuLastContact.ticks=0;
  pack.cellBalanceActive=0;
  pack.cellBalanceStatus=0;
  pack.faultedModules=0;
  pack.controlMode = packMode;

  //clear the module memory structure
  uint8_t index;
  for (index=0;index<MAX_MODULES_PER_PACK;index++){
    memset(&module[index],0,sizeof(module[index]));
  }


  bool passed;


  serialOut("");
  serialOut("");
  serialOut("        ██    ██");
  serialOut("     ██ ██ ██ ██ ██");
  serialOut("     ██ ██ ██ ██ ██     Pack Controller V1.0     (c) 2024");
  serialOut("     ██ ██ ██ ██ ██     Modular Battery Technologies, Inc");
  serialOut("     ██    ██    ██");
  serialOut("     m o d b a t t");
  serialOut("");

  passed = CAN_TestRamAccess(CAN1);
  if (passed) { sprintf(tempBuffer,  "     VCU MCP2518FD RAM TEST         (CAN1) : OK"); serialOut(tempBuffer);
  }else{        sprintf(tempBuffer,  "     VCU MCP2518FD RAM TEST         (CAN1) : FAILED!"); serialOut(tempBuffer);
  }
  passed = CAN_TestRegisterAccess(CAN1);
  if (passed) { sprintf(tempBuffer,  "     VCU MCP2518FD REGISTER TEST    (CAN1) : OK"); serialOut(tempBuffer);
  }else{        sprintf(tempBuffer,  "     VCU MCP2518FD REGISTER TEST    (CAN1) : FAILED!"); serialOut(tempBuffer);
  }
  sprintf(tempBuffer," "); serialOut(tempBuffer);

  if(hwPlatform == PLATFORM_NUCLEO){
    passed = CAN_TestRamAccess(CAN2);
    if (passed) { sprintf(tempBuffer,"     MCU MCP2518FD RAM TEST         (CAN2) : OK"); serialOut(tempBuffer);
    }else{        sprintf(tempBuffer,"     MCU MCP2518FD RAM TEST         (CAN2) : FAILED!"); serialOut(tempBuffer);
    }

    passed = CAN_TestRegisterAccess(CAN2);
    if (passed) { sprintf(tempBuffer,"     MCU MCP2518FD REGISTER TEST    (CAN2) : OK"); serialOut(tempBuffer);
    }else{        sprintf(tempBuffer,"     MCU MCP2518FD REGISTER TEST    (CAN2) : FAILED!"); serialOut(tempBuffer);
    }
    sprintf(tempBuffer," "); serialOut(tempBuffer);
  }else{
    // PLATFORM_MODBATT
    passed = CAN_TestRamAccess(CAN2);
    if (passed) { sprintf(tempBuffer,"     LO-MCU MCP2518FD RAM TEST      (CAN2) : OK"); serialOut(tempBuffer);
    }else{        sprintf(tempBuffer,"     LO-MCU MCP2518FD RAM TEST      (CAN2) : FAILED!"); serialOut(tempBuffer);
    }

    passed = CAN_TestRegisterAccess(CAN2);
    if (passed) { sprintf(tempBuffer,"     LO-MCU MCP2518FD REGISTER TEST (CAN2) : OK"); serialOut(tempBuffer);
    }else{        sprintf(tempBuffer,"     LO-MCU MCP2518FD REGISTER TEST (CAN2) : FAILED!"); serialOut(tempBuffer);
    }
    sprintf(tempBuffer," "); serialOut(tempBuffer);

    passed = CAN_TestRamAccess(CAN3);
    if (passed) { sprintf(tempBuffer,"     HI-MCU MCP2518FD RAM TEST      (CAN3) : OK"); serialOut(tempBuffer);
    }else{        sprintf(tempBuffer,"     HI-MCU MCP2518FD RAM TEST      (CAN3) : FAILED!"); serialOut(tempBuffer);
    }

    passed = CAN_TestRegisterAccess(CAN3);
    if (passed) { sprintf(tempBuffer,"     HI-MCU MCP2518FD REGISTER TEST (CAN3) : OK"); serialOut(tempBuffer);
    }else{        sprintf(tempBuffer,"     HI-MCU MCP2518FD REGISTER TEST (CAN3) : FAILED!"); serialOut(tempBuffer);
    }
    sprintf(tempBuffer," "); serialOut(tempBuffer);

  }
  appData.state = PC_STATE_INIT;
}


/***************************************************************************************************************
*     P C U _ T a s k s                                                            P A C K   C O N T R O L L E R
***************************************************************************************************************/
void PCU_Tasks(void)
{
  uint8_t index;
  uint8_t moduleId;
  uint8_t firstModuleIndex;
  uint32_t elapsedTicks;
  static uint8_t nextModuleToPoll = 0;
  static lastContact_t lastStatusPoll = {0, 0};

  if(appData.state == PC_STATE_INIT){  // Application initialization

    DRV_CANFDSPI_Init(CAN1);  // VCU interface
    DRV_CANFDSPI_Init(CAN2);  // Module Controller interface

    MCU_IsolateAllModules();
    MCU_DeRegisterAllModules();
    
    // Always show startup message
    sprintf(tempBuffer,"MCU STARTUP - Pack controller initialized, module count=%d", pack.moduleCount); 
    serialOut(tempBuffer);
    
    // Request module announcements on startup
    MCU_RequestModuleAnnouncement();
    lastAnnounceRequest.ticks = htim1.Instance->CNT;
    lastAnnounceRequest.overflows = etTimerOverflows;

    pack.vcuRequestedState = packOff;


   // set up a reference time
    pack.vcuLastContact.overflows = etTimerOverflows;
    pack.vcuLastContact.ticks = htim1.Instance->CNT;

    appData.state = PC_STATE_RUN;

  }else if (appData.state == PC_STATE_RUN){

    //Check for CAN2 RX Interrupt (module controller)
    if(can1RxInterrupt)
      VCU_ReceiveMessages();

    //Check for CAN1 RX Interrupt (VCU)
    if(can2RxInterrupt)
      MCU_ReceiveMessages();

    //Check for expired last contact from VCU
    elapsedTicks = VCU_TicksSinceLastMessage();
    if(elapsedTicks > VCU_ET_TIMEOUT){
      if ((pack.state == packOn) || (pack.state == packStandby) || (pack.state == packPrecharge)){
        if((debugLevel & (DBG_VCU + DBG_ERRORS)) == (DBG_VCU + DBG_ERRORS)){ sprintf(tempBuffer,"VCU ERROR - LOST CONTACT TIMEOUT!"); serialOut(tempBuffer);}
        pack.vcuRequestedState = packOff;
      }
    }else if(elapsedTicks > VCU_ET_WARNING){
      if ((pack.state == packOn) || (pack.state == packPrecharge)){
        if((debugLevel & (DBG_VCU + DBG_ERRORS)) == (DBG_VCU + DBG_ERRORS)){ sprintf(tempBuffer,"VCU WARNING - LOST CONTACT WARNING!"); serialOut(tempBuffer);}
        pack.vcuRequestedState = packStandby;
      }
    }
    
    // Send periodic announcement requests
    uint32_t timeSinceLastAnnounce = 0;
    if ((etTimerOverflows - lastAnnounceRequest.overflows) == 0){
      timeSinceLastAnnounce = htim1.Instance->CNT - lastAnnounceRequest.ticks;
    } else {
      timeSinceLastAnnounce = ((htim1.Init.Period +1) - lastAnnounceRequest.ticks) + 
                              ((htim1.Init.Period +1) * (etTimerOverflows - (lastAnnounceRequest.overflows +1))) + 
                              (htim1.Instance->CNT);
    }
    
    if(timeSinceLastAnnounce > MCU_ANNOUNCE_REQUEST_INTERVAL){
      MCU_RequestModuleAnnouncement();
      lastAnnounceRequest.ticks = htim1.Instance->CNT;
      lastAnnounceRequest.overflows = etTimerOverflows;
    }

    //Check for expired last contact from module
    ShowDebugMessage(MSG_POLLING_CYCLE, pack.moduleCount);
    for (index =0;index < MAX_MODULES_PER_PACK;index++){
      if(!module[index].isRegistered || module[index].uniqueId == 0) continue;
      elapsedTicks = MCU_TicksSinceLastMessage(module[index].moduleId);
      ShowDebugMessage(MSG_MODULE_CHECK, module[index].moduleId, elapsedTicks, 
                       module[index].statusPending, module[index].faultCode.commsError);
      if(elapsedTicks > MCU_ET_TIMEOUT && (module[index].statusPending == true)){
        // Increment consecutive timeout counter
        module[index].consecutiveTimeouts++;
        module[index].statusMessagesReceived = 0;  // Clear any partial status
        
        if(module[index].consecutiveTimeouts >= MCU_MAX_CONSECUTIVE_TIMEOUTS){
          // Max timeouts reached - deregister the module
          if((debugLevel & ( DBG_MCU + DBG_ERRORS)) == ( DBG_MCU + DBG_ERRORS) ){ 
            sprintf(tempBuffer,"MCU ERROR - Module ID=%02x exceeded %d consecutive timeouts - DEREGISTERING",
                    module[index].moduleId, MCU_MAX_CONSECUTIVE_TIMEOUTS); 
            serialOut(tempBuffer);
          }
          
          // Send deregister message to the module
          MCU_DeRegisterModule(module[index].moduleId);
          
          // Log removal from pack
          ShowDebugMessage(MSG_DEREGISTER, module[index].moduleId, module[index].uniqueId, index);
          
          // Mark module as deregistered (don't remove from array)
          module[index].isRegistered = false;
          
          // Update module counts
          MCU_UpdateModuleCounts();
          
          // Continue without adjusting index (no array shift)
        }
        else if( module[index].faultCode.commsError == false){
          // First timeout or still under limit - isolate module
          if((debugLevel & ( DBG_MCU + DBG_ERRORS)) == ( DBG_MCU + DBG_ERRORS) ){ 
            sprintf(tempBuffer,"MCU ERROR - Module timeout ID=%02x (timeout %d of %d)",
                    module[index].moduleId, module[index].consecutiveTimeouts, MCU_MAX_CONSECUTIVE_TIMEOUTS); 
            serialOut(tempBuffer);
          }
          if (pack.vcuRequestedState == packPrecharge && module[index].currentState == moduleOn){
            // This was the first module on and its faulted - select another!
            pack.powerStatus.powerStage = stageSelectModule;
          }
          // turn off the faulted module and flag the fault
          module[index].nextState = moduleOff;
          module[index].faultCode.commsError = true;
        }
      }else if(elapsedTicks > MCU_STATUS_INTERVAL && (module[index].statusPending == false)){
        // Send State
        ShowDebugMessage(MSG_STATUS_REQUEST, module[index].moduleId, index);
        MCU_RequestModuleStatus(module[index].moduleId);
        // Have we received the hardware info? This should have been sent at registration
        if(module[index].hardwarePending)
          // Not received, so lets request it
          MCU_RequestHardware(module[index].moduleId);
      }else{
        // timers have not been exceeded
        if(module[index].faultCode.commsError == true){
          // if the module was in fault, bring it back online
          module[index].faultCode.commsError  = false;
        }
        ShowDebugMessage(MSG_MODULE_CHECK, module[index].moduleId, elapsedTicks, 
                         module[index].statusPending, module[index].faultCode.commsError);
      }
    }
    
    // Round-robin polling of modules
    if(pack.moduleCount > 0){
      uint32_t timeSinceLastPoll = MCU_ElapsedTicks(&lastStatusPoll);
      
      // Poll one module every 100ms to distribute the load
      if(timeSinceLastPoll > 100){
        // Find the next valid module to poll
        uint8_t modulesToCheck = pack.moduleCount;
        while(modulesToCheck > 0){
          if(nextModuleToPoll >= pack.moduleCount){
            nextModuleToPoll = 0;
          }
          
          // Only poll modules that are not in timeout/error state
          if(module[nextModuleToPoll].statusPending == false && 
             module[nextModuleToPoll].faultCode.commsError == false){
            ShowDebugMessage(MSG_STATUS_REQUEST, module[nextModuleToPoll].moduleId, nextModuleToPoll);
            MCU_RequestModuleStatus(module[nextModuleToPoll].moduleId);
            
            // Have we received the hardware info?
            if(module[nextModuleToPoll].hardwarePending){
              MCU_RequestHardware(module[nextModuleToPoll].moduleId);
            }
            
            // Move to next module for next poll
            nextModuleToPoll++;
            
            // Update last poll time
            lastStatusPoll.ticks = htim1.Instance->CNT;
            lastStatusPoll.overflows = etTimerOverflows;
            break;
          }
          
          nextModuleToPoll++;
          modulesToCheck--;
        }
      }
    }
  }

  if (pack.controlMode == dmcMode){
   // DIRECT MODULE CONTROL MODE
   // Command the modules
    for (index =0;index < MAX_MODULES_PER_PACK;index++){
      if(!module[index].isRegistered || module[index].uniqueId == 0) continue;
      // Handle the  over current condition
      if(module[index].faultCode.overCurrent == true){
        if (module[index].currentState != moduleOff){
          // Turn off the module
          module[index].nextState = moduleOff;
          // clear the over current flag
          module[index].faultCode.overCurrent = false;
        }
      } else if (module[index].faultCode.commsError == false && module[index].faultCode.hwIncompatible == false ){
        // No faults - have we already commanded the module?
        if((module[index].command.commandStatus == commandIssued) && (module[index].command.commandedState == module[index].nextState)){
          // module has been commanded, allow some delay before re-issuing the command
          if(MCU_TicksSinceLastStateTx(module[index].moduleId) > MCU_STATE_TX_INTERVAL){
            // Command the module
            MCU_TransmitState(module[index].moduleId,module[index].nextState);
          }
        }else {
          ShowDebugMessage(MSG_STATE_TRANSITION, module[index].moduleId, 
                           module[index].currentState, module[index].nextState,
                           module[index].command.commandedState, module[index].command.commandStatus);
          MCU_TransmitState(module[index].moduleId,module[index].nextState);
        }
      }
    }
    // This should fire every 500ms
    if(sendState > 0){
      // Send Module Data to VCU for module of interest
      VCU_TransmitModuleState();
      VCU_TransmitModulePower();
      VCU_TransmitModuleCellVoltage();
      VCU_TransmitModuleCellTemp();
      VCU_TransmitModuleLimits();
      /*
       * NOT YET IMPLEMENTED :
       *
       * VCU_TransmitModuleCellId();
       * VCU_TransmitModuleList();
       *
       */
      sendState = 0;
    }
  } else if(pack.controlMode == packMode){
    // PACK CONTROL MODE
    // Process power up of first module for PRECHARGE and ON states
    if(pack.vcuRequestedState == packOn || pack.vcuRequestedState == packPrecharge){
      // check for requirement to select and power up the first module
      if(pack.powerStatus.powerStage == stageSelectModule){
        // Select module with highest voltage
        moduleId = MCU_FindMaxVoltageModule();
        if(moduleId > 0 && moduleId <= pack.activeModules) {
          ShowDebugMessage(MSG_VOLTAGE_SELECTION, moduleId, module[moduleId-1].mmv);
        }
        if (moduleId == 0){
          // All modules report 0V!
          if((debugLevel & (DBG_MCU + DBG_ERRORS)) == (DBG_MCU + DBG_ERRORS) && ((pack.errorCounts.firstModule % 5000) == 0) ){ sprintf(tempBuffer,"MCU WARNING - all modules report 0V"); serialOut(tempBuffer);}
          pack.errorCounts.firstModule ++;
          if (pack.errorCounts.firstModule > 64999) pack.errorCounts.firstModule = 0;
        }else if (moduleId == pack.moduleCount +1 ){
          // no module available)
          if((debugLevel & (DBG_MCU + DBG_ERRORS)) == (DBG_MCU + DBG_ERRORS) && ((pack.errorCounts.firstModule % 5000) == 0)){ sprintf(tempBuffer,"MCU WARNING - no modules available"); serialOut(tempBuffer);}
          pack.errorCounts.firstModule ++;
          if (pack.errorCounts.firstModule > 64999) pack.errorCounts.firstModule = 0;
        }else{
          //we have a valid ID - store the module Id
          pack.powerStatus.firstModuleId = moduleId;
          // move to the next power state
          pack.powerStatus.powerStage = stagePowerOnModule;
        }
      }
      // Are we ready to power up the first module?
      if(pack.powerStatus.powerStage == stagePowerOnModule){
        // first check the the module is not already on - we need to find the index
        firstModuleIndex = MCU_ModuleIndexFromId(pack.powerStatus.firstModuleId);
        if (firstModuleIndex < MAX_MODULES_PER_PACK){
          // check that our first module hasnt gone into fault
          if(module[firstModuleIndex].faultCode.commsError == true || module[firstModuleIndex].faultCode.hwIncompatible == true){
             // module has gone into fault or hw incompatible -  go back a stage and select another
             pack.powerStatus.powerStage = stageSelectModule;
             if((debugLevel & (DBG_MCU + DBG_ERRORS)) == (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Selected module %02x in fault - selecting another",pack.powerStatus.firstModuleId); serialOut(tempBuffer);}
          } else if (pack.vcuRequestedState == packOn){
            if (module[firstModuleIndex].currentState == moduleOn){
              // mark the pack as requested by the vcu
              pack.state = packOn;
              // set powerStage idle
              pack.powerStatus.powerStage = stageIdle;
            } else {
              // Command the module to turn on
                module[firstModuleIndex].nextState = moduleOn;
            }
          } else if (pack.vcuRequestedState == packPrecharge){
            if (module[firstModuleIndex].currentState == modulePrecharge){
              // mark the pack as requested by the vcu
              pack.state = packPrecharge;
              // set powerStage idle
              pack.powerStatus.powerStage = stageIdle;
            } else{
              // Command the module to pre-charge state
                module[firstModuleIndex].nextState = modulePrecharge;
            }
          }
        } else {
           //Unable to find module index
           if((debugLevel & (DBG_MCU + DBG_ERRORS)) == (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - module index not found!"); serialOut(tempBuffer);}
        }
      }
    }
    // Command the rest of the modules
    for (index =0;index < MAX_MODULES_PER_PACK;index++){
      if(!module[index].isRegistered || module[index].uniqueId == 0) continue;
      // Handle the  over current condition
      if(module[index].faultCode.overCurrent == true){
        if (pack.vcuRequestedState != packOff){
          // Command the module to standby
          module[index].nextState = moduleStandby;
        } else {
          // Turn off the module
          module[index].nextState = moduleOff;
          // clear the over current flag
          module[index].faultCode.overCurrent = false;
        }
      } else if (module[index].faultCode.commsError == false && module[index].faultCode.hwIncompatible == false ){
        // Handle the pack states
        switch (pack.vcuRequestedState){
          // ON
          case packOn :
            if(pack.state == packOn){
              module[index].nextState = moduleOn;
            }
            // work out pack status - pack soc is stored as per the module soc and needs to be converted for calculation
            if      (pack.soc < (PACK_EMPTY_SOC_THRESHOLD / PERCENTAGE_FACTOR )) { pack.status = packStatusEmpty; } // < 5% = Empty
            else if (pack.soc > (PACK_FULL_SOC_THRESHOLD  / PERCENTAGE_FACTOR )) { pack.status = packStatusFull;  } // > 95% = Full
            else                                                                 { pack.status = packStatusNormal;} // 5% to 95% = Normal
            break;
          // PRECHARGE
          case packPrecharge :
            if (pack.state == packPrecharge && index != firstModuleIndex){
              module[index].nextState = moduleStandby;
            }
            pack.status = packStatusOff;
            break;
          case packStandby:
            module[index].nextState = moduleStandby;
            pack.state  = packStandby;
            pack.status = packStatusOff;
            break;
          case packOff:
            module[index].nextState = moduleOff;
            pack.state  = packOff;
            pack.status = packStatusOff;
            break;
          default:
            break;
        }
      }
      // Have we already commanded the module?
      if((module[index].command.commandStatus == commandIssued) && (module[index].command.commandedState == module[index].nextState)){
        // module has been commanded, allow some delay before re-issuing the command
        if(MCU_TicksSinceLastStateTx(module[index].moduleId) > MCU_STATE_TX_INTERVAL){
          // Command the module
          MCU_TransmitState(module[index].moduleId,module[index].nextState);
        }
      }else {
        MCU_TransmitState(module[index].moduleId,module[index].nextState);
      }
    }

    //Update our pack statistics
    MCU_UpdateStats();

    // This should fire every 200ms
    if(sendMaxState >0){
      // Broadcast Maximum State to modules
      MCU_TransmitMaxState(pack.vcuRequestedState);
    }

    // This should fire every 500ms
    if(sendState > 0){
      // Send BMS Data to VCU
      if (pack.rtcValid == false) VCU_RequestTime();
      VCU_TransmitBmsState();
      VCU_TransmitBmsData1();
      VCU_TransmitBmsData2();
      VCU_TransmitBmsData3();
      VCU_TransmitBmsData5();
      VCU_TransmitBmsData8();
      VCU_TransmitBmsData9();
      VCU_TransmitBmsData10();
      sendState=0;
    }
  }
}



/***************************************************************************************************************
*
*                              Section: Application Local Functions
*
***************************************************************************************************************/


/***************************************************************************************************************
*     D R V _ C A N F D S P I _ I n i t                                            P A C K   C O N T R O L L E R
***************************************************************************************************************/
void DRV_CANFDSPI_Init(CANFDSPI_MODULE_ID index)
{
  // Reset device
  DRV_CANFDSPI_Reset(index);

  // Enable ECC and initialize RAM
  DRV_CANFDSPI_EccEnable(index);

  DRV_CANFDSPI_RamInit(index, 0xff);

  // Configure device
  DRV_CANFDSPI_ConfigureObjectReset(&config);
  config.IsoCrcEnable = 1;
  config.StoreInTEF = 0;

  DRV_CANFDSPI_Configure(index, &config);

  // Setup TX FIFO
  DRV_CANFDSPI_TransmitChannelConfigureObjectReset(&txConfig);
  txConfig.FifoSize = 7;
  txConfig.PayLoadSize = CAN_PLSIZE_64;
  txConfig.TxPriority = 1;

  DRV_CANFDSPI_TransmitChannelConfigure(index, MCU_TX_FIFO, &txConfig);

  // Setup RX FIFO
  DRV_CANFDSPI_ReceiveChannelConfigureObjectReset(&rxConfig);
  rxConfig.FifoSize = 15;
  rxConfig.PayLoadSize = CAN_PLSIZE_64;

  DRV_CANFDSPI_ReceiveChannelConfigure(index, MCU_RX_FIFO, &rxConfig);

  // Setup RX Filter
  fObj.word = 0;
  fObj.bF.SID = 0x00;
  fObj.bF.EXIDE = 0;
  fObj.bF.EID = 0x00;

  DRV_CANFDSPI_FilterObjectConfigure(index, CAN_FILTER0, &fObj.bF);

  // Setup RX Mask
  mObj.word = 0;
  mObj.bF.MSID = 0x0;
  mObj.bF.MIDE = 0; // Both standard and extended frames accepted
  mObj.bF.MEID = 0x0;
  DRV_CANFDSPI_FilterMaskConfigure(index, CAN_FILTER0, &mObj.bF);

  // Link FIFO and Filter
  DRV_CANFDSPI_FilterToFifoLink(index, CAN_FILTER0, MCU_RX_FIFO, true);

  // Setup Bit Time
  DRV_CANFDSPI_BitTimeConfigure(index, CAN_500K_2M, CAN_SSP_MODE_AUTO, CAN_SYSCLK_40M);

  // Setup Transmit and Receive Interrupts
  DRV_CANFDSPI_GpioModeConfigure(index, GPIO_MODE_INT, GPIO_MODE_INT);
#ifdef MCU_USE_TX_INT
  DRV_CANFDSPI_TransmitChannelEventEnable(index, MCU_TX_FIFO, CAN_TX_FIFO_NOT_FULL_EVENT);
#endif
  DRV_CANFDSPI_ReceiveChannelEventEnable(index, MCU_RX_FIFO, CAN_RX_FIFO_NOT_EMPTY_EVENT);
  DRV_CANFDSPI_ModuleEventEnable(index, CAN_TX_EVENT | CAN_RX_EVENT);

  // Select Normal Mode
  DRV_CANFDSPI_OperationModeSelect(index, CAN_NORMAL_MODE);
}



/***************************************************************************************************************
*     M C U _ U p d a t e S t a t s                                               P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_UpdateStats(void)
{

  uint8_t index;
  uint8_t modulesOn            = 0;
  uint32_t voltage             = 0;
  float moduleCurrent          = 0;
  float totalCurrent           = 0;
  float moduleMaxChargeA       = 0;
  float moduleMaxDischargeA    = 0;
  uint16_t lowestSoc           = 255;
  uint16_t lowestSoh           = 255;
  uint32_t totalAvgCellVolt    = 0;
  uint16_t highestCellVolt     = 0;
  uint16_t lowestCellVolt      = 65535;
  uint32_t totalAvgCellTemp    = 0;
  uint16_t highestCellTemp     = 0;
  uint16_t lowestCellTemp      = 65535;
  float    maxChargeA          = 0;
  float    maxDischargeA       = 0;
  float    packCurrent         = 0;
  uint32_t totalMaxChargeEndV  = 0;
  uint8_t  activeModules       = 0;
  uint8_t  modLowestCellVolt   = 0;
  uint8_t  modHighestCellVolt  = 0;
  uint8_t  modLowestCellTemp   = 0;
  uint8_t  modHighestCellTemp  = 0;


  for(index = 0; index < MAX_MODULES_PER_PACK; index++){
    if(!module[index].isRegistered || module[index].uniqueId == 0) continue;
    // only generate stats for modules that are not in fault or in over current
    if(module[index].faultCode.commsError == false && module[index].faultCode.overCurrent ==  false && module[index].faultCode.hwIncompatible == false){
      // sum the currents of all modules that are ON and average the voltages
      if(module[index].currentState == moduleOn) {
        // calculate module max currents in Amps
        moduleMaxChargeA    = MODULE_CURRENT_BASE + (module[index].maxChargeA    * MODULE_CURRENT_FACTOR);
        moduleMaxDischargeA = MODULE_CURRENT_BASE + (module[index].maxDischargeA * MODULE_CURRENT_FACTOR);
        //sum the voltage - averaged later
        voltage = voltage + module[index].mmv;
        //increment module on count
        modulesOn++;
        //calculate module current in amps
        moduleCurrent       = MODULE_CURRENT_BASE + (module[index].mmc           * MODULE_CURRENT_FACTOR);

       // Check for over current condition. Negative current flows out of battery, positive current flows into battery
       // ALLOW FOR +/- 0.3A ACCURACY FROM MODULE?? ie. If current limit is zero and the module is reporting 0.25A then allow for this.
       if(moduleCurrent - MODULE_CURRENT_TOLERANCE > moduleMaxChargeA){
         module[index].faultCode.overCurrent = true;
          if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - module charge current (%.2fA) exceeds specification (max %.2fA)",moduleCurrent, moduleMaxChargeA); serialOut(tempBuffer);}
        } else if(moduleCurrent + MODULE_CURRENT_TOLERANCE < moduleMaxDischargeA) {
          module[index].faultCode.overCurrent = true;
          if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - module discharge current (%.2fA) exceeds specification (max %.2fA)",moduleCurrent, moduleMaxDischargeA); serialOut(tempBuffer);}
        }
        // have we now put the module into over current?
       if(module[index].faultCode.overCurrent == true){
          // are we in pre-charge (just the one module on)?
          if (pack.vcuRequestedState == packPrecharge){
            // ah crap - this was the first module on and its over current - go back and select another
            pack.powerStatus.powerStage = stageSelectModule;
          }
        }

        totalCurrent = totalCurrent + moduleCurrent;
        maxDischargeA      = maxDischargeA        + moduleMaxDischargeA;
        maxChargeA         = maxChargeA           + moduleMaxChargeA;
      }
      // sum the maxCharge, maxDischarge currents, and average the maxChargeEndV, cellAvgVolt, cellAvgTemp
      totalMaxChargeEndV = totalMaxChargeEndV   + module[index].maxChargeEndV;
      totalAvgCellVolt   = totalAvgCellVolt     + module[index].cellAvgVolt;
      totalAvgCellTemp   = totalAvgCellTemp     + module[index].cellAvgTemp;
      // highest/lowest
      if( module[index].soc < lowestSoc) lowestSoc = module[index].soc;
      if( module[index].soh < lowestSoh) lowestSoh = module[index].soh;
      if( module[index].cellLoVolt < lowestCellVolt){
        lowestCellVolt  = module[index].cellLoVolt;
        modLowestCellVolt = module[index].moduleId;
      }
      if( module[index].cellHiVolt > highestCellVolt){
        highestCellVolt = module[index].cellHiVolt;
        modHighestCellVolt = module[index].moduleId;
      }
      if( module[index].cellHiTemp > highestCellTemp){
        highestCellTemp = module[index].cellHiTemp;
        modHighestCellTemp = module[index].moduleId;
      }
      if( module[index].cellLoTemp < lowestCellTemp){
        lowestCellTemp  = module[index].cellLoTemp;
        modLowestCellTemp = module[index].moduleId;
      }
      activeModules++; // a module that is flagged overcurrent in the preceeeding code is still active atm until it gets sent the standby
    }
  }
  // Pack active module count
  pack.activeModules = activeModules;

  // Pack faulted module count
  pack.faultedModules = pack.moduleCount - pack.activeModules;

  // Pack Voltage and Current
  if (modulesOn > 0){
    pack.voltage = voltage / modulesOn;
    //Check for max/min current out of range - set to min/max and flag error
    if(totalCurrent > (PACK_CURRENT_BASE + (65535 * PACK_CURRENT_FACTOR))){
      if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Total current (%.2fA) exceeds specification (max %.2fA)",totalCurrent, (PACK_CURRENT_BASE + (65535 * PACK_CURRENT_FACTOR))); serialOut(tempBuffer);}
      totalCurrent = (PACK_CURRENT_BASE + (65535 * PACK_CURRENT_FACTOR));
    }
    else if(totalCurrent < PACK_CURRENT_BASE){
      if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Total current (%.2fA) exceeds specification (max %dA)",totalCurrent, PACK_CURRENT_BASE); serialOut(tempBuffer);}
      totalCurrent = PACK_CURRENT_BASE;
    }
    // value is now within limits
    packCurrent = (totalCurrent/PACK_CURRENT_FACTOR)-(PACK_CURRENT_BASE/PACK_CURRENT_FACTOR);
    pack.current = (uint16_t) packCurrent;

  }else{
    pack.voltage = 0;
    packCurrent = (0 / PACK_CURRENT_FACTOR) - (PACK_CURRENT_BASE / PACK_CURRENT_FACTOR); // 0 Amps converted.
    pack.current = (uint16_t) packCurrent;
  }

  // Maximum Pack charge/discharge current and end voltage
  if(maxChargeA > (PACK_CURRENT_BASE + (65535 * PACK_CURRENT_FACTOR))){
    if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Total maxChargeA (%.2fA) exceeds specification (max %.2fA)",maxChargeA, (PACK_CURRENT_BASE + (65535 * PACK_CURRENT_FACTOR))); serialOut(tempBuffer);}
    maxChargeA = (PACK_CURRENT_BASE + (65535 * PACK_CURRENT_FACTOR));
  }else if(maxChargeA < PACK_CURRENT_BASE){
    if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Total maxChargeA (%.2fA) exceeds specification (max %dA)",maxChargeA, PACK_CURRENT_BASE); serialOut(tempBuffer);}
    maxChargeA = PACK_CURRENT_BASE;
  }
  // value is within limits
  pack.maxChargeA = (maxChargeA/PACK_CURRENT_FACTOR)-(PACK_CURRENT_BASE/PACK_CURRENT_FACTOR);

  // Maximum Pack charge/discharge current and end voltage
  if(maxDischargeA > (PACK_CURRENT_BASE + (65535 * PACK_CURRENT_FACTOR))){
    if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Total maxDischargeA (%.2fA) exceeds specification (max %.2fA)",maxDischargeA, (PACK_CURRENT_BASE + (65535 * PACK_CURRENT_FACTOR))); serialOut(tempBuffer);}
    maxDischargeA = (PACK_CURRENT_BASE + (65535 * PACK_CURRENT_FACTOR));
  }else if(maxDischargeA < PACK_CURRENT_BASE){
    if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Total maxDischargeA (%.2fA) exceeds specification (max %dA)",maxDischargeA, PACK_CURRENT_BASE); serialOut(tempBuffer);}
    maxDischargeA = PACK_CURRENT_BASE;
  }
  // value is now within limits
  pack.maxChargeA = (maxChargeA/PACK_CURRENT_FACTOR)-(PACK_CURRENT_BASE/PACK_CURRENT_FACTOR);
  pack.maxDischargeA = (maxDischargeA/PACK_CURRENT_FACTOR)-(PACK_CURRENT_BASE/PACK_CURRENT_FACTOR);

  if(pack.activeModules > 0) pack.maxChargeEndV = totalMaxChargeEndV / pack.activeModules;
  else pack.maxChargeEndV = 0;

  // Pack SOC = SOC of weakest module
  if(pack.activeModules > 0 && lowestSoc < 255) pack.soc = lowestSoc;
  else pack.soc = 0;

  // Pack SOH = SOH of weakest module
  if(pack.activeModules > 0 && lowestSoh < 255) pack.soh = lowestSoh;
  else pack.soh = 0;

  // Pack Cell Avg Volt
  if(pack.activeModules > 0) pack.cellAvgVolt = totalAvgCellVolt / pack.activeModules;
  else pack.cellAvgVolt = 0;

  // Pack Hi/Lo Cell Volt
  pack.cellHiVolt = highestCellVolt;
  pack.modCellHiVolt = modHighestCellVolt;
  if (lowestCellVolt < 65535){
    pack.cellLoVolt = lowestCellVolt;
    pack.modCellLoVolt = modLowestCellVolt;
  }
  else{
    pack.cellLoVolt = 0;
    pack.modCellLoVolt = 0;
  }

  // Pack Cell Avg Temp
  if(pack.activeModules > 0) pack.cellAvgTemp = totalAvgCellTemp / pack.activeModules;
  else pack.cellAvgTemp = 0; //-55 degrees!

  // Pack Hi/Lo Cell Temp
  pack.cellHiTemp = highestCellTemp;
  pack.modCellHiTemp = modHighestCellTemp;
  if (lowestCellTemp < 65535){
    pack.cellLoTemp = lowestCellTemp;
    pack.modCellLoTemp = modLowestCellTemp;
  }
  else{
    pack.cellLoTemp = 0; //-55 degrees!
    pack.modCellLoTemp = 0;
  }
}


/***************************************************************************************************************
*     M C U _ R e c e i v e M e s s a g e s                                       P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_ReceiveMessages(void)
{

  // Check if FIFO is not empty
  DRV_CANFDSPI_ReceiveChannelEventGet(CAN2, MCU_RX_FIFO, &rxFlags);

  while ( rxFlags & CAN_RX_FIFO_NOT_EMPTY_EVENT){

    // Get message
    DRV_CANFDSPI_ReceiveMessageGet(CAN2, MCU_RX_FIFO, &rxObj, rxd, MAX_DATA_BYTES);

    // Raw CAN message logging disabled - use specific message handlers

    // Reset timeout for ANY message from a module (using EID as module ID)
    if(rxObj.bF.id.EID > 0 && rxObj.bF.id.EID <= MAX_MODULES_PER_PACK){
        uint8_t moduleIndex = MCU_ModuleIndexFromId(rxObj.bF.id.EID);
        if(moduleIndex < MAX_MODULES_PER_PACK){
            MCU_UpdateModuleContact(moduleIndex);
        }
    }

    switch (rxObj.bF.id.SID) {
      case ID_MODULE_ANNOUNCEMENT:
        // Announcement from module - register it
        MCU_RegisterModule();
        break;
      case ID_MODULE_DETAIL:
        // Cell Information from module - process it
        MCU_ProcessCellDetail();
        break;
      case ID_MODULE_HARDWARE:
        MCU_ProcessModuleHardware();
        break;
      case ID_MODULE_STATUS_1:
        // Status packet from module - process it
        MCU_ProcessModuleStatus1();
        break;
      case ID_MODULE_STATUS_2:
        // Status packet from module - process it
        MCU_ProcessModuleStatus2();
        break;
      case ID_MODULE_STATUS_3:
        // Status packet from module - process it
        MCU_ProcessModuleStatus3();
        break;
      case ID_MODULE_TIME_REQUEST:
        // Module is requesting time
        MCU_ProcessModuleTime();
        break;
      case ID_MODULE_CELL_COMM_STATUS1:
        // Cell communication Status #1
        MCU_ProcessCellCommStatus1();
        break;
      default:
        // Unknown Message
        ShowDebugMessage(MSG_UNKNOWN_CAN_ID, rxObj.bF.id.SID);
        break;
    }

    // check for any more messages
    DRV_CANFDSPI_ReceiveChannelEventGet(CAN2, MCU_RX_FIFO, &rxFlags);
  }
}

/***************************************************************************************************************
*     M C U _ T r a n s m i t M e s s a g e Q u e u e                              P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_TransmitMessageQueue(CANFDSPI_MODULE_ID index)
{
    uint8_t attempts = MAX_TXQUEUE_ATTEMPTS;

    // Check if FIFO is not full
    do {
      DRV_CANFDSPI_TransmitChannelEventGet(index, MCU_TX_FIFO, &txFlags);
      if (attempts == 0) {
        Nop();
        Nop();
        DRV_CANFDSPI_ErrorCountStateGet(index, &tec, &rec, &errorFlags);
        ShowDebugMessage(MSG_TX_FIFO_ERROR, index, tec, rec, errorFlags);

        //Flush channel
        DRV_CANFDSPI_TransmitChannelFlush(index, MCU_TX_FIFO);

        return;
      }
      attempts--;
    }
    while (!(txFlags & CAN_TX_FIFO_NOT_FULL_EVENT));

    // Load message and transmit
    uint8_t n = DRV_CANFDSPI_DlcToDataBytes(txObj.bF.ctrl.DLC);
    
    // Raw CAN message logging disabled - use specific message handlers

    DRV_CANFDSPI_TransmitChannelLoad(index, MCU_TX_FIFO, &txObj, txd, n, true);
}

/***************************************************************************************************************
*     M C U _ R e g i s t e r M o d u l e                                          P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_RegisterModule(void){

  CANFRM_MODULE_ANNOUNCEMENT announcement;
  CANFRM_MODULE_REGISTRATION registration;
  uint8_t moduleIndex = 0;
  uint8_t index;

  // copy data to announcement structure
  memcpy(&announcement, rxd,sizeof(announcement));
  ShowDebugMessage(ID_MODULE_ANNOUNCEMENT, announcement.moduleFw, announcement.moduleMfgId, announcement.modulePartId, announcement.moduleUniqueId);

  // Check if module already exists (registered or not)
  moduleIndex = MAX_MODULES_PER_PACK; // Invalid index
  for(index = 0; index < MAX_MODULES_PER_PACK; index++){
    if(module[index].uniqueId == announcement.moduleUniqueId){
      moduleIndex = index;
      break;
    }
  }

  if(moduleIndex < MAX_MODULES_PER_PACK){
    // Existing module - just mark as registered
    module[moduleIndex].isRegistered = true;
    module[moduleIndex].faultCode.commsError = 0;
    module[moduleIndex].lastContact.ticks = htim1.Instance->CNT;
    module[moduleIndex].lastContact.overflows = etTimerOverflows;
    module[moduleIndex].consecutiveTimeouts = 0;  // Reset timeout counter on re-registration
    module[moduleIndex].statusMessagesReceived = 0;  // Reset status tracking
    module[moduleIndex].statusPending = false;  // Start with false to allow polling
    module[moduleIndex].hardwarePending = true;  // Re-request hardware info
    
    // Update module counts
    MCU_UpdateModuleCounts();
    
    ShowDebugMessage(MSG_MODULE_REREGISTER, module[moduleIndex].moduleId);
  }
  else {
    // New module - find first empty slot
    for(index = 0; index < MAX_MODULES_PER_PACK; index++){
      if(module[index].uniqueId == 0){  // Empty slot
        moduleIndex = index;
        break;
      }
    }
    
    if(moduleIndex < MAX_MODULES_PER_PACK){
      // Initialize new module
      module[moduleIndex].moduleId = moduleIndex + 1;  // ID = index + 1
      module[moduleIndex].uniqueId = announcement.moduleUniqueId;
      module[moduleIndex].isRegistered = true;
      module[moduleIndex].fwVersion = announcement.moduleFw;
      module[moduleIndex].partId = announcement.modulePartId;
      module[moduleIndex].mfgId = announcement.moduleMfgId;
      module[moduleIndex].lastContact.ticks = htim1.Instance->CNT;
      module[moduleIndex].lastContact.overflows = etTimerOverflows;
      module[moduleIndex].statusPending = false;  // Start with false to allow immediate polling
      module[moduleIndex].consecutiveTimeouts = 0;  // Initialize timeout counter for new module
      module[moduleIndex].statusMessagesReceived = 0;  // Initialize status tracking
      
      // Update module counts
      MCU_UpdateModuleCounts();
      
      ShowDebugMessage(MSG_NEW_MODULE_REG, module[moduleIndex].moduleId);
    }
    else {
      // No more slots available
      if(debugMessages & DBG_MSG_ANNOUNCE){
        sprintf(tempBuffer,"MCU ERROR - No slots available for module UID=%08x", (int)announcement.moduleUniqueId);
        serialOut(tempBuffer);
      }
      return;
    }
  }

  // send the details back to the module
  registration.moduleId       = module[moduleIndex].moduleId;
  registration.controllerId   = pack.id;
  registration.modulePartId   = module[moduleIndex].partId;
  registration.moduleMfgId    = module[moduleIndex].mfgId;
  registration.moduleUniqueId = module[moduleIndex].uniqueId;

    // clear bitfields
  txObj.word[0] = 0;                              // Configure transmit message
  txObj.word[1] = 0;
  txObj.word[2] = 0;

  memcpy(txd, &registration, sizeof(registration));

  txObj.bF.id.SID = ID_MODULE_REGISTRATION;        // Standard ID
  txObj.bF.id.EID = module[moduleIndex].moduleId;  // Extended ID

  txObj.bF.ctrl.BRS = 0;                          // Bit Rate Switch - use DBR when set, NBR when cleared
  txObj.bF.ctrl.DLC = CAN_DLC_8;                  // 8 bytes to transmit
  txObj.bF.ctrl.FDF = 0;                          // Frame Data Format - CAN FD when set, CAN 2.0 when cleared
  txObj.bF.ctrl.IDE = 1;                          // ID Extension selection - send base frame when cleared, extended frame when set

  ShowDebugMessage(ID_MODULE_REGISTRATION, registration.moduleId, registration.controllerId, registration.moduleMfgId, registration.modulePartId, registration.moduleUniqueId);
  MCU_TransmitMessageQueue(CAN2);                     // Send it
  
  // Reset timeouts for all modules during registration (to account for polling delays)
  MCU_ResetAllModuleTimeouts();
  
  // Request initial status from newly registered module
  MCU_RequestModuleStatus(module[moduleIndex].moduleId);
}

/***************************************************************************************************************
*     M C U _ D e R e g i s t e r M o d u l e                                      P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_DeRegisterModule(uint8_t moduleId){
    CANFRM_MODULE_DEREGISTER deRegistration;

    // configure the packet - format like other module-specific messages
    deRegistration.moduleId = moduleId;
    deRegistration.controllerId = pack.id;

    // Clear transmit object
    txObj.word[0] = 0;                              // Configure transmit message
    txObj.word[1] = 0;
    txObj.word[2] = 0;

    // copy de-registration packet to txd structure
    memcpy(txd, &deRegistration, sizeof(deRegistration));

    txObj.bF.id.SID = ID_MODULE_DEREGISTER;         // Standard ID - 0x518 for individual deregister
    txObj.bF.id.EID = moduleId;                     // Extended ID - specific module

    txObj.bF.ctrl.BRS = 0;                          // Bit Rate Switch - use DBR when set, NBR when cleared
    txObj.bF.ctrl.DLC = CAN_DLC_2;                  // 2 bytes to transmit (moduleId + controllerId)
    txObj.bF.ctrl.FDF = 0;                          // Frame Data Format - CAN FD when set, CAN 2.0 when cleared
    txObj.bF.ctrl.IDE = 1;                          // ID Extension selection - send base frame when cleared, extended frame when set

    ShowDebugMessage(ID_MODULE_DEREGISTER, moduleId);
    MCU_TransmitMessageQueue(CAN2);                  // Send it
}

/***************************************************************************************************************
*     M C U _ D e R e g i s t e r A l l M o d u l e s                              P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_DeRegisterAllModules(void){
    CANFRM_MODULE_ALL_DEREGISTER deRegistration;

    // configure the packet
    deRegistration.controllerId = pack.id;

      // register the new module
    txObj.word[0] = 0;                              // Configure transmit message
    txObj.word[1] = 0;
    txObj.word[2] = 0;

    // copy de-registration packet to txd structure
    memcpy(txd, &deRegistration, sizeof(deRegistration));

    txObj.bF.id.SID = ID_MODULE_ALL_DEREGISTER;     // Standard ID
    txObj.bF.id.EID = 0;                            // Extended ID

    txObj.bF.ctrl.BRS = 0;                          // Bit Rate Switch - use DBR when set, NBR when cleared
    txObj.bF.ctrl.DLC = CAN_DLC_1;                  // 8 bytes to transmit
    txObj.bF.ctrl.FDF = 0;                          // Frame Data Format - CAN FD when set, CAN 2.0 when cleared
    txObj.bF.ctrl.IDE = 1;                          // ID Extension selection - send base frame when cleared, extended frame when set

    ShowDebugMessage(ID_MODULE_ALL_DEREGISTER);
    MCU_TransmitMessageQueue(CAN2);                     // Send it
    
    // Mark all modules as unregistered locally
    for(int i = 0; i < MAX_MODULES_PER_PACK; i++){
        if(module[i].uniqueId != 0){
            module[i].isRegistered = false;
        }
    }
    
    // Update module counts
    MCU_UpdateModuleCounts();
}

/***************************************************************************************************************
*     M C U _ I s o l a t e A l l M o d u l e s                                    P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_IsolateAllModules(void){
  CANFRM_MODULE_ALL_ISOLATE isolate;
  memset(&isolate,0,sizeof(isolate));

  // configure the packet
  isolate.controllerId = pack.id;

    // register the new module
  txObj.word[0] = 0;                              // Configure transmit message
  txObj.word[1] = 0;
  txObj.word[2] = 0;

  // copy isolation packet to txd structure
  memcpy(txd, &isolate, sizeof(isolate));

  txObj.bF.id.SID = ID_MODULE_ALL_ISOLATE;        // Standard ID
  txObj.bF.id.EID = 0;                            // Extended ID

  txObj.bF.ctrl.BRS = 0;                          // Bit Rate Switch - use DBR when set, NBR when cleared
  txObj.bF.ctrl.DLC = CAN_DLC_1;                  // 8 bytes to transmit
  txObj.bF.ctrl.FDF = 0;                          // Frame Data Format - CAN FD when set, CAN 2.0 when cleared
  txObj.bF.ctrl.IDE = 1;                          // ID Extension selection - send base frame when cleared, extended frame when set

  ShowDebugMessage(ID_MODULE_ALL_ISOLATE);
  MCU_TransmitMessageQueue(CAN2);                     // Send it
}

/***************************************************************************************************************
*     M C U _ R e q u e s t M o d u l e A n n o u n c e m e n t                    P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_RequestModuleAnnouncement(void){
  CANFRM_MODULE_ANNOUNCE_REQUEST announceRequest;
  
  // configure the packet
  announceRequest.controllerId = pack.id;
  
  // clear bitfields
  txObj.word[0] = 0;                              // Configure transmit message
  txObj.word[1] = 0;
  txObj.word[2] = 0;
  
  // copy announcement request packet to txd structure
  memcpy(txd, &announceRequest, sizeof(announceRequest));
  
  txObj.bF.id.SID = ID_MODULE_ANNOUNCE_REQUEST;   // Standard ID
  txObj.bF.id.EID = 0;                            // Extended ID - broadcast to all
  
  txObj.bF.ctrl.BRS = 0;                          // Bit Rate Switch - use DBR when set, NBR when cleared
  txObj.bF.ctrl.DLC = CAN_DLC_1;                  // 1 byte to transmit
  txObj.bF.ctrl.FDF = 0;                          // Frame Data Format - CAN FD when set, CAN 2.0 when cleared
  txObj.bF.ctrl.IDE = 1;                          // ID Extension selection - send base frame when cleared, extended frame when set
  
  ShowDebugMessage(ID_MODULE_ANNOUNCE_REQUEST);
  
  // Special case: Reset once-only flags on announcement request (temporary diagnostic aid)
  // This allows us to see new debug messages every 5 seconds when announcements are sent
  ResetDebugOnceOnly();
  
  MCU_TransmitMessageQueue(CAN2);                  // Send it
}

/***************************************************************************************************************
*     M C U _ P r o c e s s M o d u l e T i m e                                    P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_ProcessModuleTime(void){

  time_t packTime;
  CANFRM_MODULE_TIME moduleTime;

  ShowDebugMessage(ID_MODULE_TIME_REQUEST, rxd[0] & 0x1F);  // Module ID from first byte

  // read the RTC as time_t
  packTime = readRTC();

  // set up the frame
  moduleTime.rtcValid = pack.rtcValid;
  moduleTime.time = packTime;

  txObj.word[0] = 0;                              // Configure transmit message
  txObj.word[1] = 0;
  txObj.word[2] = 0;

  // copy moduleTime frame to txd structure
  memcpy(txd, &moduleTime, sizeof(moduleTime));

  txObj.bF.id.SID = ID_MODULE_SET_TIME;     // Standard ID
  txObj.bF.id.EID = 0;                            // Extended ID

  txObj.bF.ctrl.BRS = 0;                          // Bit Rate Switch - use DBR when set, NBR when cleared
  txObj.bF.ctrl.DLC = CAN_DLC_8;                  // 8 bytes to transmit
  txObj.bF.ctrl.FDF = 0;                          // Frame Data Format - CAN FD when set, CAN 2.0 when cleared
  txObj.bF.ctrl.IDE = 1;                          // ID Extension selection - send base frame when cleared, extended frame when set

  ShowDebugMessage(ID_MODULE_SET_TIME);  // Simplified - just log that time was set
  MCU_TransmitMessageQueue(CAN2);                     // Send it
}


/***************************************************************************************************************
*     M C U _ R e q u e s t H a r d w a r e                                        P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_RequestHardware(uint8_t moduleId){

  CANFRM_MODULE_HW_REQUEST hardwareRequest;
  uint8_t moduleIndex;
  uint8_t index;

  //find the module index
  moduleIndex = MAX_MODULES_PER_PACK;
  for(index = 0; index < MAX_MODULES_PER_PACK; index++){
    if(!module[index].isRegistered || module[index].uniqueId == 0) continue;
    //if(status.moduleId == module[index].moduleId)
    if(moduleId == module[index].moduleId)
      moduleIndex = index; // found it - save the index
    }
  if (moduleIndex == MAX_MODULES_PER_PACK){
    // Unregistered module
    if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Unregistered module in MCU_RequestHardware()"); serialOut(tempBuffer);}
  }else{

    // set the hardware pending flag
    module[moduleIndex].hardwarePending = true;

    // set up the message
    hardwareRequest.moduleId = moduleId;

     // clear bit fields
    txObj.word[0] = 0;                              // Configure transmit message
    txObj.word[1] = 0;
    txObj.word[2] = 0;

    memcpy(txd, &hardwareRequest, sizeof(hardwareRequest));

    txObj.bF.id.SID = ID_MODULE_HARDWARE_REQUEST;  // Standard ID
    txObj.bF.id.EID = moduleId;                    // Extended ID

    txObj.bF.ctrl.BRS = 0;                         // Bit Rate Switch - use DBR when set, NBR when cleared
    txObj.bF.ctrl.DLC = CAN_DLC_1;                 // 1 bytes to transmit
    txObj.bF.ctrl.FDF = 0;                         // Frame Data Format - CAN FD when set, CAN 2.0 when cleared
    txObj.bF.ctrl.IDE = 1;                         // ID Extension selection - send base frame when cleared, extended frame when set

    ShowDebugMessage(ID_MODULE_HARDWARE_REQUEST, moduleId);
    MCU_TransmitMessageQueue(CAN2);                    // Send it
  }
}


/***************************************************************************************************************
*     M C U _ P r o c e s s M o d u l e H a r d w a r e                            P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_ProcessModuleHardware(void){

  CANFRM_MODULE_HARDWARE hardware;
  uint8_t moduleIndex;
  uint8_t index;
  float moduleMaxChargeA;
  float moduleMaxDischargeA;
  float moduleMaxEndVoltage;
  //float maxEndVoltage;

  // copy received data to status structure
  memset(&hardware,0,sizeof(hardware));
  memcpy(&hardware, rxd, sizeof(hardware));

  //find the module index
  moduleIndex = MAX_MODULES_PER_PACK;
  for(index = 0; index < MAX_MODULES_PER_PACK; index++){
    if(!module[index].isRegistered || module[index].uniqueId == 0) continue;
    //if(status.moduleId == module[index].moduleId)
    if(rxObj.bF.id.EID == module[index].moduleId)
      moduleIndex = index; // found it - save the index
  }

  if (moduleIndex == MAX_MODULES_PER_PACK){
    // Unregistered module
    if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Unregistered module in MCU_ProcessModuleHardware()"); serialOut(tempBuffer);}
  }else{

    // save the data
    module[moduleIndex].maxChargeA    = hardware.maxChargeA;
    module[moduleIndex].maxDischargeA = hardware.maxDischargeA;
    module[moduleIndex].maxChargeEndV = hardware.maxChargeEndV;
    module[moduleIndex].hwVersion     = hardware.hwVersion;

    // update last contact time
    module[moduleIndex].lastContact.ticks     = htim1.Instance->CNT;
    module[moduleIndex].lastContact.overflows = etTimerOverflows;

    // clear the hardware pending flag
    module[moduleIndex].hardwarePending = false;

    // check reported values for max charge and discharge current
    moduleMaxChargeA    = MODULE_CURRENT_BASE + (module[moduleIndex].maxChargeA    * MODULE_CURRENT_FACTOR);
    moduleMaxDischargeA = MODULE_CURRENT_BASE + (module[moduleIndex].maxDischargeA * MODULE_CURRENT_FACTOR);

    // Check currents are valid and within range
    if ( moduleMaxChargeA < 0){
      // error - incompatible signs used - charge must be > 0!
      if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Module ID=%x incompatible. max charge value (%.2fA) < 0!",rxObj.bF.id.EID,moduleMaxChargeA ); serialOut(tempBuffer);}
      // Set to zero for both
      module[moduleIndex].maxChargeA     = (0/MODULE_CURRENT_FACTOR) - (MODULE_CURRENT_BASE/MODULE_CURRENT_FACTOR);
      module[moduleIndex].maxDischargeA  = (0/MODULE_CURRENT_FACTOR) - (MODULE_CURRENT_BASE/MODULE_CURRENT_FACTOR);
      //Flag the module as incompatible and turn it off
      module[moduleIndex].faultCode.hwIncompatible = true;
      MCU_TransmitState(module[moduleIndex].moduleId,moduleOff);
    }else if (moduleMaxChargeA > MODULE_MAX_CHARGE_A){
      // warning - value exceeds defaults - clip to defaults
      if((debugLevel & DBG_MCU) == DBG_MCU ){ sprintf(tempBuffer,"MCU WARNING - Module ID=%x  max charge value (%.2fA) clipped to %.2fA",rxObj.bF.id.EID, moduleMaxChargeA, MODULE_MAX_CHARGE_A); serialOut(tempBuffer);}
      module[moduleIndex].maxChargeA = (MODULE_MAX_CHARGE_A/MODULE_CURRENT_FACTOR) - (MODULE_CURRENT_BASE/MODULE_CURRENT_FACTOR);
    }
    if (moduleMaxDischargeA > 0){
      // error - incompatible signs used - discharge must be < 0!
      if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Module ID=%x in FAULT as max discharge value (%.2fA) > 0!",rxObj.bF.id.EID,moduleMaxDischargeA); serialOut(tempBuffer);}
      // Set to zero for both
      module[moduleIndex].maxChargeA     = (0/MODULE_CURRENT_FACTOR) - (MODULE_CURRENT_BASE/MODULE_CURRENT_FACTOR);
      module[moduleIndex].maxDischargeA  = (0/MODULE_CURRENT_FACTOR) - (MODULE_CURRENT_BASE/MODULE_CURRENT_FACTOR);
      //Flag the module as incompatible and turn it off
      module[moduleIndex].faultCode.hwIncompatible = true;
      MCU_TransmitState(module[moduleIndex].moduleId,moduleOff);
    } else if (moduleMaxDischargeA < MODULE_MAX_DISCHARGE_A) {
      // warning - value exceeds defaults - clip to defaults
      if((debugLevel & DBG_MCU) == DBG_MCU ){ sprintf(tempBuffer,"MCU WARNING - Module ID=%x  max discharge value (%.2fA) clipped to %.2fA",rxObj.bF.id.EID, moduleMaxDischargeA, MODULE_MAX_DISCHARGE_A); serialOut(tempBuffer);}
      module[moduleIndex].maxDischargeA = (MODULE_MAX_DISCHARGE_A/MODULE_CURRENT_FACTOR) - (MODULE_CURRENT_BASE/MODULE_CURRENT_FACTOR);
    }
    if(debugLevel & DBG_MCU){

      moduleMaxChargeA    = MODULE_CURRENT_BASE + (module[moduleIndex].maxChargeA    * MODULE_CURRENT_FACTOR);
      moduleMaxDischargeA = MODULE_CURRENT_BASE + (module[moduleIndex].maxDischargeA * MODULE_CURRENT_FACTOR);
      moduleMaxEndVoltage = MODULE_VOLTAGE_BASE + (module[moduleIndex].maxChargeEndV * MODULE_VOLTAGE_FACTOR);

      ShowDebugMessage(ID_MODULE_HARDWARE, rxObj.bF.id.EID);
    }
  }
}


/***************************************************************************************************************
*     M C U _ R e q u e s t M o d u l e S t a t u s                               P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_RequestModuleStatus(uint8_t moduleId){

  CANFRM_MODULE_STATUS_REQUEST statusRequest;
  uint8_t moduleIndex;
  uint8_t index;

  //find the module index
  moduleIndex = MAX_MODULES_PER_PACK;
  for(index = 0; index < MAX_MODULES_PER_PACK; index++){
    if(!module[index].isRegistered || module[index].uniqueId == 0) continue;
    //if(status.moduleId == module[index].moduleId)
    if(moduleId == module[index].moduleId)
      moduleIndex = index; // found it - save the index
    }
  if (moduleIndex == MAX_MODULES_PER_PACK){
    // Unregistered module
    if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Unregistered module in MCU_RequestModuleStatus()"); serialOut(tempBuffer);}
  }else{

    // set request flag
    module[moduleIndex].statusPending = true;
    module[moduleIndex].statusMessagesReceived = 0;  // Clear previous status bits

    // request cell detail packet for cell 0
    statusRequest.moduleId = moduleId;

     // clear bit fields
    txObj.word[0] = 0;                              // Configure transmit message
    txObj.word[1] = 0;
    txObj.word[2] = 0;

    memcpy(txd, &statusRequest, sizeof(statusRequest));

    txObj.bF.id.SID = ID_MODULE_STATUS_REQUEST;    // Standard ID
    txObj.bF.id.EID = moduleId;                    // Extended ID

    txObj.bF.ctrl.BRS = 0;                         // Bit Rate Switch - use DBR when set, NBR when cleared
    txObj.bF.ctrl.DLC = CAN_DLC_1;                 // 1 bytes to transmit
    txObj.bF.ctrl.FDF = 0;                         // Frame Data Format - CAN FD when set, CAN 2.0 when cleared
    txObj.bF.ctrl.IDE = 1;                         // ID Extension selection - send base frame when cleared, extended frame when set

    // Use new debug message system for polling message
    ShowDebugMessage(ID_MODULE_STATUS_REQUEST, moduleId);
    MCU_TransmitMessageQueue(CAN2);                    // Send it
    
    // Reset timeout when we request status from a module
    MCU_UpdateModuleContact(moduleIndex);
  }
}




/***************************************************************************************************************
*     M C U _ F i n d M a x V o l t a g e M o d u l e                              P A C K   C O N T R O L L E R
***************************************************************************************************************/
uint8_t MCU_FindMaxVoltageModule(void){

  uint8_t index;
  uint8_t activeModules = 0;
  uint16_t maxVoltage   = 0;
  uint8_t  moduleId = pack.moduleCount + 1; // default this to the last + 1

  //find the module index of the module with the highest mmv that is not in fault
  for(index = 0; index < MAX_MODULES_PER_PACK; index++){
    if(!module[index].isRegistered || module[index].uniqueId == 0) continue;
    if(module[index].mmv > maxVoltage && module[index].faultCode.commsError == false && module[index].faultCode.overCurrent == false && module[index].faultCode.hwIncompatible == false ){
      maxVoltage = module[index].mmv;
      moduleId = module[index].moduleId;
      activeModules++;
    }
  }
  if(maxVoltage == 0 && activeModules > 0 ) return 0;
  else return moduleId;
}



/***************************************************************************************************************
*     M C U _ P r o c e s s M o d u l e S t a t u s 1                              P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_ProcessModuleStatus1(void){

  CANFRM_MODULE_STATUS_1 status1;
  uint8_t moduleIndex;

  // copy received data to status structure
  memset(&status1,0,sizeof(status1));
  memcpy(&status1, rxd, sizeof(status1));

  // Debug output when status is received
  ShowDebugMessage(ID_MODULE_STATUS_1, 
                   rxObj.bF.id.EID, 
                   status1.moduleState & 0x0F,           // Lower 4 bits
                   (status1.moduleStatus) & 0x0F,        // Upper 4 bits  
                   status1.moduleSoc,
                   status1.moduleSoh,
                   status1.cellCount,
                   status1.moduleMmv,                     // module measured voltage
                   (int16_t)status1.moduleMmc);           // module measured current

  // Find the module using the helper function
  moduleIndex = MCU_ModuleIndexFromId(rxObj.bF.id.EID);
  if (moduleIndex == MAX_MODULES_PER_PACK){
    // Unregistered module
    ShowDebugMessage(MSG_UNREGISTERED_MOD, rxObj.bF.id.EID);  // Use EID for module ID
  }else{
    // Track which status message was received
    module[moduleIndex].statusMessagesReceived |= (1 << 0);  // Status1 received
    
    // Only clear statusPending when all 3 received
    if(module[moduleIndex].statusMessagesReceived == 0x07) {  // All 3 bits set
        module[moduleIndex].statusPending = false;
        module[moduleIndex].statusMessagesReceived = 0;  // Reset for next time
    }
    
    // Log timeout counter reset if it was non-zero
    if(module[moduleIndex].consecutiveTimeouts > 0) {
      ShowDebugMessage(MSG_TIMEOUT_RESET, module[moduleIndex].moduleId, module[moduleIndex].consecutiveTimeouts);
    }
    module[moduleIndex].consecutiveTimeouts = 0;  // Reset timeout counter on successful response

    // save the data
    module[moduleIndex].mmc           = status1.moduleMmc; //MODULE_CURRENT_BASE + (MODULE_CURRENT_FACTOR * status1.moduleMmc);
    module[moduleIndex].mmv           = status1.moduleMmv; //MODULE_VOLTAGE_BASE + (MODULE_VOLTAGE_FACTOR * status1.moduleMmv);
    module[moduleIndex].soc           = status1.moduleSoc; //PERCENTAGE_BASE + (PERCENTAGE_FACTOR * status1.moduleSoc);
    module[moduleIndex].soh           = status1.moduleSoh; //PERCENTAGE_BASE + (PERCENTAGE_FACTOR * status1.moduleSoh);
    module[moduleIndex].currentState  = status1.moduleState;
    module[moduleIndex].status        = status1.moduleStatus;
    module[moduleIndex].cellCount     = status1.cellCount;

    // update last contact time
    module[moduleIndex].lastContact.ticks     = htim1.Instance->CNT;
    module[moduleIndex].lastContact.overflows = etTimerOverflows;

    if(module[moduleIndex].currentState ==  module[moduleIndex].command.commandedState){
     // update the command status if the current state is equal to the commmanded state
     module[moduleIndex].command.commandStatus = commandActive;
    }


    if(debugLevel & DBG_MCU){

      float moduleVoltage;
      float moduleCurrent;
      float stateOfCharge;
      float stateOfHealth;
      char  strState[15];
      char  strStatus[15];

      // State
      switch (module[moduleIndex].currentState){
        case 0   : sprintf(strState,"Off(0)"); break;  // both relays off
        case 1   : sprintf(strState,"Standby(1)"); break;  // mechanical on, FET off on all modules
        case 3   : sprintf(strState,"On(3)"); break;  // both relays on for all modules."
        default  : sprintf(strState,"ERROR(%d)",module[moduleIndex].currentState); break;
      }
      // Status
      switch (module[moduleIndex].status){
        case 0  : sprintf(strStatus,"Off(0)");    break; // charge prohibited/discharge prohibited
        case 1  : sprintf(strStatus,"Empty(1)");  break; // charge allowed/discharge prohibited - pack is empty, state is ON"
        case 2  : sprintf(strStatus,"Normal(2)"); break; // charge allowed/discharge allowed - pack in a normal state, state is ON
        case 3  : sprintf(strStatus,"Full(3)");   break; // charge prohibited/discharge allowed - pack is full, state is ON
        default : sprintf(strStatus,"ERROR(%d)",module[moduleIndex].status); break;
      }

      moduleVoltage = MODULE_VOLTAGE_BASE + (module[moduleIndex].mmv  * MODULE_VOLTAGE_FACTOR);
      moduleCurrent = MODULE_CURRENT_BASE + (module[moduleIndex].mmc  * MODULE_CURRENT_FACTOR);
      stateOfCharge = PERCENTAGE_BASE     + (module[moduleIndex].soc  * PERCENTAGE_FACTOR);
      stateOfHealth = PERCENTAGE_BASE     + (module[moduleIndex].soh  * PERCENTAGE_FACTOR);


      // Verbose status logging now handled by ShowDebugMessage
    }
  }
}


/***************************************************************************************************************
*     M C U _ P r o c e s s M o d u l e S t a t u s 2                              P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_ProcessModuleStatus2(void){

  CANFRM_MODULE_STATUS_2 status2;
  uint8_t moduleIndex;

  // copy received data to status structure
  memset(&status2,0,sizeof(status2));
  memcpy(&status2, rxd, sizeof(status2));

  // Debug output when status is received (skip in minimal mode)
  if(debugMessages & DBG_MSG_STATUS2 && !(debugMessages & DBG_MSG_MINIMAL)){
    sprintf(tempBuffer,"MCU RX 0x503 Status #2: ID=%02x, LoV=%dmV, HiV=%dmV, AvgV=%dmV, TotalV=%dmV", 
            rxObj.bF.id.EID,
            status2.cellLoVolt,
            status2.cellHiVolt,
            status2.cellAvgVolt,
            status2.cellTotalV);
    serialOut(tempBuffer);
  }

  // Find the module using the helper function
  moduleIndex = MCU_ModuleIndexFromId(rxObj.bF.id.EID);
  if (moduleIndex == MAX_MODULES_PER_PACK){
    // Unregistered module
    if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Unregistered module in MCU_ProcessModuleStatus2()"); serialOut(tempBuffer);}
  }else{
    // Track which status message was received
    module[moduleIndex].statusMessagesReceived |= (1 << 1);  // Status2 received
    
    // Only clear statusPending when all 3 received
    if(module[moduleIndex].statusMessagesReceived == 0x07) {  // All 3 bits set
        module[moduleIndex].statusPending = false;
        module[moduleIndex].statusMessagesReceived = 0;  // Reset for next time
    }
    
    // Log timeout counter reset if it was non-zero
    if(module[moduleIndex].consecutiveTimeouts > 0) {
      ShowDebugMessage(MSG_TIMEOUT_RESET, module[moduleIndex].moduleId, module[moduleIndex].consecutiveTimeouts);
    }
    module[moduleIndex].consecutiveTimeouts = 0;  // Reset timeout counter on successful response

    // save the data
    module[moduleIndex].cellAvgVolt           = status2.cellAvgVolt;
    module[moduleIndex].cellHiVolt            = status2.cellHiVolt;
    module[moduleIndex].cellLoVolt            = status2.cellLoVolt;
    module[moduleIndex].cellTotalVolt         = status2.cellTotalV;

    // update last contact time
    module[moduleIndex].lastContact.ticks     = htim1.Instance->CNT;
    module[moduleIndex].lastContact.overflows = etTimerOverflows;

    if(debugLevel & DBG_MCU){

      float cellAvgVolt;
      float cellHiVolt;
      float cellLoVolt;
      float cellTotalVolt;

      cellAvgVolt   = CELL_VOLTAGE_BASE + (module[moduleIndex].cellAvgVolt    * CELL_VOLTAGE_FACTOR);
      cellHiVolt    = CELL_VOLTAGE_BASE + (module[moduleIndex].cellHiVolt     * CELL_VOLTAGE_FACTOR);
      cellLoVolt    = CELL_VOLTAGE_BASE + (module[moduleIndex].cellLoVolt     * CELL_VOLTAGE_FACTOR);
      cellTotalVolt = CELL_VOLTAGE_BASE + (module[moduleIndex].cellTotalVolt  * CELL_TOTAL_VOLTAGE_FACTOR);

      // Verbose status logging now handled by ShowDebugMessage
    }
  }
}

/***************************************************************************************************************
*     M C U _ P r o c e s s M o d u l e S t a t u s 3                              P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_ProcessModuleStatus3(void){

  CANFRM_MODULE_STATUS_3 status3;
  uint8_t moduleIndex;

  // copy received data to status structure
  memset(&status3,0,sizeof(status3));
  memcpy(&status3, rxd, sizeof(status3));

  // Debug output when status is received (skip in minimal mode)
  if(debugMessages & DBG_MSG_STATUS3 && !(debugMessages & DBG_MSG_MINIMAL)){
    sprintf(tempBuffer,"MCU RX 0x504 Status #3: ID=%02x, LoT=%dC, HiT=%dC, AvgT=%dC", 
            rxObj.bF.id.EID,
            status3.cellLoTemp,
            status3.cellHiTemp,
            status3.cellAvgTemp);
    serialOut(tempBuffer);
  }

  // Find the module using the helper function
  moduleIndex = MCU_ModuleIndexFromId(rxObj.bF.id.EID);
  if (moduleIndex == MAX_MODULES_PER_PACK){
    // Unregistered module
    if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Unregistered module in MCU_ProcessModuleStatus3()"); serialOut(tempBuffer);}
  }else{
    // Track which status message was received
    module[moduleIndex].statusMessagesReceived |= (1 << 2);  // Status3 received
    
    // Only clear statusPending when all 3 received
    if(module[moduleIndex].statusMessagesReceived == 0x07) {  // All 3 bits set
        module[moduleIndex].statusPending = false;
        module[moduleIndex].statusMessagesReceived = 0;  // Reset for next time
    }
    
    // Log timeout counter reset if it was non-zero
    if(module[moduleIndex].consecutiveTimeouts > 0) {
      ShowDebugMessage(MSG_TIMEOUT_RESET, module[moduleIndex].moduleId, module[moduleIndex].consecutiveTimeouts);
    }
    module[moduleIndex].consecutiveTimeouts = 0;  // Reset timeout counter on successful response

    // save the data
    module[moduleIndex].cellAvgTemp           = status3.cellAvgTemp;
    module[moduleIndex].cellHiTemp            = status3.cellHiTemp;
    module[moduleIndex].cellLoTemp            = status3.cellLoTemp;

    // update last contact time
    module[moduleIndex].lastContact.ticks     = htim1.Instance->CNT;
    module[moduleIndex].lastContact.overflows = etTimerOverflows;

    if(debugLevel & DBG_MCU){

      float cellAvgTemp;
      float cellHiTemp;
      float cellLoTemp;

      cellAvgTemp = TEMPERATURE_BASE + (module[moduleIndex].cellAvgTemp * TEMPERATURE_FACTOR);
      cellHiTemp  = TEMPERATURE_BASE + (module[moduleIndex].cellHiTemp  * TEMPERATURE_FACTOR);
      cellLoTemp  = TEMPERATURE_BASE + (module[moduleIndex].cellLoTemp  * TEMPERATURE_FACTOR);


      // Verbose status logging now handled by ShowDebugMessage
    }
  }
}


/***************************************************************************************************************
*     M C U _ P r o c e s s C e l l C o m m S t a t u s 1                          P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_ProcessCellCommStatus1(void){

  CANFRM_MODULE_CELL_COMM_STATUS_1 cellStatus1;
  uint8_t moduleIndex;

  // copy received data to status structure
  memset(&cellStatus1,0,sizeof(cellStatus1));
  memcpy(&cellStatus1, rxd, sizeof(cellStatus1));
  
  // Find the module using the existing helper function
  moduleIndex = MCU_ModuleIndexFromId(rxObj.bF.id.EID);
  
  if(moduleIndex < MAX_MODULES_PER_PACK){
    // Cell Status is valid communication - clear pending status and reset timeout
    module[moduleIndex].statusPending = false;
    module[moduleIndex].statusMessagesReceived = 0;
    module[moduleIndex].consecutiveTimeouts = 0;
    
    // Update last contact time
    module[moduleIndex].lastContact.ticks = htim1.Instance->CNT;
    module[moduleIndex].lastContact.overflows = etTimerOverflows;
  }

  if(debugLevel & DBG_MCU){ 
    char eCellCPUs[30];
    char eCellI2CFault[20];

    // Process range of low/high cell messages (if any)
    if ((0xff == cellStatus1.leastCellMsgs) &&
        (0 == cellStatus1.mostCellMsgs))
    {
      sprintf(eCellCPUs,"No cells");
    }
    else {
      if (cellStatus1.leastCellMsgs == cellStatus1.mostCellMsgs) {
        sprintf(eCellCPUs, "%u cells", cellStatus1.leastCellMsgs);
      }
      else {
        sprintf(eCellCPUs, "cells %u-%u", cellStatus1.leastCellMsgs, cellStatus1.mostCellMsgs);
      }
    }
    // State either "No faults" or "First fault=%u" cell
    if (0xff == cellStatus1.cellI2cFaultFirst) {
      sprintf(eCellI2CFault, "I2C OK");
    }
    else {
      sprintf(eCellI2CFault, "Cell %u I2C fault", cellStatus1.cellI2cFaultFirst);
    }

    sprintf(tempBuffer,"MCU RX 0x507 Cell Status #1: ID=%02x, %s, Total I2C err=%d, %s, Framing errors=%d",
      rxObj.bF.id.EID, eCellCPUs, cellStatus1.i2cErrors, eCellI2CFault, cellStatus1.framingErrors);

    serialOut(tempBuffer);
  }
}


/***************************************************************************************************************
*     M C U _ R e q u e s t C e l l D e t a i l                                    P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_RequestCellDetail(uint8_t moduleId){

  CANFRM_MODULE_DETAIL_REQUEST detailRequest;

  // request cell detail packet for cell 0
  detailRequest.moduleId = moduleId;
  detailRequest.cellId = 0;

   // clear bit fields
  txObj.word[0] = 0;                              // Configure transmit message
  txObj.word[1] = 0;
  txObj.word[2] = 0;

  memcpy(txd, &detailRequest, sizeof(detailRequest));

  txObj.bF.id.SID = ID_MODULE_DETAIL_REQUEST;    // Standard ID
  txObj.bF.id.EID = moduleId;                    // Extended ID

  txObj.bF.ctrl.BRS = 0;                         // Bit Rate Switch - use DBR when set, NBR when cleared
  txObj.bF.ctrl.DLC = CAN_DLC_3;                 // 3 bytes to transmit
  txObj.bF.ctrl.FDF = 0;                         // Frame Data Format - CAN FD when set, CAN 2.0 when cleared
  txObj.bF.ctrl.IDE = 1;                         // ID Extension selection - send base frame when cleared, extended frame when set

  ShowDebugMessage(MSG_CELL_DETAIL_REQ, moduleId);  // Simplified

  MCU_TransmitMessageQueue(CAN2);                    // Send it
}


/***************************************************************************************************************
*     M C U _ T r a n s m i t S t a t e                                            P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_TransmitState(uint8_t moduleId, moduleState state){

  CANFRM_MODULE_STATE_CHANGE stateChange;
  uint8_t index;

  // set up the frame
  stateChange.moduleId = moduleId;
  stateChange.state = state;
  stateChange.UNUSED_12_15 = 0;
  stateChange.hvBusVoltage = pack.vcuHvBusVoltage;

   // clear bit fields
  txObj.word[0] = 0;                              // Configure transmit message
  txObj.word[1] = 0;
  txObj.word[2] = 0;

  memcpy(txd, &stateChange, sizeof(stateChange));

  txObj.bF.id.SID = ID_MODULE_STATE_CHANGE;     // Standard ID
  txObj.bF.id.EID = moduleId;                    // Extended ID

  txObj.bF.ctrl.BRS = 0;                         // Bit Rate Switch - use DBR when set, NBR when cleared
  txObj.bF.ctrl.DLC = CAN_DLC_4;                 // 4 bytes to transmit
  txObj.bF.ctrl.FDF = 0;                         // Frame Data Format - CAN FD when set, CAN 2.0 when cleared
  txObj.bF.ctrl.IDE = 1;                         // ID Extension selection - send base frame when cleared, extended frame when set

  ShowDebugMessage(ID_MODULE_STATE_CHANGE, moduleId, state);
  MCU_TransmitMessageQueue(CAN2);                    // Send it

  // Update commanded state and command status
  index = MCU_ModuleIndexFromId(moduleId);
  if(index < MAX_MODULES_PER_PACK){
    module[index].command.commandedState  = state;
    module[index].command.commandStatus   = commandIssued;
    module[index].lastTransmit.ticks      = htim1.Instance->CNT;
    module[index].lastTransmit.overflows  = etTimerOverflows;
    // Reset timeout when we transmit TO a module
    MCU_UpdateModuleContact(index);
  }
}


/***************************************************************************************************************
*     M C U _ T r a n s m i t M a x S t a t e                                      P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_TransmitMaxState(moduleState state){

  // This is a broadcast to all module to define their maximum permissible state
  // i.e. They will be able to set state to anything up to and including the maximum state

  CANFRM_MODULE_MAX_STATE maxState;

  maxState.maximumState = state;
  maxState.UNUSED_4_7 = 0;

   // clear bit fields
  txObj.word[0] = 0;                              // Configure transmit message
  txObj.word[1] = 0;
  txObj.word[2] = 0;

  memcpy(txd, &maxState, sizeof(maxState));

  txObj.bF.id.SID = ID_MODULE_MAX_STATE;         // Standard ID
  txObj.bF.id.EID = pack.id;                     // Extended ID = controller ID

  txObj.bF.ctrl.BRS = 0;                         // Bit Rate Switch - use DBR when set, NBR when cleared
  txObj.bF.ctrl.DLC = CAN_DLC_1;                 // 1 bytes to transmit
  txObj.bF.ctrl.FDF = 0;                         // Frame Data Format - CAN FD when set, CAN 2.0 when cleared
  txObj.bF.ctrl.IDE = 1;                         // ID Extension selection - send base frame when cleared, extended frame when set

//  if(debugLevel & DBG_MCU){ sprintf(tempBuffer,"MCU TX 0x517 Maximum Permissible State, STATE=%02x",state); serialOut(tempBuffer);}
  MCU_TransmitMessageQueue(CAN2);                    // Send it

  /*
  // Update commanded state and command status
  index = MCU_ModuleIndexFromId(moduleId);
  if(index < MAX_MODULES_PER_PACK){
    module[index].command.commandedState  = state;
    module[index].command.commandStatus   = commandIssued;
    module[index].lastTransmit.ticks      = htim1.Instance->CNT;
    module[index].lastTransmit.overflows  = etTimerOverflows;
    // Reset timeout when we transmit TO a module
    MCU_UpdateModuleContact(index);
  }
  */
}

/***************************************************************************************************************
*     M C U _ P r o c e s s C e l l D e t a i l                                    P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_ProcessCellDetail(void){

  CANFRM_MODULE_DETAIL_REQUEST detailRequest;
  CANFRM_MODULE_DETAIL cellDetail;
  uint8_t moduleIndex = 0;
  uint8_t index;


  // copy data to announcement structure
  memcpy(&cellDetail, rxd,sizeof(cellDetail));
  ShowDebugMessage(ID_MODULE_DETAIL, rxd[0] & 0x1F);  // Simplified - just log module ID

  //check whether the module is already registered and perhaps lost its registration
  moduleIndex = MAX_MODULES_PER_PACK; //default the index to the next entry (we are using 0 so next index is the moduleCount)
  for(index = 0; index < MAX_MODULES_PER_PACK; index++){
    if(!module[index].isRegistered || module[index].uniqueId == 0) continue;
    //if(cellDetail.moduleId == module[index].moduleId)
    if(rxObj.bF.id.EID == module[index].moduleId)
      moduleIndex = index; // module is already registered, save the index
  }
  
  if (moduleIndex == MAX_MODULES_PER_PACK){
    // Unregistered module
    if((debugLevel & (DBG_MCU + DBG_ERRORS))== (DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Unregistered module in MCU_ProcessCellDetail()"); serialOut(tempBuffer);}
    return;
  }
  
  // store the details
  module[moduleIndex].cellCount = cellDetail.cellCount;
  module[moduleIndex].cell[cellDetail.cellId].soc = cellDetail.cellSoc;
  module[moduleIndex].cell[cellDetail.cellId].soh = cellDetail.cellSoh;
  module[moduleIndex].cell[cellDetail.cellId].temp = cellDetail.cellTemp;
  module[moduleIndex].cell[cellDetail.cellId].voltage= cellDetail.cellVoltage;

  module[moduleIndex].lastContact.ticks = htim1.Instance->CNT;
  module[moduleIndex].lastContact.overflows = etTimerOverflows;

  // request the next cell detail packet
  if (cellDetail.cellId < (cellDetail.cellCount -1)){

    detailRequest.cellId = cellDetail.cellId +1;

     // clear bit fields
    txObj.word[0] = 0;                              // Configure transmit message
    txObj.word[1] = 0;
    txObj.word[2] = 0;

    memcpy(txd, &detailRequest, sizeof(detailRequest));

    txObj.bF.id.SID = ID_MODULE_DETAIL_REQUEST;    // Standard ID
    txObj.bF.id.EID = rxObj.bF.id.EID;             // Extended ID

    txObj.bF.ctrl.BRS = 0;                          // Bit Rate Switch - use DBR when set, NBR when cleared
    txObj.bF.ctrl.DLC = CAN_DLC_2;                  // 2 bytes to transmit
    txObj.bF.ctrl.FDF = 0;                          // Frame Data Format - CAN FD when set, CAN 2.0 when cleared
    txObj.bF.ctrl.IDE = 1;                          // ID Extension selection - send base frame when cleared, extended frame when set

    if(debugLevel & DBG_MCU){ sprintf(tempBuffer,"MCU TX 0x515 Request detail: ID=%02x, CELL=%02x",rxObj.bF.id.EID,detailRequest.cellId ); serialOut(tempBuffer);}
    MCU_TransmitMessageQueue(CAN2);                     // Send it
  }
}

/***************************************************************************************************************
*     M C U _ U p d a t e M o d u l e C o n t a c t                              P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_UpdateModuleContact(uint8_t moduleIndex)
{
    if(moduleIndex < MAX_MODULES_PER_PACK && module[moduleIndex].isRegistered){
        module[moduleIndex].lastContact.ticks = htim1.Instance->CNT;
        module[moduleIndex].lastContact.overflows = etTimerOverflows;
        module[moduleIndex].consecutiveTimeouts = 0;  // Reset timeout counter on any contact
    }
}

/***************************************************************************************************************
*     M C U _ R e s e t A l l M o d u l e T i m e o u t s                        P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_ResetAllModuleTimeouts(void)
{
    for(int i = 0; i < MAX_MODULES_PER_PACK; i++){
        if(module[i].isRegistered && module[i].uniqueId != 0){
            module[i].lastContact.ticks = htim1.Instance->CNT;
            module[i].lastContact.overflows = etTimerOverflows;
            // Don't reset consecutiveTimeouts here - only on actual contact
        }
    }
}

/***************************************************************************************************************
*     M C U _ M o d u l e I n d e x F r o m I d                                    P A C K   C O N T R O L L E R
***************************************************************************************************************/
uint8_t MCU_ModuleIndexFromId(uint8_t moduleId)
{
   uint8_t moduleIndex;
   uint8_t index;

   //find the module index
   moduleIndex = MAX_MODULES_PER_PACK;
   for(index = 0; index < MAX_MODULES_PER_PACK; index++){
     if(moduleId == module[index].moduleId && module[index].isRegistered)
       moduleIndex = index; // found it - save the index
     }
   if (moduleIndex != MAX_MODULES_PER_PACK) return moduleIndex;
   else return MAX_MODULES_PER_PACK; // unregistered module

}

/***************************************************************************************************************
*     M C U _ U p d a t e M o d u l e C o u n t s                                  P A C K   C O N T R O L L E R
***************************************************************************************************************/
void MCU_UpdateModuleCounts(void)
{
    pack.totalModules = 0;
    pack.activeModules = 0;
    pack.moduleCount = 0;  // Keep for compatibility
    
    for(int i = 0; i < MAX_MODULES_PER_PACK; i++){
        if(module[i].uniqueId != 0){
            pack.totalModules++;
            if(module[i].isRegistered){
                pack.activeModules++;
                pack.moduleCount++;  // Keep for compatibility
            }
        }
    }
}


/***************************************************************************************************************
*     M C U _ T i c k S i n c e L a s t M e s s a g e                              P A C K   C O N T R O L L E R
***************************************************************************************************************/
uint32_t MCU_TicksSinceLastMessage(uint8_t moduleId)
{
  uint32_t elapsedTicks;
  uint32_t timerCNT;
  uint32_t overFlows;

  //take a snapshot of the timer counter and overflows
  timerCNT = htim1.Instance->CNT;
  overFlows = etTimerOverflows;

  uint8_t moduleIndex = MCU_ModuleIndexFromId(moduleId);
  if(moduleIndex < MAX_MODULES_PER_PACK){
    if ((overFlows - module[moduleIndex].lastContact.overflows) == 0){
      elapsedTicks = timerCNT - module[moduleIndex].lastContact.ticks;
    } else {
      //             (           ticks last contact to overflow point               ) + (                           ticks in completed overflows                               ) + ( ticks in current timer period)
      elapsedTicks = ((htim1.Init.Period +1) - module[moduleIndex].lastContact.ticks) + ( (htim1.Init.Period +1) * (overFlows - (module[moduleIndex].lastContact.overflows +1))) + (timerCNT);
    }
    return elapsedTicks;
  } else {
    // Unregistered module
    if((debugLevel & ( DBG_MCU + DBG_ERRORS))== ( DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Unregistered module in MCU_TicksSinceLastMessage()"); serialOut(tempBuffer);}
    return 0; // TODO Handle this error
  }
}

/***************************************************************************************************************
*     M C U _ T i c k S i n c e L a s t S t a t e T x                              P A C K   C O N T R O L L E R
***************************************************************************************************************/
uint32_t MCU_TicksSinceLastStateTx(uint8_t moduleId)
{
  uint32_t elapsedTicks;
  uint32_t timerCNT;
  uint32_t overFlows;

  //take a snapshot of the timer counter and overflows
  timerCNT = htim1.Instance->CNT;
  overFlows = etTimerOverflows;

  uint8_t moduleIndex = MCU_ModuleIndexFromId(moduleId);
  if(moduleIndex < MAX_MODULES_PER_PACK){
    if ((overFlows - module[moduleIndex].lastTransmit.overflows) ==0){
      elapsedTicks = timerCNT - module[moduleIndex].lastTransmit.ticks;
    } else {
      //             (           ticks last contact to overflow point                ) + (                           ticks in completed overflows                                ) + ( ticks in current timer period)
      elapsedTicks = ((htim1.Init.Period +1) - module[moduleIndex].lastTransmit.ticks) + ( (htim1.Init.Period +1) * (overFlows - (module[moduleIndex].lastTransmit.overflows +1))) + (timerCNT);
    }
    return elapsedTicks;
  } else {
    // Unregistered module
    if((debugLevel & ( DBG_MCU + DBG_ERRORS))== ( DBG_MCU + DBG_ERRORS)){ sprintf(tempBuffer,"MCU ERROR - Unregistered module in MCU_TicksSinceLastTx()"); serialOut(tempBuffer);}
    return 0; // TODO Handle this error
  }
}

/***************************************************************************************************************
*     M C U _ E l a p s e d T i c k s                                              P A C K   C O N T R O L L E R
***************************************************************************************************************/
uint32_t MCU_ElapsedTicks(lastContact_t* pLastContact)
{
  uint32_t elapsedTicks;
  uint32_t timerCNT;
  uint32_t overFlows;

  //take a snapshot of the timer counter and overflows
  timerCNT = htim1.Instance->CNT;
  overFlows = etTimerOverflows;

  if ((overFlows - pLastContact->overflows) == 0){
    elapsedTicks = timerCNT - pLastContact->ticks;
  } else {
    //             (           ticks last contact to overflow point               ) + (                           ticks in completed overflows                               ) + ( ticks in current timer period)
    elapsedTicks = ((htim1.Init.Period +1) - pLastContact->ticks) + ( (htim1.Init.Period +1) * (overFlows - (pLastContact->overflows +1))) + (timerCNT);
  }
  return elapsedTicks;
}

/***************************************************************************************************************
*     C A N _ T e s t R e g i s t e r A c c e s s                                  P A C K   C O N T R O L L E R
***************************************************************************************************************/
bool CAN_TestRegisterAccess(CANFDSPI_MODULE_ID index)
{
  // Variables
  uint8_t length;
  bool good = false;
  Nop();

  // Verify read/write with different access length
  // Note: registers can be accessed in multiples of bytes
  for (length = 1; length <= MAX_DATA_BYTES; length++) {
    for (i = 0; i < length; i++) {
        txd[i] = rand() & 0x7f; // Bit 31 of Filter objects is not implemented
        rxd[i] = 0xff;
    }
    Nop();

    // Write data to registers
    DRV_CANFDSPI_WriteByteArray(index, cREGADDR_CiFLTOBJ, txd, length);

    // Read data back from registers
    DRV_CANFDSPI_ReadByteArray(index, cREGADDR_CiFLTOBJ, rxd, length);

    // Verify
    good = false;
    for (i = 0; i < length; i++) {
        good = txd[i] == rxd[i];

      if (!good) {
        Nop();
        Nop();

        // Data mismatch
        return false;
      }
    }
  }
  Nop();
  Nop();
  return true;
}

//! Test RAM access
/***************************************************************************************************************
*     C A N _ T e s t R a m A c c e s s                                            P A C K   C O N T R O L L E R
***************************************************************************************************************/
bool CAN_TestRamAccess(CANFDSPI_MODULE_ID index)
{
  // Variables
  uint8_t length;
  bool good = false;
  Nop();

  // Verify read/write with different access length
  // Note: RAM can only be accessed in multiples of 4 bytes
  for (length = 4; length <= MAX_DATA_BYTES; length += 4) {
    for (i = 0; i < length; i++) {
        txd[i] = rand() & 0xff;
        rxd[i] = 0xff;
    }
    Nop();

    // Write data to RAM
    DRV_CANFDSPI_WriteByteArray(index, cRAMADDR_START, txd, length);

    // Read data back from RAM
    DRV_CANFDSPI_ReadByteArray(index, cRAMADDR_START, rxd, length);

    // Verify
    good = false;
    for (i = 0; i < length; i++) {
        good = txd[i] == rxd[i];

      if (!good) {
        Nop();
        Nop();

        // Data mismatch
        return false;
      }
    }
  }
  return true;
}





