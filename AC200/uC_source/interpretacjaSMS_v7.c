#ifndef INCLUDE

#include "interpretacjaSMS.h"
#include "komendy.h"
#include "ctype.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdlib.h> 
#include <string.h>

#include "zapiseeprom.h"
#include "wewy.h"

uchar nr_usunietego_uzytkownika_z_smsa;

#endif

#define przeskocz_biale_znaki(BUF)	while ( isspace(*BUF) ) ++BUF

uchar sprawdz_kod(const uchar** buf_sms)
{
	przeskocz_biale_znaki(*buf_sms);
	if ( (*buf_sms)[0] == kod_modulu[0] && (*buf_sms)[1] == kod_modulu[1] && (*buf_sms)[2] == kod_modulu[2] 
		&& (*buf_sms)[3] == kod_modulu[3] )
	{
		*buf_sms += LICZBA_BAJTOW_KODU_DOSTEPU;
		return TRUE;
	}
	else
		return FALSE;
}

uchar sprawdz_reset_ustawien(const uchar* buf)
{
	static const char res_ust[] PROGMEM = INSTRUKCJA_SMS_RESET_WSZYSTKICH_USTAWIEN;
	return memcmp_R(buf, res_ust) == 0;
}

#define LICZBA_INSTRUKCJI_SMS		13
#define MAX_LICZBA_ZNAKOW_INSTRUKCJI_SMS	8
// uwaga: wa�na jest kolejno�� oraz usytuowanie pomi�dzy nimi
static const uchar instrukcja_sms[LICZBA_INSTRUKCJI_SMS][MAX_LICZBA_ZNAKOW_INSTRUKCJI_SMS] PROGMEM =
{	// przed każdą instrukcją musi być kod, litery mogą być duże lub małe
	"\x04" "CODE",	// CODE EFGH (zmiana kodu dost�pu)
	"\x03" "ADD",	//
	"\x03" "DEL",		//
	"\x04" "XXXX",	// RESET
	"\x06" "REPORT",	// REPORT
	"\x04" "USER",	// USER
	"\x05" "START",	// START (odblokowuje globalne sterowanie)
	"\x04" "STOP",	// STOP (blokuje globalne sterowanie)
	"\x04" "OPEN",	// OPEN CLIP / OPEN DTMF
	"\x05" "CLOSE",	// CLOSE CLIP / CLOSE DTMF
	"\x04" "CLIP",	// (stare) – nieaktywne
	"\x04" "DTMF",	// (stare) – nieaktywne
	"\x03" "CON",	// (stare) – nieaktywne
};

enum {
INSTRUKCJA_CODE		,	// code EFGH [K]
INSTRUKCJA_ADD  		,
INSTRUKCJA_DEL  	,
INSTRUKCJA_RESET  	,
INSTRUKCJA_REPORT ,// report
INSTRUKCJA_USER		,	 // user +48505691117 E C B R [K] // user del +48505691117 [K]
INSTRUKCJA_START	,	// start (odblokowuje sterowanie wyjsciem)
INSTRUKCJA_STOP		,	// stop (blokuje sterowanie wyjsciem)
INSTRUKCJA_OPEN		,	// open (tryb publiczny - kazdy moze otworzyc)
INSTRUKCJA_CLOSE	,	// close (tryb prywatny - tylko lista)
INSTRUKCJA_CLIP		,	// clip (tryb automatyczny - wlacza wyjscie po odebraniu)
INSTRUKCJA_DTMF		,	// dtmf (tryb klawiatury - czeka na klawisz)
INSTRUKCJA_CON		,	// con (powiadomienia - numer lub OFF)
};

uchar	interpretuj_instrukcje_sms(const uchar** buf_sms, const uchar start, 
	const uchar end)        
{
	przeskocz_biale_znaki(*buf_sms);
	uchar i;
	for (i = start; i < end; ++i)
	{
		const void* p = &instrukcja_sms[0][0] + i*MAX_LICZBA_ZNAKOW_INSTRUKCJI_SMS;
		const uchar l = pgm_read_byte(p);
		if ( memcmp_P(*buf_sms, p+1, l) == 0 )
		{
			*buf_sms += l; 
			return i;
		}
	}
	return i;
}

uchar pobierz_long(const uchar** buf_sms, long* wartosc)
{
	const uchar* buf_sms_pom = *buf_sms;
	*wartosc = strtol(*buf_sms, (char**) buf_sms, 10);
	return *buf_sms != buf_sms_pom;
}

uchar pomin_znak(const uchar** buf_sms, const uchar wartosc)
{
	przeskocz_biale_znaki(*buf_sms);
	if ( **buf_sms == toupper(wartosc) )
	{
		++*buf_sms;
		return TRUE;
	}
	else
		return FALSE;
}

