/**************************************************************************************************************
 * @file           : sd_transfer.h                                                P A C K   C O N T R O L L E R
 * @brief          : SD card sector transfer protocol structures and functions
 ***************************************************************************************************************
 * Copyright (C) 2025 Modular Battery Technologies, Inc.
 * All rights reserved
 **************************************************************************************************************/
#ifndef INC_SD_TRANSFER_H_
#define INC_SD_TRANSFER_H_

#include <stdint.h>
#include <stdbool.h>
#include "can_id_module.h"

// Transfer states
typedef enum {
    SD_XFER_IDLE = 0,
    SD_XFER_REQUESTING,
    SD_XFER_RECEIVING,
    SD_XFER_COMPLETE,
    SD_XFER_ERROR,
    SD_XFER_TIMEOUT
} sd_transfer_state_t;

// SD card sector request structure
typedef struct {
    uint8_t command;        // SD_CMD_READ_SECTOR
    uint8_t transfer_id;    // Unique ID for this transfer
    uint32_t sector_num;    // Sector number to read
    uint8_t options;        // Bit 0: priority, others reserved
    uint8_t checksum;       // XOR checksum of bytes 0-6
} __attribute__((packed)) sd_sector_request_t;

// SD window acknowledgment structure
typedef struct {
    uint8_t command;        // SD_CMD_WINDOW_ACK
    uint8_t transfer_id;    // Transfer being acknowledged
    uint8_t window_id;      // Window being acknowledged (0-3)
    uint16_t bitmap;        // Received chunk bitmap
    uint8_t status;         // 0x00=OK, 0x01=Retry, 0xFF=Abort
    uint16_t crc16;         // Running CRC16 of received data
} __attribute__((packed)) sd_window_ack_t;

// SD transfer status structure
typedef struct {
    uint8_t command;        // SD_CMD_TRANSFER_STATUS
    uint8_t transfer_id;    // Transfer ID
    uint8_t status_code;    // Status code (SD_STATUS_*)
    uint8_t windows_done;   // Number of windows completed
    uint16_t final_crc;     // Final CRC16 of complete sector
    uint16_t time_ms;       // Time elapsed in milliseconds
} __attribute__((packed)) sd_transfer_status_t;

// Transfer tracking structure (Pack side)
typedef struct {
    uint8_t module_id;              // Module we're transferring from
    uint8_t transfer_id;            // Transfer ID
    sd_transfer_state_t state;      // Current state
    uint32_t sector_num;            // Sector being transferred

    // Buffer management
    uint8_t buffer[SD_SECTOR_SIZE]; // 512-byte sector buffer
    uint16_t chunk_bitmap[SD_WINDOWS_PER_SECTOR]; // Received chunks per window
    uint8_t current_window;          // Current window being received (0-3)

    // Timing
    uint32_t start_time;            // Transfer start time
    uint32_t window_timeout;        // Window timeout timestamp
    uint8_t retry_count;            // Retries for current window

    // Verification
    uint16_t running_crc;           // Running CRC16
    uint16_t expected_crc;          // Expected final CRC from module
} sd_transfer_context_t;

// Extended CAN ID structure for SD data chunks
typedef union {
    uint32_t raw;
    struct {
        uint32_t module_id : 8;     // Bits 0-7: Module ID
        uint32_t transfer_id : 2;   // Bits 8-9: Transfer ID (0-3)
        uint32_t chunk_num : 4;     // Bits 10-13: Chunk in window (0-15)
        uint32_t window_id : 2;     // Bits 14-15: Window ID (0-3)
        uint32_t last_chunk : 1;    // Bit 16: Last chunk flag
        uint32_t mode : 1;          // Bit 17: 1 = Data transfer
        uint32_t base_id : 11;      // Bits 18-28: Base CAN ID (0x3F1)
    } __attribute__((packed)) fields;
} sd_data_ext_id_t;

// Function prototypes for Pack Controller
void sd_transfer_init(void);
bool sd_request_sector(uint8_t module_id, uint32_t sector_num, uint8_t transfer_id);
bool sd_process_data_chunk(uint32_t ext_id, const uint8_t* data);
bool sd_process_transfer_status(uint8_t module_id, const sd_transfer_status_t* status);
void sd_send_window_ack(uint8_t module_id, uint8_t transfer_id, uint8_t window_id, uint16_t bitmap);
void sd_check_timeouts(void);
sd_transfer_context_t* sd_get_transfer(uint8_t transfer_id);
bool sd_is_transfer_complete(uint8_t transfer_id);
const uint8_t* sd_get_sector_data(uint8_t transfer_id);

// CRC16 calculation
uint16_t sd_crc16(const uint8_t* data, uint16_t length);
uint16_t sd_crc16_update(uint16_t crc, uint8_t data);

#endif /* INC_SD_TRANSFER_H_ */