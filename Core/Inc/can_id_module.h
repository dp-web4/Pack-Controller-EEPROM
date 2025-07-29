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

#endif /* INC_CAN_ID_MODULE_H_ */
