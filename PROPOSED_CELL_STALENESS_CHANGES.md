# Proposed Changes: Stale Cell Data Visualization

## Problem
When cell communication times out, we're still displaying the last known values but there's no visual indication that the data is stale/outdated.

## Solution
Add per-cell timestamp tracking and display stale cells with greyed out text to indicate the data is not current.

## Implementation Plan

### 1. Add Per-Cell Timestamps to ModuleInfo Structure
In `module_manager.h`, add to the `ModuleInfo` struct:
```cpp
// Add after existing cell data vectors
std::vector<DWORD> cellLastUpdateTimes;  // Per-cell last update timestamps
```

### 2. Initialize Cell Timestamps
When resizing cell arrays, also resize timestamp array and initialize to 0.

### 3. Update Timestamps When Receiving Cell Data
In `ProcessModuleDetail()` when we receive cell data:
- Update the specific cell's timestamp to current GetTickCount()
- This happens around line 1316 where we store cell voltage/temperature

### 4. Modify UpdateCellDisplay() to Show Staleness
Use different colors based on data age:
- **Fresh data** (< 2 seconds old): Normal black text
- **Stale data** (2-5 seconds old): Dark gray text
- **Very stale** (> 5 seconds old): Light gray text
- **No data** (voltage = 0.0): Red "---" to show never received

### 5. Visual Implementation in UpdateCellDisplay
```cpp
// In UpdateCellDisplay around line 871
DWORD currentTime = GetTickCount();
for (int i = 0; i < cellCount; i++) {
    CellGrid->Cells[0][i + 1] = IntToStr(i + 1);

    // Check data staleness
    DWORD timeSinceUpdate = 0;
    if (i < module->cellLastUpdateTimes.size() && module->cellLastUpdateTimes[i] > 0) {
        timeSinceUpdate = currentTime - module->cellLastUpdateTimes[i];
    }

    // Set color based on staleness
    TColor textColor;
    if (module->cellVoltages[i] == 0.0f) {
        // Never received data
        textColor = clRed;
        CellGrid->Cells[1][i + 1] = "---";
        CellGrid->Cells[2][i + 1] = "---";
    } else if (timeSinceUpdate == 0 || module->cellLastUpdateTimes[i] == 0) {
        // No timestamp yet (shouldn't happen)
        textColor = clBlack;
        CellGrid->Cells[1][i + 1] = FloatToStrF(module->cellVoltages[i], ffFixed, 7, 3);
        CellGrid->Cells[2][i + 1] = FloatToStrF(module->cellTemperatures[i], ffFixed, 7, 1);
    } else if (timeSinceUpdate < 2000) {
        // Fresh data (< 2 seconds)
        textColor = clBlack;
        CellGrid->Cells[1][i + 1] = FloatToStrF(module->cellVoltages[i], ffFixed, 7, 3);
        CellGrid->Cells[2][i + 1] = FloatToStrF(module->cellTemperatures[i], ffFixed, 7, 1);
    } else if (timeSinceUpdate < 5000) {
        // Stale data (2-5 seconds)
        textColor = clGray;
        CellGrid->Cells[1][i + 1] = FloatToStrF(module->cellVoltages[i], ffFixed, 7, 3);
        CellGrid->Cells[2][i + 1] = FloatToStrF(module->cellTemperatures[i], ffFixed, 7, 1);
    } else {
        // Very stale data (> 5 seconds)
        textColor = clSilver;  // Light gray
        CellGrid->Cells[1][i + 1] = FloatToStrF(module->cellVoltages[i], ffFixed, 7, 3);
        CellGrid->Cells[2][i + 1] = FloatToStrF(module->cellTemperatures[i], ffFixed, 7, 1);
    }

    // Apply color to the row
    // Note: TStringGrid doesn't have per-cell color directly, need to use DrawCell event
}
```

### 6. Alternative Implementation Using OnDrawCell Event
Since TStringGrid doesn't support per-cell colors directly, we need to handle the OnDrawCell event:

1. Add a data structure to track cell staleness states
2. Implement CellGridDrawCell event handler
3. Draw cells with appropriate colors based on staleness

## Files to Modify
1. `module_manager.h` - Add cellLastUpdateTimes vector
2. `pack_emulator_main.cpp`:
   - Update ProcessModuleDetail() to record timestamps
   - Modify UpdateCellDisplay() to check staleness
   - Add OnDrawCell event handler for CellGrid
3. `pack_emulator_main.h` - Add OnDrawCell event declaration

## Testing
1. Connect to modules and view cell data
2. Stop one module from responding
3. Verify cells gradually change color as data becomes stale
4. Verify fresh data returns to normal color when communication resumes

## Benefits
- Clear visual indication of data freshness
- Easy to spot communication issues with specific cells
- Helps diagnose intermittent communication problems
- Maintains last known values while showing they're outdated