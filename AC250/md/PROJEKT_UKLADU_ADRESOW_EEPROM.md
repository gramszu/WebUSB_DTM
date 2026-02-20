# ZOPTYMALIZOWANY Układ Adresów EEPROM
# ATmega328PB - 199 Numerów (Mój numer: 5 B zamiast 10 B)

## 🎯 OPTYMALIZACJA: Mój numer = 5 bajtów (jak wszystkie inne)

**Zmiana:** "Mój numer telefonu" używa tego samego formatu co numery w bazie (5 bajtów BCD)
**Zysk:** +5 bajtów zapasu (było 3 B → teraz 8 B)

---

## 🗺️ ZOPTYMALIZOWANY UKŁAD ADRESÓW

### Sekcja 1: NAGŁÓWEK I PODSTAWOWA KONFIGURACJA (0x0000 - 0x0007)

| Adres Dec | Adres Hex | Nazwa | Rozmiar | Opis |
|-----------|-----------|-------|---------|------|
| 0 | 0x0000 | `CHECKSUM` | 1 B | Suma kontrolna całej pamięci |
| 1-4 | 0x0001-0x0004 | `KOD_DOSTEPU` | 4 B | 4-cyfrowy kod ASCII (np. "1234") |
| 5 | 0x0005 | `USTAWIENIE_STANOW_WYJSC` | 1 B | Stany wyjść (bit mask) |
| 6-7 | 0x0006-0x0007 | `USTAWIENIE_WYJSCIA` | 2 B | Konfiguracja wyjścia (16-bit) |

**Razem: 8 bajtów**

---

### Sekcja 2: NUMERY TELEFONÓW (0x0008 - 0x03E7)

| Adres Dec | Adres Hex | Nazwa | Rozmiar | Opis |
|-----------|-----------|-------|---------|------|
| 8-12 | 0x0008-0x000C | `NUMER_TELEFONU[0]` | 5 B | Numer 1 (BCD, 10 cyfr) |
| 13-17 | 0x000D-0x0011 | `NUMER_TELEFONU[1]` | 5 B | Numer 2 |
| ... | ... | ... | ... | ... |
| 995-999 | 0x03E3-0x03E7 | `NUMER_TELEFONU[198]` | 5 B | Numer 199 (ostatni) |

**Razem: 995 bajtów (199 × 5)**

---

### Sekcja 3: TRYBY PRACY (0x03E8 - 0x03E9)

| Adres Dec | Adres Hex | Nazwa | Rozmiar | Wartości | Opis |
|-----------|-----------|-------|---------|----------|------|
| 1000 | 0x03E8 | `TRYB_PRACY` | 1 B | 0=Private, 1=Public | Tryb dostępu |
| 1001 | 0x03E9 | `TRYB_CLIP_DTMF` | 1 B | 0=DTMF, 1=CLIP | Metoda rozpoznawania |

**Razem: 2 bajty**

---

### Sekcja 4: FUNKCJA SKRYBA (0x03EA - 0x03EE)

| Adres Dec | Adres Hex | Nazwa | Rozmiar | Opis |
|-----------|-----------|-------|---------|------|
| 1002 | 0x03EA | `SKRYBA_WLACZONA` | 1 B | 0=Wyłączona, 1=Włączona |
| 1003 | 0x03EB | `SKRYBA_TRYB_BACKUP` | 1 B | Backup poprzedniego trybu |
| 1004 | 0x03EC | `SKRYBA_LIMIT_L` | 1 B | Limit użytkowników (low byte) |
| 1005 | 0x03ED | `SKRYBA_LIMIT_H` | 1 B | Limit użytkowników (high byte) |
| 1006 | 0x03EE | `BLOKADA_SYSTEMU` | 1 B | 0=Aktywny, 1=Zablokowany |

**Razem: 5 bajtów**

---

### Sekcja 5: FUNKCJA TIME (0x03EF - 0x03F2)

| Adres Dec | Adres Hex | Nazwa | Rozmiar | Zakres | Opis |
|-----------|-----------|-------|---------|--------|------|
| 1007 | 0x03EF | `TIME_START_H` | 1 B | 0-23 | Godzina rozpoczęcia (24h) |
| 1008 | 0x03F0 | `TIME_START_M` | 1 B | 0-59 | Minuta rozpoczęcia |
| 1009 | 0x03F1 | `TIME_STOP_H` | 1 B | 0-23 | Godzina zakończenia (24h) |
| 1010 | 0x03F2 | `TIME_STOP_M` | 1 B | 0-59 | Minuta zakończenia |

