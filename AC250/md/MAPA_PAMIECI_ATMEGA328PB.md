# Mapa Pamięci ATmega328PB - AC800-DTM-HS-RC3

## 📊 Podsumowanie Wykorzystania Pamięci

### Specyfikacja ATmega328PB
| Typ Pamięci | Dostępne | Wykorzystane | Wolne | % Wykorzystania |
|-------------|----------|--------------|-------|-----------------|
| **Flash (Program)** | 32768 B (32 KB) | 26286 B | 6482 B | **80.2%** |
| **SRAM (RAM)** | 2048 B (2 KB) | 1824 B | 224 B | **89.1%** |
| **EEPROM** | 1024 B (1 KB) | 4096 B* | -3072 B | **400%** ⚠️ |

> **⚠️ KRYTYCZNY PROBLEM**: EEPROM wymaga 4096 bajtów (4 KB), ale ATmega328PB ma tylko 1024 bajty (1 KB)!

---

## 🔴 PROBLEM: Przekroczenie Pamięci EEPROM

### Aktualna Konfiguracja
- **Maksymalna liczba numerów**: 800
- **Bajty na numer**: 5
- **Wymagana pamięć na numery**: 800 × 5 = **4000 bajtów**
- **Całkowite zapotrzebowanie EEPROM**: **4096 bajtów**
- **Dostępna pamięć EEPROM**: **1024 bajty**
- **Niedobór**: **-3072 bajty** ❌

### 💡 Rekomendowane Rozwiązania

#### Opcja 1: Zmniejszenie liczby numerów do 200 (WYBRANE PRZEZ UŻYTKOWNIKA)
```c
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA 200  // było 800
```
- Numery telefonów: 200 × 5 = 1000 bajtów
- Konfiguracja (kod, stany, time, skryba, tryby): 15 bajtów
- **Całkowite zapotrzebowanie: 1015 bajtów** ✅ (9 bajtów zapasu)
- **UWAGA**: Wymaga usunięcia adresów DEBUG z EEPROM!

#### Opcja 2: Zmniejszenie liczby numerów do 180 (BEZPIECZNIEJSZE)
```c
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA 180  // było 800
```
- Numery telefonów: 180 × 5 = 900 bajtów
- Konfiguracja: 15 bajtów
- **Całkowite zapotrzebowanie: 915 bajtów** ✅ (109 bajtów zapasu)

#### Opcja 3: Zmniejszenie liczby numerów do 150 (BARDZO BEZPIECZNE)
```c
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA 150  // było 800
```
- Numery telefonów: 150 × 5 = 750 bajtów
- Konfiguracja: 15 bajtów
- **Całkowite zapotrzebowanie: 765 bajtów** ✅ (259 bajtów zapasu)

---

## 💾 Szczegółowa Mapa Pamięci FLASH (Program Memory)

### Wykorzystanie: 26286 / 32768 bajtów (80.2%)

| Sekcja | Adres Start | Adres Koniec | Rozmiar | Opis |
|--------|-------------|--------------|---------|------|
| `.vectors` | 0x0000 | 0x00B3 | 180 B | Tabela wektorów przerwań (45 wektorów) |
| `.progmem` | 0x00B4 | 0x0672 | 1471 B | Stałe w pamięci Flash (PROGMEM) |
| `.text` | 0x0674 | 0x65D5 | 24930 B | Kod programu |
| **RAZEM** | | | **26581 B** | |

### Rozkład Kodu według Modułów

