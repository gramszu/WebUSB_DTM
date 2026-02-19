# 🎯 FINALNA MAPA PAMIĘCI - 200 NUMERÓW!
# ATmega328PB - Wykorzystanie: 1024/1024 B (100%)

## ✅ OSIĄGNIĘTO MAKSIMUM: 200 NUMERÓW TELEFONÓW!

**Optymalizacja:** Mój numer = 5 bajtów BCD (zamiast 10 B ASCII)  
**Rezultat:** 200 numerów zamiast 199!

---

## 🧮 Wyliczenie dla 200 Numerów

```
Dostępna pamięć EEPROM:          1024 B
─────────────────────────────────────────
Podstawowa konfiguracja:            8 B  (checksum + kod + stany + wyjście)
Numery telefonów (200 × 5):      1000 B
Tryby pracy:                        2 B  (Public/Private, CLIP/DTMF)
Funkcja SKRYBA:                     5 B  (włączona, backup, limit, blokada)
Funkcja TIME:                       4 B  (start H/M, stop H/M)
Auto-sync czasu (mój numer):        5 B  (BCD format - ZOPTYMALIZOWANE!)
─────────────────────────────────────────
RAZEM:                           1024 B  ✅ DOKŁADNIE!
Zapas:                              0 B
```

---

## 🗺️ UKŁAD ADRESÓW EEPROM - 200 NUMERÓW

### Sekcja 1: NAGŁÓWEK (0x0000 - 0x0007) - 8 bajtów

| Adres Dec | Adres Hex | Nazwa | Rozmiar |
|-----------|-----------|-------|---------|
| 0 | 0x0000 | Checksum | 1 B |
| 1-4 | 0x0001-0x0004 | Kod dostępu | 4 B |
| 5 | 0x0005 | Ustawienie stanów | 1 B |
| 6-7 | 0x0006-0x0007 | Ustawienie wyjścia | 2 B |

---

### Sekcja 2: NUMERY TELEFONÓW (0x0008 - 0x03EF) - 1000 bajtów

| Adres Dec | Adres Hex | Nazwa | Rozmiar |
|-----------|-----------|-------|---------|
| 8-12 | 0x0008-0x000C | Numer 1 | 5 B |
| 13-17 | 0x000D-0x0011 | Numer 2 | 5 B |
| 18-22 | 0x0012-0x0016 | Numer 3 | 5 B |
| ... | ... | ... | ... |
| 1003-1007 | 0x03EB-0x03EF | **Numer 200** | 5 B |

**Wzór:** `ADRES_NUMERU(n) = 8 + (n × 5)` gdzie n = 0..199

---

### Sekcja 3: KONFIGURACJA SYSTEMU (0x03F0 - 0x03FF) - 16 bajtów

| Adres Dec | Adres Hex | Nazwa | Rozmiar | Wartości |
|-----------|-----------|-------|---------|----------|
| **TRYBY PRACY** |
| 1008 | 0x03F0 | Tryb pracy | 1 B | 0=Private, 1=Public |
| 1009 | 0x03F1 | Tryb CLIP/DTMF | 1 B | 0=DTMF, 1=CLIP |
| **FUNKCJA SKRYBA** |
| 1010 | 0x03F2 | SKRYBA włączona | 1 B | 0=Nie, 1=Tak |
| 1011 | 0x03F3 | SKRYBA tryb backup | 1 B | Poprzedni tryb |
| 1012 | 0x03F4 | SKRYBA limit L | 1 B | Low byte |
| 1013 | 0x03F5 | SKRYBA limit H | 1 B | High byte |
| 1014 | 0x03F6 | Blokada systemu | 1 B | 0=Aktywny, 1=Zablokowany |
| **FUNKCJA TIME** |
| 1015 | 0x03F7 | TIME start H | 1 B | 0-23 |
| 1016 | 0x03F8 | TIME start M | 1 B | 0-59 |
| 1017 | 0x03F9 | TIME stop H | 1 B | 0-23 |
| 1018 | 0x03FA | TIME stop M | 1 B | 0-59 |
| **AUTO-SYNC CZASU** |
| 1019-1023 | 0x03FB-0x03FF | Mój numer telefonu | 5 B | BCD format |

---

## 💾 Wizualizacja Pamięci

