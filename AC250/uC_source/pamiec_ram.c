#include "pamiec_ram.h"

uchar wysylany_blok_SIM900[MAX_LICZBA_WYSYLANYCH_ZNAKOW_SIM900];
// uchar tekst_odebranego_smsa[MAX_LICZBA_ZNAKOW_SMS+1];
uchar tekst_wysylanego_smsa[MAX_LICZBA_ZNAKOW_SMS + 1];
// uchar tekst_wysylanego_echa[MAX_LICZBA_ZNAKOW_SMS+1];

// CON - powiadomienia o otwarciach bramy (tylko RAM, nie EEPROM)
uchar numer_powiadomien[17];     // MAX_LICZBA_ZNAKOW_TELEFON + 1 = 17
uchar powiadomienia_aktywne = 0; // FALSE