**Razem: 4 bajty**

---

### Sekcja 6: AUTO-SYNC CZASU (0x03F3 - 0x03F7) ✨ ZOPTYMALIZOWANE!

| Adres Dec | Adres Hex | Nazwa | Rozmiar | Format | Opis |
|-----------|-----------|-------|---------|--------|------|
| 1011-1015 | 0x03F3-0x03F7 | `MOJ_NUMER_TELEFONU` | **5 B** | **BCD** | Własny numer (jak inne numery) |

**Razem: 5 bajtów** (było 10 B → oszczędność 5 B!)

**Format BCD (Binary Coded Decimal):**
- 5 bajtów = 10 cyfr (każdy bajt = 2 cyfry)
- Przykład: `+48123456789` → BCD: `48 12 34 56 78`
- Taki sam format jak numery w bazie danych

---

### Sekcja 7: ZAPAS (0x03F8 - 0x03FF) ✨ ZWIĘKSZONY!

| Adres Dec | Adres Hex | Nazwa | Rozmiar | Opis |
|-----------|-----------|-------|---------|------|
| 1016-1023 | 0x03F8-0x03FF | `ZAPAS` | **8 B** | Wolne bajty na przyszłość |

**Razem: 8 bajtów** (było 3 B → zysk +5 B!)

---

## 📊 Podsumowanie Wykorzystania

| Sekcja | Adresy | Rozmiar | % EEPROM | Zmiana |
|--------|--------|---------|----------|--------|
| 1. Nagłówek i konfiguracja | 0x0000-0x0007 | 8 B | 0.8% | - |
| 2. Numery telefonów (199×5) | 0x0008-0x03E7 | 995 B | 97.2% | - |
| 3. Tryby pracy | 0x03E8-0x03E9 | 2 B | 0.2% | - |
| 4. Funkcja SKRYBA | 0x03EA-0x03EE | 5 B | 0.5% | - |
| 5. Funkcja TIME | 0x03EF-0x03F2 | 4 B | 0.4% | - |
| 6. Auto-sync czasu | 0x03F3-0x03F7 | **5 B** | 0.5% | **-5 B** ✅ |
| 7. Zapas | 0x03F8-0x03FF | **8 B** | 0.8% | **+5 B** ✅ |
| **RAZEM** | **0x0000-0x03FF** | **1024 B** | **100%** | - |

---

## 💾 Wizualizacja Pamięci EEPROM (ZOPTYMALIZOWANA)

```
0x0000 ┌────────────────────────────────────────────┐
       │ CHECKSUM (1 B)                             │
0x0001 ├────────────────────────────────────────────┤
       │ KOD DOSTĘPU (4 B)                          │
0x0005 ├────────────────────────────────────────────┤
       │ USTAWIENIE STANÓW (1 B)                    │
0x0006 ├────────────────────────────────────────────┤
       │ USTAWIENIE WYJŚCIA (2 B)                   │
0x0008 ╞════════════════════════════════════════════╡
       │                                            │
       │ NUMERY TELEFONÓW (995 B)                   │
       │ 199 numerów × 5 B (BCD)                    │
       │                                            │
0x03E8 ╞════════════════════════════════════════════╡
       │ TRYB PRACY (1 B)                           │
0x03E9 ├────────────────────────────────────────────┤
       │ TRYB CLIP/DTMF (1 B)                       │
0x03EA ╞════════════════════════════════════════════╡
       │ SKRYBA (5 B)                               │
       │ - Włączona, Backup, Limit L/H, Blokada     │
0x03EF ╞════════════════════════════════════════════╡
       │ TIME (4 B)                                 │
       │ - Start H/M, Stop H/M                      │
0x03F3 ╞════════════════════════════════════════════╡
       │ MÓJ NUMER (5 B) ← ZOPTYMALIZOWANE!         │
       │ Format BCD (jak inne numery)               │
0x03F8 ╞════════════════════════════════════════════╡
       │ ZAPAS (8 B) ← ZWIĘKSZONY!                  │
       │ Wolne na przyszłość                        │
0x0400 └────────────────────────────────────────────┘
```

---

