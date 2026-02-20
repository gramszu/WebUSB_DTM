# 📘 AC200-DTM-F2 - Kompletna Dokumentacja Techniczna

**Wersja:** 1.0  
**Data:** 2025-12-22  
**Mikrokontroler:** ATmega328PB  
**EEPROM:** 1024 B (100%)

---

## 📊 1. MAPA PAMIĘCI EEPROM (1024 B)

### Układ Ogólny

```
┌──────────────────────────────────────────────────────────┐
│ ADRES      │ ROZMIAR │ NAZWA                             │
├──────────────────────────────────────────────────────────┤
│ 0x0000     │ 1 B     │ Checksum                          │
│ 0x0001-0x0004 │ 4 B  │ Kod dostępu (ASCII)               │
│ 0x0005     │ 1 B     │ Stany wyjść (bit mask)            │
│ 0x0006-0x0007 │ 2 B  │ Ustawienie wyjścia (16-bit)       │
├──────────────────────────────────────────────────────────┤
│ 0x0008-0x03EF │ 1000 B │ NUMERY TELEFONÓW (200 × 5 B)    │
│            │         │ - Pozycje 1-194: Zwykli użytkow. │
│            │         │ - Pozycje 195-200: Super User    │
├──────────────────────────────────────────────────────────┤
│ 0x03F0-0x03FF │ 16 B  │ KONFIGURACJA SYSTEMU             │
└──────────────────────────────────────────────────────────┘
```

### Szczegółowa Tabela Adresów

| Adres Dec | Adres Hex | Nazwa | Rozmiar | Wartości | Opis |
|-----------|-----------|-------|---------|----------|------|
| **NAGŁÓWEK (0-7)** |
| 0 | 0x0000 | Checksum | 1 B | 0x00-0xFF | Suma kontrolna |
| 1-4 | 0x0001-0x0004 | Kod dostępu | 4 B | ASCII | Domyślnie: "ABCD" |
| 5 | 0x0005 | Stany wyjść | 1 B | Bit mask | Stan przekaźników |
| 6-7 | 0x0006-0x0007 | Ustawienie wyjścia | 2 B | 16-bit | Konfiguracja wyjść |
| **NUMERY TELEFONÓW (8-1007)** |
| 8-12 | 0x0008-0x000C | Numer 1 | 5 B | BCD | Pozycja 1 |
| 13-17 | 0x000D-0x0011 | Numer 2 | 5 B | BCD | Pozycja 2 |
| ... | ... | ... | ... | ... | ... |
| 978-982 | 0x3D2-0x3D6 | Numer 195 | 5 B | BCD | **Super User 1** |
| 983-987 | 0x3D7-0x3DB | Numer 196 | 5 B | BCD | **Super User 2** |
| 988-992 | 0x3DC-0x3E0 | Numer 197 | 5 B | BCD | **Super User 3** |
| 993-997 | 0x3E1-0x3E5 | Numer 198 | 5 B | BCD | **Super User 4** |
| 998-1002 | 0x3E6-0x3EA | Numer 199 | 5 B | BCD | **Super User 5** |
| 1003-1007 | 0x3EB-0x3EF | Numer 200 | 5 B | BCD | **Super User 6** |
| **KONFIGURACJA (1008-1023)** |
| 1008 | 0x3F0 | Tryb pracy | 1 B | 0/1 | 0=Private, 1=Public |
| 1009 | 0x3F1 | Tryb CLIP/DTMF | 1 B | 0/1 | 0=DTMF, 1=CLIP |
| 1010 | 0x3F2 | SKRYBA włączona | 1 B | 0/1 | Funkcja auto-zapisu |
| 1011 | 0x3F3 | SKRYBA backup | 1 B | 0/1 | Poprzedni tryb |
| 1012 | 0x3F4 | SKRYBA limit L | 1 B | 0-255 | Low byte (1-200) |
| 1013 | 0x3F5 | SKRYBA limit H | 1 B | 0-255 | High byte |
| 1014 | 0x3F6 | Blokada systemu | 1 B | 0/1 | 0=Aktywny, 1=Zablokowany |
| 1015 | 0x3F7 | TIME start H | 1 B | 0-23/0xFF | Godzina startu |
| 1016 | 0x3F8 | TIME start M | 1 B | 0-59/0xFF | Minuta startu |
| 1017 | 0x3F9 | TIME stop H | 1 B | 0-23/0xFF | Godzina stopu |
| 1018 | 0x3FA | TIME stop M | 1 B | 0-59/0xFF | Minuta stopu |
| 1019-1023 | 0x3FB-0x3FF | Mój numer | 5 B | BCD | Numer karty SIM |

