
#include "konfiguracja.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "zapiseeprom.h"

uchar konwersja_znaku_telefonu(const uchar znak)
{
	switch ( znak )
	{
	case '0': case '1': case '2': case '3': case '4': 
	case '5': case '6': case '7': case '8': case '9':
		return znak - '0';
	case '+': return ZNAK_NUMERU_TELEFONU_PLUS;
	case '*': return ZNAK_NUMERU_TELEFONU_GWIAZDKA;
	case '#': return ZNAK_NUMERU_TELEFONU_KRZYZ;
	default: return ZNAK_NUMERU_TELEFONU_NIEZNANY;
	}
}

void konwertuj_telefon_na_blok_eeprom(const uchar* telefon_ptr_begin,
	const uchar* telefon_ptr_end, uchar* blok_ptr)
{
	memset(blok_ptr, 0xff, LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM);
	uchar l = telefon_ptr_end - telefon_ptr_begin;
	if ( l < 2 )
		return;
	uchar pos_buf = 0;
	telefon_ptr_end--;
	while ( l-- )
	{
		uchar s1 = konwersja_znaku_telefonu(*telefon_ptr_end--);
		s1 <<= 4;
		s1 |= konwersja_znaku_telefonu(*telefon_ptr_end);
		if ( l == 0 )
			s1 |= 0x0f;
		else
			--telefon_ptr_end, --l;
		blok_ptr[pos_buf++] = s1;
	}
}

uchar konwersja_znaku_telefonu_eeprom(const uchar wartosc)
{
	static const uchar znaki[16] PROGMEM = 
	{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '*', '#', '!', '!', '\0' };
	return pgm_read_byte(znaki + wartosc);
}

uchar konwertuj_blok_eeprom_na_telefon(const uchar* blok_ptr, uchar* telefon_ptr,
	const uchar max_liczba_znakow)	// nie dzia³a dobrze dla nieparzystych max_liczba_znakow
{
	*telefon_ptr = '\0';
	const uchar* ptr = blok_ptr;
	uchar i = 0;
	while ( *ptr != 0xff && i < LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM )
		++ptr, ++i;
	if ( ptr == blok_ptr )
		return 0;
	--ptr;
	if ( i * 2 > max_liczba_znakow )
		ptr = blok_ptr + (max_liczba_znakow - 1) / 2;
	i = 0;
	if ( /*(max_liczba_znakow & 0x01) != 0 */ (*ptr & 0x0f) == 0x0f &&
		(*telefon_ptr = konwersja_znaku_telefonu_eeprom((*ptr & 0xf0) >> 4)) != '\0' )
		telefon_ptr++, ++i, --ptr;
	do
	{
		if ( ptr < blok_ptr )
			break;
		if ( (*telefon_ptr = konwersja_znaku_telefonu_eeprom((*ptr & 0x0f))) != '\0' )
			++telefon_ptr, ++i;
		if ( i == max_liczba_znakow )
			break;
		*telefon_ptr++ = konwersja_znaku_telefonu_eeprom((*ptr & 0xf0) >> 4);
		if ( ++i == max_liczba_znakow )
			break;
	} while ( ptr-- != blok_ptr );
	*telefon_ptr = '\0';
	return i;
}

uchar kopiuj_blok_eeprom_na_telefon(const void* eeprom_ptr, uchar* telefon_ptr,
	const uchar max_liczba_znakow)	// nie dzia³a dobrze dla nieparzystych max_liczba_znakow
{
	uchar buf[LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM];
	memcpy_E(buf, eeprom_ptr, LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM);
	return konwertuj_blok_eeprom_na_telefon(buf, telefon_ptr, max_liczba_znakow);
}

uchar porownaj_numer_telefonu_blok(const uchar* telefon_blok_ptr, const void* eeprom_ptr)
{
	if ( telefon_blok_ptr[0] == 0xff || eeprom_read_byte(eeprom_ptr) == 0xff )
		return FALSE;
	while ( 1 )
	{
		const uchar b = eeprom_read_byte(eeprom_ptr);
		uchar a = 0xf0;
		uchar c = b & a;
		uchar d = *telefon_blok_ptr & a;
		if ( c == a || d == a )
			return (c == a && d == a);
		if ( c != d )
			return FALSE;
		a = 0x0f;
		c = b & a;
		d = *telefon_blok_ptr & a;
		if ( c == a || d == a )
			return (c == a && d == a);
		if ( c != d )
			return FALSE;
		++eeprom_ptr;
		++telefon_blok_ptr;
	}
}

uchar porownaj_numer_telefonu(const uchar* telefon_ptr, const void* eeprom_ptr)
{
	uchar buf[LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM];
	uchar l = strlen(telefon_ptr);
	konwertuj_telefon_na_blok_eeprom(telefon_ptr, telefon_ptr + l, buf);
	return porownaj_numer_telefonu_blok(buf, eeprom_ptr);
}