| Moduł | Rozmiar | % Flash | Główne Funkcje |
|-------|---------|---------|----------------|
| `main.o` | 13270 B | 50.5% | Główna logika sterowania, obsługa SMS, SIM900 |
| `sim900.o` | 1040 B | 4.0% | Komunikacja z modułem GSM |
| `pdu.o` | 1188 B | 4.5% | Konwersja PDU dla SMS |
| `interpretacjaSMS.o` | 2556 B | 9.7% | Parsowanie i interpretacja komend SMS |
| `poleceniagsm.o` | 1134 B | 4.3% | Kolejkowanie poleceń GSM |
| `konfiguracja.o` | 624 B | 2.4% | Konwersja numerów telefonów |
| `zapiseeprom.o` | 450 B | 1.7% | Zapis do EEPROM |
| `komendy.o` | 176 B | 0.7% | Zarządzanie kolejką komend |
| `wewy.o` | 512 B | 1.9% | Obsługa wejść/wyjść |
| `pamiec_ram.o` | 0 B | 0.0% | Tylko deklaracje zmiennych |
| **Biblioteki AVR** | 5336 B | 20.3% | libc, libgcc, EEPROM |

### Najważniejsze Funkcje (Top 20)

| Funkcja | Moduł | Przybliżony Rozmiar | Opis |
|---------|-------|---------------------|------|
| `obsluga_komendy_SIM900` | main.o | ~800 B | Obsługa komend od modułu GSM |
| `wykonanie_polecenia_sms` | main.o | ~600 B | Wykonanie poleceń z SMS |
| `odpowiedz_na_polecenie` | main.o | ~700 B | Generowanie odpowiedzi SMS |
| `steruj_SIM900_100MS` | main.o | ~500 B | Sterowanie modułem co 100ms |
| `wykonanie_komend_SIM900` | main.o | ~2000 B | Wykonywanie kolejki komend GSM |
| `interpretuj_wiadomosc_sms` | interpretacjaSMS.o | ~400 B | Parsowanie treści SMS |
| `vfprintf` | libc.a | ~918 B | Formatowanie sprintf |
| `polecenia_konczace_gsm` | poleceniagsm.o | ~1000 B | Wykrywanie zakończenia komend AT |
| `konwertuj_pdu_na_blok_wysylany` | pdu.o | ~200 B | Konwersja SMS do formatu PDU |

---

## 🗂️ Szczegółowa Mapa Pamięci RAM (SRAM)

### Wykorzystanie: 1824 / 2048 bajtów (89.1%)

### Podział RAM

| Sekcja | Rozmiar | Opis |
|--------|---------|------|
| `.data` | ~100 B | Zmienne zainicjalizowane |
| `.bss` | ~1724 B | Zmienne niezainicjalizowane |
| **RAZEM** | **1824 B** | |
| **Wolne** | **224 B** | Zapas na stos |

### Największe Bufory w RAM

| Zmienna | Rozmiar | Moduł | Opis |
|---------|---------|-------|------|
| `wysylany_blok_SIM900` | 400 B | pamiec_ram.o | Bufor wysyłania do SIM900 |
| `odebrany_blok_SIM900` | 401 B | sim900.o | Bufor odbierania z SIM900 |
| `bufor_pdu` | 200 B | main.o | Bufor PDU dla SMS |
| `tekst_wysylanego_smsa` | 161 B | pamiec_ram.o | Tekst wysyłanego SMS |
| `bufor_eeprom` | 50 B | zapiseeprom.o | Bufor zapisu EEPROM |
| `numer_telefonu_wysylanego_smsa` | 33 B | main.o | Numer odbiorcy SMS |
| `bufor_ustaw_czas` | 32 B | komendy.o | Bufor ustawiania czasu |
| `komendy_kolejka` | 30 B | komendy.o | Kolejka komend |
| `numer_telefonu_do_ktorego_dzwonic` | 33 B | main.o | Numer do dzwonienia |
| `numer_telefonu_odebranego_smsa` | 17 B | main.o | Numer nadawcy SMS |
| `moj_numer_telefonu` | 17 B | main.o | Własny numer telefonu |
| `numer_telefonu_skryba` | 20 B | main.o | Numer funkcji SKRYBA |
| `numer_telefonu_ktory_dzwoni` | 17 B | main.o | Numer dzwoniącego |
| `nazwa_operatora` | 11 B | main.o | Nazwa operatora GSM |
| `zarejestrowane_komendy_od_SIM900` | 10 B | poleceniagsm.o | Rejestr komend |
| `polozenie_otrzymanych_komend_SIM900` | 10 B | sim900.o | Pozycje komend |

