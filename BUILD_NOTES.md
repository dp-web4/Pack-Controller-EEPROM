# Build Notes

## Unused Variable Warnings

The following unused variable warnings are **intentional** and left in place for future use:

### In MCU_ProcessModuleHardware (line ~1354)
- `moduleMaxEndVoltage`

### In MCU_ProcessModuleStatus1 (line ~1585-1588)
- `moduleVoltage`
- `moduleCurrent`
- `stateOfCharge`
- `stateOfHealth`

### In MCU_ProcessModuleStatus2 (line ~1683-1686)
- `cellAvgVolt`
- `cellHiVolt`
- `cellLoVolt`
- `cellTotalVolt`

### In MCU_ProcessModuleStatus3 (line ~1759-1761)
- `cellAvgTemp`
- `cellHiTemp`
- `cellLoTemp`

### MCU_ShouldLogMessage function (line ~85)
- Static function defined but not currently used

## Rationale

These variables contain calculated values that were useful during functional module testing. They are intentionally preserved for:
1. Future debug message implementation when module functionality testing resumes
2. Quick access to pre-calculated values when needed
3. Documentation of what values are available for debugging

These will be incorporated into proper debug messages when module functional testing becomes a priority again.

## TX FIFO Error Handling

TX FIFO errors have their own debug flag (`DBG_MSG_TX_FIFO_ERROR`) separate from general CAN errors. This prevents flooding while preserving the ability to enable them when needed for CAN bus debugging.