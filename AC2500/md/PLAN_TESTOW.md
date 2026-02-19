# Plan Testów AC200-DTM-F2

## 1. Testy Podstawowe (Obowiązkowe)

### 1.1 Test Uruchomienia
- [ ] Po podłączeniu zasilania LED SYS miga (nie zalogowany w sieci)
- [ ] Po ~30s LED SYS gaśnie (zalogowany w sieci)
- [ ] Brak nieprawidłowych resetów

### 1.2 Test Kodu Dostępu
- [ ] SMS: `ABCD` → Odpowiedź: `OK`
- [ ] SMS: `WXYZ` (zły kod) → Brak odpowiedzi
- [ ] SMS: `ABCD CODE EFGH` → Kod zmieniony
- [ ] SMS: `EFGH` → Odpowiedź: `OK`
- [ ] SMS: `EFGH CODE ABCD` → Przywrócenie kodu

### 1.3 Test Dodawania/Usuwania Numerów
- [ ] SMS: `ABCD ADD 123456789` → Numer dodany
- [ ] Połączenie z 123456789 → Brama otwiera się
- [ ] SMS: `ABCD DEL 123456789` → Numer usunięty
- [ ] Połączenie z 123456789 → Brama NIE otwiera się

### 1.4 Test Trybu CLIP/DTMF
**CLIP Mode:**
- [ ] SMS: `ABCD CLOSE CLIP` → Tryb CLIP, Prywatny
- [ ] Połączenie z zapisanego numeru → Brama otwiera się automatycznie
- [ ] Połączenie z niezapisanego numeru → Brama NIE otwiera się

**DTMF Mode:**
- [ ] SMS: `ABCD CLOSE DTMF` → Tryb DTMF, Prywatny
- [ ] Połączenie z zapisanego numeru → Modem odbiera
- [ ] Naciśnij `1` na telefonie → Brama otwiera się
- [ ] Połączenie z niezapisanego numeru → Modem NIE odbiera

### 1.5 Test Trybu Publiczny/Prywatny
- [ ] SMS: `ABCD OPEN CLIP` → Tryb Publiczny
- [ ] Połączenie z dowolnego numeru → Brama otwiera się
- [ ] SMS: `ABCD CLOSE CLIP` → Tryb Prywatny
- [ ] Połączenie z niezapisanego numeru → Brama NIE otwiera się

---

## 2. Testy Zaawansowane

### 2.1 Test Funkcji SKRYBA
- [ ] SMS: `ABCD SKRYBA ON 10` → Skryba włączona (limit 10)
- [ ] Tryb automatycznie zmienia się na OPEN CLIP
- [ ] Połączenie z nowego numeru → Brama otwiera się + numer zapisany
- [ ] SMS: `ABCD REPORT` → Lista zawiera nowy numer
- [ ] Po dodaniu 10 numerów → Skryba automatycznie wyłącza się
- [ ] SMS: `ABCD SKRYBA OFF` → Skryba wyłączona

### 2.2 Test Harmonogramu (TIME)
- [ ] SMS: `ABCD TIME 08:00 18:00` → Harmonogram ustawiony
- [ ] Przed 08:00 → Brama zablokowana (nawet dla zapisanych numerów)
- [ ] 08:00-18:00 → Brama działa normalnie
- [ ] Po 18:00 → Brama zablokowana
- [ ] SMS: `ABCD TIME OFF` → Harmonogram wyłączony
- [ ] O dowolnej godzinie → Brama działa normalnie

### 2.3 Test Blokady Systemu (START/STOP)
- [ ] SMS: `ABCD STOP` → System zablokowany
- [ ] Połączenie z zapisanego numeru → Brama NIE otwiera się
- [ ] SMS: `ABCD START` → System odblokowany
- [ ] Połączenie z zapisanego numeru → Brama otwiera się

### 2.4 Test Super Userów (SUB)
- [ ] SMS: `ABCD SUB 987654321` → Dodany jako Super User (pozycja 195-200)
- [ ] Połączenie z 987654321 → Brama otwiera się
- [ ] Super User działa nawet gdy Skryba osiągnie limit

### 2.5 Test Ustawiania Czasu (SET)
- [ ] SMS: `ABCD SET` → Odpowiedź z aktualnym czasem RTC
- [ ] SMS: `ABCD SET 12:30:45` → Czas ustawiony
- [ ] SMS: `ABCD SET` → Odpowiedź: `Time: 12:30:xx`

---

## 3. Testy Nowej Funkcji "Mój Numer"

### 3.1 Test Ustawiania Numeru
- [ ] SMS: `ABCD MYNUM 123456789` → Odpowiedź: `Numer zapisany: 123456789`
- [ ] SMS: `ABCD MYNUM 12345678` (8 cyfr) → Błąd (wymaga 9 cyfr)
- [ ] SMS: `ABCD MYNUM 1234567890` (10 cyfr) → Błąd (wymaga 9 cyfr)
- [ ] SMS: `ABCD MYNUM 12345ABCD` (litery) → Błąd (tylko cyfry)

### 3.2 Test Auto-Sync Czasu
- [ ] Ustaw `ABCD MYNUM 123456789`
- [ ] Reset urządzenia (power cycle)
- [ ] Czekaj ~30s po zalogowaniu do sieci
- [ ] Sprawdź czy otrzymałeś SMS: `Synchronizacja Czasu`
- [ ] SMS powinien przyjść TYLKO jeśli RTC pokazuje 00:00:xx