### Inne Zmienne RAM

| Zmienna | Rozmiar | Opis |
|---------|---------|------|
| `kod_modulu` | 4 B | Kod dostępu |
| `czas_trwania_impulsu*` | 4×4 B | Liczniki impulsów |
| `licznik_przelacznik_wyjscia` | 4 B | Licznik przełącznika |
| `pozycja_w_eeprom` | 2 B | Pozycja w EEPROM |
| `liczba_wysylanych_znakow_SIM900` | 2 B | Licznik znaków |
| `liczba_odebranych_znakow_SIM900` | 2 B | Licznik znaków |
| Inne zmienne 1-bajtowe | ~50 B | Flagi, liczniki, stany |

### ⚠️ Ostrzeżenie RAM
- **Tylko 224 bajty wolne** (10.9% zapasu)
- Stos może potrzebować 100-150 bajtów
- **Rzeczywisty zapas: ~74-124 bajty** - BARDZO MAŁO!

---

## 📝 Szczegółowa Mapa Pamięci EEPROM

### ⚠️ WYMAGANE: 4096 bajtów | DOSTĘPNE: 1024 bajty

| Obszar | Adres | Rozmiar | Aktualne | Po Zmianie na 200 | Opis |
|--------|-------|---------|----------|-------------------|------|
| **Checksum** | 0x0000 | 1 B | ✓ | ✓ | Suma kontrolna |
| **Kod dostępu** | 0x0001-0x0004 | 4 B | ✓ | ✓ | 4-cyfrowy kod ASCII |
| **Ustawienie stanów** | 0x0005 | 1 B | ✓ | ✓ | Stany wyjść |
| **Ustawienie wyjścia** | 0x0006-0x0007 | 2 B | ✓ | ✓ | Konfiguracja wyjścia |
| **Numery telefonów** | 0x0008-0x0FA7 | 4000 B | ❌ | 1000 B ✓ | 800→200 numerów × 5 B |
| **Konfiguracja** | 0x03F0-0x03F8 | 9 B | ❌ | ✓ | Skryba, Time, Tryby (przeniesione) |
| **WOLNE** | 0x03F9-0x03FF | 7 B | - | ✓ | Zapas |
| ~~**Skryba**~~ | ~~0x0FF9 (4089)~~ | ~~1 B~~ | ❌ | ❌ | Poza zakresem - przeniesione! |
| ~~**Time Start H**~~ | ~~0x0FFA (4090)~~ | ~~1 B~~ | ❌ | ❌ | Poza zakresem - przeniesione! |
| ~~**Time Start M**~~ | ~~0x0FFB (4091)~~ | ~~1 B~~ | ❌ | ❌ | Poza zakresem - przeniesione! |
| ~~**Time Stop H**~~ | ~~0x0FFC (4092)~~ | ~~1 B~~ | ❌ | ❌ | Poza zakresem - przeniesione! |
| ~~**Time Stop M**~~ | ~~0x0FFD (4093)~~ | ~~1 B~~ | ❌ | ❌ | Poza zakresem - przeniesione! |
| ~~**Tryb pracy**~~ | ~~0x0FFE (4094)~~ | ~~1 B~~ | ❌ | ❌ | Poza zakresem - przeniesione! |
| ~~**Tryb CLIP/DTMF**~~ | ~~0x0FFF (4095)~~ | ~~1 B~~ | ❌ | ❌ | Poza zakresem - przeniesione! |

