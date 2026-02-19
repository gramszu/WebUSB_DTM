#ifndef KOMENDY_H_INCLUDE
#define KOMENDY_H_INCLUDE

#include "enumkomendy.h"
#include "narzedzia.h"

/*
typedef enum {
        KOMENDA_KOLEJKI_BRAK_KOMENDY,
        KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT,
    // ... (usuniete, bo sa w enumkomendy.h)
} komenda_typ;
*/

#define LICZBA_KOMEND 10
extern komenda_typ komendy_kolejka[LICZBA_KOMEND];

void dodaj_komende(komenda_typ komenda);
void filtruj_i_dodaj_komende(komenda_typ komenda);
void filtruj_komendy_z_przedzialu(komenda_typ start, komenda_typ end);
void usun_komende(void);
uchar czy_sa_komendy_z_przedzialu(komenda_typ start, komenda_typ end);
// void usun_komende_z_kolejki(komenda_typ komenda);

extern char rtc_czas[9];
extern char bufor_ustaw_czas[30];

// Zmienne do obslugi blokady czasowej
// extern uchar blokada_sterowania_czasowa; // REMOVED
// extern uchar czas_start_h, czas_start_m; // REMOVED
// extern uchar czas_stop_h, czas_stop_m;   // REMOVED

#endif
