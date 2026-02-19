# Analiza Statyczna: Weryfikacja Adresów EEPROM

**Data:** 2025-12-22  
**Projekt:** AC800-DTM-HS-RC3  
**Cel:** Weryfikacja zgodności adresów między C (adresyeeprom.h) a Python (AC800-DTM-HS.py)

---

## ✅ WYNIK WERYFIKACJI: WSZYSTKIE ADRESY ZGODNE

---

## 📊 Tabela Porównawcza Adresów

| Funkcja | C (adresyeeprom.h) | Python (AC800-DTM-HS.py) | Hex | Status |
|---------|-------------------|-------------------------|-----|--------|
| **PODSTAWOWE** |
| Checksum | `ADRES_EEPROM_CHECKSUM = 0` | - | 0x0000 | ✅ |
| Kod dostępu | `ADRES_EEPROM_KOD_DOSTEPU = 1` | - | 0x0001 | ✅ |
| **TRYBY PRACY** |
| Tryb pracy | `ADRES_EEPROM_TRYB_PRACY = 1008` | `ADDR_MODE = 1008` | 0x03F0 | ✅ |
| CLIP/DTMF | `ADRES_EEPROM_TRYB_CLIP_DTMF = 1009` | `ADDR_CLIP_DTMF = 1009` | 0x03F1 | ✅ |
| **FUNKCJA SKRYBA** |
| SKRYBA włączona | `ADRES_EEPROM_SKRYBA = 1010` | `ADDR_SKRYBA = 1010` | 0x03F2 | ✅ |
| SKRYBA backup | `ADRES_EEPROM_SKRYBA_TRYB_BACKUP = 1011` | - | 0x03F3 | ✅ |
| SKRYBA limit L | `ADRES_EEPROM_SKRYBA_LIMIT_L = 1012` | `ADDR_SKRYBA_LIMIT_L = 1012` | 0x03F4 | ✅ |
| SKRYBA limit H | `ADRES_EEPROM_SKRYBA_LIMIT_H = 1013` | `ADDR_SKRYBA_LIMIT_H = 1013` | 0x03F5 | ✅ |
| Blokada systemu | `ADRES_EEPROM_BLOKADA_SYSTEMU = 1014` | `ADDR_STATUS = 1014` | 0x03F6 | ✅ |
| **FUNKCJA TIME** |
| TIME start H | `ADRES_EEPROM_CZAS_START_H = 1015` | `ADDR_TIME_START_H = 1015` | 0x03F7 | ✅ |
| TIME start M | `ADRES_EEPROM_CZAS_START_M = 1016` | `ADDR_TIME_START_M = 1016` | 0x03F8 | ✅ |
| TIME stop H | `ADRES_EEPROM_CZAS_STOP_H = 1017` | `ADDR_TIME_STOP_H = 1017` | 0x03F9 | ✅ |
| TIME stop M | `ADRES_EEPROM_CZAS_STOP_M = 1018` | `ADDR_TIME_STOP_M = 1018` | 0x03FA | ✅ |
| **AUTO-SYNC** |
| Mój numer | `ADRES_EEPROM_MOJE_NUMER_START = 1019` | `ADDR_MYNUM = 1019` | 0x03FB | ✅ |

---

## 🔍 Szczegółowa Analiza

### 1. Konfiguracja Podstawowa

**C (adresyeeprom.h):**
```c
#define ADRES_EEPROM_CHECKSUM                       0       // 0x0000
#define ADRES_EEPROM_KOD_DOSTEPU                    1       // 0x0001
#define LICZBA_BAJTOW_KODU_DOSTEPU                  4
#define EEPROM_USTAWIENIE_STANOW_WYJSC              5       // 0x0005
#define EEPROM_USTAWIENIE_WYJSCIA                   6       // 0x0006 (2 bajty)
```

**Python (AC800-DTM-HS.py):**
```python
# Brak bezpośrednich adresów - obsługiwane przez avrdude
```

**Status:** ✅ Zgodne (GUI nie modyfikuje tych adresów bezpośrednio)

---

### 2. Numery Telefonów

**C (adresyeeprom.h):**
```c
#define MAX_LICZBA_ZNAKOW_TELEFON                   16
#define LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM      5       // BCD format
#define EEPROM_NUMER_TELEFONU_BRAMA_0               8       // 0x0008
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA          200
```

**Python (AC800-DTM-HS.py):**
```python
"NUM_ENTRIES": 200,
"RANGE_START": 0x08,
```

**Obliczenia:**
- Pierwszy numer: 0x0008 (8)
- Ostatni numer: 8 + (199 × 5) = 8 + 995 = 1003 (0x03EB)
- Koniec ostatniego: 1003 + 5 - 1 = 1007 (0x03EF)

