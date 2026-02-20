# Przewodnik: Jak Przeprowadzać Testy Regresji

## Kiedy Wykonywać Testy Regresji?

**Smoke Test** - wykonuj po każdej zmianie w kodzie (szybki, ~5 minut)
**Full Regression** - wykonuj przed wydaniem nowej wersji (pełny, ~30-45 minut)

---

## SMOKE TEST (Szybki Test Regresji)

### Cel
Szybkie sprawdzenie czy podstawowe funkcje działają po zmianie kodu.

### Przygotowanie (1 min)
1. Podłącz urządzenie do zasilania
2. Włóż kartę SIM z aktywnym numerem
3. Przygotuj telefon testowy
4. Uruchom GUI `AC200-DTM-F2.py` na komputerze

### Krok 1: Test Startu (1 min)
```
✓ LED SYS miga po włączeniu
✓ Po ~30s LED gaśnie (zalogowany w sieci)
✓ Brak ciągłych resetów
```

### Krok 2: Test Komendy SMS (1 min)
```
Wyślij SMS: ABCD REPORT
✓ Otrzymujesz odpowiedź z listą numerów (może być pusta: *#)
```

### Krok 3: Test Bramy (2 min)
```
1. Wyślij SMS: ABCD ADD 123456789
   ✓ Otrzymujesz potwierdzenie

2. Zadzwoń z numeru 123456789
   ✓ Brama otwiera się (przekaźnik klika)
   
3. Wyślij SMS: ABCD DEL 123456789
   ✓ Numer usunięty
```

### Krok 4: Test GUI (1 min)
```
1. Wybierz port COM w GUI
2. Kliknij "Odczytaj dane ze sterownika"
   ✓ Lista numerów się wyświetla
   ✓ Kod dostępu poprawny
   ✓ Brak błędów
```

### Wynik Smoke Test
- **PASS** - wszystkie 4 kroki ✓ → Możesz kontynuować pracę
- **FAIL** - którykolwiek krok ✗ → Napraw błąd przed dalszą pracą

---

## FULL REGRESSION (Pełny Test Regresji)

### Cel
Kompleksowe sprawdzenie wszystkich funkcji przed wydaniem.

### Przygotowanie (5 min)
1. Świeże wgranie firmware (upload.sh)
2. Wyczyść EEPROM lub wgraj domyślny: `default_eeprom_AC200.hex`
3. Przygotuj 2 telefony testowe (jeden zapisany, jeden nie)
4. Otwórz `PLAN_TESTOW.md` do zaznaczania postępów

---

### CZĘŚĆ 1: Testy Podstawowe (15 min)

#### 1.1 Kod Dostępu (2 min)
```bash
# Test 1: Poprawny kod z komendą
SMS: ABCD REPORT
Oczekiwane: Odpowiedź z listą (może być pusta: *#)
Status: [ ]

# Test 2: Zły kod
SMS: WXYZ REPORT
Oczekiwane: Brak odpowiedzi
Status: [ ]

# Test 3: Zmiana kodu
SMS: ABCD CODE EFGH
Oczekiwane: Kod zmieniony (potwierdzenie SMS)
Status: [ ]

# Test 4: Nowy kod działa
SMS: EFGH REPORT
Oczekiwane: Odpowiedź z listą
Status: [ ]

# Test 5: Przywróć kod
SMS: EFGH CODE ABCD
Status: [ ]
```

#### 1.2 Dodawanie/Usuwanie (3 min)
```bash
# Test 1: Dodaj numer
SMS: ABCD ADD 123456789
Status: [ ]

# Test 2: Połączenie działa
Zadzwoń z 123456789
Oczekiwane: Brama otwiera się
Status: [ ]

# Test 3: Usuń numer
SMS: ABCD DEL 123456789
Status: [ ]

# Test 4: Połączenie nie działa
Zadzwoń z 123456789
Oczekiwane: Brama NIE otwiera się
Status: [ ]
```

#### 1.3 Tryb CLIP (5 min)
```bash
# Przygotowanie
SMS: ABCD ADD 123456789
SMS: ABCD CLOSE CLIP

# Test 1: Zapisany numer
Zadzwoń z 123456789
Oczekiwane: Brama otwiera się automatycznie
Status: [ ]

# Test 2: Niezapisany numer
Zadzwoń z innego numeru
Oczekiwane: Brama NIE otwiera się
Status: [ ]
```

#### 1.4 Tryb DTMF (5 min)
```bash
# Przygotowanie
SMS: ABCD CLOSE DTMF

# Test 1: Zapisany numer + DTMF
Zadzwoń z 123456789
Oczekiwane: Modem odbiera
Naciśnij: 1
Oczekiwane: Brama otwiera się
Status: [ ]

# Test 2: Niezapisany numer
Zadzwoń z innego numeru
Oczekiwane: Modem NIE odbiera
Status: [ ]
```