uchar czy_jest_znak(const uchar** buf_sms, const uchar wartosc)
{
	przeskocz_biale_znaki(*buf_sms);
	const uchar* buf_sms_pom = *buf_sms;
	const uchar w = toupper(wartosc);
	uchar ch;
	while ( (ch = *buf_sms_pom++) != '\0' && ch != w )
		;
	if ( ch == w )
	{
		*buf_sms = buf_sms_pom;
		return TRUE;
	}
	else
		return FALSE;
}

void pobierz_wyraz(const uchar** buf_sms, uchar* buf, 
	uchar max_liczba_znakow)
{
	przeskocz_biale_znaki(*buf_sms);
	while ( max_liczba_znakow-- && !isspace(**buf_sms) && (*buf = **buf_sms) != '\0' )
	{
		buf++;
		(*buf_sms)++;
	}
	*buf = '\0';
}

uchar pobierz_numer_telefonu(const uchar** buf_sms, uchar* buf_telefon, 
	const uchar rozmiar_bufora)
{
	przeskocz_biale_znaki(*buf_sms);
	const uchar* tel = *buf_sms;
	uchar* buf = buf_telefon;
	uchar l = 0;
	while ( konwersja_znaku_telefonu(*tel) != ZNAK_NUMERU_TELEFONU_NIEZNANY 
		&& ++l < rozmiar_bufora )
		*buf++ = *tel++;
	*buf = '\0';
	if ( buf_telefon[0] != '\0' )
	{
		*buf_sms = tel;
		return TRUE;
	}
	else
		return FALSE;
}

