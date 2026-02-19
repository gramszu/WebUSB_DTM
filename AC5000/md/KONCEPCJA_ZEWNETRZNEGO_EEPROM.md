# Koncepcja Integracji Zewnętrznego EEPROM (AT24C512)
## Architektura Hybrydowa dla AC200-DTM-F3

### 1. Cel Zmiany
Zwiększenie pojemności pamięci na bazę numerów telefonów przy zachowaniu stabilności systemu i kompatybilności procesu aktualizacji.

*   **Obecnie (ATmega328PB Internal):** ~1 KB (ok. 200 numerów).
*   **Docelowo (AT24C512 External):** 64 KB (ok. 13 000 numerów).

### 2. Architektura Hybrydowa

Zdecydowano o zastosowaniu modelu hybrydowego, który rozdziela dane systemowe od danych użytkownika.

| Typ Danych | Lokalizacja | Opis | Metoda Dostępu |
| :--- | :--- | :--- | :--- |
| **Konfiguracja Systemowa** | **Wewnętrzny EEPROM** (ATmega) | Flagi (Public/Private, CLIP/DTMF), czasy, blokady, PIN. | `avrdude` (jak dotychczas) |
| **Baza Numerów** | **Zewnętrzny EEPROM** (I2C) | Numery telefonów użytkowników. | UART -> Komendy aplikacji (Service Mode) |

#### Dlaczego taki podział?
1.  **Niezawodność:** System zawsze "wstanie" poprawnie, korzystając ze stabilnej pamięci wewnętrznej dla krytycznej konfiguracji. Awaria magistrali I2C nie unieruchomi całkowicie urządzenia (bootloader i config działają).
2.  **Bezpieczeństwo:** Nie wymagamy modyfikacji bootloadera. Aktualizacja firmware i naprawa systemu nadal odbywa się przez standardowe `avrdude`.
3.  **Wydajność:** Zewnętrzny EEPROM umożliwia szybki zapis blokowy, a protokół UART pozwala na sprawną transmisję dużej bazy danych.

### 3. Wymagania Sprzętowe

*   **Układ:** AT24C512 (64 KB / 512 Kbit).
*   **Interfejs:** I2C (TWI).
*   **Połączenia (ATmega328PB):**
    *   **SDA:** Pin PC4 (fizyczny ADC4/SDA).
    *   **SCL:** Pin PC5 (fizyczny ADC5/SCL).
    *   **Adres:** A0, A1 do GND (Adres bazowy: 0xA0 / 0b1010000).
    *   **Pull-up:** Rezystory 4.7kΩ na liniach SDA i SCL do VCC.

### 4. Plan Implementacji (Oprogramowanie)

#### A. Firmware (ATmega328PB)
1.  **Obsługa I2C (TWI):** Implementacja sterownika (Init, Start, Stop, Read, Write).
2.  **Protokół UART:** Rozszerzenie obsługi komend o nowe instrukcje "Service Mode":
    *   `CMD_EXT_WRITE_PAGE` (Zapisz stronę numerów).
    *   `CMD_EXT_READ_PAGE` (Odczytaj stronę numerów).
    *   `CMD_EXT_CLEAR_ALL` (Szybkie czyszczenie).

#### B. Aplikacja PC (Python)
1.  **Tryb Serwisowy:** Nowa zakładka lub tryb w GUI do zarządzania "Dużą Bazą".
2.  **Komunikacja:** Bezpośrednie użycie biblioteki `serial` do wysyłania komend (zamiast wywoływania `run_avrdude`).
3.  **Zarządzanie:** Import/Eksport CSV dostosowany do dużej liczby rekordów.

### 5. Uwagi Końcowe
To podejście pozwala na płynne przejście. Można najpierw zaimplementować obsługę sprzętową, a aplikacja PC wciąż będzie działać "po staremu" (korzystając tylko z małej bazy wewn.), dopóki nie zostanie zaktualizowana do obsługi trybu hybrydowego.