### Format BCD (Binary Coded Decimal)

Każdy bajt przechowuje 2 cyfry:
```
Bajt: 0xAB
  A = cyfra dziesiątek (0-9)
  B = cyfra jednostek (0-9)

Przykład: Numer 123456789
  Bajt 0: 0x01 (0,1)
  Bajt 1: 0x23 (2,3)
  Bajt 2: 0x45 (4,5)
  Bajt 3: 0x67 (6,7)
  Bajt 4: 0x89 (8,9)
```

---

## 📱 2. KOMENDY SMS

### Format Ogólny
```
ABCD <KOMENDA> [parametry]
```
- `ABCD` = Kod dostępu (domyślnie, można zmienić)
- Komendy nie rozróżniają wielkości liter
- Parametry oddzielone spacjami

---

### 2.1 Zarządzanie Kodem Dostępu

#### `CODE <nowy_kod>`
**Opis:** Zmienia 4-cyfrowy kod dostępu  
**Parametry:**
- `nowy_kod`: 4 znaki (A-Z, 0-9)

**Przykłady:**
```
ABCD CODE 1234    → Zmienia kod na "1234"
ABCD CODE EFGH    → Zmienia kod na "EFGH"
```

**Odpowiedź:** Potwierdzenie zmiany

---

### 2.2 Zarządzanie Numerami Telefonów

#### `ADD <numer>`
**Opis:** Dodaje numer do listy (pozycje 1-194)  
**Parametry:**
- `numer`: Numer telefonu (3-9 cyfr, ostatnie 9 cyfr)

**Przykłady:**
```
ABCD ADD 123456789
ABCD ADD +48505691117    → Zapisze: 505691117
```

**Odpowiedź:** Potwierdzenie dodania

---

#### `DEL <numer>`
**Opis:** Usuwa numer z listy  
**Parametry:**
- `numer`: Numer telefonu do usunięcia

**Przykłady:**
```
ABCD DEL 123456789
ABCD DEL +48505691117
```

**Odpowiedź:** Potwierdzenie usunięcia

---

#### `SUB <numer>`
**Opis:** Dodaje numer jako **Super User** (pozycje 195-200)  
**Funkcje Super User:**
- Omija blokadę systemu (START/STOP)
- Omija blokadę czasową (TIME)
- Zawsze ma dostęp

**Parametry:**
- `numer`: Numer telefonu Super Usera

**Przykłady:**
```
ABCD SUB 123456789
ABCD SUB +48505691117
```

**Odpowiedź:** "Super User dodany na pozycji 195-200" lub "Brak wolnych pozycji"

---

### 2.3 Tryby Pracy

#### `OPEN [CLIP|DTMF]`
**Opis:** Włącza tryb **publiczny** (każdy może otworzyć bramę)  
**Parametry:**
- `CLIP`: Sterowanie przez połączenie (domyślne)
- `DTMF`: Sterowanie przez tony DTMF

**Przykłady:**
```
ABCD OPEN           → Publiczny (zachowuje obecny podtryb)
ABCD OPEN CLIP      → Publiczny + CLIP
ABCD OPEN DTMF      → Publiczny + DTMF
```

**Odpowiedź:** Potwierdzenie zmiany

---

#### `CLOSE [CLIP|DTMF]`
**Opis:** Włącza tryb **prywatny** (tylko numery z listy)  
**Parametry:**
- `CLIP`: Sterowanie przez połączenie
- `DTMF`: Sterowanie przez tony DTMF

