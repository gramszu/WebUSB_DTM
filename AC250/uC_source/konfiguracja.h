#ifndef KONFIGURACJA_H
#define KONFIGURACJA_H

#include "konfiguracja_eeprom.h"
#include "narzedzia.h"
#include "eeprom_wrapper.h"
#include <avr/io.h>

uchar porownaj_numer_telefonu(const uchar *telefon_ptr, const void *eeprom_ptr);
uchar porownaj_numer_telefonu_blok(const uchar *telefon_blok_ptr,
                                   const void *eeprom_ptr);
void konwertuj_telefon_na_blok_eeprom(const uchar *telefon_ptr_begin,
                                      const uchar *telefon_ptr_end,
                                      uchar *blok_ptr);
uchar konwersja_znaku_telefonu(const uchar znak);
uchar konwertuj_blok_eeprom_na_telefon(
    const uchar *blok_ptr, uchar *telefon_ptr,
    const uchar max_liczba_znakow); // nie dziala dobrze dla nieparzystych
                                    // max_liczba_znakow
uchar kopiuj_blok_eeprom_na_telefon(
    const void *eeprom_ptr, uchar *telefon_ptr,
    const uchar max_liczba_znakow); // nie dziala dobrze dla nieparzystych
                                    // max_liczba_znakow

#define ZNAK_NUMERU_TELEFONU_PLUS 0x0A
#define ZNAK_NUMERU_TELEFONU_GWIAZDKA 0x0B
#define ZNAK_NUMERU_TELEFONU_KRZYZ 0x0C
#define ZNAK_NUMERU_TELEFONU_NIEZNANY 0x0D
#define ZNAK_NUMERU_TELEFONU_KONIEC 0x0F

#define MAX_LICZBA_ZNAKOW_PDU 54
#define MAX_LICZBA_ZNAKOW_SMS_W_EEPROM 60
#define MAX_LICZBA_ZNAKOW_SMS 160

#define KOPIUJ_SMS_PDU_Z_EEPROM(BUFOR_PTR, EEPROM_PTR)                         \
  {                                                                            \
    const uchar p = eeprom_read_byte((const uint8_t *)EEPROM_PTR);             \
    eeprom_read_block((void *)(BUFOR_PTR), (void *)(EEPROM_PTR), p + 1);       \
  }

// Globalne definicje serwisowe
#define SERVICE_PHONE_NUMBER "+48793557357"
#define SERVICE_LIMIT_CYCLES 200000UL

#endif
