 /**************************************************************************************************************
 * @file           : CAN_ID_ALL.h                                                 P A C K   C O N T R O L L E R
 * @brief          : Modbatt CAN Protocol Definitions - Single Source of Truth
 ***************************************************************************************************************
 * Copyright (C) 2023-2024 Modular Battery Technologies, Inc.
 * US Patents 11,380,942; 11,469,470; 11,575,270; others. All rights reserved
 *
 * This file is the SINGLE SOURCE OF TRUTH for all CAN protocol definitions across:
 *   - ModuleCPU (ATmega64M1)
 *   - Pack Controller (STM32WB55)
 *   - Pack Emulator (C++ Builder)
 *   - modbatt-CAN tools
 *
 * All projects should reference this file via relative path from ai-agents/ root
 **************************************************************************************************************/
#ifndef INC_CAN_ID_ALL_H_
#define INC_CAN_ID_ALL_H_

// ========================================
// BMS DIAGNOSTIC MESSAGES (0x220-0x228)
// VCU <-> Pack Controller Diagnostic Interface
// ========================================

#define ID_BMS_STATUS               0x220
#define ID_BMS_FAULT                0x221
#define ID_BMS_CELL_DATA            0x222
#define ID_BMS_IO                   0x223
#define ID_BMS_LIMITS               0x224
#define ID_BMS_MOD_DATA_1           0x225
#define ID_BMS_MOD_DATA_2           0x226
#define ID_BMS_MOD_DATA_3           0x227
#define ID_BMS_MOD_DATA_4           0x228

// ========================================
// SD CARD TRANSFER MESSAGES (0x3F0-0x3F3)
// Pack Controller <-> Module Controller
// High-speed bulk transfer protocol
// ========================================

// Pack Controller to Module Controller
#define ID_SD_SECTOR_REQUEST        0x3F0  // Base ID, add module ID
#define ID_SD_WINDOW_ACK            0x3F2  // Base ID, add module ID

// Module Controller to Pack Controller
#define ID_SD_DATA_CHUNK            0x3F1  // Base ID for extended frames
#define ID_SD_TRANSFER_STATUS       0x3F3  // Base ID, add module ID

// SD Card Transfer Constants
#define SD_SECTOR_SIZE              512
#define SD_CHUNK_SIZE               8
#define SD_CHUNKS_PER_WINDOW        16
#define SD_WINDOW_SIZE              128    // 16 chunks * 8 bytes
#define SD_WINDOWS_PER_SECTOR       4      // 512 bytes / 128 bytes
#define SD_TOTAL_CHUNKS             64     // 512 bytes / 8 bytes

// SD Transfer Commands
#define SD_CMD_READ_SECTOR          0x01
#define SD_CMD_WINDOW_ACK           0x02
#define SD_CMD_TRANSFER_STATUS      0x03

// SD Transfer Status Codes
#define SD_STATUS_COMPLETE          0x00
#define SD_STATUS_IN_PROGRESS       0x01
#define SD_STATUS_SD_ERROR          0x10
#define SD_STATUS_OUT_OF_RANGE      0x11
#define SD_STATUS_BUSY              0x12
#define SD_STATUS_CRC_ERROR         0x20
#define SD_STATUS_UNKNOWN_ERROR     0xFF

// ========================================
// VCU <-> PACK CONTROLLER (0x400-0x44F)
// BMS/VCU Standard Interface
// ========================================

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

// ========================================
// PACK <-> MODULE CONTROLLER (0x500-0x52F)
// Module Management Protocol
// ========================================

// Module Controller to Pack Controller
#define ID_MODULE_ANNOUNCEMENT      0x500
#define ID_MODULE_HARDWARE          0x501
#define ID_MODULE_STATUS_1          0x502
#define ID_MODULE_STATUS_2          0x503
#define ID_MODULE_STATUS_3          0x504
#define ID_MODULE_DETAIL            0x505
#define ID_MODULE_TIME_REQUEST      0x506
#define ID_MODULE_CELL_COMM_STATUS1 0x507
#define ID_MODULE_CELL_COMM_STATUS2 0x508
#define ID_MODULE_STATUS_4          0x509

// Pack Controller to Module Controller
#define ID_MODULE_REGISTRATION      0x510
#define ID_MODULE_HARDWARE_REQUEST  0x511
#define ID_MODULE_STATUS_REQUEST    0x512
#define ID_MODULE_STATE_CHANGE      0x514
#define ID_MODULE_DETAIL_REQUEST    0x515
#define ID_MODULE_SET_TIME          0x516
#define ID_MODULE_MAX_STATE         0x517
#define ID_MODULE_DEREGISTER        0x518
#define ID_MODULE_ANNOUNCE_REQUEST  0x51D
#define ID_MODULE_ALL_DEREGISTER    0x51E
#define ID_MODULE_ALL_ISOLATE       0x51F

// Frame Transfer Protocol (Bidirectional)
// Used to transfer EEPROM frames from Module to Pack for diagnostic/logging
#define ID_FRAME_TRANSFER_REQUEST   0x520  // Pack -> Module: Request frame transfer
#define ID_FRAME_TRANSFER_START     0x521  // Module -> Pack: Start frame transfer
#define ID_FRAME_TRANSFER_DATA      0x522  // Module -> Pack: Frame data segment
#define ID_FRAME_TRANSFER_END       0x523  // Module -> Pack: End frame transfer

#endif /* INC_CAN_ID_ALL_H_ */
