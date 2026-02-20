
#include "sim900.h"
#include "poleceniagsm.h"
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <string.h>
//#include "bufpomoc.h"
//#include "USB.h"

uchar bledny_PIN = FALSE;

uchar poziom_sieci_gsm = POZIOM_SIECI_BLAD;

uchar porownaj_znaki(PGM_P polecenie, const uint poczatek, 
	const uchar liczba_znakow, const uint odebrane)
{
	if ( liczba_znakow <= odebrane )
	{
		uchar* w = &odebrany_blok_SIM900[poczatek];
		uchar l = 0;
		while ( l < liczba_znakow && *w == pgm_read_byte(polecenie) )
			++w, ++polecenie, ++l;
		return l == liczba_znakow;
	}
	return FALSE;
}

void resetuj_polecenie_konczace(void)
{
	const uchar sreg = SREG;
	cli();
	poczatek_polecenia_SIM900 = 0;
	zakonczenie_polecenia_SIM900 = 0;
	SREG = sreg;
	return;
}

uint numer_bledu_cme = 0;
uint numer_bledu_cms = 0;

#define MAX_LICZBA_ZAREJESTROWANYCH_KOMEND_OD_SIM900	(2*LICZBA_POLOZEN_OTRZYMANYCH_KOMEND_SIM900)
enum KomendySIM900 zarejestrowane_komendy_od_SIM900[MAX_LICZBA_ZAREJESTROWANYCH_KOMEND_OD_SIM900];
uchar liczba_komend_w_kolejce_SIM900;