### Nowa Mapa EEPROM (po zmianie na 200 numerów)

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
│      ...      │   ... (197 numerów więcej) ...             │
│ 0x03E3-0x03E7 │ Numer 200 (5 bajtów)                       │
│ (995-999)     │                                             │
├─────────────────────────────────────────────────────────────┤
│ 0x03E8 (1000) │ Tryb pracy (1 bajt)                        │
│ 0x03E9 (1001) │ Tryb CLIP/DTMF (1 bajt)                    │
│ 0x03EA (1002) │ Skryba (1 bajt)                            │
│ 0x03EB (1003) │ Time Start H (1 bajt)                      │
│ 0x03EC (1004) │ Time Start M (1 bajt)                      │
│ 0x03ED (1005) │ Time Stop H (1 bajt)                       │
│ 0x03EE (1006) │ Time Stop M (1 bajt)                       │
├─────────────────────────────────────────────────────────────┤
│ 0x03EF-0x03FF │ WOLNE (17 bajtów) - ZAPAS                  │
│ (1007-1023)   │                                             │
├─────────────────────────────────────────────────────────────┤
│ 0x0400 (1024) │ KONIEC PAMIĘCI EEPROM                      │
└─────────────────────────────────────────────────────────────┘

✅ Wszystkie adresy w zakresie 0-1023 (1024 bajty)
✅ 17 bajtów zapasu na przyszłe rozszerzenia
```

### 🔧 Wymagane Zmiany w Kodzie

#### 1. Zmiana liczby numerów w `adresyeeprom.h`:
```c
// PRZED:
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA 800

// PO:
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA 180
```

#### 2. Relokacja adresów konfiguracyjnych:

**STARE adresy (poza zakresem 1024 B):**
```c
#define ADRES_EEPROM_SKRYBA 4089                    // ❌ Poza zakresem!
#define ADRES_EEPROM_CZAS_START_H 4090              // ❌ Poza zakresem!
#define ADRES_EEPROM_CZAS_START_M 4091              // ❌ Poza zakresem!
#define ADRES_EEPROM_CZAS_STOP_H 4092               // ❌ Poza zakresem!
#define ADRES_EEPROM_CZAS_STOP_M 4093               // ❌ Poza zakresem!
#define ADRES_EEPROM_TRYB_PRACY 4094                // ❌ Poza zakresem!
#define ADRES_EEPROM_TRYB_CLIP_DTMF 4095            // ❌ Poza zakresem!
```

**NOWE adresy (w zakresie 1024 B) - dla 200 numerów:**
```c
// Po 200 numerach: 0x0008 + (200 * 5) = 0x0008 + 1000 = 0x03E8 (1000)
// Dostępne: 1000-1023 = 24 bajty

#define ADRES_EEPROM_TRYB_PRACY 1000                // 0x3E8
#define ADRES_EEPROM_TRYB_CLIP_DTMF 1001            // 0x3E9
#define ADRES_EEPROM_SKRYBA 1002                    // 0x3EA
#define ADRES_EEPROM_CZAS_START_H 1003              // 0x3EB
#define ADRES_EEPROM_CZAS_START_M 1004              // 0x3EC
#define ADRES_EEPROM_CZAS_STOP_H 1005               // 0x3ED
#define ADRES_EEPROM_CZAS_STOP_M 1006               // 0x3EE

// Opcjonalne (jeśli potrzebne):
#define ADRES_EEPROM_SKRYBA_TRYB_BACKUP 1007        // 0x3EF
#define ADRES_EEPROM_SKRYBA_LIMIT_L 1008            // 0x3F0
#define ADRES_EEPROM_SKRYBA_LIMIT_H 1009            // 0x3F1
#define ADRES_EEPROM_BLOKADA_SYSTEMU 1010           // 0x3F2
#define ADRES_EEPROM_MOJE_NUMER_START 1011          // 0x3F3 (max 10 bajtów → 1011-1020)
// 1021-1023 (0x3FD-0x3FF) - zapas (3 bajty)

