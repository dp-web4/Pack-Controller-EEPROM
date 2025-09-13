# Pack Controller Emulator Changelog

## 2025-09-12 - Fixed Cell Detail Request Sequencing

### Problem
Pack Controller Emulator was skipping cells when requesting cell details from modules. Cells were being requested out of order (e.g., 5, 6, 8, 9, 11, 0, 3... instead of 0, 1, 2, 3...).

### Root Cause
The `CellPollTimerTimer` was unconditionally incrementing `nextCellToRequest` even when `SendCellDetailRequest` didn't actually send the request due to waiting for a previous response. This caused cell numbers to be skipped.

### Fix
Moved the increment logic from `CellPollTimerTimer` to `SendCellDetailRequest`:
- Only increment `nextCellToRequest` after successfully sending the request
- Re-queue the same cell if still waiting or if send fails
- This ensures no cells are ever skipped

### Changes Made
1. **pack_emulator_main.cpp - CellPollTimerTimer**:
   - Removed increment of `nextCellToRequest`
   - Just sets `messageFlags.cellId = nextCellToRequest`

2. **pack_emulator_main.cpp - SendCellDetailRequest**:
   - Added increment logic after successful send
   - Re-queues same cell if waiting or on failure
   - Improved timeout logging

### Testing
- Verify cells are now requested in sequential order: 0, 1, 2, ... 12, 0, 1, ...
- Test with timeouts to ensure no cells are skipped
- Confirm module receives all cell requests in order