## 🔧 KOD C - ZOPTYMALIZOWANY `adresyeeprom.h`

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
// SEKCJA 2: NUMERY TELEFONÓW (0x0008 - 0x03E7)
// ============================================================================

#define MAX_LICZBA_ZNAKOW_TELEFON                   16
#define LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM      5       // BCD format

#define EEPROM_NUMER_TELEFONU_BRAMA_0               8       // 0x0008
#define EEPROM_NUMER_TELEFONU_BRAMA(NR) \
    (EEPROM_NUMER_TELEFONU_BRAMA_0 + ((NR) * LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM))

#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA          199
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA_USER     199

// ============================================================================
// SEKCJA 3: TRYBY PRACY (0x03E8 - 0x03E9)
// ============================================================================

#define ADRES_EEPROM_TRYB_PRACY                     1000    // 0x3E8
// Wartości: 0 = Private (tylko autoryzowani), 1 = Public (wszyscy)

#define ADRES_EEPROM_TRYB_CLIP_DTMF                 1001    // 0x3E9
// Wartości: 0 = DTMF (kody tonowe), 1 = CLIP (rozpoznawanie numeru)

// ============================================================================
// SEKCJA 4: FUNKCJA SKRYBA (0x03EA - 0x03EE)
// ============================================================================

#define ADRES_EEPROM_SKRYBA                         1002    // 0x3EA
#define ADRES_EEPROM_SKRYBA_TRYB_BACKUP             1003    // 0x3EB
#define ADRES_EEPROM_SKRYBA_LIMIT_L                 1004    // 0x3EC
#define ADRES_EEPROM_SKRYBA_LIMIT_H                 1005    // 0x3ED
#define ADRES_EEPROM_BLOKADA_SYSTEMU                1006    // 0x3EE

// ============================================================================
// SEKCJA 5: FUNKCJA TIME (0x03EF - 0x03F2)
// ============================================================================

#define ADRES_EEPROM_CZAS_START_H                   1007    // 0x3EF
#define ADRES_EEPROM_CZAS_START_M                   1008    // 0x3F0
#define ADRES_EEPROM_CZAS_STOP_H                    1009    // 0x3F1
#define ADRES_EEPROM_CZAS_STOP_M                    1010    // 0x3F2

// ============================================================================
// SEKCJA 6: AUTO-SYNC CZASU (0x03F3 - 0x03F7) - ZOPTYMALIZOWANE!
// ============================================================================

#define ADRES_EEPROM_MOJE_NUMER_START               1011    // 0x3F3
// Własny numer telefonu urządzenia (5 bajtów BCD - taki sam format jak inne numery!)
// Format BCD: 5 bajtów = 10 cyfr
// Przykład: +48123456789 → BCD: 48 12 34 56 78
// Używane do automatycznej synchronizacji czasu z sieci GSM

// ============================================================================
// SEKCJA 7: ZAPAS (0x03F8 - 0x03FF) - ZWIĘKSZONY!
// ============================================================================

#define ADRES_EEPROM_ZAPAS_START                    1016    // 0x3F8
// 8 bajtów wolnych (1016-1023)

// Możliwe wykorzystanie w przyszłości:
#define ADRES_EEPROM_WERSJA_FIRMWARE                1016    // 0x3F8 (1 B)
#define ADRES_EEPROM_FLAGI_SYSTEMOWE                1017    // 0x3F9 (1 B)
#define ADRES_EEPROM_CRC16_L                        1018    // 0x3FA (1 B)
#define ADRES_EEPROM_CRC16_H                        1019    // 0x3FB (1 B)
// 1020-1023 (0x3FC-0x3FF) - nadal wolne (4 B)

// ============================================================================
// MAKRA POMOCNICZE
// ============================================================================

// Sprawdzenie czy numer jest w zakresie
#define NUMER_W_ZAKRESIE(nr) \
    ((nr) >= 0 && (nr) < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA)

// Obliczenie adresu końca numerów
#define ADRES_KONCA_NUMEROW \
    (EEPROM_NUMER_TELEFONU_BRAMA_0 + \
     (MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA * LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM) - 1)

// Weryfikacja poprawności układu (compile-time check)
#if (ADRES_KONCA_NUMEROW >= ADRES_EEPROM_TRYB_PRACY)
    #error "Numery telefonów nachodzą na konfigurację!"
#endif

