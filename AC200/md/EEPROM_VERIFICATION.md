# Weryfikacja Adresów EEPROM - AC800

## Obliczenia Adresów

### Kod Dostępu
- **C**: `ADRES_EEPROM_KOD_DOSTEPU` = 1 (bajty 1-4)
- **Python**: `data[0x01:0x05]` = bajty 1-4
- ✅ **Zgodne**

### Numery Telefonów

**C Firmware:**
```c
ADRES_EEPROM_KOD_DOSTEPU = 1
LICZBA_BAJTOW_KODU_DOSTEPU = 4
EEPROM_USTAWIENIE_STANOW_WYJSC = 1 + 4 = 5
EEPROM_USTAWIENIE_WYJSCIA = 5 + 1 = 6
EEPROM_NUMER_TELEFONU_BRAMA_0 = 6 + 2 = 8
```

**Python:**
```python
RANGE_START = 0x08 = 8
```

✅ **Zgodne** - numery zaczynają się od adresu 8

### Numer 800 (indeks 799)

**Obliczenia:**
- Adres początkowy: 8 + (799 × 5) = 8 + 3995 = **4003**
- Zakres: **4003-4007** (5 bajtów)

**Adresy Konfiguracyjne:**
- Skryba: 4089
- Czas Start H: 4090
- Czas Start M: 4091
- Czas Stop H: 4092
- Czas Stop M: 4093
- Tryb: 4094

### ✅ Brak Kolizji!
Numer 800 kończy się na adresie **4007**, a konfiguracja zaczyna się od **4089**.
Różnica: 4089 - 4007 = **82 bajty wolne**

## Problem w Pythonie?

Sprawdzam logikę `generate_hex_configs`:

```python
for i in range(num_entries):  # 0-799
    current_start = start_address + i * entry_size  # 8 + i*5
    current_end = current_start + entry_size - 1
    
    if current_end >= current_config["EEPROM_SIZE"]:  # 4096
        break
```

**Dla numeru 800 (i=799):**
- `current_start` = 8 + 799×5 = 4003
- `current_end` = 4003 + 5 - 1 = 4007
- Sprawdzenie: `4007 >= 4096`? **NIE** ✅

**Wszystkie 800 numerów powinno być generowanych!**

## Możliwe Problemy

1. **Kod dostępu**: Czy Python poprawnie odczytuje z adresów 1-4?
   - ✅ Tak: `data[0x01:0x05]`

2. **Numer 800**: Czy jest generowany?
   - ✅ Tak: `current_end = 4007 < 4096`

3. **Wyświetlanie**: Czy GUI pokazuje wszystkie 800 numerów?
   - ❓ Do sprawdzenia

## Rekomendacja

Uruchom Python i sprawdź:
1. Czy w `numbers_text` widać "Numer 800"
2. Czy kod dostępu jest poprawnie odczytywany/zapisywany
3. Czy po zapisie i odczycie numer 800 jest zachowany