**Przykłady:**
```
ABCD CLOSE          → Prywatny (wyłącza SKRYBA)
ABCD CLOSE CLIP     → Prywatny + CLIP
ABCD CLOSE DTMF     → Prywatny + DTMF
```

**Odpowiedź:** Potwierdzenie zmiany

**Uwaga:** `CLOSE` automatycznie wyłącza funkcję SKRYBA

---

### 2.4 Funkcja SKRYBA (Auto-zapis)

#### `SKRYBA ON [limit]`
**Opis:** Włącza automatyczne dodawanie nieznanych numerów  
**Parametry:**
- `limit`: Opcjonalny limit użytkowników (1-200, domyślnie 200)

**Przykłady:**
```
ABCD SKRYBA ON        → Włącza SKRYBA (limit 200)
ABCD SKRYBA ON 50     → Włącza SKRYBA (limit 50)
```

**Działanie:**
1. Automatycznie ustawia tryb **OPEN CLIP**
2. Zapisuje obecny tryb do przywrócenia
3. Dodaje nieznane numery do pozycji 1-194 (omija Super User)

**Odpowiedź:** Potwierdzenie włączenia

---

#### `SKRYBA OFF`
**Opis:** Wyłącza funkcję SKRYBA  

**Przykład:**
```
ABCD SKRYBA OFF
```

**Działanie:**
1. Wyłącza auto-zapis
2. Przywraca poprzedni tryb pracy

**Odpowiedź:** Potwierdzenie wyłączenia

---

### 2.5 Harmonogram Czasowy (TIME)

#### `TIME <HH:MM> <HH:MM>`
**Opis:** Ustawia harmonogram dostępu (start-stop)  
**Parametry:**
- Pierwszy `HH:MM`: Godzina rozpoczęcia
- Drugi `HH:MM`: Godzina zakończenia

**Przykłady:**
```
ABCD TIME 08:00 18:00    → Dostęp 8:00-18:00
ABCD TIME 08:00#18:00    → Separator # opcjonalny
ABCD TIME 22:00 06:00    → Dostęp 22:00-6:00 (przez noc)
```

**Działanie:**
- Poza harmonogramem: Tylko Super User ma dostęp
- W harmonogramie: Normalne działanie

**Odpowiedź:** Potwierdzenie ustawienia

---

#### `TIME OFF`
**Opis:** Wyłącza harmonogram czasowy

**Przykład:**
```
ABCD TIME OFF
```

**Odpowiedź:** Potwierdzenie wyłączenia

---

### 2.6 Zarządzanie Czasem

#### `SET <HH:MM:SS>`
**Opis:** Ustawia czas RTC modułu GSM  
**Parametry:**
- `HH`: Godzina (0-23)
- `MM`: Minuta (0-59)
- `SS`: Sekunda (0-59)

**Przykłady:**
```
ABCD SET 15:30:00    → Ustawia czas na 15:30:00
```

**Odpowiedź:** Potwierdzenie ustawienia

**Uwaga:** Wyłącza auto-sync czasu z SMS

---

#### `SET` (bez parametrów)
**Opis:** Zwraca aktualny czas RTC

**Przykład:**
```
ABCD SET
```

**Odpowiedź:** "Time: 15:30:45"

---

### 2.7 Blokada Systemu

#### `START`
**Opis:** Odblokowuje system (normalny tryb pracy)

**Przykład:**
```
ABCD START
```

**Działanie:**
- Usuwa blokadę systemu
- Wszystkie funkcje działają normalnie

**Odpowiedź:** Potwierdzenie odblokowania

---

#### `STOP`
**Opis:** Blokuje system (tylko REPORT i Super User)

**Przykład:**
```
ABCD STOP
```

**Działanie:**
- Blokuje wszystkie komendy oprócz REPORT i START
- Super User (195-200) omija blokadę

**Odpowiedź:** Potwierdzenie zablokowania

---

### 2.8 Raporty i Diagnostyka