---

### CZĘŚĆ 2: Testy "Mój Numer" (10 min)

#### 2.1 Ustawianie Numeru (3 min)
```bash
# Test 1: Poprawny numer (9 cyfr)
SMS: ABCD MYNUM 123456789
Oczekiwane: "Numer zapisany: 123456789"
Status: [ ]

# Test 2: Za mało cyfr
SMS: ABCD MYNUM 12345678
Oczekiwane: Błąd
Status: [ ]

# Test 3: Za dużo cyfr
SMS: ABCD MYNUM 1234567890
Oczekiwane: Błąd
Status: [ ]

# Test 4: Litery
SMS: ABCD MYNUM 12345ABCD
Oczekiwane: Błąd
Status: [ ]
```

#### 2.2 Wyświetlanie w DEBUG (2 min)
```bash
SMS: ABCD DEBUG
Oczekiwane: Odpowiedź zawiera "Moj nr: 123456789"
NIE powinno być śmieci (??UsW?1234)
Status: [ ]
```

#### 2.3 Auto-Sync (5 min)
```bash
# Przygotowanie
SMS: ABCD MYNUM 123456789

# Test: Reset i auto-sync
1. Odłącz zasilanie
2. Podłącz zasilanie
3. Czekaj ~30s (zalogowanie do sieci)
4. Sprawdź SMS

Oczekiwane: 
- Jeśli RTC = 00:00:xx → SMS "Synchronizacja Czasu"
- Jeśli RTC ≠ 00:00:xx → Brak SMS
Status: [ ]
```

---

### CZĘŚĆ 3: Testy GUI (10 min)

#### 3.1 Odczyt z Urządzenia (3 min)
```bash
1. Uruchom AC200-DTM-F2.py
2. Wybierz port COM
3. Kliknij "Odczytaj dane ze sterownika"

Sprawdź:
✓ Lista numerów wyświetla się [ ]
✓ Kod dostępu poprawny [ ]
✓ Status/Tryb poprawne [ ]
✓ "Mój numer" = 123456789 [ ]
```

#### 3.2 Zapis do Urządzenia (4 min)
```bash
1. Dodaj numer 987654321 przez GUI
2. Zmień kod na WXYZ
3. Ustaw "Mój numer" na 555666777
4. Kliknij "Wgraj dane do sterownika"
5. Kliknij "Odczytaj dane ze sterownika"

Sprawdź:
✓ Numer 987654321 zapisany [ ]
✓ Kod WXYZ działa [ ]
✓ "Mój numer" = 555666777 [ ]
```

#### 3.3 EEPROM Viewer (3 min)
```bash
1. W GUI sprawdź adresy 1019-1023 (0x3FB-0x3FF)

Dla numeru 555666777 (z zerem: 0555666777):
Oczekiwane BCD (odwrócone): 77 66 55 05 00
Status: [ ]
```

---

### CZĘŚĆ 4: Smoke Test Końcowy (5 min)

Powtórz Smoke Test z początku dokumentu aby upewnić się że wszystko nadal działa.

---

## Raportowanie Wyników

### Jeśli wszystkie testy PASS ✅
```
✅ REGRESSION PASS
Data: [data]
Commit: [git commit hash]
Firmware: 25,064 bytes
Tester: [imię]
```

### Jeśli jakiś test FAIL ❌
```
❌ REGRESSION FAIL
Data: [data]
Commit: [git commit hash]
Błąd: [opis błędu]
Test: [który test nie przeszedł]
Kroki reprodukcji: [dokładne kroki]
```

Załącz:
- Logi z `bramster.log`
- Screenshot GUI (jeśli dotyczy)
- Zrzut EEPROM (jeśli dotyczy)

---

## Skrócona Wersja (Quick Reference)

### Smoke Test (5 min)
1. ✓ Urządzenie startuje
2. ✓ `ABCD REPORT` → Lista numerów
3. ✓ Dodaj numer → Połączenie działa
4. ✓ GUI odczytuje dane

### Full Regression (45 min)
1. Testy podstawowe (15 min)
2. Testy "Mój Numer" (10 min)
3. Testy GUI (10 min)
4. Smoke test końcowy (5 min)
5. Raport wyników (5 min)

---

## Automatyzacja (Przyszłość)

Możliwe usprawnienia:
- [ ] Skrypt Python do automatycznego wysyłania SMS
- [ ] Automatyczne sprawdzanie odpowiedzi
- [ ] CI/CD pipeline z testami
- [ ] Symulator modemu do testów bez hardware
