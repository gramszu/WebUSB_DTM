# Instrukcja Edycji Treści Dymków (Tooltips)

Poniżej znajduje się lista miejsc w kodzie, gdzie znajdują się teksty dymków podpowiedzi. Aby zmienić treść, należy edytować odpowiedni plik HTML i znaleźć wskazaną linię.

> **Uwaga:** Numery linii są przybliżone. Najlepiej szukać tekstu za pomocą `Ctrl+F` (lub `Cmd+F`).

## Plik: `SKIN_GUI/index.html` (Wersja Jasna/Nowa)

| Sekcja / Karta | Tekst (Fragment) | Przybliżona Linia |
| :--- | :--- | :--- |
| **Główny Grid** | `<div id="configGrid">` | ~70 |
| **Połączenie** | `Tu zarządzasz połączeniem USB...` | ~44 |
| **Kod dostępu** | `Ustaw 4-znakowy kod dostępu...` | ~79 |
| **Tryb sterowania** | `Wybierz w jaki sposób sterownik...` | ~88 |
| **Status sterownika** | `Włącz lub zablokuj całkowicie...` | ~99 |
| **Tryb pracy** | `Tryb Prywatny (tylko lista) lub...` | ~114 |
| **Funkcja Skryba** | `Funkcja rejestrująca historię...` | ~129 |
| **Konfiguracja wyjścia** | `Skonfiguruj czy wyjście ma być...` | ~145 |
| **Mój numer (MyNum)** | `Twój numer telefonu używany...` | ~166 |
| **Harmonogram** | `Ogranicz działanie sterownika...` | ~175 |
| **Przycisk Pobierz** | `Odczytuje aktualną konfigurację...` | ~213 |
| **Przycisk Wgraj** | `Zapisuje bieżącą konfigurację...` | ~217 |
| **Przycisk Eksport** | `Zapisuje ustawienia do pliku CSV...` | ~221 |
| **Przycisk Import** | `Zapisuje ustawienia z pliku CSV...` | ~225 |
| **Numery telefonów** | `Super User (1-6) - może sterowa...` | ~247 |
| **Przycisk Wyczyść** | `Usuwa historię logów z ekranu.` | ~294 |

## Plik: `GUI_WWW/index.html` (Wersja Podstawowa)

Teksty znajdują się w analogicznych miejscach jak w `SKIN_GUI/index.html`, zazwyczaj o kilka linii wcześniej lub później. Szukaj frazy `<div class="help-tooltip">`.

---

### Jak edytować?

1. Otwórz plik `index.html` w edytorze tekstowym (Notatnik, VS Code, itp.).
2. Wciśnij `Ctrl+F` i wpisz fragment tekstu, który chcesz zmienić (np. "Tu zarządzasz").
3. Zmień tekst wewnątrz znacznika `div`:
   ```html
   <div class="help-tooltip">TUTAJ WPISZ SWÓJ NOWY TEKST</div>
   ```
4. Zapisz plik (`Ctrl+S`).
5. Odśwież stronę w przeglądarce, aby zobaczyć zmiany.
