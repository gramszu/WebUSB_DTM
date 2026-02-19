
#include "pdu.h"
#include <string.h>
#include "narzedzia.h"
#include <avr/pgmspace.h>
#include "konfiguracja.h"

#define ALPHABET_SIZE 128

static const uchar GSM_Default_Alphabet[ALPHABET_SIZE] PROGMEM = {

	/* ETSI GSM 03.38, version 6.0.1, section 6.2.1; Default alphabet */
	/* Characters in hex position 10, [12 to 1a] and 24 are not present on
	   latin1 charset, so we cannot reproduce on the screen, however they are
	   greek symbol not present even on my Nokia */

	'@',  0xa3, '$',  0xa5, 0xe8, 0xe9, 0xf9, 0xec, 
	0xf2, 0xc7, '\n', 0xd8, 0xf8, '\r', 0xc5, 0xe5,
	'?',  '_',  '?',  '?',  '?',  '?',  '?',  '?',
	'?',  '?',  '?',  '?',  0xc6, 0xe6, 0xdf, 0xc9,
	' ',  '!',  '\"', '#',  0xa4,  '%',  '&', '\'',
	'(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	'8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	0xa1, 'A',  'B',  'C',  'D',  'E',  'F',  'G',
	'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
	'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
	'X',  'Y',  'Z',  0xc4, 0xd6, 0xd1, 0xdc, 0xa7,
	0xbf, 'a',  'b',  'c',  'd',  'e',  'f',  'g',
	'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	'x',  'y',  'z',  0xe4, 0xf6, 0xf1, 0xfc, 0xe0
};

void pdu_to_ascii(const uchar* pdu, uchar* bufor, const uchar dim_bufor)
{
	const uchar l = *pdu++;
	uchar* ascii = bufor;
	uchar flaga = 0x80;
	uchar pom = 0;
	uchar w = 0;
	const uchar roz_1 = min(dim_bufor-1, l);
	for (uchar i = 0; i < roz_1; )
{
		*ascii++ = pgm_read_byte(GSM_Default_Alphabet + (((*pdu & (~flaga)) << w) + pom));
		const uchar k = ++i & 0x07;
		if ( k != 0 )
		{
			pom = (*pdu++ & flaga) >> (7 - w);
			flaga = (flaga >> 1) + 0x80;
		}
		else
		{
			flaga = 0x80;
			pom = 0;
		}
		w = k;
	}
	*ascii = '\0';
}

static const uchar GSM_Default_DeAlphabet[256] PROGMEM = {
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x0a, 0x3f, 0x3f, 0x0d, 0x3f, 0x3f, 
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 
	0x20, 0x21, 0x22, 0x23, 0x02, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	0x00, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x3f, 0x3f, 0x3f, 0x3f, 0x11, 
	0x3f, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 
	0x3f, 0x40, 0x3f, 0x01, 0x24, 0x03, 0x3f, 0x5f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x60, 
	0x3f, 0x3f, 0x3f, 0x3f, 0x5b, 0x0e, 0x1c, 0x09, 0x3f, 0x1f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 
	0x3f, 0x5d, 0x3f, 0x3f, 0x3f, 0x3f, 0x5c, 0x3f, 0x0b, 0x3f, 0x3f, 0x3f, 0x5e, 0x3f, 0x3f, 0x1e, 
	0x7f, 0x3f, 0x3f, 0x3f, 0x7b, 0x0f, 0x1d, 0x3f, 0x04, 0x05, 0x3f, 0x3f, 0x07, 0x3f, 0x3f, 0x3f, 
	0x3f, 0x7d, 0x08, 0x3f, 0x3f, 0x3f, 0x7c, 0x3f, 0x0c, 0x06, 0x3f, 0x3f, 0x7e, 0x3f, 0x3f, 0x3f
};