**Status:** ✅ Zgodne

---

### 3. Tryby Pracy

**C (adresyeeprom.h):**
```c
#define ADRES_EEPROM_TRYB_PRACY                     1008    // 0x3F0
#define ADRES_EEPROM_TRYB_CLIP_DTMF                 1009    // 0x3F1
```

**Python (AC800-DTM-HS.py):**
```python
"ADDR_MODE": 1008,
"ADDR_CLIP_DTMF": 1009,
```

**Status:** ✅ Zgodne (100%)

---

### 4. Funkcja SKRYBA

**C (adresyeeprom.h):**
```c
#define ADRES_EEPROM_SKRYBA                         1010    // 0x3F2
#define ADRES_EEPROM_SKRYBA_TRYB_BACKUP             1011    // 0x3F3
#define ADRES_EEPROM_SKRYBA_LIMIT_L                 1012    // 0x3F4
#define ADRES_EEPROM_SKRYBA_LIMIT_H                 1013    // 0x3F5
#define ADRES_EEPROM_BLOKADA_SYSTEMU                1014    // 0x3F6
```

**Python (AC800-DTM-HS.py):**
```python
"ADDR_SKRYBA": 1010,
# ADDR_SKRYBA_TRYB_BACKUP nie używany w GUI (zarządzany przez firmware)
"ADDR_SKRYBA_LIMIT_L": 1012,
"ADDR_SKRYBA_LIMIT_H": 1013,
"ADDR_STATUS": 1014,  # To jest BLOKADA_SYSTEMU
```

**Uwaga:** `ADDR_STATUS` w GUI = `ADRES_EEPROM_BLOKADA_SYSTEMU` w C  
**Status:** ✅ Zgodne (różne nazwy, te same adresy)

---

### 5. Funkcja TIME

**C (adresyeeprom.h):**
```c
#define ADRES_EEPROM_CZAS_START_H                   1015    // 0x3F7
#define ADRES_EEPROM_CZAS_START_M                   1016    // 0x3F8
#define ADRES_EEPROM_CZAS_STOP_H                    1017    // 0x3F9
#define ADRES_EEPROM_CZAS_STOP_M                    1018    // 0x3FA
```

**Python (AC800-DTM-HS.py):**
```python
"ADDR_TIME_START_H": 1015,
"ADDR_TIME_START_M": 1016,
"ADDR_TIME_STOP_H": 1017,
"ADDR_TIME_STOP_M": 1018,
```

**Status:** ✅ Zgodne (100%)

---

### 6. Auto-Sync Czasu

**C (adresyeeprom.h):**
```c
#define ADRES_EEPROM_MOJE_NUMER_START               1019    // 0x3FB
// Mój numer: 1019-1023 (0x03FB-0x03FF) - 5 bajtów BCD
```

**Python (AC800-DTM-HS.py):**
```python
"ADDR_MYNUM": 1019,  # Własny numer telefonu (5 bajtów BCD)
```

**Status:** ✅ Zgodne

⚠️ **UWAGA:** GUI może jeszcze używać starego formatu (10 B ASCII). Sprawdzę kod odczytu/zapisu.

---

## 🔍 Weryfikacja Kodu GUI - Odczyt/Zapis "Mój Numer"

### Odczyt (linie 589-599):
```python
# MYNUM (własny numer telefonu)
if self.config.EEPROM_ADDR_MYNUM and len(data) > self.config.EEPROM_ADDR_MYNUM + 9:
    # Odczytaj 10 bajtów (maksymalnie 9 cyfr + null terminator)
    mynum_bytes = data[self.config.EEPROM_ADDR_MYNUM:self.config.EEPROM_ADDR_MYNUM + 10]
```

### Zapis (linie 653-666):
```python
# MYNUM (własny numer telefonu)
if self.config.EEPROM_ADDR_MYNUM:
    mynum_str = self.mynum_var.get().strip()
    # Walidacja: tylko cyfry, 3-9 znaków
    if mynum_str and mynum_str.isdigit() and 3 <= len(mynum_str) <= 9:
        # Zapisz numer jako ASCII
        for i, char in enumerate(mynum_str):
            data[self.config.EEPROM_ADDR_MYNUM + i] = ord(char)
```

### ⚠️ PROBLEM ZNALEZIONY!

**GUI używa formatu ASCII (10 B), ale firmware używa BCD (5 B)!**

**Skutek:**
- GUI zapisze numer jako ASCII na adresach 1019-1028
- Firmware oczekuje BCD na adresach 1019-1023
- **NIEZGODNOŚĆ FORMATÓW!**

---

## 🔧 Wymagane Poprawki w GUI

### Opcja 1: Zmiana GUI na BCD (zalecane)

