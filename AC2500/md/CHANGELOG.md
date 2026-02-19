# ATMEGA328-AC200 Firmware - Changelog

## 2026-01-26 - Major Refactoring and Bug Fixes

### DTMF Mode Fixes

#### 1. Fixed Report Label for DTMF Mode
**Problem**: Command `SYS` was showing `Tryb: Close` or `Tryb: Open` but not indicating DTMF mode.

**Solution**: Modified `generuj_raport_sys()` in `main.c` to display:
- `Tryb: Close DTMF` or `Tryb: Open DTMF` when `tryb_clip == FALSE`
- `Tryb: Close Clip` or `Tryb: Open Clip` when `tryb_clip == TRUE`

**Files Modified**:
- `uC_source/main.c` (lines 294-315)

#### 2. Fixed CON Notifications in DTMF Mode
**Problem**: Notifications (CON command) were not being sent when the gate was controlled in DTMF mode (after pressing '1' during a call).

**Root Cause**: The notification SMS was being constructed during the active call, but the shared SMS buffer (`tekst_wysylanego_smsa`) was being overwritten by other processes before the call ended, resulting in corrupted or empty messages.

**Solution**: Implemented deferred notification logic:
1. When gate state changes during an active call, only a flag (`oczekujace_powiadomienie_con`) is set
2. The actual SMS message is reconstructed fresh in `zakonczono_rozmowe_telefoniczna()` immediately before sending
3. This ensures the message content is valid and not corrupted

**Files Modified**:
- `uC_source/main.c`:
  - Added `oczekujace_powiadomienie_con` flag (line 450)
  - Modified `zakonczono_rozmowe_telefoniczna()` to reconstruct and send notification (lines 1012-1033)
  - Updated notification watcher to defer SMS construction if call is active (lines 2230-2264)

### Additional Fixes

#### 3. Fixed Service Limit Hour Typo
**Problem**: Compilation error due to invalid integer constant `12o` in `SERVICE_LIMIT_HOUR`.

**Solution**: Changed `12o` to `12` in `main.c` (line 46).

**Files Modified**:
- `uC_source/main.c` (line 46)

### New Upload Script

#### 4. Created 328pb.sh Upload Script
**Purpose**: Alternative upload script for ATmega328PB without bootloader and with custom fuse bits.

**Configuration**:
- **No bootloader** (urboot.hex not flashed)
- **Fuse bits**:
  - `lfuse: 0xFD`
  - `hfuse: 0xC9` (BOOTRST disabled)
  - `efuse: 0xF6` (BOD 2.7V)
  - `lock: 0x3F` (No lock)

**Files Created**:
- `uC_source/328pb.sh`

### Shelved Features

#### Modular Feature System (Branch: feature/modular_firmware)
A compile-time feature flag system was developed but shelved for future work:
- Created `config_features.h` with `#define` macros for all commands and reports
- Refactored `interpretacjaSMS.c` and `main.c` to conditionally compile features
- Allows selective enabling/disabling of SMS commands and report components
- Estimated RAM savings: ~70 bytes with minimal feature set

**Status**: Moved to branch `feature/modular_firmware` with context file `PROMPT_FOR_FUTURE.md`

## Technical Details

### Memory Usage (Current Build)
- **Flash (.text)**: 28,900 bytes
- **RAM (.data + .bss)**: 1,831 bytes (254 + 1,577)
- **EEPROM**: 1,024 bytes

### Compilation
- **Toolchain**: AVR GCC 8.5.0
- **Target**: ATmega328PB
- **F_CPU**: 7,372,800 Hz

### Upload Methods
1. **upload.sh**: With urboot bootloader (512B), fuses: 0xFD/0xDE/0xF5, lock: 0x3C
2. **328pb.sh**: Without bootloader, fuses: 0xFD/0xC9/0xF6, lock: 0x3F

## Testing Recommendations

1. **DTMF Mode Report**: Send `SYS` command and verify it shows `DTMF` label
2. **DTMF Notifications**: 
   - Configure CON with a phone number
   - Call the device in DTMF mode
   - Press '1' to open gate
   - Verify notification SMS is received after hanging up
3. **CLIP Mode**: Verify existing functionality still works correctly

## Repository Structure

```
ATMEGA328-AC200/
├── uC_source/           # Firmware source code
│   ├── main.c           # Main firmware logic
│   ├── interpretacjaSMS.c  # SMS command parsing
│   ├── upload.sh        # Upload with bootloader
│   ├── 328pb.sh         # Upload without bootloader
│   └── ...
├── GUI_WWW/             # Web-based configurator
├── SKIN_GUI/            # Alternative GUI skin
└── public_html/         # Web interface
```

## Notes

- All changes have been tested for compilation
- Hardware testing recommended before deployment
- Original repository: `AC200-DTM-FS-UART-WWW`
- This repository: `ATMEGA328-AC200` (private)
