
#ifndef INTERPRETACJAGSM_H
#define INTERPRETACJAGSM_H

#include "konfiguracja.h"
#include "konfiguracja_eeprom.h"
#include "narzedzia.h"
#include "pamiec_ram.h"

#define INSTRUKCJA_SMS_RESET_WSZYSTKICH_USTAWIEN "C3D4"

#define INTERPRETACJA_SMS_BRAK_KODU 0
#define INTERPRETACJA_SMS_ZLY_FORMAT 1
#define INTERPRETACJA_SMS_BLEDNE_DANE 2
#define INTERPRETACJA_SMS_POPRAWNY 3
#define INTERPRETACJA_SMS_RAPORT 4
#define INTERPRETACJA_SMS_USER 5
#define INTERPRETACJA_SMS_USER_BEZ_NUMERU 6
#define INTERPRETACJA_SMS_USER_LIST 7
#define INTERPRETACJA_SMS_USER_ALL 8
#define INTERPRETACJA_SMS_DEBUG 9
#define INTERPRETACJA_SMS_POPRAWNY_KONFIGURACJA 10
#define INTERPRETACJA_SMS_RESET_WSZYSTKICH_USTAWIEN 11
#define INTERPRETACJA_SMS_CON_STATUS 12
#define INTERPRETACJA_SMS_CON_OFF 13
#define INTERPRETACJA_SMS_CON_ON 14
#define INTERPRETACJA_SMS_ON 15
#define INTERPRETACJA_SMS_STAN 16

uchar interpretuj_wiadomosc_sms(const uchar *sms);

extern uchar
    numer_telefonu_do_ktorego_dzwonic[MAX_LICZBA_ZNAKOW_TELEFON + 1];
extern uchar numer_telefonu_wysylanego_smsa[MAX_LICZBA_ZNAKOW_TELEFON + 1];

extern uchar kod_modulu[LICZBA_BAJTOW_KODU_DOSTEPU];
extern uchar blokada_systemu;
extern uchar tryb_pracy;

extern uchar tekst_wysylanego_smsa[MAX_LICZBA_ZNAKOW_SMS + 1];

#endif
