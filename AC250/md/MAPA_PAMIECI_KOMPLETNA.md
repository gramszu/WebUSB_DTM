# Mapa Pamięci ATmega328PB - AC800-DTM-HS-RC3
# Szczegółowa Analiza WSZYSTKICH Funkcji

## 📊 Podsumowanie Wykorzystania Pamięci

### Specyfikacja ATmega328PB
| Typ Pamięci | Dostępne | Wykorzystane | Wolne | % Wykorzystania |
|-------------|----------|--------------|-------|-----------------|
| **Flash (Program)** | 32768 B (32 KB) | 26286 B | 6482 B | **80.2%** |
| **SRAM (RAM)** | 2048 B (2 KB) | 1824 B | 224 B | **89.1%** |
| **EEPROM** | 1024 B (1 KB) | Zależy od liczby numerów | - | - |

---

## 🔍 KOMPLETNA ANALIZA WSZYSTKICH FUNKCJI EEPROM

### Wszystkie Funkcje Zdefiniowane w `adresyeeprom.h`

| Funkcja | Adresy Aktualne | Liczba Bajtów | Opis |
|---------|-----------------|---------------|------|
| **Checksum** | 0 | 1 B | Suma kontrolna |
| **Kod dostępu** | 1-4 | 4 B | 4-cyfrowy kod ASCII |
| **Ustawienie stanów** | 5 | 1 B | Stany wyjść |
| **Ustawienie wyjścia** | 6-7 | 2 B | Konfiguracja wyjścia |
| **Numery telefonów** | 8 - ... | **N × 5 B** | N numerów × 5 bajtów każdy |
| | | | |
| **Mój numer (auto-sync)** | 4040-4049 | 10 B | Własny numer urządzenia dla auto-sync czasu |
| **Debug start** | 4050 | 1 B | Debug marker |
| **Debug licznik resetów** | 4060 | 1 B | Licznik resetów systemu |
| **Debug USER 1** | 4070 | 1 B | Komenda USER otrzymana |
| **Debug USER 2** | 4071 | 1 B | flaga_wysylanie_smsa |
| **Debug USER 3** | 4072 | 1 B | licznik_report_user |
| **Debug USER 4** | 4073 | 1 B | liczba_sms_w_kolejce |
| **Debug USER 5** | 4074 | 1 B | liczba_wszystkich_komend |
| **Debug USER 6** | 4075 | 1 B | znaleziono (0/1) |
| **Debug USER 7** | 4076 | 1 B | dodano_komende_wyslij (0/1) |
| **Debug SKRYBA 1** | 4080 | 1 B | CLIP otrzymany |
| **Debug SKRYBA 2** | 4081 | 1 B | skryba_wlaczona |
| **Debug SKRYBA 3** | 4082 | 1 B | !znaleziono |
| **Debug SKRYBA 4** | 4083 | 1 B | komenda dodana |
| **Debug SKRYBA 5** | 4084 | 1 B | komenda wykonana |
| **Skryba limit L** | 4085 | 1 B | Low byte limitu użytkowników |
| **Skryba limit H** | 4086 | 1 B | High byte limitu użytkowników |
| **Blokada systemu** | 4087 | 1 B | Status blokady (0=Aktywny, 1=Zablokowany) |
| **Skryba tryb backup** | 4088 | 1 B | Backup poprzedniego trybu |
| **Skryba** | 4089 | 1 B | Funkcja SKRYBA włączona/wyłączona |
| **Time Start H** | 4090 | 1 B | Godzina startu |
| **Time Start M** | 4091 | 1 B | Minuta startu |
| **Time Stop H** | 4092 | 1 B | Godzina stopu |
| **Time Stop M** | 4093 | 1 B | Minuta stopu |
| **Tryb pracy** | 4094 | 1 B | Public/Private mode |
| **Tryb CLIP/DTMF** | 4095 | 1 B | 0=DTMF, 1=CLIP |

### Podsumowanie Stałych Danych (bez numerów telefonów)

| Kategoria | Bajty | Szczegóły |
|-----------|-------|-----------|
| **Podstawowa konfiguracja** | 8 B | Checksum(1) + Kod dostępu(4) + Stany(1) + Wyjście(2) |
| **Funkcje główne** | 7 B | Skryba(1) + Time Start/Stop(4) + Tryby(2) |
| **Funkcje SKRYBA rozszerzone** | 4 B | Limit L/H(2) + Blokada(1) + Backup(1) |
| **Auto-sync czasu** | 10 B | Mój numer telefonu (max 10 znaków) |
| **Debug SKRYBA** | 5 B | 5 zmiennych diagnostycznych |
| **Debug USER** | 7 B | 7 zmiennych diagnostycznych |
| **Debug inne** | 2 B | Start marker(1) + Licznik resetów(1) |
| **RAZEM (bez numerów)** | **43 B** | Wszystkie funkcje razem |