#if (ADRES_EEPROM_ZAPAS_START + 8 > 1024)
    #error "Adresy wykraczają poza pamięć EEPROM (1024 bajty)!"
#endif

// Weryfikacja że mój numer używa tego samego formatu co inne numery
#if ((ADRES_EEPROM_MOJE_NUMER_START + LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM - 1) >= ADRES_EEPROM_ZAPAS_START)
    #error "Mój numer nachodzi na sekcję zapasu!"
#endif

#endif // ADRESYEEPROM_H
```

---

## 📋 Tabela Szybkiego Odniesienia (ZAKTUALIZOWANA)

| Funkcja | Adres Dec | Adres Hex | Rozmiar | Format |
|---------|-----------|-----------|---------|--------|
| Checksum | 0 | 0x0000 | 1 B | 0-255 |
| Kod dostępu | 1-4 | 0x0001-0x0004 | 4 B | ASCII |
| Stany wyjść | 5 | 0x0005 | 1 B | Bit mask |
| Ustawienie wyjścia | 6-7 | 0x0006-0x0007 | 2 B | 16-bit |
| Numer 1 | 8-12 | 0x0008-0x000C | 5 B | BCD |
| ... | ... | ... | ... | ... |
| Numer 199 | 995-999 | 0x03E3-0x03E7 | 5 B | BCD |
| Tryb pracy | 1000 | 0x03E8 | 1 B | 0/1 |
| Tryb CLIP/DTMF | 1001 | 0x03E9 | 1 B | 0/1 |
| SKRYBA | 1002-1006 | 0x03EA-0x03EE | 5 B | - |
| TIME | 1007-1010 | 0x03EF-0x03F2 | 4 B | - |
| **Mój numer** | **1011-1015** | **0x03F3-0x03F7** | **5 B** | **BCD** ✅ |
| **Zapas** | **1016-1023** | **0x03F8-0x03FF** | **8 B** | **Wolne** ✅ |

---

## ✅ Korzyści z Optymalizacji

### Przed (10 bajtów na mój numer):
- Mój numer: 10 B (ASCII)
- Zapas: 3 B
- Format: Inny niż numery w bazie

### Po (5 bajtów na mój numer):
- Mój numer: **5 B (BCD)** ✅
- Zapas: **8 B** ✅
- Format: **Taki sam jak numery w bazie** ✅

### Zalety:
1. ✅ **+5 bajtów zapasu** (3 B → 8 B)
2. ✅ **Spójny format** - wszystkie numery w BCD
3. ✅ **Mniej kodu** - te same funkcje konwersji
4. ✅ **Łatwiejsze w utrzymaniu** - jeden format dla wszystkich numerów

---

## 🔄 Porównanie Formatów

### Format BCD (5 bajtów):
```
Numer: +48123456789
BCD:   48 12 34 56 78 (5 bajtów)
       ↑  ↑  ↑  ↑  ↑
       każdy bajt = 2 cyfry
```

### Format ASCII (10 bajtów - STARY):
```
Numer: +48123456789
ASCII: 2B 34 38 31 32 33 34 35 36 37 38 39 00 (13 bajtów z null!)
       ↑  ↑  ↑  ↑  ↑  ↑  ↑  ↑  ↑  ↑  ↑  ↑  ↑
       +  4  8  1  2  3  4  5  6  7  8  9  \0
```

**BCD jest 2x bardziej efektywny!** 🎯

---

## 💡 Możliwe Wykorzystanie Zapasu (8 bajtów)

| Adres | Nazwa | Rozmiar | Opis |
|-------|-------|---------|------|
| 1016 | Wersja firmware | 1 B | Numer wersji (np. 3 = v3.0) |
| 1017 | Flagi systemowe | 1 B | Bit flags (8 flag) |
| 1018-1019 | CRC16 | 2 B | Suma kontrolna CRC16 (lepsze niż checksum) |
| 1020 | Licznik resetów | 1 B | Liczba resetów systemu |
| 1021 | Ostatni błąd | 1 B | Kod ostatniego błędu |
| 1022-1023 | Wolne | 2 B | Zapas na przyszłość |

---

*Dokument wygenerowany: 2025-12-22*
*Wersja: ZOPTYMALIZOWANA*
*Optymalizacja: Mój numer 5 B (BCD) zamiast 10 B (ASCII)*
*Zysk: +5 bajtów zapasu (3 B → 8 B)*