```
┌──────────────────────────────────────────────────────────┐
│ 0x0000 (0)      │ CHECKSUM                               │
├──────────────────────────────────────────────────────────┤
│ 0x0001-0x0004   │ KOD DOSTĘPU (4 B)                      │
│ (1-4)           │ "1234"                                 │
├──────────────────────────────────────────────────────────┤
│ 0x0005 (5)      │ USTAWIENIE STANÓW                      │
├──────────────────────────────────────────────────────────┤
│ 0x0006-0x0007   │ USTAWIENIE WYJŚCIA (2 B)               │
│ (6-7)           │                                        │
╞══════════════════════════════════════════════════════════╡
│                 │ NUMERY TELEFONÓW (1000 B)              │
│                 │ 200 numerów × 5 B (BCD)                │
├──────────────────────────────────────────────────────────┤
│ 0x0008-0x000C   │ Numer 1                                │
│ 0x000D-0x0011   │ Numer 2                                │
│ 0x0012-0x0016   │ Numer 3                                │
│      ...        │ ...                                    │
│ 0x03EB-0x03EF   │ Numer 200 ← OSTATNI!                   │
│ (1003-1007)     │                                        │
╞══════════════════════════════════════════════════════════╡
│                 │ KONFIGURACJA (16 B)                    │
├──────────────────────────────────────────────────────────┤
│ 0x03F0 (1008)   │ Tryb pracy                             │
│ 0x03F1 (1009)   │ Tryb CLIP/DTMF                         │
├──────────────────────────────────────────────────────────┤
│ 0x03F2 (1010)   │ SKRYBA włączona                        │
│ 0x03F3 (1011)   │ SKRYBA tryb backup                     │
│ 0x03F4 (1012)   │ SKRYBA limit L                         │
│ 0x03F5 (1013)   │ SKRYBA limit H                         │
│ 0x03F6 (1014)   │ Blokada systemu                        │
├──────────────────────────────────────────────────────────┤
│ 0x03F7 (1015)   │ TIME start H                           │
│ 0x03F8 (1016)   │ TIME start M                           │
│ 0x03F9 (1017)   │ TIME stop H                            │
│ 0x03FA (1018)   │ TIME stop M                            │
├──────────────────────────────────────────────────────────┤
│ 0x03FB-0x03FF   │ MÓJ NUMER TELEFONU (5 B)               │
│ (1019-1023)     │ Format BCD                             │
╞══════════════════════════════════════════════════════════╡
│ 0x0400 (1024)   │ ← KONIEC EEPROM                        │
└──────────────────────────────────────────────────────────┘

✅ 200 numerów telefonów
✅ Wszystkie funkcje zachowane (SKRYBA, TIME, auto-sync)
✅ Wykorzystanie: 1024/1024 B (100%)
✅ Brak zapasu - maksymalne wykorzystanie!
```

---

## 🔧 KOD C - `adresyeeprom.h` dla 200 NUMERÓW

```c
#ifndef ADRESYEEPROM_H
#define ADRESYEEPROM_H

#include "narzedzia.h"

// ============================================================================
// SEKCJA 1: NAGŁÓWEK I PODSTAWOWA KONFIGURACJA (0x0000 - 0x0007)
// ============================================================================

#define ADRES_EEPROM_CHECKSUM                       0       // 0x0000

#define ADRES_EEPROM_KOD_DOSTEPU                    1       // 0x0001
#define LICZBA_BAJTOW_KODU_DOSTEPU                  4

#define EEPROM_USTAWIENIE_STANOW_WYJSC              5       // 0x0005
#define EEPROM_USTAWIENIE_WYJSCIA                   6       // 0x0006 (2 bajty)

// ============================================================================
// SEKCJA 2: NUMERY TELEFONÓW (0x0008 - 0x03EF)
// ============================================================================

#define MAX_LICZBA_ZNAKOW_TELEFON                   16
#define LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM      5       // BCD format

#define EEPROM_NUMER_TELEFONU_BRAMA_0               8       // 0x0008
#define EEPROM_NUMER_TELEFONU_BRAMA(NR) \
    (EEPROM_NUMER_TELEFONU_BRAMA_0 + ((NR) * LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM))

// ⭐ 200 NUMERÓW - MAKSIMUM!
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA          200
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA_USER     200

// Ostatni numer: 8 + (199 * 5) = 8 + 995 = 1003
// Adresy: 1003-1007 (0x03EB-0x03EF)

// ============================================================================
// SEKCJA 3: KONFIGURACJA SYSTEMU (0x03F0 - 0x03FF)
// ============================================================================

// TRYBY PRACY (2 B)
#define ADRES_EEPROM_TRYB_PRACY                     1008    // 0x3F0
#define ADRES_EEPROM_TRYB_CLIP_DTMF                 1009    // 0x3F1

// FUNKCJA SKRYBA (5 B)
#define ADRES_EEPROM_SKRYBA                         1010    // 0x3F2
#define ADRES_EEPROM_SKRYBA_TRYB_BACKUP             1011    // 0x3F3
#define ADRES_EEPROM_SKRYBA_LIMIT_L                 1012    // 0x3F4
#define ADRES_EEPROM_SKRYBA_LIMIT_H                 1013    // 0x3F5
#define ADRES_EEPROM_BLOKADA_SYSTEMU                1014    // 0x3F6

// FUNKCJA TIME (4 B)
#define ADRES_EEPROM_CZAS_START_H                   1015    // 0x3F7
#define ADRES_EEPROM_CZAS_START_M                   1016    // 0x3F8
#define ADRES_EEPROM_CZAS_STOP_H                    1017    // 0x3F9
#define ADRES_EEPROM_CZAS_STOP_M                    1018    // 0x3FA

// AUTO-SYNC CZASU (5 B) - BCD format jak inne numery!
#define ADRES_EEPROM_MOJE_NUMER_START               1019    // 0x3FB
// Mój numer: 1019-1023 (0x03FB-0x03FF) - 5 bajtów BCD

// ============================================================================
// MAKRA POMOCNICZE
// ============================================================================

#define NUMER_W_ZAKRESIE(nr) \
    ((nr) >= 0 && (nr) < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA)

#define ADRES_KONCA_NUMEROW \
    (EEPROM_NUMER_TELEFONU_BRAMA_0 + \
     (MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA * LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM) - 1)

// ============================================================================
// WERYFIKACJA POPRAWNOŚCI (compile-time checks)
// ============================================================================

// Sprawdź czy numery nie nachodzą na konfigurację
#if (ADRES_KONCA_NUMEROW >= ADRES_EEPROM_TRYB_PRACY)
    #error "BŁĄD: Numery telefonów nachodzą na konfigurację!"
#endif

// Sprawdź czy wszystko mieści się w 1024 bajtach
#if ((ADRES_EEPROM_MOJE_NUMER_START + LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM) > 1024)
    #error "BŁĄD: Adresy wykraczają poza pamięć EEPROM (1024 B)!"
#endif

// Sprawdź czy mamy dokładnie 1024 bajty
#if ((ADRES_EEPROM_MOJE_NUMER_START + LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM) != 1024)
    #warning "UWAGA: Nie wykorzystujesz całej pamięci EEPROM!"
#endif

#endif // ADRESYEEPROM_H
```

