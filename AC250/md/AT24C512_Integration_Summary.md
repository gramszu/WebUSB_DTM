# Podsumowanie: Integracja AT24C512 z ATmega328PB

**Data:** 2025-12-24  
**Temat:** Dodanie zewnętrznej pamięci EEPROM AT24C512 do systemu bramki AC-200-DTM-F2

---

## 🎯 Cel projektu

Rozszerzenie systemu o zewnętrzną pamięć EEPROM AT24C512 (64 KB) w celu:
- Zwiększenia liczby przechowywanych numerów telefonów (z 200 do 2,000)
- Dodania funkcji logowania zdarzeń (4,682 wpisy z datą i godziną)
- Umożliwienia przyszłej rozbudowy (zdalna konfiguracja przez GPRS - opcjonalnie)

---

## 📊 Specyfikacja AT24C512

### Podstawowe parametry:
- **Pojemność:** 512 kbit = 64 KB = 65,536 bajtów
- **Interfejs:** I2C (TWI)
- **Adresowanie:** 16-bit (0x0000 - 0xFFFF)
- **Adres I2C:** 0xA0 (domyślny, konfigurowalne piny A0-A2)
- **Rozmiar strony:** 128 bajtów (512 stron)
- **Prędkość I2C:** 100 kHz (standard mode)
- **Cykle zapisu:** 1,000,000
- **Retencja danych:** 100 lat

### Połączenie sprzętowe:
- **SDA:** PC4 (ATmega328PB)
- **SCL:** PC5 (ATmega328PB)
- **Pull-up:** 4.7kΩ na SDA i SCL
- **Zasilanie:** 3.3V lub 5V

---

## 🗂️ Mapa pamięci AT24C512

### Struktura (64 KB):

| Sekcja | Adres | Rozmiar | Zawartość |
|--------|-------|---------|-----------|
| **Nagłówek** | 0x0000-0x001F | 32 B | Magic number, wersja, checksum, konfiguracja |
| **Super Users** | 0x0020-0x0067 | 72 B | 6 numerów × 12 B |
| **Zwykli Users** | 0x0070-0x5DAF | 23,928 B | 1,994 numerów × 12 B |
| **Logi** | 0x5DB0-0xEFFF | 37,456 B | 4,682 wpisy × 8 B |
| **Backup** | 0xF000-0xFFFF | 4,096 B | Rezerwowane |

### Format numeru użytkownika (12 bajtów):
```
[0-5]   Numer telefonu BCD (6 bajtów = 12 cyfr)
[6]     Flagi (aktywny, super user, itp.)
[7-11]  Nazwa użytkownika (5 znaków ASCII) - opcjonalne
```

### Format logu (8 bajtów - uproszczony):
```
[0]     Rok - 2000 (np. 25 = 2025)
[1]     Miesiąc (1-12)
[2]     Dzień (1-31)
[3]     Godzina (0-23)
[4]     Minuta (0-59)
[5-7]   Ostatnie 6 cyfr numeru telefonu (identyfikacja)
```

### Pojemność:
- **Numery telefonów:** 2,000 (6 super + 1,994 zwykłych)
- **Logi otwarć:** 4,682 wpisy
- **Szacowany czas zapełnienia logów:**
  - 10 otwarć/dzień: ~468 dni (15.6 miesięcy)
  - 20 otwarć/dzień: ~234 dni (7.8 miesięcy)
  - 50 otwarć/dzień: ~94 dni (3.1 miesiąca)

---

## ⚡ Wydajność

### Czasy dostępu I2C (100 kHz):
- **Odczyt 1 bajtu:** ~0.46 ms
- **Odczyt numeru (12 B):** ~1.4 ms
- **Zapis 1 numeru:** ~6.4 ms (z cyklem zapisu EEPROM)
- **Odczyt całej pamięci (64 KB):** ~30 sekund

### Transfer przez USART1 (115200 baud):
- **Prędkość:** 11,520 bajtów/sekundę
- **Odczyt całej pamięci:** ~36 sekund (I2C + USART)
- **Zapis całej pamięci:** ~8 sekund (USART + I2C)
- **Eksport 2,000 numerów:** ~13 sekund

### Wyszukiwanie numeru:
- **Random access:** Każda pozycja równie szybka (~1.4 ms)
- **Przeszukiwanie sekwencyjne 2,000 numerów:** ~2.8 sekundy (bez cache)
- **Z cache w RAM:** <100 ms (wymaga ATmega644P/1284P)

---

## 💾 Analiza pamięci RAM

### Obecne zużycie ATmega328PB (2 KB RAM):
```
Użyte:  1,842 B (data: 232 B, bss: 1,610 B)
Wolne:  ~206 B
```

### Wymagania dla różnych wariantów:

