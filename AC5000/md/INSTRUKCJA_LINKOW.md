# Instrukcja zmiany linków do ikonek w stopce

Ikonki znajdują się w pliku `SKIN_GUI/index.html` w sekcji `<footer>`.
Poniżej znajdują się numery linii, w których należy podmienić adres URL (wartość atrybutu `href="..."`).

## Lista ikon i numery linii

1.  **YouTube (Ikona Play - Czerwona)**
    *   **Linia:** ~254
    *   **Obecny link:** `https://www.youtube.com/@sonfy_pl/videos`
    *   **Szukaj kodu:** `<a href="https://www.youtube.com/@sonfy_pl/videos" ...`

2.  **Instrukcja Obsługi (Ikona PDF)**
    *   **Linia:** ~260
    *   **Obecny link:** `#` (Pusty/Placeholder)
    *   **Szukaj kodu:** `<a href="#" class="resource-link ... title="Instrukcja Obsługi (PDF)">`
    *   **Akcja:** Zmień `#` na adres pliku PDF (np. `https://twoja-strona.pl/instrukcja.pdf`).

3.  **Aplikacja Android (Ikona Robota - Zielona)**
    *   **Linia:** ~263
    *   **Obecny link:** `https://play.google.com/store/apps/details?id=megaelektronik.bramster_sonfy`
    *   **Szukaj kodu:** `<a href="https://play.google.com/store/apps/details?id=megaelektronik.bramster_sonfy" ...`

4.  **Aplikacja iOS (Ikona Telefonu - Czarna)**
    *   **Linia:** ~266
    *   **Obecny link:** `#` (Pusty/Placeholder)
    *   **Szukaj kodu:** `<a href="#" class="resource-link ... title="Aplikacja iOS">`
    *   **Akcja:** Zmień `#` na link do App Store.

5.  **Sklep Online (Ikona Koszyka - Niebieska)**
    *   **Linia:** ~271 (lub kolejna po iOS)
    *   **Obecny link:** `https://megaelektronik.pl`
    *   **Szukaj kodu:** `<a href="https://megaelektronik.pl" ...`

## Jak zmienić?

1.  Otwórz plik `SKIN_GUI/index.html` w edytorze tekstu (Notatnik, VS Code itp.).
2.  Znajdź odpowiednią linię (użyj funkcji szukaj `Ctrl+F` i wpisz fragment linku lub tytułu np. "Instrukcja").
3.  Podmień tekst w cudzysłowie po `href=`.
4.  Zapisz plik.

> **Uwaga:** Ikony posiadają zabezpieczenie (`class="offline-aware"`). Jeśli użytkownik nie ma internetu, link się nie otworzy i wyświetli się komunikat.
