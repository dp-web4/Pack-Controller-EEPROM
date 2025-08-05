# Proposed Polling Debug Changes

## Changes to Make

### 1. Remove Redundant uniqueId Check
**Location**: MCU_RequestModuleAnnouncement() around lines 1284 and 1320
```c
// Change from:
if(module[i].isRegistered && module[i].uniqueId != 0)

// To:
if(module[i].isRegistered)
```
**Rationale**: A registered module should never have uniqueId of 0

### 2. Modify Status Response Debug Output
**Location**: MCU_ProcessModuleStatus1() around line 1626

```c
// Comment out the response time version:
// sprintf(tempBuffer,"%d[%lu]", rxObj.bF.id.EID, (unsigned long)responseTimeMs);

// Add simple ID-only version:
sprintf(tempBuffer,"%d", rxObj.bF.id.EID);
```

### 3. Add Polling Request Debug
**Location**: MCU_RequestModuleStatus() around line 1515 (after sending request)

```c
// Add minimal mode polling indicator
if(debugMessages & DBG_MSG_STATUS_REQ && debugMessages & DBG_MSG_MINIMAL){
    extern UART_HandleTypeDef huart1;
    sprintf(tempBuffer,".%d-", moduleId);
    HAL_UART_Transmit(&huart1, (uint8_t*)tempBuffer, strlen(tempBuffer), HAL_MAX_DELAY);
}
```

## Expected Output
- When polling module 1: `.1-` (request sent)
- When response received: `1` (response from module 1)
- Full sequence: `.1-1.2-2.1-1.2-2`

## Benefits
1. Clear visibility of polling requests vs responses
2. Can see if requests are being sent but not answered
3. Simplified output focuses on the polling issue

## Questions
1. Should we keep the response time code commented for easy re-enabling?
2. Is `.1-` clear enough or prefer different format?