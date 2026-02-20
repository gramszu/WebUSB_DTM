# EEPROM - Domyślna Konfiguracja

## Plik EEPROM przy Upload

**Plik:** `default_eeprom_AC200.hex`  
**Zawartość:** PUSTY (tylko znacznik końca: `:00000001FF`)

## Auto-Naprawa przy Pierwszym Uruchomieniu

Firmware **automatycznie** ustawia domyślne wartości jeśli EEPROM jest puste:

### Kod Dostępu
```c
// main.c linie 1652-1663
if (kod_modulu[0] == 0xFF || kod_modulu[0] == 0x00) {
    kod_modulu = "ABCD";  // Automatycznie ustawione
    eeprom_update_block(...);  // Zapisane do EEPROM
}
```

**Domyślny kod:** `ABCD`

### Tryb Pracy
```c
// main.c linie 1667-1673
if (tryb_pracy == 0xFF) {
    tryb_pracy = 1;  // Publiczny
    eeprom_update_byte(...);
}
```

**Domyślny tryb:** Publiczny (1)

### Tryb CLIP/DTMF
```c
// main.c linie 1676-1682
if (tryb_clip == 0xFF) {
    tryb_clip = 1;  // CLIP
    eeprom_update_byte(...);
}
```

**Domyślny tryb:** CLIP (1)

### Harmonogram (TIME)
```c
// main.c linie 1714-1735
if (czas_start_h > 23 && czas_start_h != 0xFF) {
    // Ustaw 0xFF = wyłączony
}
```

**Domyślnie:** Wyłączony (0xFF)

### Inne Wartości

| Parametr | Adres EEPROM | Domyślna Wartość | Opis |
|----------|--------------|------------------|------|
| Kod dostępu | 0x0001-0x0004 | `ABCD` | Auto-naprawa |
| Tryb pracy | 0x03F0 | `1` (Publiczny) | Auto-naprawa |
| CLIP/DTMF | 0x03F1 | `1` (CLIP) | Auto-naprawa |
| Skryba | 0x03F2 | `0` (Wyłączona) | - |
| Skryba limit | 0x03F4-0x03F5 | `200` | Jeśli 0xFF |
| Blokada | 0x03F6 | `0` (Odblokowany) | Jeśli 0xFF |
| TIME start | 0x03F7-0x03F8 | `0xFF` (Wyłączony) | - |
| TIME stop | 0x03F9-0x03FA | `0xFF` (Wyłączony) | - |
| Mój numer | 0x03FB-0x03FF | `0xFF` (Pusty) | - |
| Numery telefonów | 0x0008-0x03EF | `0xFF` (Puste) | 200 pozycji |

## Podsumowanie

✅ **Nie musisz** przygotowywać pliku EEPROM z kodem dostępu  
✅ **Firmware sam** ustawi `ABCD` przy pierwszym uruchomieniu  
✅ **Wszystkie** kluczowe parametry mają auto-naprawę  

## Jak Zmienić Domyślny Kod?

Jeśli chcesz inny domyślny kod niż `ABCD`, musisz:

1. Edytować `main.c` linie 1654-1657:
```c
kod_modulu[0] = bufor_eeprom[0] = 'W';  // Zamiast 'A'
kod_modulu[1] = bufor_eeprom[1] = 'X';  // Zamiast 'B'
kod_modulu[2] = bufor_eeprom[2] = 'Y';  // Zamiast 'C'
kod_modulu[3] = bufor_eeprom[3] = 'Z';  // Zamiast 'D'
```

2. Przekompilować firmware
3. Wgrać nowy firmware

**Uwaga:** To zmieni domyślny kod dla **wszystkich** nowych urządzeń!