---

## 🧮 Wyliczenie Maksymalnej Liczby Numerów

**Dostępna pamięć EEPROM ATmega328PB:** 1024 bajty

### Wariant A: WSZYSTKIE funkcje (włącznie z DEBUG) ⭐ ZALECANE

```
Dostępne:           1024 B
Stałe dane:           43 B (wszystkie funkcje)
─────────────────────────
Pozostało na numery: 981 B

Liczba numerów: 981 ÷ 5 = 196.2 → 196 numerów

Weryfikacja:
  - Podstawowa konfiguracja:  8 B
  - Numery (196 × 5):       980 B
  - Pozostałe funkcje:       35 B
  ─────────────────────────────
  RAZEM:                   1023 B ✅
  
Zapas: 1 bajt
```

**✅ MAKSYMALNIE: 196 numerów** (zachowuje WSZYSTKIE funkcje)

---

### Wariant B: BEZ funkcji DEBUG

```
Dostępne:           1024 B
Stałe dane:           29 B (bez 14 B DEBUG)
─────────────────────────
Pozostało na numery: 995 B

Liczba numerów: 995 ÷ 5 = 199 numerów

Weryfikacja:
  - Podstawowa konfiguracja:  8 B
  - Numery (199 × 5):       995 B
  - Pozostałe funkcje:       21 B
  ─────────────────────────────
  RAZEM:                   1024 B ✅
  
Zapas: 0 bajtów
```

**⚠️ MAKSYMALNIE: 199 numerów** (wymaga usunięcia DEBUG)

---

### Wariant C: Z bezpiecznym zapasem (100 B)

```
Cel: 100 bajtów zapasu na przyszłość
Dostępne na dane:   924 B (1024 - 100)
Stałe dane:          29 B (bez DEBUG)
─────────────────────────
Na numery:          895 B

Liczba numerów: 895 ÷ 5 = 179 numerów

Weryfikacja:
  - Podstawowa konfiguracja:  8 B
  - Numery (179 × 5):       895 B
  - Pozostałe funkcje:       21 B
  ─────────────────────────────
  RAZEM:                    924 B ✅
  
Zapas: 100 bajtów
```

**✅ ZALECANE: 179-180 numerów** (bezpieczny zapas)

---

## 📋 Porównanie Wszystkich Wariantów

| Wariant | Liczba Numerów | Funkcje DEBUG | Auto-sync | SKRYBA | TIME | Zapas | Status |
|---------|----------------|---------------|-----------|--------|------|-------|--------|
| **A1** | **196** | ✅ Tak | ✅ Tak | ✅ Tak | ✅ Tak | 1 B | ⭐ **MAKSIMUM z wszystkim** |
| **A2** | **180** | ✅ Tak | ✅ Tak | ✅ Tak | ✅ Tak | 103 B | ⭐ **ZALECANE** |
| **B1** | **199** | ❌ Nie | ✅ Tak | ✅ Tak | ✅ Tak | 0 B | ⚠️ Bez DEBUG, brak zapasu |
| **B2** | **180** | ❌ Nie | ✅ Tak | ✅ Tak | ✅ Tak | 117 B | ✅ Bezpieczne bez DEBUG |
| **C** | **200** | ❌ Nie | ❌ Nie | ✅ Tak | ✅ Tak | 0 B | ❌ Traci auto-sync! |

---

## 🎯 OSTATECZNA REKOMENDACJA

### ⭐ OPCJA 1: **196 numerów** - Maksimum z WSZYSTKIMI funkcjami

```c
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA 196  // było 800
```

**Zalety:**
- ✅ Zachowuje **WSZYSTKIE** funkcje (DEBUG, SKRYBA, TIME, auto-sync)
- ✅ Maksymalna możliwa liczba numerów przy pełnej funkcjonalności
- ✅ Nie wymaga usuwania żadnego kodu
- ✅ 196 numerów to prawie 200 - wystarczy dla większości zastosowań

**Wady:**
- ⚠️ Tylko 1 bajt zapasu (99.9% wykorzystania)
- ⚠️ Brak miejsca na przyszłe rozszerzenia

**Wykorzystanie EEPROM:** 1023/1024 B (99.9%)

---

### ⭐ OPCJA 2: **180 numerów** - Bezpieczna z zapasem (NAJBEZPIECZNIEJSZA)

```c
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA 180  // było 800
```

**Zalety:**
- ✅ Zachowuje **WSZYSTKIE** funkcje
- ✅ **103 bajty zapasu** na przyszłe rozszerzenia
- ✅ Bezpieczne wykorzystanie (90%)
- ✅ Nie wymaga usuwania żadnego kodu
- ✅ 180 numerów wystarczy dla większości zastosowań