// ⚠️ USUŃ wszystkie adresy DEBUG - nie ma miejsca!
```

---

## 🎯 Rekomendacje i Plan Działania

### Priorytet 1: KRYTYCZNY - Naprawa EEPROM
1. ✅ **Zmniejsz liczbę numerów do 180** w `adresyeeprom.h`
2. ✅ **Przenieś adresy konfiguracyjne** do zakresu 1016-1022
3. ✅ **Zaktualizuj GUI Python** - zmień limit z 800 na 180
4. ⚠️ **UWAGA**: Istniejące dane EEPROM zostaną utracone!

### Priorytet 2: WYSOKI - Optymalizacja RAM
- Aktualnie: 224 B wolne (10.9%)
- Możliwe optymalizacje:
  - Zmniejsz `MAX_LICZBA_WYSYLANYCH_ZNAKOW_SIM900` z 400 do 300 B (oszczędność: 100 B)
  - Zmniejsz `bufor_pdu` z 200 do 160 B (oszczędność: 40 B)
  - **Potencjalna oszczędność: ~140 B → 364 B wolne (17.8%)**

### Priorytet 3: ŚREDNI - Monitorowanie Flash
- Aktualnie: 6482 B wolne (19.8%)
- Wystarczające dla dalszego rozwoju
- Zalecane: nie przekraczać 90% wykorzystania

---

## 📈 Porównanie Wariantów

| Wariant | Liczba Numerów | EEPROM Numery | EEPROM Config | EEPROM Razem | Status |
|---------|----------------|---------------|---------------|--------------|--------|
| **Aktualny** | 800 | 4000 B | 96 B | 4096 B | ❌ Nie działa (poza zakresem) |
| **Opcja 1** | 200 | 1000 B | 24 B | 1024 B | ⚠️ Dokładnie limit (0 B zapasu) |
| **Opcja 2** | 180 | 900 B | 24 B | 924 B | ✅ OK (100 B zapasu) |
| **Opcja 3** | 150 | 750 B | 24 B | 774 B | ✅ Bezpieczne (250 B zapasu) |
| **Opcja 4** | 100 | 500 B | 24 B | 524 B | ✅ Bardzo bezpieczne (500 B zapasu) |

### Zalecenie Końcowe

**Wybrano: 200 numerów (na życzenie użytkownika)**
- ⚠️ Wykorzystuje dokładnie 1024 bajty - brak zapasu!
- ⚠️ Wymaga usunięcia wszystkich adresów DEBUG z EEPROM
- ⚠️ Brak miejsca na przyszłe rozszerzenia
- ✅ Maksymalna możliwa liczba numerów dla ATmega328PB
- ⚠️ Wymaga aktualizacji GUI i dokumentacji
- ⚠️ Zalecane: rozważ 180 numerów dla bezpieczeństwa (100 B zapasu)

---

## 📋 Checklist Implementacji

- [ ] Zmień `MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA` na 200 w `adresyeeprom.h`
- [ ] Przenieś adresy konfiguracyjne (4089-4095 → 1000-1006)
- [ ] **USUŃ wszystkie adresy DEBUG** z `adresyeeprom.h` (4050-4084)
- [ ] Zaktualizuj `AC800-DTM-HS.py` - zmień limit na 200
- [ ] Zaktualizuj dokumentację użytkownika
- [ ] Wyczyść EEPROM przed pierwszym uruchomieniem
- [ ] Przetestuj zapis/odczyt wszystkich 200 numerów
- [ ] Przetestuj funkcje SKRYBA, TIME, tryby pracy
- [ ] Zweryfikuj działanie GUI z nowym limitem
- [ ] **UWAGA**: Brak zapasu w EEPROM - każda zmiana wymaga przebudowy!

---

*Dokument wygenerowany: 2025-12-22*
*Wersja firmware: AC800-DTM-HS-RC3*
*Mikrokontroler: ATmega328PB*
