# 🔧 Komenda C3D4 - Reset Wszystkich Ustawień

**Komenda:** `C3D4` (bez kodu dostępu!)  
**Typ:** Komenda awaryjna / Factory Reset  
**Priorytet:** Najwyższy

---

## 📋 Opis

Komenda **C3D4** to specjalna komenda resetująca **wszystkie ustawienia** urządzenia do wartości fabrycznych. Jest to jedyna komenda, która **NIE wymaga kodu dostępu**.

---

## ⚙️ Zakres Działania

### ✅ Co Zostaje Zresetowane:

1. **Kod dostępu** → `ABCD` (domyślny)
2. **Wszystkie numery telefonów** → Usunięte (0xFF)
3. **Tryb pracy** → `OPEN CLIP` (publiczny)
4. **Funkcja SKRYBA** → `OFF`
5. **Funkcja TIME** → `OFF` (0xFF)
6. **Blokada systemu** → `Aktywny` (odblokowany)
7. **Mój numer** → Pusty (0xFF)
8. **Stany wyjść** → Domyślne

### ❌ Co NIE Zostaje Zresetowane:

- Firmware (kod programu)
- Bootloader
- Fuse bits

---

## 🔐 Bezpieczeństwo

**UWAGA:** Komenda C3D4 **NIE wymaga kodu dostępu!**

**Powód:**
- Komenda awaryjna na wypadek zapomnienia kodu
- Ostatnia deska ratunku przed przeprogramowaniem

**Zabezpieczenia:**
1. Wymaga wysłania SMS z dokładnym tekstem: `C3D4`
2. Urządzenie mruga **25 razy** LED-em (potwierdzenie)
3. Reset następuje natychmiast

---

## 📱 Użycie

### Składnia
```
C3D4
```

**Przykład:**
```
SMS: C3D4
Odpowiedź: (25 błysków LED)
Rezultat: Wszystkie ustawienia zresetowane
```

---

## 🔄 Proces Resetowania

### Sekwencja Komend (wewnętrzna)

Komenda C3D4 uruchamia sekwencję resetowania:

```c
KOMENDA_KOLEJKI_RESET_USTAWIEN_0
KOMENDA_KOLEJKI_RESET_USTAWIEN_1
KOMENDA_KOLEJKI_RESET_USTAWIEN_2
...
KOMENDA_KOLEJKI_RESET_USTAWIEN_INICJALIZACJA
```

### Krok po Kroku

1. **Odbiór SMS:** `C3D4`
2. **Weryfikacja:** Sprawdzenie czy to dokładnie `C3D4`
3. **Potwierdzenie:** 25 błysków LED
4. **Reset:**
   - Czyszczenie EEPROM (0xFF)
   - Ustawienie domyślnego kodu: `ABCD`
   - Ustawienie trybu: `OPEN CLIP`
   - Wyłączenie SKRYBA i TIME
5. **Restart:** Urządzenie gotowe do pracy

---

## 📊 Porównanie: Przed vs Po

| Parametr | Przed C3D4 | Po C3D4 |
|----------|------------|---------|
| Kod dostępu | Dowolny (np. 1234) | **ABCD** |
| Numery | 0-200 | **Wszystkie usunięte** |
| Tryb | Dowolny | **OPEN CLIP** |
| SKRYBA | ON/OFF | **OFF** |
| TIME | Dowolny | **OFF** |
| Super User | 195-200 | **Usunięci** |
| Blokada | Dowolna | **Aktywny** |

---

## ⚠️ Kiedy Używać?

### ✅ Użyj C3D4 gdy:
- Zapomniałeś kodu dostępu
- Urządzenie jest zablokowane (STOP)
- Chcesz przywrócić ustawienia fabryczne
- Sprzedajesz/przekazujesz urządzenie
- Testowanie po naprawie