Należy zaktualizować funkcje odczytu/zapisu w `AC800-DTM-HS.py`:

**Odczyt (linie 589-599):**
```python
# MYNUM (własny numer telefonu) - FORMAT BCD
if self.config.EEPROM_ADDR_MYNUM and len(data) > self.config.EEPROM_ADDR_MYNUM + 4:
    # Odczytaj 5 bajtów BCD
    mynum_bytes = data[self.config.EEPROM_ADDR_MYNUM:self.config.EEPROM_ADDR_MYNUM + 5]
    # Konwertuj BCD na string
    mynum_str = ""
    for b in mynum_bytes:
        if b == 0xFF:
            break
        # Każdy bajt = 2 cyfry (high nibble, low nibble)
        high = (b >> 4) & 0x0F
        low = b & 0x0F
        if high <= 9:
            mynum_str += str(high)
        if low <= 9:
            mynum_str += str(low)
    self.mynum_var.set(mynum_str)
```

**Zapis (linie 653-666):**
```python
# MYNUM (własny numer telefonu) - FORMAT BCD
if self.config.EEPROM_ADDR_MYNUM:
    mynum_str = self.mynum_var.get().strip()
    # Walidacja: tylko cyfry, maksymalnie 10 cyfr (5 bajtów BCD)
    if mynum_str and mynum_str.isdigit() and len(mynum_str) <= 10:
        # Dopełnij do parzystej liczby cyfr
        if len(mynum_str) % 2 == 1:
            mynum_str = '0' + mynum_str
        
        # Konwertuj na BCD
        for i in range(0, len(mynum_str), 2):
            high = int(mynum_str[i])
            low = int(mynum_str[i+1]) if i+1 < len(mynum_str) else 0
            bcd_byte = (high << 4) | low
            data[self.config.EEPROM_ADDR_MYNUM + i//2] = bcd_byte
        
        # Wypełnij resztę 0xFF
        for i in range((len(mynum_str)+1)//2, 5):
            data[self.config.EEPROM_ADDR_MYNUM + i] = 0xFF
    else:
        # Wyczyść (ustaw na 0xFF)
        for i in range(5):
            data[self.config.EEPROM_ADDR_MYNUM + i] = 0xFF
```

### Opcja 2: Zmiana firmware na ASCII (NIE zalecane)

Wymagałoby zmiany w `main.c` i `adresyeeprom.h`, co zmniejszy liczbę numerów z 200 do 195.

---

## 📊 Podsumowanie Weryfikacji

### ✅ Zgodne Adresy (11/12)

| Kategoria | Liczba adresów | Status |
|-----------|----------------|--------|
| Tryby pracy | 2 | ✅ Zgodne |
| SKRYBA | 4 | ✅ Zgodne |
| TIME | 4 | ✅ Zgodne |
| Mój numer (adres) | 1 | ✅ Zgodny |
| **RAZEM** | **11** | **✅** |

### ⚠️ Niezgodności (1/12)

| Problem | Opis | Priorytet |
|---------|------|-----------|
| Format "Mój numer" | GUI: ASCII (10 B), Firmware: BCD (5 B) | 🔴 WYSOKI |

---

## 🎯 Rekomendacje

### 1. Natychmiastowe (Krytyczne)

✅ **Zaktualizować GUI do formatu BCD**
- Plik: `AC800-DTM-HS.py`
- Linie: 589-599 (odczyt), 653-666 (zapis)
- Kod: Zobacz sekcję "Wymagane Poprawki w GUI"

### 2. Weryfikacja po zmianach

- [ ] Przetestować odczyt numeru z EEPROM
- [ ] Przetestować zapis numeru do EEPROM
- [ ] Sprawdzić czy auto-sync czasu działa poprawnie

### 3. Dokumentacja

- [ ] Zaktualizować dokumentację GUI
- [ ] Dodać komentarze o formacie BCD

---

## 📈 Statystyki Kompilacji

```
EEPROM Region: 1024 B (0x400)
__EEPROM_REGION_LENGTH__ = 0x0400 (1024 B)
__eeprom_end = 0x810000
```

**Wykorzystanie:** 1024/1024 B (100%) ✅

---

## ✅ Wnioski

1. **Adresy EEPROM:** ✅ Wszystkie zgodne między C i Python
2. **Format danych:** ⚠️ Niezgodność w "Mój numer" (ASCII vs BCD)
3. **Priorytet:** 🔴 Wysoki - wymaga natychmiastowej poprawki
4. **Rozwiązanie:** Zaktualizować GUI do formatu BCD

---

*Analiza wygenerowana: 2025-12-22*  
*Narzędzia: avr-nm, grep, analiza kodu*  
*Status: WYMAGA POPRAWKI GUI*
