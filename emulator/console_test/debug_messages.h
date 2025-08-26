// Simplified debug message definitions for console test
#ifndef DEBUG_MESSAGES_H
#define DEBUG_MESSAGES_H

// WEB4 Key Distribution Messages (0xF010 - 0xF021 range)
#define MSG_WEB4_KEYS_LOADED        0xF010  // Keys loaded from EEPROM
#define MSG_WEB4_NO_STORED_KEYS     0xF011  // No keys in EEPROM
#define MSG_WEB4_STATUS_RECEIVED    0xF012  // Key status message received
#define MSG_WEB4_INVALID_LENGTH     0xF013  // Invalid message length
#define MSG_WEB4_INVALID_CHUNK      0xF014  // Invalid chunk number
#define MSG_WEB4_RECEPTION_START    0xF015  // Key reception started
#define MSG_WEB4_DUPLICATE_CHUNK    0xF016  // Duplicate chunk received
#define MSG_WEB4_CHUNK_RECEIVED     0xF017  // Chunk successfully received
#define MSG_WEB4_CHECKSUM_ERROR     0xF018  // Checksum validation failed
#define MSG_WEB4_PACK_KEY_STORED    0xF019  // Pack key stored
#define MSG_WEB4_APP_KEY_STORED     0xF01A  // App key stored
#define MSG_WEB4_COMPONENT_IDS_STORED 0xF01B  // Component IDs stored
#define MSG_WEB4_KEYS_SAVED_EEPROM  0xF01C  // Keys saved to EEPROM
#define MSG_WEB4_ACK_SENT           0xF01D  // Acknowledgment sent
#define MSG_WEB4_RECEPTION_TIMEOUT  0xF01E  // Reception timeout
#define MSG_WEB4_KEY_STATUS         0xF01F  // Key validity status
#define MSG_WEB4_COMPONENT_STATUS   0xF020  // Component ID status
#define MSG_WEB4_CHUNK_DATA         0xF021  // Chunk data debug

#endif // DEBUG_MESSAGES_H