 /**************************************************************************************************************
 * @file           : can_id_module.h                                              P A C K   C O N T R O L L E R
 * @brief          : Modbatt CAN packet identifiers for Pack Controller <-> Module Controller communication
 ***************************************************************************************************************
 * Copyright (C) 2023-2024 Modular Battery Technologies, Inc.
 * US Patents 11,380,942; 11,469,470; 11,575,270; others. All rights reserved
 **************************************************************************************************************/
#ifndef INC_CAN_ID_MODULE_H_
#define INC_CAN_ID_MODULE_H_

// CAN Packet IDs
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

// SD Card Transfer Messages
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

#endif /* INC_CAN_ID_MODULE_H_ */
