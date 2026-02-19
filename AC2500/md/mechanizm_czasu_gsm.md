# Mechanizm Automatycznej Synchronizacji Czasu GSM

Dokument opisuje wdrożony mechanizm automatycznej aktualizacji czasu systemowego w sterowniku bramy, wykorzystujący sieć GSM (funkcja NITZ).

## Zasada Działania

Mechanizm opiera się na dwóch filarach:
1.  **Synchronizacja Modułu GSM z Siecią (NITZ)**
2.  **Cykliczny Odczyt Czasu przez Mikrolontroler**

### 1. Synchronizacja Modułu GSM (NITZ)

Podczas inicjalizacji modułu GSM (w sekwencji startowej, przy sprawdzaniu PIN), wysyłana jest komenda:
```
AT+CLTS=1
```
Komenda ta włącza funkcję **NITZ (Network Identity and Time Zone)** w modemie SIM900/SIM800.
- Gdy modem loguje się do sieci operatora, otrzymuje od stacji bazowej pakiety z aktualną datą i czasem.
- Modem automatycznie aktualizuje swój wewnętrzny zegar (RTC) na podstawie tych danych.

### 2. Cykliczny Odczyt Czasu

Mikrokontroler (ATmega) nie posiada własnego podtrzymywanego zegara RTC, dlatego polega na zegarze modułu GSM.
- W pętli głównej programu (`main_sim900.h`, funkcja `steruj_SIM900_100MS`) zaimplementowany jest licznik.
- Co **8 sekund** (gdy `licznik_cyklu_8_sek` osiągnie 80) wysyłane jest zapytanie do modemu:
  ```
  AT+CCLK?
  ```
- Modem zwraca aktualny czas w formacie: `+CCLK: "yy/MM/dd,hh:mm:ss+zz"`.
- Mikrokontroler parsuje tę odpowiedź i aktualizuje swoje zmienne systemowe używane w harmonogramie (blokada czasowa).

## Ręczne Ustawianie Czasu (Komenda SET)

Komenda SMS `ABCD SET GG:MM:SS` (np. `ABCD SET 14:30:00`) została zachowana jako mechanizm awaryjny.
- Pozwala ona ręcznie nadpisać czas w modemie GSM.
- **Uwaga**: Jeśli funkcja NITZ jest aktywna, ręcznie ustawiony czas może zostać ponownie nadpisany przez sieć przy następnym przelogowaniu lub restarcie modułu.

## Możliwe Problemy i Ograniczenia

Automatyczna synchronizacja może nie zadziałać w następujących przypadkach:

1.  **Brak obsługi po stronie sieci**: Niektóre starsze stacje bazowe lub specyficzni operatorzy wirtualni mogą nie wysyłać ramek czasu NITZ. W takim przypadku zegar modemu będzie startował od domyślnej wartości (zazwyczaj 00:00:00) po każdym resecie zasilania.
2.  **Brak zasięgu**: Jeśli urządzenie nie może zalogować się do sieci GSM, nie otrzyma czasu.
3.  **Opóźnienie startowe**: Czas jest dostępny dopiero po poprawnym zalogowaniu do sieci. Przez pierwsze kilkadziesiąt sekund po włączeniu zasilania, czas systemowy może być nieaktualny.
4.  **Roaming**: W niektórych przypadkach roamingu krajowego lub zagranicznego, przekazywanie czasu może być zablokowane.
