#ifndef ZAPISEEPROM_H
#define ZAPISEEPROM_H

#include "narzedzia.h"
#include <stdlib.h>

#ifndef MAX_BUFOR_EEPROM
#define MAX_BUFOR_EEPROM		1024
#endif

#ifndef POLOZENIE_BUFOR_EEPROM_POMOC_BUFOR_START
#define POLOZENIE_BUFOR_EEPROM_POMOC_BUFOR_START			80
#endif

extern uchar bufor_eeprom[MAX_BUFOR_EEPROM];

extern uchar liczba_znakow_do_zapisu;

extern uchar bufor_pomoc_liczba_znakow_do_zapisu;
extern uint bufor_pomoc_adres_eeprom;

#define czy_wolny_eeprom()	(bufor_pomoc_liczba_znakow_do_zapisu == 0 && liczba_znakow_do_zapisu == 0 \
	&& !czy_zajety_bufor_eeprom && eeprom_is_ready())	// wa¿na jest kolejnoœæ

	// uwaga, w³¹cza przerwania sei()
void zapisz_znaki_w_eeprom(const void* buf, const uint pozycja_w_eeprom_,  
	const uchar liczba_znakow);
	// uwaga, w³¹cza przerwania sei()
void zapisz_znaki_w_eeprom_bez_kopiowania(const uint pozycja_w_eeprom_, 
	const uchar liczba_znakow);
	// uwaga, w³¹cza przerwania sei()
void zapisz_znak_w_eeprom(const uchar znak, const uint pozycja_w_eeprom_);
	// uwaga, w³¹cza przerwania sei()
void zapisz_bajt_w_EEPROM(void);

#ifndef memcpy_E
#define memcpy_E(sink, source, l)		eeprom_read_block((sink), (void*) (source), (l))
#endif
int memcmp_E(const void* buf, const void* eeprom_ptr, size_t size);
void* memchr_E(void* eeprom_ptr, const char ch, size_t size);
int strncmp_E(const void* buf, const void* eeprom_ptr, size_t size);
int strncmp_any_char_E(const void* buf, const void* eeprom_ptr, size_t size);
int strncpy_E(const void* buf, const void* eeprom_ptr, size_t size);
int strnlen_E(const void* eeprom_ptr, size_t size);


extern uchar czy_zajety_bufor_eeprom;

extern uchar licznik_dostep_do_eeprom;	// usun¹æ

#ifndef WIN32

#include "eeprom_wrapper.h"

uint32_t eeprom_read_long(const uint32_t *addr);
uint32_t eeprom_read_3char(const uint32_t *addr);
#define eeprom_read_3uchar	eeprom_read_3char

#endif

#endif
//void odczytaj_znaki_z_eeprom(const uint pozycja_w_eeprom_, const uchar liczba_znakow);