**Wady:**
- ⚠️ O 16 numerów mniej niż maksimum

**Wykorzystanie EEPROM:** 921/1024 B (90%)

---

## 📐 Nowa Mapa EEPROM dla 196 numerów (WSZYSTKIE funkcje)

```
┌─────────────────────────────────────────────────────────────┐
│ 0x0000 (0)    │ Checksum (1 bajt)                           │
├─────────────────────────────────────────────────────────────┤
│ 0x0001-0x0004 │ Kod dostępu (4 bajty ASCII)                 │
│ (1-4)         │   Przykład: "1234"                          │
├─────────────────────────────────────────────────────────────┤
│ 0x0005 (5)    │ Ustawienie stanów wyjść (1 bajt)           │
├─────────────────────────────────────────────────────────────┤
│ 0x0006-0x0007 │ Ustawienie wyjścia (2 bajty)               │
│ (6-7)         │                                             │
├─────────────────────────────────────────────────────────────┤
│ 0x0008-0x000C │ Numer 1 (5 bajtów)                         │
│ 0x000D-0x0011 │ Numer 2 (5 bajtów)                         │
│ 0x0012-0x0016 │ Numer 3 (5 bajtów)                         │
│      ...      │   ... (193 numery więcej) ...              │
│ 0x03D7-0x03DB │ Numer 196 (5 bajtów)                       │
│ (983-987)     │                                             │
├─────────────────────────────────────────────────────────────┤
│ 0x03DC (988)  │ Tryb pracy (1 bajt)                        │
│ 0x03DD (989)  │ Tryb CLIP/DTMF (1 bajt)                    │
│ 0x03DE (990)  │ Skryba (1 bajt)                            │
│ 0x03DF (991)  │ Time Start H (1 bajt)                      │
│ 0x03E0 (992)  │ Time Start M (1 bajt)                      │
│ 0x03E1 (993)  │ Time Stop H (1 bajt)                       │
│ 0x03E2 (994)  │ Time Stop M (1 bajt)                       │
│ 0x03E3 (995)  │ Skryba tryb backup (1 bajt)                │
│ 0x03E4 (996)  │ Skryba limit L (1 bajt)                    │
│ 0x03E5 (997)  │ Skryba limit H (1 bajt)                    │
│ 0x03E6 (998)  │ Blokada systemu (1 bajt)                   │
├─────────────────────────────────────────────────────────────┤
│ 0x03E7-0x03F0 │ Mój numer telefonu (10 bajtów)             │
│ (999-1008)    │   Auto-sync czasu                          │
├─────────────────────────────────────────────────────────────┤
│ 0x03F1 (1009) │ Debug start (1 bajt)                       │
│ 0x03F2 (1010) │ Debug licznik resetów (1 bajt)             │
│ 0x03F3-0x03F9 │ Debug USER 1-7 (7 bajtów)                  │
│ (1011-1017)   │                                             │
│ 0x03FA-0x03FE │ Debug SKRYBA 1-5 (5 bajtów)                │
│ (1018-1022)   │                                             │
├─────────────────────────────────────────────────────────────┤
│ 0x03FF (1023) │ ZAPAS (1 bajt)                             │
├─────────────────────────────────────────────────────────────┤
│ 0x0400 (1024) │ KONIEC PAMIĘCI EEPROM                      │
└─────────────────────────────────────────────────────────────┘

✅ Wszystkie adresy w zakresie 0-1023 (1024 bajty)
✅ Wszystkie funkcje zachowane
✅ 1 bajt zapasu
```

---

## 🔧 Wymagane Zmiany w Kodzie dla 196 numerów

### 1. Plik `adresyeeprom.h` - Zmiana liczby numerów:

```c
// PRZED:
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA 800

// PO:
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA 196
```

### 2. Plik `adresyeeprom.h` - Relokacja adresów:

**USUŃ stare adresy (poza zakresem):**
```c
// USUŃ te linie:
#define ADRES_EEPROM_TRYB_PRACY 4094
#define ADRES_EEPROM_TRYB_CLIP_DTMF 4095
#define ADRES_EEPROM_SKRYBA 4089
#define ADRES_EEPROM_SKRYBA_TRYB_BACKUP 4088
#define ADRES_EEPROM_SKRYBA_LIMIT_H 4086
#define ADRES_EEPROM_SKRYBA_LIMIT_L 4085
#define ADRES_EEPROM_BLOKADA_SYSTEMU 4087
#define ADRES_EEPROM_CZAS_START_H 4090
#define ADRES_EEPROM_CZAS_START_M 4091
#define ADRES_EEPROM_CZAS_STOP_H 4092
#define ADRES_EEPROM_CZAS_STOP_M 4093
#define ADRES_EEPROM_MOJE_NUMER_START 4040
#define EEPROM_DEBUG_START 4050
#define EEPROM_DEBUG_LICZNIK_RESETOW 4060
#define ADRES_EEPROM_DEBUG_USER_1 4070
// ... wszystkie DEBUG_USER i DEBUG_SKRYBA
```