uchar interpretuj_wiadomosc_sms(const uchar* sms)
{
	memcpy(bufor_eeprom, sms, MAX_BUFOR_EEPROM);
	const uchar* sms_pom = sms;
	if ( !sprawdz_kod(&sms) )
	{
		if ( sprawdz_reset_ustawien(sms) )
			return INTERPRETACJA_SMS_RESET_WSZYSTKICH_USTAWIEN;
		return INTERPRETACJA_SMS_BRAK_KODU;
	}
	{	// mo�na wstawi� ten kod do kolejki i w nast�pnym kroku interpretowa�
		for (uchar* ptr = (uchar*) sms; *ptr != '\0'; ++ptr)
			*ptr = toupper(*ptr);
	}
	
	switch ( interpretuj_instrukcje_sms(&sms, INSTRUKCJA_CODE, INSTRUKCJA_CON+1) )
	{
	case INSTRUKCJA_CODE:
	{
		przeskocz_biale_znaki(sms);
		for (uchar i = 0; i < LICZBA_BAJTOW_KODU_DOSTEPU; ++i)
		{
			const uchar znak = bufor_eeprom[(sms - sms_pom) + i];
			if ( not ((znak >= 'A' && znak <= 'Z') || (znak >= '0' && znak <= '9')) )
				return INTERPRETACJA_SMS_BLEDNE_DANE;
		}
		memcpy(kod_modulu, bufor_eeprom + (sms - sms_pom), LICZBA_BAJTOW_KODU_DOSTEPU);
		zapisz_znaki_w_eeprom(bufor_eeprom + (sms - sms_pom), (uint) ADRES_EEPROM_KOD_DOSTEPU, LICZBA_BAJTOW_KODU_DOSTEPU);
		return INTERPRETACJA_SMS_POPRAWNY;
	}
	case INSTRUKCJA_REPORT:
	{
		return INTERPRETACJA_SMS_RAPORT;
	}
	case INSTRUKCJA_USER:
	{
		// Komenda USER ma działać wyłącznie z podanym numerem.
		// Przykład: USER +48505691117 E C B R [K]
		if ( pobierz_numer_telefonu(&sms, &numer_telefonu_do_ktorego_dzwonic[0], 14) )
			return INTERPRETACJA_SMS_USER;

		// Brak numeru po "USER" – wyślij informację z instrukcją użycia,
		// zamiast pełnej listy użytkowników.
		return INTERPRETACJA_SMS_USER_BEZ_NUMERU;
	}
	case INSTRUKCJA_ADD:
	{
		if ( not pobierz_numer_telefonu(&sms, &numer_telefonu_do_ktorego_dzwonic[0], 14) )
			return INTERPRETACJA_SMS_BLEDNE_DANE;
		dodaj_komende(KOMENDA_KOLEJKI_DODAJ_UZYTKOWNIKA_BRAMA);
		return INTERPRETACJA_SMS_POPRAWNY;
	}
	case INSTRUKCJA_DEL:
	{
		if ( not pobierz_numer_telefonu(&sms, &numer_telefonu_do_ktorego_dzwonic[0], 14) )
			return INTERPRETACJA_SMS_BLEDNE_DANE;
		dodaj_komende(KOMENDA_KOLEJKI_USUN_UZYTKOWNIKA_BRAMA);
		return INTERPRETACJA_SMS_POPRAWNY;
	}
	case INSTRUKCJA_RESET:
	{
		return INTERPRETACJA_SMS_RESET_WSZYSTKICH_USTAWIEN;
	}
	case INSTRUKCJA_START:
	{
		blokada_sterowania = FALSE;
		zapisz_znaki_w_eeprom(&blokada_sterowania, (uint) EEPROM_BLOKADA_STEROWANIA, 1);
		return INTERPRETACJA_SMS_POPRAWNY;
	}
	case INSTRUKCJA_STOP:
	{
		blokada_sterowania = TRUE;
		zapisz_znaki_w_eeprom(&blokada_sterowania, (uint) EEPROM_BLOKADA_STEROWANIA, 1);
		return INTERPRETACJA_SMS_POPRAWNY;
	}
	case INSTRUKCJA_OPEN:
	{
		// Nowa składnia:
		// OPEN CLIP  – tryb publiczny + CLIP
		// OPEN DTMF  – tryb publiczny + DTMF
		const uchar podtryb = interpretuj_instrukcje_sms(&sms, INSTRUKCJA_CLIP, INSTRUKCJA_DTMF+1);
		if ( podtryb == INSTRUKCJA_CLIP )
		{
			tryb_publiczny = TRUE;
			eeprom_update_byte((void*) EEPROM_TRYB_PUBLICZNY, tryb_publiczny);

			tryb_clip = TRUE;
			eeprom_update_byte((void*) EEPROM_TRYB_CLIP_DTMF, tryb_clip);
			return INTERPRETACJA_SMS_POPRAWNY;
		}
		else if ( podtryb == INSTRUKCJA_DTMF )
		{
			tryb_publiczny = TRUE;
			eeprom_update_byte((void*) EEPROM_TRYB_PUBLICZNY, tryb_publiczny);

			tryb_clip = FALSE; // DTMF
			eeprom_update_byte((void*) EEPROM_TRYB_CLIP_DTMF, tryb_clip);
			return INTERPRETACJA_SMS_POPRAWNY;
		}
		else
		{
			// brak CLIP/DTMF po OPEN
			return INTERPRETACJA_SMS_BLEDNE_DANE;
		}
	}
	case INSTRUKCJA_CLOSE:
	{
		// Nowa składnia:
		// CLOSE CLIP  – tryb prywatny + CLIP
		// CLOSE DTMF  – tryb prywatny + DTMF
		const uchar podtryb = interpretuj_instrukcje_sms(&sms, INSTRUKCJA_CLIP, INSTRUKCJA_DTMF+1);
		if ( podtryb == INSTRUKCJA_CLIP )
		{
			tryb_publiczny = FALSE;
			zapisz_znaki_w_eeprom(&tryb_publiczny, (uint) EEPROM_TRYB_PUBLICZNY, 1);

			tryb_clip = TRUE;
			zapisz_znaki_w_eeprom(&tryb_clip, (uint) EEPROM_TRYB_CLIP_DTMF, 1);
			return INTERPRETACJA_SMS_POPRAWNY;
		}
		else if ( podtryb == INSTRUKCJA_DTMF )
		{
			tryb_publiczny = FALSE;
			zapisz_znaki_w_eeprom(&tryb_publiczny, (uint) EEPROM_TRYB_PUBLICZNY, 1);

			tryb_clip = FALSE; // DTMF
			zapisz_znaki_w_eeprom(&tryb_clip, (uint) EEPROM_TRYB_CLIP_DTMF, 1);
			return INTERPRETACJA_SMS_POPRAWNY;
		}
		else
		{
			// brak CLIP/DTMF po CLOSE
			return INTERPRETACJA_SMS_BLEDNE_DANE;
		}
	}
	case INSTRUKCJA_CLIP:
	{
		// Stara komenda – nieaktywna w nowej specyfikacji
		return INTERPRETACJA_SMS_ZLY_FORMAT;
	}
	case INSTRUKCJA_DTMF:
	{
		// Stara komenda – nieaktywna w nowej specyfikacji
		return INTERPRETACJA_SMS_ZLY_FORMAT;
	}
	case INSTRUKCJA_CON:
	{
		// Wyłączona funkcja CON: SMS "CON ..." nie zmienia żadnych ustawień
		// ani numerów, jest traktowany jak nieobsługiwany format.
		return INTERPRETACJA_SMS_ZLY_FORMAT;
	}
	}
	return INTERPRETACJA_SMS_ZLY_FORMAT;
}
