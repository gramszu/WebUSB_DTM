# Specyfikacja: Wdrożenie mostu WiFi-UART na ESP32 dla konfiguratora AC200

**Cel:** Stworzenie kompletnego oprogramowania (Firmware + Web) przekształcającego moduł ESP32 w bezprzewodowy interfejs serwisowy dla sterownika bramy AC200.

## 1. Wymagania Firmware (ESP32 - Arduino IDE)

Należy stworzyć szkic Arduino (`.ino`), który realizuje następujące funkcje:

### A. System plików
*   Użycie **LittleFS** do przechowywania plików aplikacji webowej (`index.html`, `style.css`, `uart-eeprom.js`, grafiki) w pamięci flash układu.

### B. Tryb WiFi
*   Uruchomienie ESP32 w trybie **Access Point (AP)**.
*   **SSID:** `AC200-Serwis`
*   **Hasło:** (brak lub proste, np. `12345678`)
*   **IP bramy:** `192.168.4.1`

### C. Serwer WWW
*   Użycie biblioteki `ESPAsyncWebServer`.
*   Serwowanie plików statycznych z LittleFS.
*   Konfiguracja nagłówków cache (Cache-Control) dla szybkiego ładowania.

### D. Most Serial-WebSocket
*   Uruchomienie serwera WebSocket na ścieżce `/ws`.
*   **Kierunek WWW -> UART:** Dane odebrane przez WebSocket są wysyłane na port sprzętowy `Serial2` (piny TX=17, RX=16 lub konfigurowalne).
*   **Kierunek UART -> WWW:** Dane odebrane z `Serial2` są buforowane i wysyłane natychmiast do wszystkich podłączonych klientów WebSocket (binarnie).
*   **BaudRate:** `115200` (zgodnie ze sterownikiem AC200).

## 2. Wymagania Aplikacji Web (Frontend)

Modyfikacja pliku `uart-eeprom.js` w katalogu projektu:

### A. Abstrakcja Transportu
Wydzielenie logiki komunikacji do interfejsu `Transport`, który posiada dwie implementacje:
1.  `WebSerialTransport` (istniejąca logika USB).
2.  `WebSocketTransport` (nowa logika WiFi).

### B. Auto-detekcja
*   Przy starcie sprawdzenie `window.location.hostname`.
*   Jeśli adres to `192.168.4.1` (lub inny lokalny IP), **automatyczne użycie WebSocketTransport** i połączenie z `ws://192.168.4.1/ws`.
*   Ukrycie przycisku "Połącz z portem szergowym" w trybie WiFi (połączenie ma być automatyczne przy ładowaniu strony).

### C. Obsługa błędów
*   Wyświetlenie czytelnego komunikatu w przypadku utraty połączenia WiFi.

## 3. Assety (Grafika)
*   Wszystkie zewnętrzne linki do obrazków (ikony przeglądarek, loga) w `index.html` zostaną zastąpione plikami lokalnymi z katalogu `assets/`, aby strona działała w pełni **offline**.
