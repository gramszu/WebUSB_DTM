# AC200-DTM-F3 Web GUI - UART EEPROM Configuration

Webowy interfejs do konfiguracji EEPROM przez UART z wykorzystaniem Web Serial API.

## Wymagania

### Przeglądarka
- **Chrome 89+** (zalecane)
- **Edge 89+**
- **Opera 76+**

❌ **Nie działa w:** Firefox, Safari (brak wsparcia Web Serial API)

### Sprzęt
- Urządzenie AC200-DTM-F3 z wgranym firmware UART
- Konwerter USB-UART (np. CP2102, CH340, FT232)
- Kabel USB

## Podłączenie Sprzętowe

```
Konwerter USB-UART → AC200-DTM-F3
────────────────────────────────────
RXD (konwerter)  →  PB3 (MISO, pin ISP)
TXD (konwerter)  →  PB4 (MOSI, pin ISP)
GND              →  GND
```

> ⚠️ **WAŻNE:** Odłącz AVRISP2 przed podłączeniem konwertera UART!

## Uruchomienie

### Metoda 1: Bezpośrednio z pliku (localhost)

1. Otwórz plik `index.html` w przeglądarce Chrome/Edge
2. Kliknij "Połącz z urządzeniem"
3. Wybierz port szeregowy z listy
4. Gotowe!

### Metoda 2: Przez serwer HTTP (opcjonalnie)

```bash
# Python 3
cd GUI_WWW
python3 -m http.server 8000

# Lub Node.js
npx http-server -p 8000
```

Następnie otwórz: `http://localhost:8000`

## Instrukcja Użycia

### 1. Połączenie

1. Podłącz konwerter USB-UART do komputera
2. Kliknij **"Połącz z urządzeniem"**
3. Wybierz port szeregowy (np. "USB Serial Device")
4. Status zmieni się na "Połączony" (zielona kropka)

### 2. Test Połączenia

Kliknij **"🧪 Test połączenia"** aby sprawdzić czy port działa poprawnie.

### 3. Odczyt EEPROM

1. Kliknij **"📥 Odczytaj EEPROM"**
2. Poczekaj ~1-2 sekundy
3. Dane zostaną automatycznie załadowane do formularza

### 4. Edycja Konfiguracji

Edytuj dowolne parametry:
- **Kod dostępu** (4 znaki ASCII)
- **Status** (Aktywny/Blokada)
- **Tryb** (Prywatny/Publiczny)
- **Sterowanie** (CLIP/DTMF/SMS/CLIP+SMS)
- **Funkcja Skryba** (Włączona/Wyłączona)
- **Numery telefonów** (1-250)

### 5. Zapis EEPROM

1. Kliknij **"📤 Zapisz EEPROM"**
2. Potwierdź zapis
3. Poczekaj ~4-5 sekund (zapis + automatyczna weryfikacja)
4. Sprawdź logi - powinno być "✅ EEPROM zapisany pomyślnie!"

### 6. Weryfikacja

Kliknij **"✓ Weryfikuj"** aby porównać dane w GUI z danymi w urządzeniu.

### 7. Export/Import

- **💾 Export JSON** - zapisz konfigurację do pliku JSON
- **📂 Import JSON** - wczytaj konfigurację z pliku JSON

## Protokół UART

### Komendy

| Komenda | Hex | Opis | Odpowiedź |
|---------|-----|------|-----------|
| `R` | 0x52 | Odczyt EEPROM | 1024 bajty |
| `W` | 0x57 | Zapis EEPROM | "OK\n" lub "ERR\n" |
| `V` | 0x56 | Weryfikacja | "OK\n" lub "ERR:addr\n" |

### Parametry

- **Prędkość:** 115200 baud
- **Format:** 8N1 (8 bitów, bez parzystości, 1 bit stopu)
- **Rozmiar EEPROM:** 1024 bajty

## Rozwiązywanie Problemów

### "Web Serial API nie jest wspierane"
**Rozwiązanie:** Użyj przeglądarki Chrome lub Edge (wersja 89+)

### "Nie można otworzyć portu"
**Rozwiązanie:**
- Sprawdź czy konwerter jest podłączony
- Zamknij inne programy używające portu (Arduino IDE, PuTTY, itp.)
- Odśwież stronę i spróbuj ponownie

### "Timeout podczas odczytu"
**Rozwiązanie:**
- Sprawdź połączenia (RXD↔PB3, TXD↔PB4, GND↔GND)
- Upewnij się że firmware z UART jest wgrany
- Sprawdź czy urządzenie ma zasilanie

### "Błąd weryfikacji"
**Rozwiązanie:**
- Spróbuj ponownie (może być szum na linii)
- Sprawdź jakość połączeń
- Odczytaj ponownie i porównaj

## Struktura Plików

```
GUI_WWW/
├── index.html          # Główny plik GUI
├── style.css           # Stylowanie
├── uart-eeprom.js      # Logika UART i EEPROM
└── README.md           # Ten plik
```

## Bezpieczeństwo

- Web Serial API wymaga zgody użytkownika na dostęp do portu
- Dane nie są wysyłane do internetu - wszystko działa lokalnie
- Konfiguracja jest zapisywana tylko w urządzeniu

## Wsparcie

Branch: **AC200-DTM-F3-UART**

Jeśli coś nie działa:
1. Sprawdź logi w GUI (sekcja "📋 Logi")
2. Sprawdź konsolę przeglądarki (F12 → Console)
3. Sprawdź połączenia sprzętowe

**Wersja:** RC2

## Changelog

### v RC2 (2026-01-17)
- ✅ Dodano informację o wymaganym dostępie do internetu dla zasobów zewnętrznych (YouTube, Instrukcje, Sklep)
- ✅ Poprawki w interfejsie użytkownika
- ✅ Ulepszona obsługa błędów połączenia

### v1.0 (2026-01-14)
- ✅ Pierwsza wersja webowego GUI
- ✅ Obsługa Web Serial API
- ✅ Odczyt/Zapis/Weryfikacja EEPROM
- ✅ Export/Import JSON
- ✅ Nowoczesny interfejs z dark theme
- ✅ Wsparcie dla 250 numerów telefonów
- ✅ Pełna konfiguracja parametrów

## Rozwiązywanie problemów z połączeniem (DTR/RTS)

Jeśli występują problemy z nawiązaniem komunikacji lub urządzenie nie odpowiada na komendy (np. brak paska postępu przy zapisie), sprawdź obsługę sygnałów DTR/RTS.

W pliku `uart-eeprom.js` w metodzie `connect()` znajduje się kluczowy fragment:

```javascript
// Disable DTR and RTS to prevent reset (or ensure stable state)
await this.port.setSignals({
    dataTerminalReady: false,
    requestToSend: false
});
```

**Ważne:** Niektóre konwertery USB-UART wymagają jawnego ustawienia tych linii na stan niski (`false`) podczas inicjalizacji. Usunięcie tych linii może skutkować niestabilnym połączeniem lub brakiem transmisji danych. Jeśli napotkasz problemy po modyfikacji kodu, upewnij się, że ten fragment jest obecny.