| Funkcjonalność | RAM potrzebne | ATmega328PB (2 KB) | ATmega644P (4 KB) | ATmega1284P (16 KB) |
|----------------|---------------|-------------------|-------------------|---------------------|
| **Tylko AT24C512** | ~60 B | ✅ OK (~150 B zapasu) | ✅ OK | ✅ OK |
| **+ GPRS/HTTP (ultra-oszczędny)** | ~194 B | ⚠️ Ciasno (~10 B zapasu) | ✅ OK | ✅ OK |
| **+ GPRS/HTTP (pełny)** | ~790 B | ❌ Za mało | ✅ OK | ✅ OK |
| **Projekt NA MAXA** | ~2,720 B | ❌ Za mało | ✅ OK (~1.4 KB zapasu) | ✅ Spokojnie (~13.6 KB zapasu) |

---

## 🔧 Rekomendacje procesorów AVR

### Porównanie:

| Procesor | RAM | Flash | EEPROM | Piny | Cena | Dla projektu |
|----------|-----|-------|--------|------|------|--------------|
| **ATmega328PB** | 2 KB | 32 KB | 1 KB | 32 | ~$2 | ✅ AT24C512 tylko |
| **ATmega644P** | 4 KB | 64 KB | 2 KB | 40 | ~$3-5 | ✅ **ZALECANY** (AT24C512 + GPRS) |
| **ATmega1284P** | 16 KB | 128 KB | 4 KB | 40 | ~$4-6 | ✅ Overkill (duży zapas) |
| **ATmega2560** | 8 KB | 256 KB | 4 KB | 100 | ~$8-12 | ⚠️ Za duży/drogi |

### Rekomendacja: **ATmega644P**
- **RAM:** 4 KB (2x więcej niż 328PB, wystarczy na wszystko)
- **Flash:** 64 KB (2x więcej, wystarczy na kod)
- **Cena:** ~$3-5 (tylko $1-3 drożej niż 328PB)
- **Dostępność:** DIP-40 (łatwy montaż)
- **Zapas RAM:** ~1.4 KB (wystarczy na rozbudowę)

---

## 📡 Rozważania GPRS/HTTP (opcjonalne)

### Funkcjonalność:
- Zdalna konfiguracja przez internet
- Wysyłanie logów emailem przez GPRS
- Identyfikacja urządzenia po emailu (np. `brama001@firma.pl`)

### Architektura:
```
[Urządzenie] → SMS "ABCD ONLINE 5"
    ↓ Aktywuje GPRS
    ↓ HTTP GET → serwer.pl/check_config.php
    ↓ Pobiera konfigurację (prosty format tekstowy)
    ↓ Zapisuje do AT24C512
    ↓ Rozłącza GPRS
    ↓ SMS potwierdzenia
```

### Backend:
- **Pliki tekstowe** (prostsze niż Redis/MySQL)
- **PHP** na serwerze
- **Struktura:**
  ```
  /serwer/
    ├── pending/BRAMA001.txt  ← Czeka na pobranie
    ├── logs/BRAMA001.csv     ← Logi z urządzenia
    └── devices.txt           ← Baza urządzeń (token + email)
  ```

### Bezpieczeństwo:
- Token autoryzacyjny (32 znaki) dla każdego urządzenia
- Token w AT24C512 (nie w kodzie)
- HTTPS (jeśli SIM900 obsługuje)
- Pliki poza katalogiem WWW

### Wymagania RAM:
- **Ultra-oszczędny:** ~194 B (streaming, prosty format)
- **Pełny JSON:** ~790 B (duże bufory, parser JSON)

### Status: **Rozważania na przyszłość**
- Wymaga upgrade do ATmega644P
- Opcjonalne - nie priorytet

---

## 🚀 Plan implementacji (AT24C512 dla ATmega328PB)

### Etap 1: Sterowniki (priorytet)
1. **i2c_twi.c/h** - Sterownik I2C Master (~500 B Flash, ~20 B RAM)
2. **at24c512.c/h** - Sterownik AT24C512 (~800 B Flash, ~10 B RAM)
3. **usart1_debug.c/h** - Rozszerzenie USART1 (~1.2 KB Flash, ~50 B RAM)

### Etap 2: Integracja
4. Modyfikacja `main.c` (inicjalizacja I2C, AT24C512)
5. Aktualizacja `Makefile` (nowe pliki źródłowe)
6. Opcjonalnie: modyfikacja GUI Python

### Etap 3: Weryfikacja
7. Test I2C (oscyloskop/analizator logiczny)
8. Test zapisu/odczytu przez USART1
9. Test GUI (jeśli zmodyfikowany)
10. Test stabilności (100 numerów + 100 logów)

### Szacunek pamięci (ATmega328PB):
```
Flash: ~28 KB / 32 KB (zapas ~4.7 KB) ✅
RAM:   ~1,922 B / 2,048 B (zapas ~126 B) ✅
```

