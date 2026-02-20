# Instrukcja: Aktualizacja avr-gcc z zachowaniem wsparcia ATmega328PB

Po aktualizacji `avr-gcc@9` przez Homebrew, konieczne jest ponowne dodanie wsparcia dla ATmega328PB, ponieważ Homebrew nadpisuje systemowe pliki.

## Kroki do wykonania po aktualizacji

### 1. Aktualizacja avr-gcc
```bash
brew update
brew upgrade avr-gcc@9
```

### 2. Ponowna instalacja wsparcia ATmega328PB

#### Krok 2.1: Skopiowanie plików nagłówkowych
```bash
sudo cp m328pb_support/include/avr/iom328pb.h \
  /opt/homebrew/Cellar/avr-gcc@9/*/avr/include/avr/
```

#### Krok 2.2: Skopiowanie bibliotek
```bash
sudo cp m328pb_support/gcc/dev/atmega328pb/crtatmega328pb.o \
  /opt/homebrew/Cellar/avr-gcc@9/*/avr/lib/avr5/

sudo cp m328pb_support/gcc/dev/atmega328pb/libatmega328pb.a \
  /opt/homebrew/Cellar/avr-gcc@9/*/avr/lib/avr5/
```

#### Krok 2.3: Modyfikacja io.h
Dodaj następujące linie **przed** linią zawierającą `#elif defined (__AVR_ATmega328P__)`:

```bash
sudo sed -i.bak '/^#elif defined (__AVR_ATmega328P__)/i\
#elif defined (__AVR_ATmega328PB__)\
#  include <avr/iom328pb.h>
' /opt/homebrew/Cellar/avr-gcc@9/*/avr/include/avr/io.h
```

**Lub ręcznie:**
1. Otwórz plik: `/opt/homebrew/Cellar/avr-gcc@9/*/avr/include/avr/io.h`
2. Znajdź linię: `#elif defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__)`
3. **Przed** tą linią dodaj:
   ```c
   #elif defined (__AVR_ATmega328PB__)
   #  include <avr/iom328pb.h>
   ```

### 3. Weryfikacja
```bash
cd uC_source
export PATH="/opt/homebrew/opt/avr-gcc@9/bin:$PATH"
make clean && make all
```

Kompilacja powinna zakończyć się bez ostrzeżeń i błędów.

## Uwagi
- Symbol `*` w ścieżkach oznacza wersję (np. `9.5.0`). Można użyć wildcarda lub podstawić konkretną wersję.
- Backup pliku `io.h` zostanie utworzony automatycznie jako `io.h.bak`.
- Wszystkie pliki wsparcia znajdują się w katalogu `m328pb_support/` w głównym katalogu projektu.

## Sprawdzenie wersji
```bash
avr-gcc --version
```

## Lokalizacja plików wsparcia
- Nagłówek: `m328pb_support/include/avr/iom328pb.h`
- Biblioteki: `m328pb_support/gcc/dev/atmega328pb/`