uchar* ascii_to_pdu_blok(uchar* bufor_pdu, const uchar* ascii, 
	const uchar* ascii_end)
{
	bufor_pdu[0] = (uchar)(ascii_end - ascii);
	const uchar roz_1 = bufor_pdu[0] - 1;	// rozmiar = roz_1 + 1
	uchar j = 0;
	uchar l = 0;
	uchar* pdu_ptr = bufor_pdu+1;
	uchar konwersja;
	while ( j < roz_1 )
	{
		uchar l_7 = 7 - l;
		konwersja = ((pgm_read_byte(GSM_Default_DeAlphabet + ascii[j])) & (0x7fu << l)) >> l;
		konwersja |= ((pgm_read_byte(GSM_Default_DeAlphabet + ascii[j+1])) & (0xffu >> l_7)) << l_7;
		//*pdu_ptr++ = pgm_read_byte(GSM_Default_DeAlphabet + konwersja);
		*pdu_ptr++ = konwersja;
		if ( ++l == 7 )
		{
			l = 0;
			++j;
		}
		++j;
	}
	if ( bufor_pdu[0] % 8 != 0 )
	{
		konwersja = (((pgm_read_byte(GSM_Default_DeAlphabet + ascii[j])) & (0x7fu << l)) >> l );
		//*pdu_ptr++ = pgm_read_byte(GSM_Default_DeAlphabet + konwersja);
		*pdu_ptr++ = konwersja;
	}
	return pdu_ptr;
}

void konwertuj_blok_dwa_znaki_na_znak_pdu(const uchar* blok_danych, 
	const uchar liczba_znakow, uchar* ptr)
{
	for (uint i = 0; i < liczba_znakow; ++i)
	{
		uchar ch = *blok_danych++;
		uchar wynik = ( ch < ('9' + 1)) ? (ch - '0') : (ch - ('A' - 10));
		wynik <<= 4;
		ch = *blok_danych++;
		wynik += ( ch < ('9' + 1)) ? (ch - '0') : (ch - ('A' - 10));
		*ptr++ = wynik;
	}
}

uchar* pobierz_numer_telefonu_nadawcy_z_PDU(const uchar* pdu_ptr, uchar* bufor_numer_telefonu, uchar* nietypowy_sms)
{
	//pdu_ptr += pdu_ptr[0] + 2;
	if ( (pdu_ptr[0] + 1) >= 200 )	// (1)
	{
		*bufor_numer_telefonu = 0;
		*nietypowy_sms = TRUE;
		return 0;
	}
	*nietypowy_sms = FALSE;
	pdu_ptr += pdu_ptr[0] + 1;
	if ( ! (*pdu_ptr == 0x04 || *pdu_ptr == 0x00) )	//First octet of the SMS-DELIVER PDU
		*nietypowy_sms = TRUE;
	++pdu_ptr;
	const uchar liczba_cyfr_w_telefonie = *pdu_ptr++;
	if ( liczba_cyfr_w_telefonie > MAX_LICZBA_ZNAKOW_TELEFON )	// (1)
	{
		*bufor_numer_telefonu = 0;
		*nietypowy_sms = TRUE;
		return 0;
	}
	const uchar typ_telefonu = *pdu_ptr++;
	const uchar dobry_telefon = (typ_telefonu == 129 || typ_telefonu == 145);
	if ( typ_telefonu == 145 )
		*bufor_numer_telefonu++ = '+';
	for (uchar i = 0; i < liczba_cyfr_w_telefonie; ++i)
	{
		uchar znak = *pdu_ptr;
		if ( (i & 0x01) != 0)
		{
			SWAP(znak);
			++pdu_ptr;
		}
		*bufor_numer_telefonu++ = (dobry_telefon ? ((znak & 0x0f) + '0') : '0');
	}
	*bufor_numer_telefonu = '\0';
	if ( !(liczba_cyfr_w_telefonie & 0x01) )
		--pdu_ptr;
	if ( pdu_ptr[2] != 0 )	// TP-DCS Data coding scheme
		*nietypowy_sms = TRUE;
	pdu_ptr += 3;
	return (uchar*) pdu_ptr;
}

const uchar* pobierz_date_z_PDU(const uchar* ptr, uchar* rok, uchar* miesiac, uchar* dzien)
{
	*rok = *ptr++;
	*rok = 10 * (*rok & 0x0f) + ((*rok & 0xf0) >> 4);
	*miesiac = *ptr++;
	*miesiac = 10 * (*miesiac & 0x0f) + ((*miesiac & 0xf0) >> 4);
	*dzien = *ptr++;
	*dzien = 10 * (*dzien & 0x0f) + ((*dzien & 0xf0) >> 4);
	return ptr;
}

