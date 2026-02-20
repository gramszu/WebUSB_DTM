# Weryfikacja zgodności onlyAC800.py z kodem C

## Podsumowanie zmian

Utworzono plik `onlyAC800.py` - uproszczoną wersję programu obsługującą **tylko AC 800 (ATmega1284P, 800 numerów)**.

### Usunięte elementy:
- ✓ Konfiguracja AC 200 (M328PB)
- ✓ Pole wyboru procesora w GUI
- ✓ Funkcja `switch_config()`
- ✓ Funkcja `on_processor_change()`
- ✓ Funkcja `update_ui_for_processor()`
- ✓ Zmienne: `processor_var`, `processor_options`, `processor_combobox`

### Naprawione problemy:
- ✓ **ADDR_STATUS = None** (usunięto konflikt z numerem 203)
- ✓ Domyślna konfiguracja ustawiona na M1284

## Porównanie z kodem C

### Parametry podstawowe

| Parametr | C (adresyeeprom.h) | Python (onlyAC800.py) | Status |
|----------|-------------------|----------------------|--------|
| MCU | ATmega1284P | `m1284` | ✓ |
| EEPROM Size | 4096 bytes | `4096` | ✓ |
| Max Numbers | 800 | `800` | ✓ |
| Start Address | 0x0008 (8) | `0x08` | ✓ |
| Entry Size | 5 bytes | `5` | ✓ |

### Adresy EEPROM - Numery telefonów

**C (adresyeeprom.h):**
```c
#define EEPROM_NUMER_TELEFONU_BRAMA_0 8
#define LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM 5
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA 800
```

**Python (onlyAC800.py):**
```python
"RANGE_START": 0x08,
"NUM_ENTRIES": 800,
entry_size = 5
```

**Zakres adresów:**
- Numer 1: `0x0008-0x000C` (8-12)
- Numer 800: `0x0FA3-0x0FA7` (4003-4007)
- **Status: ✓ ZGODNE**

### Adresy specjalne

| Funkcja | C (adresyeeprom.h) | Python (onlyAC800.py) | Status |
|---------|-------------------|----------------------|--------|
| SKRYBA | `4089` (0x0FF9) | `4089` | ✓ |
| TIME_START_H | `4090` (0x0FFA) | `4090` | ✓ |
| TIME_START_M | `4091` (0x0FFB) | `4091` | ✓ |
| TIME_STOP_H | `4092` (0x0FFC) | `4092` | ✓ |
| TIME_STOP_M | `4093` (0x0FFD) | `4093` | ✓ |
| MODE | `4094` (0x0FFE) | `4094` | ✓ |
| STATUS | **NIE UŻYWANE** | `None` | ✓ |

### Mapa pamięci EEPROM (4096 bajtów)

```
Adres      Zakres         Przeznaczenie                    C vs Python
─────────────────────────────────────────────────────────────────────
0x0000     0-0            Checksum                         ✓
0x0001     1-4            Kod dostępu (4 bajty)            ✓
0x0005     5              USTAWIENIE_STANOW_WYJSC          ✓
0x0006     6-7            USTAWIENIE_WYJSCIA               ✓
0x0008     8-4007         Numery telefonów (800 × 5)       ✓
0x0FA8     4008-4088      WOLNE (81 bajtów)                ✓
0x0FF9     4089           SKRYBA                           ✓
0x0FFA     4090           TIME_START_H                     ✓
0x0FFB     4091           TIME_START_M                     ✓
0x0FFC     4092           TIME_STOP_H                      ✓
0x0FFD     4093           TIME_STOP_M                      ✓
0x0FFE     4094           MODE                             ✓
0x0FFF     4095           WOLNE (1 bajt)                   ✓
```

## Naprawiony problem: ADDR_STATUS

### Przed naprawą:
```python
"ADDR_STATUS": 1022,  # ✗ KONFLIKT!
```
- Adres 1022 (0x03FE) znajduje się w zakresie numeru 203 (0x03FA-0x03FE)
- Zapis do ADDR_STATUS nadpisywał ostatni bajt numeru 203

