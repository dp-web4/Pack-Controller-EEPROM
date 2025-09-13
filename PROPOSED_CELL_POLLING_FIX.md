# Proposed Fix for Cell Detail Request Skipping

## Problem Identified

The Pack Controller Emulator is skipping cells because:

1. **CellPollTimerTimer** (every 100ms):
   - Sets messageFlags.cellId = nextCellToRequest
   - Immediately increments nextCellToRequest++
   - This happens BEFORE checking if request was actually sent

2. **SendCellDetailRequest** (called from message queue):
   - Checks if waitingForCellResponse is true
   - If still waiting (within 200ms timeout), returns WITHOUT sending
   - The cell ID that was queued is lost

3. **Result**: Cells get skipped when timing causes the request to be dropped

## Root Cause

The increment of `nextCellToRequest` happens unconditionally in CellPollTimerTimer, but the actual sending is conditional in SendCellDetailRequest. This creates a mismatch.

## Proposed Solution

### Option 1: Don't increment until confirmed sent (RECOMMENDED)
Move the increment logic to SendCellDetailRequest after successful send:

```cpp
// In CellPollTimerTimer - DON'T increment here
void __fastcall TMainForm::CellPollTimerTimer(TObject *Sender) {
    // ... existing checks ...
    
    // Set cell detail request flag with module and cell IDs
    messageFlags.cellDetail = true;
    messageFlags.cellModuleId = selectedModuleId;
    messageFlags.cellId = nextCellToRequest;
    
    // DON'T INCREMENT HERE - let SendCellDetailRequest do it
    
    lastCellRequestTime = GetTickCount();
}

// In SendCellDetailRequest - increment only after successful send
void TMainForm::SendCellDetailRequest() {
    uint8_t moduleId = messageFlags.cellModuleId;
    uint8_t cellId = messageFlags.cellId;
    
    // ... existing timeout check ...
    if (module != NULL && module->waitingForCellResponse) {
        DWORD currentTime = GetTickCount();
        if (currentTime - module->cellRequestTime < 200) {
            // Still waiting, keep the same cell ID for retry
            messageFlags.cellDetail = true;  // Re-queue for later
            return;
        }
        // Timeout - will proceed with request
    }
    
    // Send the detail request
    bool success = canInterface->SendDetailRequest(moduleId, cellId);
    
    if (success) {
        // ... existing code ...
        
        // NOW increment to next cell since we successfully sent this one
        PackEmulator::ModuleInfo* module = moduleManager->GetModule(moduleId);
        if (module != NULL) {
            uint8_t cellCount = module->cellCount;
            if (cellCount == 0) cellCount = module->cellCountMax;
            
            nextCellToRequest++;
            if (nextCellToRequest >= cellCount) {
                nextCellToRequest = 0;
            }
        }
    }
}
```

### Option 2: Track pending cell separately
Keep a separate pendingCellId that only updates when sent:

```cpp
class TMainForm {
    // Add new member
    uint8_t pendingCellId;  // Cell waiting to be sent
    
    // In CellPollTimerTimer
    if (!module->waitingForCellResponse) {
        // Only advance if not waiting
        pendingCellId = nextCellToRequest;
        nextCellToRequest++;
        if (nextCellToRequest >= cellCount) {
            nextCellToRequest = 0;
        }
    }
    messageFlags.cellId = pendingCellId;
}
```

### Option 3: Check waiting flag in timer
Only increment if not waiting:

```cpp
void __fastcall TMainForm::CellPollTimerTimer(TObject *Sender) {
    // ... existing checks ...
    
    // Check if still waiting for previous response
    PackEmulator::ModuleInfo* module = moduleManager->GetModule(selectedModuleId);
    if (module != NULL && module->waitingForCellResponse) {
        DWORD currentTime = GetTickCount();
        if (currentTime - module->cellRequestTime < 200) {
            // Still waiting, don't advance cell number
            return;
        }
    }
    
    // Safe to advance
    messageFlags.cellDetail = true;
    messageFlags.cellModuleId = selectedModuleId;
    messageFlags.cellId = nextCellToRequest;
    
    nextCellToRequest++;
    // ... rest of code
}
```

## Recommendation

**Option 1** is cleanest because it:
- Keeps increment logic with the send confirmation
- Automatically handles retries of the same cell
- Prevents any possibility of skipping

## Testing

After implementing:
1. Verify cells are requested in order: 0, 1, 2, 3, ... 12, 0, 1, ...
2. Test with artificial delays to trigger timeouts
3. Verify no cells are skipped even under timeout conditions