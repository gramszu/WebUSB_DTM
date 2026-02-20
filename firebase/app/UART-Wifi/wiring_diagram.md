# Schemat Podłączenia ESP32 do Bramster AC200

Aby ESP32 mogło komunikować się ze sterownikiem bramy, musisz połączyć je "na krzyż" (TX do RX, RX do TX) oraz zapewnić wspólne zasilanie.

## Pinout (Podłączenie)

| ESP32 Pin | Brama (Złącze PROG/USB) | Opis |
| :--- | :--- | :--- |
| **RX2** (GPIO 16) | **TX** | ESP32 odbiera dane od bramy |
| **TX2** (GPIO 17) | **RX** | ESP32 wysyła dane do bramy |
| **GND** | **GND** | Wspólna masa (Kluczowe!) |
| **VIN** (5V) | **5V** | Zasilanie ESP32 z bramy* |

> [!SUCCESS]
> **Kompatybilność Napięciowa:**
> Skoro Twój procesor **ATmega328PB** jest zasilany napięciem **3V/3.3V**, a ESP32 również pracuje na **3.3V**, możesz łączyć piny **bezpośrednio**.

> [!CAUTION]
> **UWAGA: PĘTLA MASY (GROUND LOOP) i ZASILANIE**
> Jeśli pobierasz 5V z zasilania SIM800C (AC200) i jednocześnie podłączasz komputer przez USB do ESP32, tworzysz pętlę masy. **Może to spalić port USB lub procesor!**
>
> **Zasada Bezpieczeństwa:**
> 1.  **Tryb Pracy (Wdrożenie):** ESP32 podłączone 4 przewodami do AC200 (VCC, GND, TX, RX). **BRAK kabla USB.**
> 2.  **Tryb Programowania (Komputer):** Odłącz przewód **VCC (5V)**. Zostaw tylko GND, TX, RX. ESP32 zasilasz z USB komputera.
>
> *Nigdy nie zasilaj ESP32 z dwóch źródeł jednocześnie (USB + Pin 5V), chyba że masz pewność, że Twoja płytka ma diodę zabezpieczającą (a nawet wtedy pętla masy pozostaje problemem).*

## Połączenie Bezpośrednie (3.3V <-> 3.3V)

```
Brama TX (3.3V) ---------------- ESP32 RX (GPIO 16)
Brama RX (3.3V) ---------------- ESP32 TX (GPIO 17)
GND ---------------------------- GND
VCC (5V z SIM800) -------------- ODŁĄCZ przy USB! --- VIN (5V)
```
*Zasilanie: Upewnij się, które napięcie podajesz na ESP. Jeśli masz 5V w złączu, podaj na pin 5V/VIN. Jeśli masz 3.3V, podaj na pin 3V3.*