### Po naprawie:
```python
"ADDR_STATUS": None,  # ✓ M1284 nie używa tego adresu
```
- Kod C nie definiuje ADDR_STATUS dla ATmega1284P
- Python teraz zgodny z C - nie używa tego adresu

## Weryfikacja funkcji odczytu/zapisu

### Funkcja `read_status_and_mode_from_eeprom()` (linie 428-478)

**Obsługa ADDR_STATUS:**
```python
if self.config.EEPROM_ADDR_STATUS and len(data) > self.config.EEPROM_ADDR_STATUS:
    status_byte = data[self.config.EEPROM_ADDR_STATUS]
    self.status_var.set(0 if status_byte == 0x00 else 1)
```
- Sprawdza czy `EEPROM_ADDR_STATUS` istnieje (nie jest None)
- Dla M1284: `EEPROM_ADDR_STATUS = None` → kod się nie wykona ✓

### Funkcja `write_status_and_mode_to_eeprom()` (linie 480-527)

**Obsługa ADDR_STATUS:**
```python
if self.config.EEPROM_ADDR_STATUS:
    data[self.config.EEPROM_ADDR_STATUS] = 0x00 if self.status_var.get() == 0 else 0xFF
```
- Sprawdza czy `EEPROM_ADDR_STATUS` istnieje
- Dla M1284: `EEPROM_ADDR_STATUS = None` → kod się nie wykona ✓

## Zgodność z kodem C - Skryba

**C (main.c):**
```c
uchar skryba_wlaczona = eeprom_read_byte((void *)ADRES_EEPROM_SKRYBA);
// 1 = włączona, 0 = wyłączona
```

**Python (onlyAC800.py):**
```python
skryba_byte = data[self.config.EEPROM_ADDR_SKRYBA]
self.skryba_var.set(1 if skryba_byte == 0x01 or skryba_byte == 0xFF else 0)
```
- **Status: ✓ ZGODNE**

## Zgodność z kodem C - Mode

**C (main.c):**
```c
uchar tryb_pracy = eeprom_read_byte((void *)ADRES_EEPROM_TRYB_PRACY);
// 0 = Prywatny, 1 = Publiczny
```

**Python (onlyAC800.py):**
```python
mode_byte = data[self.config.EEPROM_ADDR_MODE]
self.mode_var.set(0 if mode_byte == 0x00 else 1)
```
- **Status: ✓ ZGODNE**

## Zgodność z kodem C - Time Control

**C (adresyeeprom.h):**
```c
#define ADRES_EEPROM_CZAS_START_H 4090
#define ADRES_EEPROM_CZAS_START_M 4091
#define ADRES_EEPROM_CZAS_STOP_H 4092
#define ADRES_EEPROM_CZAS_STOP_M 4093
```

**Python (onlyAC800.py):**
```python
"ADDR_TIME_START_H": 4090,
"ADDR_TIME_START_M": 4091,
"ADDR_TIME_STOP_H": 4092,
"ADDR_TIME_STOP_M": 4093,
```

**Logika wyłączenia (0xFF):**
- C: Jeśli `czas_start_h == 0xFF` → harmonogram wyłączony
- Python: Jeśli `start_h_byte == 0xFF` → `time_enabled_var.set(0)`
- **Status: ✓ ZGODNE**

## Podsumowanie

### ✓ Wszystkie adresy EEPROM zgodne z C
### ✓ Logika odczytu/zapisu zgodna z C
### ✓ Usunięto konflikt ADDR_STATUS
### ✓ Program obsługuje tylko AC 800 (800 numerów)
### ✓ Plik kompiluje się bez błędów

## Plik wynikowy

**Lokalizacja:** `/Users/gramsz/Desktop/AC800_Skryba/v3/onlyAC800.py`

**Rozmiar:** ~66 KB (zmniejszony z ~70 KB)

**Linie kodu:** ~1615 (zmniejszone z ~1706)