const uchar* pobierz_czas_z_PDU(const uchar* ptr, uchar* godzina, uchar* minuta, uchar* sekunda)
{
	*godzina = *ptr++;
	*godzina = 10 * (*godzina & 0x0f) + ((*godzina & 0xf0) >> 4);
	*minuta = *ptr++;
	*minuta = 10 * (*minuta & 0x0f) + ((*minuta & 0xf0) >> 4);
        *sekunda = *ptr++;
        *sekunda = 10 * (*sekunda & 0x0f) + ((*sekunda & 0xf0) >> 4);
	return ptr + 1; // Pomin timezone
}

uchar* wpisz_telefon_w_formacie_sms(uchar* ptr, const uchar* telefon)
{
	uchar typ_telefonu;
	if ( *telefon == '+' )
	{
		++telefon;
		typ_telefonu = 145;
	}
	else
		typ_telefonu = 129;
	const uchar liczba_cyfr_telefonu = strlen(telefon);
	if ( liczba_cyfr_telefonu == 0 )
	{
		*ptr++ = liczba_cyfr_telefonu;
		return ptr;
	}
	*ptr++ = liczba_cyfr_telefonu;
	*ptr++ = typ_telefonu;
	const uchar k = (liczba_cyfr_telefonu + 1) / 2;
	for (uchar i = 0; i < k; ++i)
	{
		uchar ch = (*telefon++ - '0');
		if ( *telefon != '\0' )
			ch |= (*telefon++ - '0') << 4;
		else
			ch |= 0xf0;
		*ptr++ = ch;
	}
	return ptr;
}

uchar* wpisz_centrum_sms_w_formacie_sms(uchar* ptr, const uchar* telefon)
{
	if ( *telefon == '\0' )
	{
		*ptr++ = 0x00;
		return ptr;
	}
	uchar typ_telefonu;
	if ( *telefon == '+' )
	{
		++telefon;
		typ_telefonu = 145;
	}
	else
		typ_telefonu = 129;
	const uchar liczba_cyfr_telefonu = strlen(telefon);
	*ptr++ = (liczba_cyfr_telefonu + 1) / 2 + 1;
	*ptr++ = typ_telefonu;
	const uchar k = (liczba_cyfr_telefonu + 1) / 2;
	for (uchar i = 0; i < k; ++i)
	{
		uchar ch = (*telefon++ - '0');
		if ( *telefon != '\0' )
			ch |= (*telefon++ - '0') << 4;
		else
			ch |= 0xf0;
		*ptr++ = ch;
	}
	return ptr;
}

uchar* zapisz_naglowek_pdu(uchar* ptr, const uchar* telefon_odbiorcy, 
	const uchar* telefon_centrum_sms, const uchar TP_PID, const uchar TP_DCS)
{
	ptr = wpisz_centrum_sms_w_formacie_sms(ptr, telefon_centrum_sms);
	*ptr++ = 0x11;
	*ptr++ = 0x00;
	ptr = wpisz_telefon_w_formacie_sms(ptr, telefon_odbiorcy);
	*ptr++ = TP_PID;	// TP-PID. Protocol identifier 
	*ptr++ = TP_DCS;	// TP-DCS. Data coding scheme (7-bit, 8-bit, UCS2)
	*ptr++ = 0xAA;	// TP-Validity-Period. "AA" means 4 days
	return ptr;
}

void konwertuj_pdu_na_blok_wysylany(uchar* buf, const uchar* pdu_ptr, 
	const uchar dlugosc_pdu)
{
	static const char znak_wartosc_heksadecymalna[] PROGMEM = "0123456789ABCDEF";
	for (uchar i = 0; i < dlugosc_pdu; ++i)
	{
		uchar znak = *pdu_ptr++;
		volatile uchar pom = znak & 0xf0;	// bardzo wa¿ne jst s³owo volatile, bo kompilator Ÿle generuje kod!!
		SWAP(pom);
		*buf++ = pgm_read_byte(znak_wartosc_heksadecymalna + pom);
		*buf++ = pgm_read_byte(znak_wartosc_heksadecymalna + (znak & 0x0f));
	}
}

