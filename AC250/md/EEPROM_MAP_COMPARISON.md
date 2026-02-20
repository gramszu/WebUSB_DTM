# Mapa EEPROM - Porównanie Python vs C (AC800)

## Adresy Konfiguracyjne

| Funkcja | Adres Python (AC_test4.py) | Adres C (adresyeeprom.h) | Status |
|---------|---------------------------|--------------------------|--------|
| **Kod dostępu** | 1-4 | `ADRES_EEPROM_KOD_DOSTEPU` (1-4) | ✅ Zgodne |
| **Tryb pracy** (Private/Public) | 4094 | `ADRES_EEPROM_TRYB_PRACY` (4094) | ✅ Zgodne |
| **Funkcja Skryba** | 4089 | `ADRES_EEPROM_SKRYBA` (4089) | ✅ Zgodne |
| **Czas Start - Godzina** | 4090 | `ADRES_EEPROM_CZAS_START_H` (4090) | ✅ Zgodne |
| **Czas Start - Minuta** | 4091 | `ADRES_EEPROM_CZAS_START_M` (4091) | ✅ Zgodne |
| **Czas Stop - Godzina** | 4092 | `ADRES_EEPROM_CZAS_STOP_H` (4092) | ✅ Zgodne |
| **Czas Stop - Minuta** | 4093 | `ADRES_EEPROM_CZAS_STOP_M` (4093) | ✅ Zgodne |
| **Status sterownika** | 1022 (nieużywane) | ❌ Brak w C | ⚠️ Python ma, C nie używa |

## Numery Telefonów

| Parametr | Python | C | Status |
|----------|--------|---|--------|
| **Początek listy** | 8 (0x08) | `EEPROM_NUMER_TELEFONU_BRAMA_0` (8) | ✅ Zgodne |
| **Bajty na numer** | 5 | `LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM` (5) | ✅ Zgodne |
| **Maksymalna liczba** | 800 | `MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA` (800) | ✅ Zgodne |
| **Zakres adresów** | 8 - 4007 | Obliczany: 8 + (800 × 5) - 1 = 4007 | ✅ Zgodne |

## Adresy Debug (tylko C)

Poniższe adresy są używane **tylko przez firmware C** do celów diagnostycznych. Python ich nie odczytuje ani nie zapisuje.

### Debug SKRYBA
- `ADRES_EEPROM_DEBUG_SKRYBA_1` (4080) - CLIP otrzymany
- `ADRES_EEPROM_DEBUG_SKRYBA_2` (4081) - skryba_wlaczona
- `ADRES_EEPROM_DEBUG_SKRYBA_3` (4082) - !znaleziono
- `ADRES_EEPROM_DEBUG_SKRYBA_4` (4083) - komenda dodana
- `ADRES_EEPROM_DEBUG_SKRYBA_5` (4084) - komenda wykonana

### Debug USER
- `ADRES_EEPROM_DEBUG_USER_1` (4070) - Komenda USER otrzymana
- `ADRES_EEPROM_DEBUG_USER_2` (4071) - flaga_wysylanie_smsa
- `ADRES_EEPROM_DEBUG_USER_3` (4072) - licznik_report_user
- `ADRES_EEPROM_DEBUG_USER_4` (4073) - liczba_sms_w_kolejce
- `ADRES_EEPROM_DEBUG_USER_5` (4074) - liczba_wszystkich_komend
- `ADRES_EEPROM_DEBUG_USER_6` (4075) - znaleziono (0/1)
- `ADRES_EEPROM_DEBUG_USER_7` (4076) - dodano_komende_wyslij (0/1)

### Debug Ogólne
- `EEPROM_DEBUG_START` (4050)
- `EEPROM_DEBUG_LICZNIK_RESETOW` (4060)

## Inne Adresy (tylko C)

| Funkcja | Adres | Opis |
|---------|-------|------|
| `EEPROM_USTAWIENIE_STANOW_WYJSC` | 5 | Ustawienie stanów wyjść |
| `EEPROM_USTAWIENIE_WYJSCIA` | 6 | Ustawienie wyjścia |
| `ADRES_EEPROM_SKRYBA_TRYB_BACKUP` | 4088 | Backup poprzedniego trybu |

## Podsumowanie

### ✅ Co działa poprawnie:
1. **Kod dostępu** - Python i C używają tego samego zakresu (1-4)
2. **Tryb pracy** - Adres 4094 jest zgodny
3. **Funkcja Skryba** - Adres 4089 jest zgodny
4. **Kontrola czasu** - Wszystkie 4 adresy (4090-4093) są zgodne
5. **Numery telefonów** - Zakres 8-4007 dla 800 numerów jest zgodny

### ⚠️ Różnice:
1. **Status sterownika** (adres 1022):
   - Python: Definiuje i próbuje odczytać/zapisać
   - C: **Nie używa** tego adresu w AC800
   - **Rozwiązanie**: Python ukrywa tę funkcję dla AC800 w GUI

### 📊 Wykorzystanie EEPROM (4096 bajtów):
- **0-7**: Kod dostępu + ustawienia wyjść
- **8-4007**: Numery telefonów (800 × 5 bajtów)
- **4050-4076**: Debug (tylko C)
- **4088-4094**: Konfiguracja (Skryba, Czas, Tryb)
- **Wolne**: ~11 bajtów

## Format Danych

### Tryb Pracy (adres 4094)
- Python: `0x00` = Private, `0x01` = Public
- C: `0` = Private, `1` = Public
- ✅ Zgodne

### Funkcja Skryba (adres 4089)
- Python: `0x00` = Wyłączona, `0x01` = Włączona
- C: `0` = Wyłączona, `1` = Włączona
- ✅ Zgodne

### Czas (adresy 4090-4093)
- Format: Wartości dziesiętne (0-23 dla godzin, 0-59 dla minut)
- ✅ Zgodne