#### `REPORT`
**Opis:** Zwraca raport statusu urządzenia

**Przykład:**
```
ABCD REPORT
```

**Odpowiedź:**
```
AC200-DTM-F2
Tryb: Public/Private
CLIP/DTMF: CLIP
SKRYBA: ON (limit 50)
TIME: 08:00-18:00
Status: Aktywny
Czas: 15:30:45
www.sonfy.pl
```

---

#### `USER <numer>`
**Opis:** Sprawdza czy numer jest na liście  
**Parametry:**
- `numer`: Numer telefonu do sprawdzenia

**Przykłady:**
```
ABCD USER 123456789
ABCD USER +48505691117
```

**Odpowiedź:**
- Jeśli znaleziony: Pozycja na liście
- Jeśli nie znaleziony: "Nie znaleziono"

---

#### `USER` (bez parametrów)
**Opis:** Wyświetla instrukcję użycia

**Przykład:**
```
ABCD USER
```

**Odpowiedź:** Instrukcja komendy USER

---

### 2.9 Konfiguracja Auto-Sync

#### `MYNUM <numer>`
**Opis:** Ustawia własny numer telefonu (dla auto-sync czasu)  
**Parametry:**
- `numer`: Numer karty SIM (3-10 cyfr)

**Przykłady:**
```
ABCD MYNUM 123456789
ABCD MYNUM +48505691117    → Zapisze: 0505691117
```

**Działanie:**
- Zapisuje numer w formacie BCD (5 bajtów)
- Używany do auto-sync czasu po restarcie

**Odpowiedź:** "Numer zapisany: 123456789"

---

### 2.10 Reset

#### `XXXX`
**Opis:** Resetuje wszystkie ustawienia do domyślnych

**Przykład:**
```
ABCD XXXX
```

**Działanie:**
- Usuwa wszystkie numery
- Przywraca domyślny kod (ABCD)
- Resetuje konfigurację

**Odpowiedź:** Potwierdzenie resetu

**⚠️ UWAGA:** Nieodwracalne!

---

## 🔧 3. FUNKCJE SYSTEMU

### 3.1 Super User (Pozycje 195-200)

**Uprawnienia:**
- ✅ Omija blokadę systemu (STOP)
- ✅ Omija harmonogram czasowy (TIME)
- ✅ Zawsze może otworzyć bramę
- ✅ Zawsze może wysyłać komendy SMS

**Dodawanie:**
```
ABCD SUB 123456789
```

**Adresy EEPROM:**
- Pozycja 195: 978-982 (0x3D2-0x3D6)
- Pozycja 196: 983-987 (0x3D7-0x3DB)
- Pozycja 197: 988-992 (0x3DC-0x3E0)
- Pozycja 198: 993-997 (0x3E1-0x3E5)
- Pozycja 199: 998-1002 (0x3E6-0x3EA)
- Pozycja 200: 1003-1007 (0x3EB-0x3EF)

---

### 3.2 Funkcja SKRYBA

**Opis:** Automatyczne dodawanie nieznanych numerów

**Działanie:**
1. Włączenie: `ABCD SKRYBA ON [limit]`
2. Automatycznie ustawia **OPEN CLIP**
3. Przy każdym połączeniu:
   - Sprawdza czy numer jest na liście
   - Jeśli nie: Dodaje do pierwszej wolnej pozycji (1-194)
   - Omija pozycje Super User (195-200)
4. Limit: Maksymalna liczba użytkowników (1-200)

**Wyłączenie:**
- `ABCD SKRYBA OFF` - przywraca poprzedni tryb
- `ABCD CLOSE` - automatycznie wyłącza SKRYBA

---

### 3.3 Harmonogram Czasowy (TIME)

**Opis:** Ogranicza dostęp do określonych godzin

**Ustawienie:**
```
ABCD TIME 08:00 18:00    → Dostęp tylko 8:00-18:00
```

**Działanie:**
- **W harmonogramie:** Normalne działanie (Public/Private)
- **Poza harmonogramem:** Tylko Super User ma dostęp

