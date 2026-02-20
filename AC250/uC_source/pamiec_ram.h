#include "narzedzia.h"

#define MAX_LICZBA_WYSYLANYCH_ZNAKOW_SIM900 320
extern uchar wysylany_blok_SIM900[MAX_LICZBA_WYSYLANYCH_ZNAKOW_SIM900];
// Limity SMS:
// - odbior (dekodowany tekst z PDU) trzymamy krótki, bo komendy są krótkie
// - wysylka pozostaje pełna 160 znaków
#define MAX_LICZBA_ZNAKOW_SMS_ODBIOR 30
#define MAX_LICZBA_ZNAKOW_SMS 160
// extern uchar tekst_odebranego_smsa[MAX_LICZBA_ZNAKOW_SMS_ODBIOR+1];
#define tekst_odebranego_smsa                                                  \
  (&wysylany_blok_SIM900[MAX_LICZBA_WYSYLANYCH_ZNAKOW_SIM900 -                 \
                         MAX_LICZBA_ZNAKOW_SMS_ODBIOR - 1])
extern uchar tekst_wysylanego_smsa[MAX_LICZBA_ZNAKOW_SMS + 1];
// extern uchar tekst_wysylanego_echa[MAX_LICZBA_ZNAKOW_SMS+1];
