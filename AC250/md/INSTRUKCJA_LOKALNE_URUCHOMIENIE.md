# Instrukcja uruchamiania konfiguratora lokalnie

## Szybki start

### 1. Otwórz terminal
- Naciśnij `Cmd + Spacja`, wpisz "Terminal" i naciśnij Enter

### 2. Przejdź do katalogu projektu
```bash
cd ~/Desktop/ATMEGA328-AC200/public_html
```

### 3. Uruchom serwer lokalny
```bash
python3 -m http.server 8000
```

Zobaczysz komunikat:
```
Serving HTTP on :: port 8000 (http://[::]:8000/) ...
```

### 4. Otwórz w przeglądarce
Wpisz w pasku adresu:
```
http://localhost:8000
```

### 5. Zatrzymanie serwera
Gdy skończysz pracę, naciśnij w terminalu:
```
Ctrl + C
```

---

## Dostęp z innych urządzeń (telefon, tablet, drugi komputer)

### 1. Sprawdź swój adres IP
W nowym oknie terminala:
```bash
ifconfig | grep "inet " | grep -v 127.0.0.1
```

Zobaczysz coś w stylu:
```
inet 192.168.1.18 netmask ...
```

Twój adres IP to: `192.168.1.18`

### 2. Uruchom serwer z dostępem sieciowym
```bash
cd ~/Desktop/ATMEGA328-AC200/public_html
python3 -m http.server 8000 --bind 0.0.0.0
```

### 3. Otwórz na innym urządzeniu
W przeglądarce na telefonie/tablecie wpisz:
```
http://192.168.1.18:8000
```
*(Zastąp `192.168.1.18` swoim adresem IP)*

**Uwaga:** Oba urządzenia muszą być w tej samej sieci WiFi!

---

## Adresy dostępu

| Skąd łączysz | Adres |
|--------------|-------|
| Ten sam komputer | `http://localhost:8000` |
| Ten sam komputer (alternatywnie) | `http://127.0.0.1:8000` |
| Inny komputer/telefon w sieci | `http://192.168.1.18:8000` |

---

## Rozwiązywanie problemów

### Port 8000 jest zajęty
Jeśli zobaczysz błąd `Address already in use`, użyj innego portu:
```bash
python3 -m http.server 8001
```
Wtedy otwórz: `http://localhost:8001`

### Serwer nie odpowiada
1. Sprawdź, czy terminal z serwerem nadal działa
2. Upewnij się, że jesteś w katalogu `public_html`
3. Sprawdź firewall (System Preferences → Security → Firewall)

### Nie mogę połączyć się z telefonu
1. Upewnij się, że użyłeś flagi `--bind 0.0.0.0`
2. Sprawdź, czy telefon i komputer są w tej samej sieci WiFi
3. Sprawdź firewall na komputerze

---

## Dlaczego nie mogę otworzyć pliku bezpośrednio?

Gdy otwierasz `index.html` bezpośrednio (przez `file://`), przeglądarka:
- ❌ Nie może załadować plików z ścieżkami bezwzględnymi (`/core/intro.js`)
- ❌ Blokuje niektóre funkcje JavaScript ze względów bezpieczeństwa
- ❌ Nie ładuje stylów CSS i grafik

Lokalny serwer HTTP rozwiązuje te problemy! ✅