uchar polecenia_konczace_gsm(void)
{
	for(uchar i = 8; i != 0; --i)
	{
		const uchar sreg = SREG;
		cli();
		uchar pom = czy_jest_nowe_polecenie_SIM900;
		if ( !pom )
		{
			SREG = sreg;
			if ( liczba_komend_w_kolejce_SIM900 != 0 )
			{
				komenda_SIM900 = zarejestrowane_komendy_od_SIM900[--liczba_komend_w_kolejce_SIM900];
				zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900] = BRAK_KOMENDY_SIM900;
				return komenda_SIM900;
			}
			return komenda_SIM900 = BRAK_KOMENDY_SIM900;
		}
		if ( pom >= LICZBA_POLOZEN_OTRZYMANYCH_KOMEND_SIM900 )
			pom = LICZBA_POLOZEN_OTRZYMANYCH_KOMEND_SIM900-1;
		const uint poczatek = polozenie_otrzymanych_komend_SIM900[pom];
		polozenie_otrzymanych_komend_SIM900[pom] = 0;
		const uint zakonczenie = polozenie_otrzymanych_komend_SIM900[--pom];
		//podglad(poczatek & 0xff);
		//podglad(zakonczenie & 0xff);
		czy_jest_nowe_polecenie_SIM900 = pom;
		SREG = sreg;
		if ( zakonczenie <= poczatek )
			continue;
		if ( liczba_komend_w_kolejce_SIM900 >= MAX_LICZBA_ZAREJESTROWANYCH_KOMEND_OD_SIM900 )
		{
			liczba_komend_w_kolejce_SIM900 = MAX_LICZBA_ZAREJESTROWANYCH_KOMEND_OD_SIM900 - 1;
		}
		const uint z = zakonczenie - poczatek - 4;
		const uint p = poczatek+2;
#define czy_polecenie_sim(POL)	porownaj_znaki(POL, p, strlen_R(POL), z)
#define czy_polecenie_sim_od_tylu(POL)	porownaj_znaki_od_tylu(POL, zakonczenie - 3, strlen_R(POL), z)
		const uchar d0 = odebrany_blok_SIM900[zakonczenie-3];
		const uchar d1 = odebrany_blok_SIM900[zakonczenie-4];
		const uchar d2 = odebrany_blok_SIM900[zakonczenie-5];
		const uchar d5 = odebrany_blok_SIM900[poczatek+5];
	//	static const char polecenie_OK[] PROGMEM = "OK";
		if ( d0 == 'K' && d1 == 'O' && d2 != ' ' && zakonczenie > 5 )
		{
			zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900++] = OK_KOMENDA_SIM900;
			continue;
		}
		static const char polecenie_CMGS[] PROGMEM = "+CMGS:";
		if ( d5 == 'G' && czy_polecenie_sim(polecenie_CMGS) )
		{
			zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900++] = CMGS_KOMENDA_SIM900;
			continue;
		}
		static const char polecenie_ERROR[] PROGMEM = "ERROR";
		if ( d1 == 'O' && czy_polecenie_sim(polecenie_ERROR) )
		{
			zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900++] = ERROR_KOMENDA_SIM900;
			continue;
		}
		static const char polecenie_MO_RING[] PROGMEM = "MO RING";
		if ( d0 == 'G' && czy_polecenie_sim(polecenie_MO_RING) )
		{
			zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900++] = MO_RING_KOMENDA_SIM900;
			continue;
		}
		static const char polecenie_MO_CONNECTED[] PROGMEM = "MO CONNECTED";
		if ( d0 == 'D' && czy_polecenie_sim(polecenie_MO_CONNECTED) )
		{
			zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900++] = MO_CONNECTED_KOMENDA_SIM900;
			continue;
		}
		//static const char polecenie_CONNECT[] PROGMEM = "CONNECT";
		//if ( d0 == 'T' && czy_polecenie_sim(polecenie_CONNECT) )
		//{
		//	zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900++] = CONNECT_KOMENDA_SIM900;
		//	continue;
		//}
		static const char polecenie_CLIP[] PROGMEM = "+CLIP:";
		if ( d5 == 'I' && czy_polecenie_sim(polecenie_CLIP) )
		{
			zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900++] = CLIP_KOMENDA_SIM900;
			continue;
		}
		static const char polecenie_DTMF[] PROGMEM = "+DTMF:";
		if ( d5 == 'M' && czy_polecenie_sim(polecenie_DTMF) )
		{
			zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900++] = DTMF_KOMENDA_SIM900;
			continue;
		}
		static const char polecenie_CMTI[] PROGMEM = "+CMTI:";	// nowy SMS
		if ( d5 == 'T' && czy_polecenie_sim(polecenie_CMTI) )
		{
			zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900++] = CMTI_KOMENDA_SIM900;
			continue;
		}
		static const char polecenie_NO_CARRIER[] PROGMEM = "NO CARRIER";
		if ( d1 == 'E' && czy_polecenie_sim(polecenie_NO_CARRIER) )
		{
			zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900++] = NO_CARRIER_KOMENDA_SIM900;
			continue;
		}
		static const char polecenie_NO_DIALTONE[] PROGMEM = "NO DIALTONE";
		if ( d0 == 'E' && czy_polecenie_sim(polecenie_NO_DIALTONE) )
		{
			zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900++] = NO_DIALTONE_KOMENDA_SIM900;
			continue;
		}
		static const char polecenie_BUSY[] PROGMEM = "BUSY";
		if ( d0 == 'Y' && czy_polecenie_sim(polecenie_BUSY) )
		{
			zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900++] = BUSY_KOMENDA_SIM900;
			continue;
		}
		static const char polecenie_CMEERROR[] PROGMEM = "+CME ERROR:";
		if ( d5 == 'E' && czy_polecenie_sim(polecenie_CMEERROR) )
		{
			numer_bledu_cme = (uint) 
				strtoul(&odebrany_blok_SIM900[p+strlen_R(polecenie_CMEERROR)+1], NULL, 10);
			zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900++] = CME_ERROR_KOMENDA_SIM900;
			continue;
		}
		static const char polecenie_CMSERROR[] PROGMEM = "+CMS ERROR:";
		if ( d5 == 'S' && czy_polecenie_sim(polecenie_CMSERROR) )
		{
			numer_bledu_cms = (uint) 
				strtoul(&odebrany_blok_SIM900[p+strlen_R(polecenie_CMSERROR)+1], NULL, 10);
			zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900++] = CMS_ERROR_KOMENDA_SIM900;
			continue;
		}
		static const char polecenie_CUSD[] PROGMEM = "+CUSD:";	// nowy Report 
		if ( d5 == 'S' && czy_polecenie_sim(polecenie_CUSD) )
		{
			zarejestrowane_komendy_od_SIM900[liczba_komend_w_kolejce_SIM900++] = CUSD_KOMENDA_SIM900;
			continue;
		}
	}
	return BRAK_KOMENDY_SIM900;
}

uchar czy_jest_nowa_komenda_SIM900(void)
{
	if ( polecenia_konczace_gsm() )
	{
		switch ( komenda_SIM900 )
		{
		case OK_KOMENDA_SIM900:
		case ERROR_KOMENDA_SIM900:
		case DANE_ZE_STRONY_INTERNETOWEJ_KOMENDA_SIM900:
		{
			const uchar sreg = SREG;
			cli();
			resetuj_polecenie_konczace();
			zakoncz_odbieranie_SIM900();
			SREG = sreg;
		}
		}
		return komenda_SIM900;
	}
	return BRAK_KOMENDY_SIM900;
}
