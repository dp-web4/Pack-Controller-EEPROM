 /**************************************************************************************************************
 * @file           : can_id_bms_vcu.h                                              P A C K   C O N T R O L L E R
 * @brief          : Modbatt CAN packet identifiers for BMS/VCU interface
 ***************************************************************************************************************
 * Copyright (C) 2023-2024 Modular Battery Technologies, Inc.
 * US Patents 11,380,942; 11,469,470; 11,575,270; others. All rights reserved
 **************************************************************************************************************/
#ifndef INC_CAN_ID_BMS_VCU_H_
#define INC_CAN_ID_BMS_VCU_H_

// CAN Frame IDs
// VCU to Pack Controller
#define ID_VCU_COMMAND              0x400
#define ID_VCU_TIME                 0x401
#define ID_VCU_READ_EEPROM          0x402
#define ID_VCU_WRITE_EEPROM         0x403
#define ID_VCU_MODULE_COMMAND       0x404
#define ID_VCU_KEEP_ALIVE           0x405
#define ID_VCU_REQUEST_MODULE_LIST  0x406

// Web4 Key Distribution (VCU to Pack Controller)
#define ID_VCU_WEB4_PACK_KEY_HALF   0x407    // Pack controller's device key half
#define ID_VCU_WEB4_APP_KEY_HALF    0x408    // App's device key half  
#define ID_VCU_WEB4_COMPONENT_IDS   0x409    // Component IDs
#define ID_VCU_WEB4_KEY_STATUS      0x40A    // Key distribution status/confirmation

// Pack Controller to VCU
#define ID_BMS_STATE                0x410

#define ID_MODULE_STATE             0x411
#define ID_MODULE_POWER             0x412
#define ID_MODULE_CELL_VOLTAGE      0x413
#define ID_MODULE_CELL_TEMP         0x414
#define ID_MODULE_CELL_ID           0x415
#define ID_MODULE_LIMITS            0x416
#define ID_MODULE_LIST              0x417

#define ID_BMS_DATA_1               0x421
#define ID_BMS_DATA_2               0x422
#define ID_BMS_DATA_3               0x423
#define ID_BMS_DATA_4               0x424
#define ID_BMS_DATA_5               0x425
#define ID_BMS_DATA_6               0x426
#define ID_BMS_DATA_7               0x427
#define ID_BMS_DATA_8               0x428
#define ID_BMS_DATA_9               0x429
#define ID_BMS_DATA_10              0x430
#define ID_BMS_TIME_REQUEST         0x440
#define ID_BMS_EEPROM_DATA          0x441

// Web4 Key Distribution Responses (Pack Controller to VCU)
#define ID_BMS_WEB4_PACK_KEY_ACK    0x4A7    // Pack key half acknowledgment (0x407 + 0xA0)
#define ID_BMS_WEB4_APP_KEY_ACK     0x4A8    // App key half acknowledgment (0x408 + 0xA0)
#define ID_BMS_WEB4_COMPONENT_ACK   0x4A9    // Component IDs acknowledgment (0x409 + 0xA0)



#endif /* INC_CAN_ID_BMS_VCU_H_ */
