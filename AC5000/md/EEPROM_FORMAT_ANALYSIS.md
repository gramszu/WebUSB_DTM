# Analiza Problemu Zapisu/Odczytu EEPROM

## Problem
Użytkownik zgłasza, że dane są źle zapisywane i odczytywane między Python GUI a firmware C.

## Odczyt z Urządzenia (przez AVRISP mkII)
```
🔑 Kod dostępu: ....  (bajty 1-4 = puste/domyślne)
⚙️ Tryb pracy: Prywatny  (adres 4094 = 0x00)
📝 Funkcja Skryba: Wyłączona  (adres 4089 = 0x00)
⏰ Harmonogram: Wyłączony  (adres 4090 = 0xFF)
📞 Numery: 0  (wszystkie puste)
```

## Porównanie Format

Razem porównujemy:

| Funkcja | C Firmware | Python Zapis | Python Odczyt | Status |
|---------|-----------|--------------|---------------|--------|
| **Tryb Pracy** | | | | |
| - Prywatny | `0` | `0x00` ✅ | `mode_byte == 0x00` → 0 ✅ | ✅ Zgodne |
| - Publiczny | `1` | `0x01` ✅ | `mode_byte != 0x00` → 1 ✅ | ✅ Zgodne |
| - Niezainicjalizowane | `0xFF` → zapisz `1` | - | `0xFF != 0x00` → 1 ✅ | ✅ Zgodne |
| **Funkcja Skryba** | | | | |
| - Wyłączona | `0` (FALSE) | `0x00` ✅ | `skryba_byte == 0x01 or 0xFF` → 0 ✅ | ✅ Zgodne |
| - Włączona | `1` (TRUE) | `0x01` ✅ | `skryba_byte == 0x01 or 0xFF` → 1 ✅ | ✅ Zgodne |
| **Harmonogram** | | | | |
| - Wyłączony | `0xFF` | `0xFF` ✅ | `start_h == 0xFF` → disabled ✅ | ✅ Zgodne |
| - Włączony | wartości 0-23, 0-59 | wartości ✅ | wartości → enabled ✅ | ✅ Zgodne |

## Wnioski

### ✅ Format Danych Jest Poprawny!

Python używa **dokładnie tych samych wartości** co C firmware:
- Tryb: `0` = Prywatny, `1` = Publiczny
- Skryba: `0` = Wyłączona, `1` = Włączona
- Harmonogram: `0xFF` = Wyłączony, wartości liczbowe = Włączony

### ❓ Dlaczego Użytkownik Widzi Problem?

Możliwe przyczyny:

1. **EEPROM jest puste/wyczyszczone**
   - Urządzenie ma świeże/wyczyszczone EEPROM
   - Wszystkie bajty = 0xFF lub 0x00
   - Trzeba zapisać dane przez Python GUI

2. **Problem z zapisem przez Python GUI**
   - GUI może używać innego programatora (urclock bootloader)
   - AVRISP mkII odczytuje bezpośrednio EEPROM
   - Bootloader może zapisywać w inne miejsce?

3. **Różne urządzenia**
   - Odczyt przez AVRISP mkII z jednego urządzenia
   - Zapis przez GUI do innego urządzenia?

## Rekomendacje

### Test 1: Zapis przez Python GUI
1. Uruchom `AC_test5.py`
2. Ustaw:
   - Kod dostępu: `TEST`
   - Tryb: Publiczny
   - Skryba: Włączona
   - Harmonogram: 08:00 - 18:00 (zaznacz checkbox)
   - Dodaj numer: `123456789`
3. Zapisz do urządzenia (przez bootloader/urclock)
4. Odczytaj przez AVRISP mkII: `python3 read_eeprom_device.py`
5. Sprawdź, czy dane się zgadzają

### Test 2: Porównanie Programatorów
Jeśli Python GUI używa **bootloadera (urclock)** a odczyt jest przez **AVRISP mkII**, mogą być różnice:
- Bootloader zapisuje przez UART
- AVRISP mkII zapisuje/odczytuje przez ISP

**Rozwiązanie:** Użyj tego samego programatora do zapisu i odczytu.

### Test 3: Sprawdź Adresy w GUI
Dodaj logowanie w Python GUI, aby zobaczyć dokładnie, co jest zapisywane:
```python
print(f"Zapisuję na adres 4094: {data[4094]:02X}")
print(f"Zapisuję na adres 4089: {data[4089]:02X}")
```

## Następne Kroki

1. Spróbuj zapisać dane przez Python GUI
2. Odczytaj ponownie przez AVRISP mkII
3. Porównaj wartości
4. Jeśli nadal problem - sprawdź, czy GUI używa poprawnych adresów EEPROM
