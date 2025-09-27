/**************************************************************************************************************
 * @file           : sd_frame.h                                                   P A C K   C O N T R O L L E R
 * @brief          : Frame-based SD card data storage structures and functions
 ***************************************************************************************************************
 * Copyright (C) 2025 Modular Battery Technologies, Inc.
 * All rights reserved
 **************************************************************************************************************/
#ifndef INC_SD_FRAME_H_
#define INC_SD_FRAME_H_

#include <stdint.h>
#include <stdbool.h>

// Frame constants
#define SD_FRAME_SIZE           1024    // Total frame size in bytes
#define SD_FRAME_HEADER_SIZE    32      // Frame header size
#define SD_FRAME_DATA_SIZE      992     // Frame data area size
#define SD_SECTORS_PER_FRAME    2       // Number of SD sectors per frame

// Maximum values
#define MAX_CELLS_PER_MODULE    94      // Maximum cells supported
#define MAX_FRAME_NUMBER        0xFFFFFF // 24-bit frame number (16.7M frames)

// Frame status flags
#define FRAME_STATUS_VALID      0x01    // Frame has valid data
#define FRAME_STATUS_PARTIAL    0x02    // Frame partially filled
#define FRAME_STATUS_CORRUPT    0x04    // Frame CRC failed
#define FRAME_STATUS_WRITING    0x08    // Frame being written to SD

// CAN message IDs for frame operations
#define ID_FRAME_INFO_REQUEST   0x3E0   // Pack → Module (+ module_id)
#define ID_FRAME_INFO_RESPONSE  0x3E1   // Module → Pack (+ module_id)
#define ID_FRAME_REQUEST        0x3E2   // Pack → Module (+ module_id)
#define ID_FRAME_DATA           0x3E3   // Module → Pack (extended ID)
#define ID_FRAME_STATUS         0x3E4   // Module → Pack (+ module_id)

// Frame commands
#define FRAME_CMD_GET_INFO      0x10    // Get current frame info
#define FRAME_CMD_GET_FRAME     0x11    // Get frame data
#define FRAME_CMD_GET_CURRENT   0x12    // Get current (partial) frame
#define FRAME_CMD_STOP_TRANSFER 0x13    // Stop ongoing transfer

// Frame transfer status codes
#define FRAME_STATUS_OK         0x00    // Transfer successful
#define FRAME_STATUS_BUSY       0x01    // Module busy with another transfer
#define FRAME_STATUS_NOT_FOUND  0x02    // Requested frame doesn't exist
#define FRAME_STATUS_SD_ERROR   0x03    // SD card read error
#define FRAME_STATUS_CRC_ERROR  0x04    // Frame CRC mismatch

// Frame structure (1024 bytes total)
typedef struct {
    // Header (32 bytes)
    uint32_t frame_number;          // Sequential frame ID
    uint32_t timestamp;             // RTC timestamp when frame started
    uint16_t granularity;           // Number of string readings in frame
    uint16_t current_index;         // Current write position (0 to granularity-1)
    uint8_t  cells_expected;        // Number of cells per string
    uint8_t  module_id;             // Module that owns this frame
    uint16_t frame_crc;             // CRC16 of entire frame
    uint8_t  status_flags;          // Frame status flags
    uint8_t  reserved[15];          // Reserved for future use

    // Data area (992 bytes)
    uint8_t  data[SD_FRAME_DATA_SIZE];  // Circular buffer of string readings
} __attribute__((packed)) sd_frame_t;

// Compile-time check for frame size
_Static_assert(sizeof(sd_frame_t) == SD_FRAME_SIZE, "Frame structure must be exactly 1024 bytes");

// String reading structure (size varies with cell count)
typedef struct {
    uint16_t voltage[0];        // Variable length array for voltages
    // uint16_t temperature[0]; // Temperatures follow voltages
} string_reading_t;

// Frame info response structure
typedef struct {
    uint32_t current_frame_num;    // Current frame number (24 bits used)
    uint8_t  current_index;        // Current position in frame
    uint8_t  granularity;          // Readings per frame
    uint8_t  cells_expected;       // Cells per reading
    uint8_t  reserved;             // Alignment
} __attribute__((packed)) frame_info_t;

// Frame request structure
typedef struct {
    uint8_t  command;              // FRAME_CMD_GET_FRAME
    uint32_t frame_number;         // Requested frame (24 bits used)
    uint8_t  transfer_id;          // Unique transfer ID
    uint8_t  reserved[2];          // Padding to 8 bytes
} __attribute__((packed)) frame_request_t;

// Frame transfer context (for managing transfers)
typedef struct {
    bool     active;               // Transfer in progress
    uint8_t  transfer_id;          // Current transfer ID
    uint32_t frame_number;         // Frame being transferred
    uint8_t  current_window;       // Current window (0-7 for 1KB)
    uint16_t chunk_bitmap[8];      // Received chunks per window
    uint32_t start_time;           // Transfer start timestamp
    sd_frame_t* frame_buffer;      // Pointer to frame data
} frame_transfer_context_t;

// Function prototypes

// Frame initialization and management
void sd_frame_init(uint8_t module_id, uint8_t cells_expected);
uint16_t sd_frame_calculate_granularity(uint8_t cells_expected);
bool sd_frame_load_from_sd(uint32_t frame_number, sd_frame_t* frame);
bool sd_frame_save_to_sd(const sd_frame_t* frame);

// Data operations
bool sd_frame_add_reading(const uint16_t* voltages, const uint16_t* temperatures);
bool sd_frame_is_full(void);
void sd_frame_advance(void);
sd_frame_t* sd_frame_get_current(void);

// EEPROM operations
uint32_t sd_frame_read_counter_from_eeprom(void);
void sd_frame_write_counter_to_eeprom(uint32_t frame_number);
void sd_frame_update_counter_bytewise(uint32_t new_frame_number);

// CAN protocol handlers (Pack side)
void sd_frame_request_info(uint8_t module_id);
void sd_frame_request_data(uint8_t module_id, uint32_t frame_number, uint8_t transfer_id);
bool sd_frame_process_info_response(uint8_t module_id, const frame_info_t* info);
bool sd_frame_process_data_chunk(uint32_t ext_id, const uint8_t* data);
bool sd_frame_send_window_ack(uint8_t module_id, uint8_t window_id, uint16_t bitmap);

// CAN protocol handlers (Module side)
void sd_frame_handle_info_request(uint8_t requester_id);
void sd_frame_handle_frame_request(const frame_request_t* request);
void sd_frame_send_current_frame(uint8_t requester_id);
void sd_frame_send_frame_window(uint8_t window_id);

// Transfer management
bool sd_frame_start_transfer(uint8_t module_id, uint32_t frame_number);
void sd_frame_abort_transfer(uint8_t transfer_id);
frame_transfer_context_t* sd_frame_get_transfer(uint8_t transfer_id);
void sd_frame_check_timeouts(void);

// Utility functions
uint16_t sd_frame_calculate_crc(const sd_frame_t* frame);
bool sd_frame_validate_crc(const sd_frame_t* frame);
void sd_frame_clear(sd_frame_t* frame);

// Sector address calculation
static inline uint32_t sd_frame_to_sector(uint32_t frame_number) {
    return frame_number * SD_SECTORS_PER_FRAME;
}

// Reading offset calculation within frame data
static inline uint16_t sd_frame_get_reading_offset(uint16_t index, uint8_t cells_expected) {
    return index * cells_expected * 4; // 2 bytes voltage + 2 bytes temp per cell
}

#endif /* INC_SD_FRAME_H_ */