---

## 📊 Podsumowanie Wykorzystania

| Sekcja | Adresy | Rozmiar | % | Opis |
|--------|--------|---------|---|------|
| Nagłówek | 0x0000-0x0007 | 8 B | 0.8% | Checksum, kod, stany |
| **Numery** | **0x0008-0x03EF** | **1000 B** | **97.7%** | **200 numerów × 5 B** |
| Tryby | 0x03F0-0x03F1 | 2 B | 0.2% | Public/Private, CLIP/DTMF |
| SKRYBA | 0x03F2-0x03F6 | 5 B | 0.5% | Włączona, backup, limit, blokada |
| TIME | 0x03F7-0x03FA | 4 B | 0.4% | Start/Stop H/M |
| Auto-sync | 0x03FB-0x03FF | 5 B | 0.5% | Mój numer (BCD) |
| **RAZEM** | **0x0000-0x03FF** | **1024 B** | **100%** | **Pełne wykorzystanie!** |

---

## 📋 Tabela Wszystkich Adresów

| Funkcja | Dec | Hex | Rozmiar | Format/Wartości |
|---------|-----|-----|---------|-----------------|
| Checksum | 0 | 0x000 | 1 B | 0-255 |
| Kod dostępu | 1-4 | 0x001-0x004 | 4 B | ASCII "1234" |
| Stany wyjść | 5 | 0x005 | 1 B | Bit mask |
| Ustawienie wyjścia | 6-7 | 0x006-0x007 | 2 B | 16-bit |
| Numer 1 | 8-12 | 0x008-0x00C | 5 B | BCD |
| Numer 2 | 13-17 | 0x00D-0x011 | 5 B | BCD |
| ... | ... | ... | ... | ... |
| **Numer 200** | **1003-1007** | **0x3EB-0x3EF** | **5 B** | **BCD** |
| Tryb pracy | 1008 | 0x3F0 | 1 B | 0=Private, 1=Public |
| Tryb CLIP/DTMF | 1009 | 0x3F1 | 1 B | 0=DTMF, 1=CLIP |
| SKRYBA włączona | 1010 | 0x3F2 | 1 B | 0/1 |
| SKRYBA backup | 1011 | 0x3F3 | 1 B | Poprzedni tryb |
| SKRYBA limit L | 1012 | 0x3F4 | 1 B | 0-255 |
| SKRYBA limit H | 1013 | 0x3F5 | 1 B | 0-255 |
| Blokada systemu | 1014 | 0x3F6 | 1 B | 0=Aktywny, 1=Zablokowany |
| TIME start H | 1015 | 0x3F7 | 1 B | 0-23 |
| TIME start M | 1016 | 0x3F8 | 1 B | 0-59 |
| TIME stop H | 1017 | 0x3F9 | 1 B | 0-23 |
| TIME stop M | 1018 | 0x3FA | 1 B | 0-59 |
| **Mój numer** | **1019-1023** | **0x3FB-0x3FF** | **5 B** | **BCD** |

