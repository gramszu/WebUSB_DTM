# Kompletna mapa pamięci EEPROM - AC 800

## Porównanie Python vs C - wszystkie adresy

| Obszar | Adres | Rozmiar | Python | C (adresyeeprom.h) | Status |
|--------|-------|---------|--------|-------------------|--------|
| **Checksum** | 0x0000 (0) | 1 bajt | `data[0]` | `eeprom[0]` | ✓ |
| **Kod dostępu** | 0x0001-0x0004 (1-4) | 4 bajty | `data[0x01:0x05]` | `ADRES_EEPROM_KOD_DOSTEPU` | ✓ |
| **Ustawienie stanów** | 0x0005 (5) | 1 bajt | `data[5]` | `EEPROM_USTAWIENIE_STANOW_WYJSC` | ✓ |
| **Ustawienie wyjścia** | 0x0006-0x0007 (6-7) | 2 bajty | `data[6:8]` | `EEPROM_USTAWIENIE_WYJSCIA` | ✓ |
| **Numery telefonów** | 0x0008-0x0FA7 (8-4007) | 4000 bajtów | `800 × 5` | `EEPROM_NUMER_TELEFONU_BRAMA(0-799)` | ✓ |
| - Numer 1 | 0x0008-0x000C (8-12) | 5 bajtów | `data[8:13]` | `BRAMA(0)` | ✓ |
| - Numer 2 | 0x000D-0x0011 (13-17) | 5 bajtów | `data[13:18]` | `BRAMA(1)` | ✓ |
| - Numer 800 | 0x0FA3-0x0FA7 (4003-4007) | 5 bajtów | `data[4003:4008]` | `BRAMA(799)` | ✓ |
| **WOLNE** | 0x0FA8-0x0FF8 (4008-4088) | 81 bajtów | - | Niewykorzystane | - |
| **Skryba** | 0x0FF9 (4089) | 1 bajt | `ADDR_SKRYBA` | `ADRES_EEPROM_SKRYBA` | ✓ |
| **Time Start H** | 0x0FFA (4090) | 1 bajt | `ADDR_TIME_START_H` | `ADRES_EEPROM_CZAS_START_H` | ✓ |
| **Time Start M** | 0x0FFB (4091) | 1 bajt | `ADDR_TIME_START_M` | `ADRES_EEPROM_CZAS_START_M` | ✓ |
| **Time Stop H** | 0x0FFC (4092) | 1 bajt | `ADDR_TIME_STOP_H` | `ADRES_EEPROM_CZAS_STOP_H` | ✓ |
| **Time Stop M** | 0x0FFD (4093) | 1 bajt | `ADDR_TIME_STOP_M` | `ADRES_EEPROM_CZAS_STOP_M` | ✓ |
| **Tryb pracy** | 0x0FFE (4094) | 1 bajt | `ADDR_MODE` | `ADRES_EEPROM_TRYB_PRACY` | ✓ |
| **WOLNE** | 0x0FFF (4095) | 1 bajt | - | Niewykorzystany | - |

## Szczegóły kodu dostępu

### C (adresyeeprom.h)
```c
#define ADRES_EEPROM_KOD_DOSTEPU 1
#define LICZBA_BAJTOW_KODU_DOSTEPU 4
```

### Python (onlyAC800.py)

**Odczyt:**
```python
def update_ascii_field_from_data(self, data: bytes) -> None:
    ascii_text = self.bytes_to_ascii_preview(data[0x01:0x05])  # 4 bajty
    self.ascii_var.set(ascii_text)
```

**Zapis:**
```python
def apply_ascii_to_data(self, data: bytearray, ascii_text: str) -> None:
    chars = list(ascii_text[:4])  # Max 4 znaki
    for i, ch in enumerate(chars):
        data[0x01 + i] = ord(ch)  # Adresy 1,2,3,4
```

**GUI:**
- Pole tekstowe: `entry_ascii` (max 4 znaki ASCII)
- Label: "Zmiana kodu dostępu"
- Przycisk: "Zmień" → wywołuje `sync_ascii_into_textarea()`

## Wizualizacja pamięci

```
┌─────────────────────────────────────────────────────────────┐
│ 0x0000 │ Checksum (1 bajt)                                  │
├─────────────────────────────────────────────────────────────┤
│ 0x0001 │ Kod dostępu (4 bajty ASCII)                        │
│ 0x0002 │   - Bajt 1                                         │
│ 0x0003 │   - Bajt 2                                         │
│ 0x0004 │   - Bajt 3                                         │
├─────────────────────────────────────────────────────────────┤
│ 0x0005 │ Ustawienie stanów wyjść                            │
│ 0x0006 │ Ustawienie wyjścia (2 bajty)                       │
│ 0x0007 │                                                     │
├─────────────────────────────────────────────────────────────┤
│ 0x0008 │ Numer 1 (5 bajtów)                                 │
│   ...  │   ...                                               │
│ 0x000C │                                                     │
├─────────────────────────────────────────────────────────────┤
│ 0x000D │ Numer 2 (5 bajtów)                                 │
│   ...  │   ...                                               │
│        │   ... (798 numerów więcej) ...                     │
├─────────────────────────────────────────────────────────────┤
│ 0x0FA3 │ Numer 800 (5 bajtów)                               │
│   ...  │   ...                                               │
│ 0x0FA7 │                                                     │
├─────────────────────────────────────────────────────────────┤
│ 0x0FA8 │ WOLNE (81 bajtów)                                  │
│   ...  │   ...                                               │
│ 0x0FF8 │                                                     │
├─────────────────────────────────────────────────────────────┤
│ 0x0FF9 │ Skryba (1 bajt)                                    │
│ 0x0FFA │ Time Start H (1 bajt)                              │
│ 0x0FFB │ Time Start M (1 bajt)                              │
│ 0x0FFC │ Time Stop H (1 bajt)                               │
│ 0x0FFD │ Time Stop M (1 bajt)                               │
│ 0x0FFE │ Tryb pracy (1 bajt)                                │
├─────────────────────────────────────────────────────────────┤
│ 0x0FFF │ WOLNE (1 bajt)                                     │
└─────────────────────────────────────────────────────────────┘
```

## ✓ Potwierdzenie zgodności

Wszystkie adresy w `onlyAC800.py` są **w 100% zgodne** z definicjami w `adresyeeprom.h`:
- Kod dostępu: 0x0001-0x0004 ✓
- Numery telefonów: 0x0008-0x0FA7 (800 numerów) ✓
- Skryba: 0x0FF9 ✓
- Time Control: 0x0FFA-0x0FFD ✓
- Tryb pracy: 0x0FFE ✓
