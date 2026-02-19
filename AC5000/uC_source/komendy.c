#include "komendy.h"
#include <string.h> // For memset

komenda_typ komendy_kolejka[LICZBA_KOMEND];

char rtc_czas[9] = "00:00:00";
char bufor_ustaw_czas[30];

// Inicjalizacja zmiennych blokady czasowej
// uchar blokada_sterowania_czasowa = FALSE; // REMOVED
// uchar czas_start_h = 0xFF, czas_start_m = 0xFF; // REMOVED
// uchar czas_stop_h = 0xFF, czas_stop_m = 0xFF;   // REMOVED

void dodaj_komende(komenda_typ komenda) {
  for (uchar i = 0; i < LICZBA_KOMEND; ++i) {
    if (komendy_kolejka[i] == KOMENDA_KOLEJKI_BRAK_KOMENDY) {
      komendy_kolejka[i] = komenda;
      return;
    }
  }
}

void filtruj_i_dodaj_komende(komenda_typ komenda) {
  for (uchar i = 0; i < LICZBA_KOMEND; ++i) {
    if (komendy_kolejka[i] == komenda)
      return;
    if (komendy_kolejka[i] == KOMENDA_KOLEJKI_BRAK_KOMENDY) {
      komendy_kolejka[i] = komenda;
      return;
    }
  }
}

void filtruj_komendy_z_przedzialu(komenda_typ start, komenda_typ end) {
  uchar i = 0;
  while (i < LICZBA_KOMEND) {
    if (komendy_kolejka[i] >= start && komendy_kolejka[i] <= end) {
      // Usun te komende (przesun reszte)
      for (uchar j = i; j < LICZBA_KOMEND - 1; ++j) {
        komendy_kolejka[j] = komendy_kolejka[j + 1];
      }
      komendy_kolejka[LICZBA_KOMEND - 1] = KOMENDA_KOLEJKI_BRAK_KOMENDY;
      // Nie inkrementujemy i, bo na to miejsce weszla nowa komenda
    } else {
      ++i;
    }
  }
}

void usun_komende(void) {
  for (uchar i = 1; i < LICZBA_KOMEND; ++i)
    komendy_kolejka[i - 1] = komendy_kolejka[i];
  komendy_kolejka[LICZBA_KOMEND - 1] = KOMENDA_KOLEJKI_BRAK_KOMENDY;
}

uchar czy_sa_komendy_z_przedzialu(komenda_typ start, komenda_typ end) {
  for (uchar i = 0; i < LICZBA_KOMEND; ++i) {
    if (komendy_kolejka[i] >= start && komendy_kolejka[i] <= end)
      return TRUE;
  }
  return FALSE;
}