**DODAJ nowe adresy (w zakresie 0-1023):**
```c
// Po 196 numerach: 0x0008 + (196 * 5) = 0x0008 + 980 = 0x03DC (988)

// Funkcje główne
#define ADRES_EEPROM_TRYB_PRACY 988                 // 0x3DC
#define ADRES_EEPROM_TRYB_CLIP_DTMF 989             // 0x3DD
#define ADRES_EEPROM_SKRYBA 990                     // 0x3DE
#define ADRES_EEPROM_CZAS_START_H 991               // 0x3DF
#define ADRES_EEPROM_CZAS_START_M 992               // 0x3E0
#define ADRES_EEPROM_CZAS_STOP_H 993                // 0x3E1
#define ADRES_EEPROM_CZAS_STOP_M 994                // 0x3E2

// Funkcje SKRYBA rozszerzone
#define ADRES_EEPROM_SKRYBA_TRYB_BACKUP 995         // 0x3E3
#define ADRES_EEPROM_SKRYBA_LIMIT_L 996             // 0x3E4
#define ADRES_EEPROM_SKRYBA_LIMIT_H 997             // 0x3E5
#define ADRES_EEPROM_BLOKADA_SYSTEMU 998            // 0x3E6

// Auto-sync czasu
#define ADRES_EEPROM_MOJE_NUMER_START 999           // 0x3E7 (10 bajtów: 999-1008)

// Debug
#define EEPROM_DEBUG_START 1009                     // 0x3F1
#define EEPROM_DEBUG_LICZNIK_RESETOW 1010           // 0x3F2
#define ADRES_EEPROM_DEBUG_USER_1 1011              // 0x3F3
#define ADRES_EEPROM_DEBUG_USER_2 1012              // 0x3F4
#define ADRES_EEPROM_DEBUG_USER_3 1013              // 0x3F5
#define ADRES_EEPROM_DEBUG_USER_4 1014              // 0x3F6
#define ADRES_EEPROM_DEBUG_USER_5 1015              // 0x3F7
#define ADRES_EEPROM_DEBUG_USER_6 1016              // 0x3F8
#define ADRES_EEPROM_DEBUG_USER_7 1017              // 0x3F9
#define ADRES_EEPROM_DEBUG_SKRYBA_1 1018            // 0x3FA
#define ADRES_EEPROM_DEBUG_SKRYBA_2 1019            // 0x3FB
#define ADRES_EEPROM_DEBUG_SKRYBA_3 1020            // 0x3FC
#define ADRES_EEPROM_DEBUG_SKRYBA_4 1021            // 0x3FD
#define ADRES_EEPROM_DEBUG_SKRYBA_5 1022            // 0x3FE
```

### 3. Plik `AC800-DTM-HS.py` - Zmiana limitu GUI:

```python
# PRZED:
MAX_NUMBERS = 800

# PO:
MAX_NUMBERS = 196
```

---

## 📋 Checklist Implementacji

- [ ] Zmień `MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA` na **196** w `adresyeeprom.h`
- [ ] Przenieś WSZYSTKIE adresy konfiguracyjne (988-1022) w `adresyeeprom.h`
- [ ] Zaktualizuj `AC800-DTM-HS.py` - zmień limit na **196**
- [ ] Zaktualizuj dokumentację użytkownika
- [ ] **WYCZYŚĆ EEPROM** przed pierwszym uruchomieniem (stare dane będą nieważne!)
- [ ] Przetestuj zapis/odczyt wszystkich 196 numerów
- [ ] Przetestuj funkcje SKRYBA (limit, blokada, backup)
- [ ] Przetestuj funkcje TIME (start/stop)
- [ ] Przetestuj auto-sync czasu (mój numer)
- [ ] Przetestuj funkcje DEBUG (jeśli używane)
- [ ] Zweryfikuj działanie GUI z nowym limitem

---

## ⚠️ WAŻNE OSTRZEŻENIA

1. **Utrata danych**: Po zmianie adresów EEPROM, wszystkie stare dane będą nieważne!
2. **Backup**: Przed zmianą zrób backup numerów telefonów przez GUI (CSV)
3. **Czyszczenie**: Po wgraniu nowego firmware wyczyść EEPROM lub wpisz dane ponownie
4. **Testowanie**: Dokładnie przetestuj wszystkie funkcje po zmianie

---

*Dokument wygenerowany: 2025-12-22*
*Wersja firmware: AC800-DTM-HS-RC3*
*Mikrokontroler: ATmega328PB*
*Analiza: WSZYSTKIE funkcje zachowane przy 196 numerach*