---

## 📝 Komendy USART1 (diagnostyka)

### Nowe komendy dla AT24C512:
```
EREAD <addr>           - Odczyt bajtu z AT24C512
EWRITE <addr> <val>    - Zapis bajtu do AT24C512
EDUMP <start> <end>    - Dump zakresu pamięci (hex)
EUSER <nr>             - Odczyt/zapis numeru użytkownika
ELOG <start> <count>   - Eksport logów (CSV format)
ECLEAR                 - Wyczyść wszystkie logi
```

### Przykłady użycia:
```
> EWRITE 0x0000 0xAC
OK

> EREAD 0x0000
0xAC

> EDUMP 0x0000 0x0010
0000: AC 20 00 00 00 00 00 00 00 00 00 00 00 00 00 00

> EUSER 0 48123456789
OK - User 0 saved

> ELOG 0 10
Date,Time,Phone
2025-12-24,01:35,...456789
2025-12-23,15:20,...654321
```

---

## 🔍 Szczegóły techniczne AT24C512

### Strony (Pages):
- **Rozmiar strony:** 128 bajtów
- **Liczba stron:** 512
- **Ograniczenie zapisu:** Musi być w obrębie jednej strony
- **Odczyt:** Bez ograniczeń (można czytać przez strony)

### Przykład granic stron:
```
Strona 0:   0x0000 - 0x007F (128 B)
Strona 1:   0x0080 - 0x00FF (128 B)
Strona 2:   0x0100 - 0x017F (128 B)
...
Strona 511: 0xFF00 - 0xFFFF (128 B)
```

### Obliczanie numeru strony:
```
Numer strony = Adres ÷ 128
Pozycja w stronie = Adres % 128

Przykład: adres 0x1234 (4660)
  Strona = 4660 ÷ 128 = 36
  Pozycja = 4660 % 128 = 52
  → Strona 36, bajt 52
```

### Maksymalna pojemność (wiele układów):
- **1 układ AT24C512:** 64 KB
- **8 układów AT24C512:** 512 KB (różne adresy I2C: 0xA0-0xAE)
- **Dla projektu:** 1 układ wystarczy

---

## ✅ Decyzje i ustalenia

### Potwierdzone:
1. ✅ AT24C512 (64 KB) jako zewnętrzna pamięć
2. ✅ 2,000 numerów + 4,682 logi (uproszczony format 8 B)
3. ✅ Komunikacja przez I2C (SDA=PC4, SCL=PC5)
4. ✅ Dostęp przez USART1 (diagnostyka + konfiguracja)
5. ✅ Mapa pamięci zdefiniowana i zaakceptowana

### Do decyzji:
1. ❓ Procesor: ATmega328PB (tylko AT24C512) vs ATmega644P (+ GPRS)
2. ❓ GPRS/HTTP: Teraz vs Przyszłość
3. ❓ GUI Python: Modyfikować vs Zostaw jak jest
4. ❓ Migracja danych: Z wbudowanej EEPROM do AT24C512?
5. ❓ Device ID: W AT24C512 vs Hardcoded

### Status: **Gotowe do implementacji AT24C512 dla ATmega328PB**

---

## 📚 Dokumenty powiązane

- [at24c512_memory_map.md](file:///Users/gramsz/.gemini/antigravity/brain/78dd5ee6-06e7-4060-b3c2-228b38dccd01/at24c512_memory_map.md) - Szczegółowa mapa pamięci
- [implementation_plan.md](file:///Users/gramsz/.gemini/antigravity/brain/78dd5ee6-06e7-4060-b3c2-228b38dccd01/implementation_plan.md) - Plan implementacji
- [task.md](file:///Users/gramsz/.gemini/antigravity/brain/78dd5ee6-06e7-4060-b3c2-228b38dccd01/task.md) - Lista zadań

---

## 🎓 Wnioski

### Zalety AT24C512:
- ✅ 64x więcej pamięci niż wbudowana EEPROM (64 KB vs 1 KB)
- ✅ 10x więcej numerów (2,000 vs 200)
- ✅ Historia zdarzeń (4,682 logi z datą/godziną)
- ✅ Szybki dostęp (random access ~1.4 ms)
- ✅ Tani (~$1-2 za układ)
- ✅ Łatwa integracja (I2C, 2 piny)

### Ograniczenia ATmega328PB:
- ⚠️ Tylko ~200 B wolnego RAM
- ⚠️ GPRS/HTTP wymaga upgrade do ATmega644P
- ⚠️ Przeszukiwanie 2,000 numerów bez cache: ~2.8 s

### Rekomendacja finalna:
**ATmega644P + AT24C512** dla pełnej funkcjonalności (AT24C512 + GPRS + zapas RAM)

---

**Koniec podsumowania**
