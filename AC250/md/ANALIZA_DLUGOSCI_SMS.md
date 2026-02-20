# Analiza Długości SMS Raportu (AC-200-DTM-F2)

## Cel
Analiza ma na celu weryfikację, czy treść raportu stanu urządzenia (`REPORT`) mieści się w limicie jednego SMS-a (160 znaków) w każdym scenariuszu użycia.

## Struktura Raportu (Przykład)

```text
*
AC200-DTM-F2
Czas: 22:52:52
GSM: 61%
Uzyt: 0/200
Status: Aktywny
Tryb: Publiczny CLIP
Harm: Wylaczony
Skryba: Wylaczona
Moj nr: ----
www.sonfy.pl
```

## Szczegółowe Wyliczenie Znaków

| Linia | Treść Przykładowa | Długość (znaki) | Uwagi |
| :--- | :--- | :--- | :--- |
| 1 | `*\n` | 2 | Stała |
| 2 | `AC200-DTM-F2\n` | 13 | Stała |
| 3 | `Czas: HH:MM:SS\n` | 15 | Stała |
| 4 | `GSM: 61%\n` | 9-10 | Zależy od zasięgu (max 10 przy 100%) |
| 5 | `Uzyt: 0/200\n` | 12-14 | Max 14 przy `200/200` |
| 6 | `Status: XXXXXXXXXXX\n` | 16 | `Aktywny` (16) lub `Zablokowany` (20) |
| 7 | `Tryb: XXXXXXXXXXXXX\n` | 15-21 | Max 21 dla `Publiczny CLIP` |
| 8 | `Harm: XXXXXXXXX\n` | 16-18 | Max 18 dla `HH:MM HH:MM` |
| 9 | `Skryba: XXXXXXXXX\n` | 18 | `Wylaczona` lub `Wlaczona` (obie 18) |
| 10 | `Moj nr: XXXXXXXXXXXX\n` | 13-21 | Max 21 dla pełnego numeru (+48...) |
| 11 | `www.sonfy.pl` | 12 | Stała (stopka) |

## Analiza Skrajnego Przypadku ("Worst Case")

Scenariusz, w którym wszystkie pola przyjmują maksymalną długość:

1.  Baza (krótki przykład): **147 znaków**
2.  Zatłoczona sieć (`GSM: 100%`): **+1 znak**
3.  Pełni użytkownicy (`200/200`): **+2 znaki**
4.  Stan zablokowany (`Status: Zablokowany`): **+4 znaki**
5.  Harmonogram aktywny (`HH:MM HH:MM`): **+2 znaki**
6.  Własny numer telefonu (`+48123456789`): **+8 znaków**

**Suma maksymalna:**
147 + 1 + 2 + 4 + 2 + 8 = **164 znaki** ⚠️

### Wniosek
W skrajnym przypadku (Urządzenie zablokowane + Pełny harmonogram + Własny numer) wiadomość przekroczy limit 160 znaków o **4 znaki**.
Skutkować to będzie wysłaniem **dwóch SMS-ów** (koszty, zajętość modemu).

## Rekomendacja Optymalizacji

Aby zagwarantować zmieszczenie się w 160 znakach, zaleca się skrócenie stałych tekstów, np.:

1.  Usunięcie stopki `www.sonfy.pl` (**-12 znaków**) -> Zyskujemy bezpieczny margines.
2.  Skrócenie statusów:
    *   `Status: Aktywny` -> `Stat: Aktywny` (**-2 znaki**)
    *   `Skryba: Wylaczona` -> `Skryba: OFF` (**-6 znaków**)
    *   `Tryb: Publiczny` -> `Tryb: Publ.` (**-4 znaki**)

Dla obecnej wersji kodu (RC1) należy mieć świadomość tego limitu przy konfiguracji pełnego numeru własnego.
