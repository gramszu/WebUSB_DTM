# Rozwiązanie problemu z lokalnym uruchomieniem konfiguratora

## Problem
Gdy uruchamiasz `index.html` bezpośrednio przez przeglądarkę (protokół `file://`), brakuje plików CSS, JS i grafik. Strona nie działa poprawnie.

## Przyczyna
Kod HTML używa **ścieżek bezwzględnych** (zaczynających się od `/`):
```html
<script src="/core/intro.js"></script>
<link href="/style/style.css" rel="stylesheet">
```

### Dlaczego to nie działa lokalnie?
- **Na serwerze (Firebase/Hosting):** `/` = główny katalog strony (np. `https://twoja-strona.pl/`)
- **Lokalnie (`file://`):** `/` = główny katalog dysku (np. `/` na macOS)
  
Przeglądarka szuka `file:///core/intro.js` zamiast `file:///Users/.../public_html/core/intro.js`

### Dlaczego działa z ZIP lub Firebase?
- **Firebase:** Jest serwerem WWW, więc `/` działa poprawnie
- **ZIP (wersja offline):** Prawdopodobnie ma zmienione ścieżki na względne lub uruchamia lokalny serwer

## Rozwiązanie

### Opcja 1: Lokalny serwer HTTP (ZALECANE)
Uruchom prosty serwer HTTP w katalogu `public_html`:

```bash
cd /Users/robert/Desktop/ATMEGA328-AC200/public_html
python3 -m http.server 8000
```

Następnie otwórz w przeglądarce:
```
http://localhost:8000
```

**Zalety:**
- ✅ Nie wymaga zmian w kodzie
- ✅ Działa identycznie jak na Firebase
- ✅ Brak problemów z CORS
- ✅ Wszystkie pliki CSV, JS, CSS ładują się poprawnie

### Opcja 2: Zmiana ścieżek na względne (NIE ZALECANE)
Możesz zmienić wszystkie ścieżki w `index.html` z `/core/...` na `core/...`, ale:
- ❌ Wymaga modyfikacji wielu plików
- ❌ Może zepsuć wersję na Firebase
- ❌ Trudne w utrzymaniu

## Szybkie uruchomienie

### macOS/Linux:
```bash
cd public_html
python3 -m http.server 8000
```

### Alternatywy:
```bash
# Node.js (jeśli zainstalowany)
npx serve

# PHP (jeśli zainstalowany)
php -S localhost:8000
```

## Zatrzymanie serwera
Naciśnij `Ctrl+C` w terminalu.