### ❌ NIE używaj C3D4 gdy:
- Chcesz tylko zmienić kod (użyj `ABCD CODE xxxx`)
- Chcesz usunąć tylko niektóre numery (użyj `ABCD DEL`)
- Chcesz wyłączyć TIME (użyj `ABCD TIME OFF`)

---

## 🔒 Alternatywne Metody Resetu

### 1. Reset przez Kod Dostępu
```
ABCD XXXX    → Reset z kodem (wymaga znajomości kodu)
```

### 2. Reset przez Programator
```bash
# Wgraj domyślny EEPROM
avrdude -p m328pb -c usbasp -U eeprom:w:default_eeprom_AC200.hex:i
```

### 3. Reset Fizyczny
- Przycisk RESET na płytce (tylko restart, nie reset ustawień)

---

## 📝 Przykładowe Scenariusze

### Scenariusz 1: Zapomniany Kod
```
Problem: Kod dostępu zmieniony na 1234, ale zapomniany
Rozwiązanie:
1. Wyślij SMS: C3D4
2. Urządzenie resetuje się (25 błysków)
3. Nowy kod: ABCD
4. Skonfiguruj ponownie
```

### Scenariusz 2: Sprzedaż Urządzenia
```
Przed sprzedażą:
1. Wyślij SMS: C3D4
2. Wszystkie numery usunięte
3. Kod: ABCD (domyślny)
4. Tryb: OPEN CLIP
5. Urządzenie gotowe dla nowego właściciela
```

### Scenariusz 3: Urządzenie Zablokowane
```
Problem: Wysłano ABCD STOP, kod zapomniany
Rozwiązanie:
1. Wyślij SMS: C3D4 (bez kodu!)
2. Blokada usunięta
3. Kod: ABCD
4. Dostęp przywrócony
```

---

## 🛡️ Zabezpieczenia

### Dlaczego C3D4 nie wymaga kodu?

**Argument ZA:**
- Ostatnia deska ratunku
- Zapobiega konieczności przeprogramowania
- Użytkownik może odzyskać dostęp

**Argument PRZECIW:**
- Potencjalne zagrożenie bezpieczeństwa
- Ktoś może zresetować urządzenie SMS-em

**Kompromis:**
- Komenda jest nieoczywista (`C3D4`, nie `RESET`)
- Wymaga dokładnego tekstu
- Nie jest dokumentowana w instrukcji użytkownika
- Znana tylko serwisowi/administratorowi

---

## 📖 Definicja w Kodzie

**Plik:** `interpretacjaSMS.h`
```c
#define INSTRUKCJA_SMS_RESET_WSZYSTKICH_USTAWIEN "C3D4"
#define INTERPRETACJA_SMS_RESET_WSZYSTKICH_USTAWIEN 11
```

**Plik:** `main.c` (linie 599-606)
```c
case INTERPRETACJA_SMS_RESET_WSZYSTKICH_USTAWIEN: {
  if (not czy_sa_komendy_z_przedzialu(
          KOMENDA_KOLEJKI_RESET_KOD_DOSTEPU,
          KOMENDA_KOLEJKI_RESET_USTAWIEN_INICJALIZACJA)) {
    zapal_diode_led_blyski(25);  // 25 błysków = potwierdzenie
    dodaj_komende(KOMENDA_KOLEJKI_RESET_USTAWIEN_0);
  }
  break;
}
```

---

## ✅ Podsumowanie

| Właściwość | Wartość |
|------------|---------|
| **Komenda** | `C3D4` |
| **Kod dostępu** | NIE wymagany |
| **Zakres** | Wszystkie ustawienia EEPROM |
| **Potwierdzenie** | 25 błysków LED |
| **Nieodwracalne** | TAK |
| **Czas wykonania** | ~5 sekund |
| **Bezpieczeństwo** | Średnie (brak kodu) |

---

*Dokumentacja wygenerowana: 2025-12-22*  
*Model: AC200-DTM-F2*  
*Wersja firmware: 1.0*
