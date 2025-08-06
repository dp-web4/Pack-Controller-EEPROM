# Proposed: Add Specific Flag for TX FIFO Errors

## Current Situation
- MSG_TX_FIFO_ERROR and MSG_UNKNOWN_CAN_ID both use DBG_MSG_CAN_ERRORS
- We want to disable TX FIFO errors but keep unknown CAN ID messages

## Proposed Changes

### 1. Add new flag in debug.h
```c
#define DBG_MSG_TX_FIFO_ERROR     0x04000000  // TX FIFO error messages (separate from other CAN errors)
```

### 2. Update MSG_TX_FIFO_ERROR in debug.c
Change from:
```c
{MSG_TX_FIFO_ERROR, DBG_ERRORS, DBG_MSG_CAN_ERRORS,
```
To:
```c
{MSG_TX_FIFO_ERROR, DBG_ERRORS, DBG_MSG_TX_FIFO_ERROR,
```

### 3. Don't include DBG_MSG_TX_FIFO_ERROR in default config
The default DEBUG_MESSAGES would stay as is, keeping DBG_MSG_CAN_ERRORS for unknown CAN IDs but not including the new DBG_MSG_TX_FIFO_ERROR flag.

## Result
- TX FIFO errors won't flood the output
- Unknown CAN ID messages will still be shown (useful for debugging)
- TX FIFO errors can be easily enabled later if needed by adding DBG_MSG_TX_FIFO_ERROR to DEBUG_MESSAGES