### 3.3 Test Wyświetlania w DEBUG
- [ ] SMS: `ABCD DEBUG`
- [ ] Odpowiedź powinna zawierać: `Moj nr: 123456789`
- [ ] Numer NIE powinien być śmieciami (np. `??UsW?1234`)

---

## 4. Testy GUI (AC200-DTM-F2.py)

### 4.1 Test Odczytu z Urządzenia
- [ ] Uruchom `AC200-DTM-F2.py`
- [ ] Wybierz port COM
- [ ] Kliknij "Odczytaj dane ze sterownika"
- [ ] Lista numerów wyświetla się poprawnie
- [ ] Kod dostępu wyświetla się poprawnie
- [ ] Status/Tryb/Skryba wyświetlają się poprawnie
- [ ] "Mój numer" wyświetla się poprawnie (9 cyfr)

### 4.2 Test Zapisu do Urządzenia
- [ ] Dodaj numer przez GUI
- [ ] Zmień kod dostępu
- [ ] Ustaw "Mój numer" (9 cyfr)
- [ ] Kliknij "Wgraj dane do sterownika"
- [ ] Odczytaj ponownie → Wszystkie dane poprawne

### 4.3 Test CSV Export/Import
- [ ] Kliknij "Zapisz dane do CSV"
- [ ] Otwórz plik CSV → Numery poprawne
- [ ] Wyczyść wszystkie numery w GUI
- [ ] Kliknij "Odczytaj dane z CSV"
- [ ] Numery przywrócone poprawnie

### 4.4 Test EEPROM Viewer
- [ ] Sprawdź adresy 1019-1023 (0x3FB-0x3FF)
- [ ] Dla numeru `123456789` powinno być: `90 78 56 34 12` (BCD odwrócony)
- [ ] Sprawdź że format jest zgodny z innymi numerami

---

## 5. Testy Wydajnościowe

### 5.1 Test Maksymalnej Liczby Numerów
- [ ] Dodaj 200 numerów (maksimum dla ATmega328PB)
- [ ] SMS: `ABCD REPORT` → Wszystkie numery wyświetlone
- [ ] Połączenie z numeru #1 → Działa
- [ ] Połączenie z numeru #200 → Działa
- [ ] Połączenie z numeru #100 → Działa

### 5.2 Test Szybkich Połączeń
- [ ] 5 połączeń w ciągu 1 minuty → Wszystkie obsłużone
- [ ] Sprawdź czy nie ma resetów
- [ ] Sprawdź czy LED działa poprawnie

### 5.3 Test Długotrwałej Pracy
- [ ] Pozostaw urządzenie włączone przez 24h
- [ ] Sprawdź czy nie ma resetów
- [ ] Sprawdź czy modem pozostaje zalogowany
- [ ] Sprawdź czy brama nadal reaguje na połączenia

---

## 6. Testy Bezpieczeństwa

### 6.1 Test Ochrony Przed Spamem
- [ ] Wyślij 10 SMS z komendami w ciągu 1 minuty
- [ ] Sprawdź czy urządzenie nie resetuje się
- [ ] Sprawdź czy wszystkie komendy są obsłużone

### 6.2 Test Nieprawidłowych Komend
- [ ] SMS: `ABCD INVALID` → Brak odpowiedzi lub błąd
- [ ] SMS: `ABCD ADD` (bez numeru) → Błąd
- [ ] SMS: `ABCD TIME 25:00 26:00` (nieprawidłowy czas) → Błąd

---

## 7. Testy Regresji (Po Każdej Zmianie)

### 7.1 Smoke Test (Szybki)
- [ ] Urządzenie startuje poprawnie
- [ ] SMS `ABCD` → Odpowiedź `OK`
- [ ] Połączenie z zapisanego numeru → Brama otwiera się
- [ ] GUI odczytuje dane poprawnie

### 7.2 Full Regression (Pełny)
- [ ] Wszystkie testy z sekcji 1 (Podstawowe)
- [ ] Wszystkie testy z sekcji 3 (Mój Numer)
- [ ] Test GUI (sekcja 4.1 i 4.2)

---

## 8. Checklist Przed Wydaniem

- [ ] Wszystkie testy podstawowe (sekcja 1) ✅
- [ ] Testy "Mój Numer" (sekcja 3) ✅
- [ ] Test GUI odczytu/zapisu (sekcja 4.1, 4.2) ✅
- [ ] Test maksymalnej liczby numerów (sekcja 5.1) ✅
- [ ] Smoke test (sekcja 7.1) ✅
- [ ] Dokumentacja zaktualizowana ✅
- [ ] Kod zacommitowany do Git ✅
- [ ] Firmware skompilowany bez ostrzeżeń ✅

---

## Notatki

**Środowisko testowe:**
- Moduł: ATmega328PB + SIM900
- Karta SIM z aktywnym numerem
- Telefon testowy do wykonywania połączeń
- Komputer z zainstalowanym Python 3.x i GUI

**Zgłaszanie błędów:**
- Opisz dokładnie kroki reprodukcji
- Załącz logi z `bramster.log`
- Podaj wersję firmware (rozmiar flash)
- Podaj numer commitu Git