**Wyłączenie:**
```
ABCD TIME OFF
```

**Wartości EEPROM:**
- Włączony: HH (0-23), MM (0-59)
- Wyłączony: 0xFF, 0xFF

---

### 3.4 Auto-Sync Czasu

**Opis:** Automatyczna synchronizacja czasu po restarcie

**Konfiguracja:**
1. Ustaw własny numer: `ABCD MYNUM 123456789`
2. Po restarcie (czas = 00:00:xx):
   - System wykrywa nieprawidłowy czas
   - Tymczasowo włącza tryb publiczny
   - Czeka na pierwszy SMS
   - Synchronizuje czas z timestampu SMS
   - Przywraca poprzedni tryb

**Format zapisu:**
- EEPROM: 5 bajtów BCD (1019-1023)
- Maksymalnie 10 cyfr

---

## 📋 4. TABELA TRYBÓW PRACY

| Tryb | CLIP/DTMF | Opis | Komendy |
|------|-----------|------|---------|
| **Private CLIP** | CLIP | Tylko numery z listy (połączenie) | `ABCD CLOSE CLIP` |
| **Private DTMF** | DTMF | Tylko numery z listy (tony) | `ABCD CLOSE DTMF` |
| **Public CLIP** | CLIP | Każdy może otworzyć (połączenie) | `ABCD OPEN CLIP` |
| **Public DTMF** | DTMF | Każdy może otworzyć (tony) | `ABCD OPEN DTMF` |

**Uwagi:**
- CLIP: Sterowanie przez połączenie (rozłącz = otwórz)
- DTMF: Sterowanie przez tony (naciśnij cyfry)
- Private + SKRYBA = Automatycznie zmienia na Public

---

## 🔍 5. PRZYKŁADOWE SCENARIUSZE

### Scenariusz 1: Podstawowa Konfiguracja
```
1. ABCD CODE 1234           → Zmień kod
2. 1234 ADD 111222333       → Dodaj użytkownika
3. 1234 ADD 444555666       → Dodaj użytkownika
4. 1234 SUB 999888777       → Dodaj Super User
5. 1234 CLOSE CLIP          → Tryb prywatny
```

### Scenariusz 2: Harmonogram Pracy (8:00-18:00)
```
1. ABCD TIME 08:00 18:00    → Ustaw harmonogram
2. ABCD SUB 999888777       → Dodaj Super User (dostęp 24/7)
3. ABCD CLOSE CLIP          → Tryb prywatny
```

### Scenariusz 3: Auto-Zapis Gości (SKRYBA)
```
1. ABCD SKRYBA ON 50        → Włącz SKRYBA (max 50 osób)
2. (Goście dzwonią)         → Automatycznie dodawani
3. ABCD SKRYBA OFF          → Wyłącz po imprezie
```

### Scenariusz 4: Blokada Wakacyjna
```
1. ABCD STOP                → Zablokuj system
2. (Tylko Super User działa)
3. ABCD START               → Odblokuj po powrocie
```

---

## ⚙️ 6. PARAMETRY TECHNICZNE

| Parametr | Wartość |
|----------|---------|
| **Mikrokontroler** | ATmega328PB |
| **Flash** | 25044 B / 32768 B (76.4%) |
| **RAM** | 1823 B / 2048 B (89.0%) |
| **EEPROM** | 1024 B / 1024 B (100%) |
| **Liczba numerów** | 200 (194 zwykłych + 6 Super User) |
| **Format numerów** | BCD (5 bajtów = 10 cyfr) |
| **Kod dostępu** | 4 znaki (A-Z, 0-9) |
| **Częstotliwość** | 7.3728 MHz |
| **Moduł GSM** | SIM900 (USART0) |

---

## 📞 7. WSPARCIE

**Producent:** Sonfy  
**Strona:** www.sonfy.pl  
**Model:** AC200-DTM-F2  
**Wersja firmware:** 1.0

---

*Dokumentacja wygenerowana: 2025-12-22*  
*Autor: AI Assistant (Antigravity)*