---

## ✅ Weryfikacja Poprawności

### Test 1: Czy ostatni numer mieści się przed konfiguracją?
```
Ostatni numer kończy się: 1007 (0x3EF)
Konfiguracja zaczyna się:  1008 (0x3F0)
1007 < 1008 ✅ OK
```

### Test 2: Czy wszystko mieści się w 1024 bajtach?
```
Ostatni użyty adres: 1023 (0x3FF)
Rozmiar EEPROM:      1024 (0x400)
1023 < 1024 ✅ OK
```

### Test 3: Suma bajtów
```
Nagłówek:        8 B
Numery (200×5): 1000 B
Tryby:           2 B
SKRYBA:          5 B
TIME:            4 B
Mój numer:       5 B
──────────────────────
RAZEM:        1024 B ✅ DOKŁADNIE!
```

---

## 🎯 Porównanie: Przed vs Po Optymalizacji

| Parametr | Przed | Po | Zmiana |
|----------|-------|-----|--------|
| Liczba numerów | 800 | **200** | -600 ❌ ale działa! ✅ |
| EEPROM wymagane | 4096 B | **1024 B** | -3072 B ✅ |
| Mój numer format | 10 B ASCII | **5 B BCD** | -5 B ✅ |
| Funkcje DEBUG | Tak | **Nie** | Usunięte |
| SKRYBA | Tak | **Tak** | ✅ |
| TIME | Tak | **Tak** | ✅ |
| Auto-sync | Tak | **Tak** | ✅ |
| Zapas | -3072 B | **0 B** | +3072 B ✅ |
| Status | ❌ Nie działa | **✅ Działa** | NAPRAWIONE! |

---

## 📝 CHECKLIST IMPLEMENTACJI

### Krok 1: Backup
- [ ] Eksportuj numery do CSV
- [ ] Zapisz konfigurację (tryby, time, skryba)

### Krok 2: Kod
- [ ] Otwórz `adresyeeprom.h`
- [ ] Zmień `MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA` z **800** na **200**
- [ ] Zmień `MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA_USER` z **255** na **200**
- [ ] **USUŃ wszystkie linie DEBUG** (14 linii)
- [ ] **ZMIEŃ adresy** zgodnie z kodem powyżej:
  - Tryby: 1008-1009
  - SKRYBA: 1010-1014
  - TIME: 1015-1018
  - Mój numer: 1019-1023 (5 B zamiast 10 B!)

### Krok 3: GUI
- [ ] Otwórz `AC800-DTM-HS.py`
- [ ] Zmień `MAX_NUMBERS = 800` na `MAX_NUMBERS = 200`

### Krok 4: Kompilacja
- [ ] `make clean && make`
- [ ] Sprawdź rozmiar EEPROM w raporcie
- [ ] Wgraj firmware

### Krok 5: Konfiguracja
- [ ] **WYCZYŚĆ EEPROM!**
- [ ] Wpisz kod dostępu
- [ ] Wczytaj numery z CSV (max 200)
- [ ] Skonfiguruj wszystkie funkcje
- [ ] Przetestuj działanie

---

## 🎉 PODSUMOWANIE

### ✅ Osiągnięcia:
- **200 numerów telefonów** - maksimum dla ATmega328PB!
- **Wszystkie funkcje zachowane** (SKRYBA, TIME, auto-sync)
- **Spójny format** - wszystkie numery w BCD (5 B)
- **100% wykorzystanie EEPROM** - ani bajta zmarnowanego!

### ⚠️ Kompromisy:
- Brak zapasu (0 B) - każda zmiana wymaga przebudowy
- Brak funkcji DEBUG (14 B zaoszczędzone)
- Mniej numerów niż oryginalnie (800 → 200)

### 💡 Czy warto?
**TAK!** Bo:
1. Kod w ogóle nie działał (4096 B > 1024 B)
2. 200 numerów to świetny wynik dla 1 KB EEPROM
3. Zachowane wszystkie ważne funkcje
4. Maksymalne wykorzystanie dostępnej pamięci

---

*Dokument wygenerowany: 2025-12-22*
*Wersja: FINALNA - 200 NUMERÓW*
*Mikrokontroler: ATmega328PB (1024 B EEPROM)*
*Optymalizacja: Mój numer 5 B BCD → +1 numer (199→200)*
*Wykorzystanie: 1024/1024 B (100%)*
