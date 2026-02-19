#include "narzedzia.h"

#ifndef WYLACZ_POMOC_BUFOR

#if ( WLACZ_TESTY_OGOLNE || WLACZ_TESTY_SIM900 )
#define MAX_DLUGOSC_BUF_POMOC	30
uchar buf_pomoc[MAX_DLUGOSC_BUF_POMOC];

uchar buf_dlug;
#endif

static void POMOC_DODAJ(const uchar ZNAK) __attribute__((unused));

static void POMOC_DODAJ(const uchar ZNAK)
{
#ifdef WLACZ_TESTY_OGOLNE
#ifndef WLACZ_TESTY_LEON
	if ( buf_dlug < MAX_DLUGOSC_BUF_POMOC-1 )
		buf_pomoc[buf_dlug++] = ZNAK;
	else 
	{
		//memset(buf_pomoc, 0, MAX_DLUGOSC_BUF_POMOC); 
		//buf_dlug = 0;
		//buf_pomoc[buf_dlug++] = ZNAK;
	}
#endif
#endif
}

static void POMOC_DODAJ2(const uchar ZNAK1, const uchar ZNAK2) __attribute__((unused));

static void POMOC_DODAJ2(const uchar ZNAK1, const uchar ZNAK2)
{
#ifdef WLACZ_TESTY_OGOLNE
#ifndef WLACZ_TESTY_LEON
	if ( buf_dlug < MAX_DLUGOSC_BUF_POMOC-2 )
	{
		buf_pomoc[buf_dlug++] = ZNAK1;
		buf_pomoc[buf_dlug++] = ZNAK2;
	}
	else 
	{
		//memset(buf_pomoc, 0, MAX_DLUGOSC_BUF_POMOC); 
		//buf_dlug = 0;
		//buf_pomoc[buf_dlug++] = ZNAK1;
		//buf_pomoc[buf_dlug++] = ZNAK2;
	}
#endif
#endif
}

static void POMOC_DODAJ_HEX(const uchar wartosc) __attribute__((unused));

static void POMOC_DODAJ_HEX(const uchar wartosc)
{
#ifdef WLACZ_TESTY_OGOLNE
#ifndef WLACZ_TESTY_LEON
	POMOC_DODAJ('$');
	uchar b[5];
	utoa(wartosc, b, 16);
	if ( !b[1] )
		POMOC_DODAJ('0');
	POMOC_DODAJ(b[0]);
	if ( b[1] )
		POMOC_DODAJ(b[1]);
#endif
#endif
}

static void POMOC_DODAJ_TABLICE(const uchar* ptr) __attribute__((unused));

static void POMOC_DODAJ_TABLICE(const uchar* ptr)
{
#ifdef WLACZ_TESTY_SIM900
	while ( *ptr )
	{
		//if ( buf_dlug < MAX_DLUGOSC_BUF_POMOC-1 )
		//	buf_pomoc[buf_dlug++] = *ptr++;
		//else
		{
			//memset(buf_pomoc, 0, MAX_DLUGOSC_BUF_POMOC); 
			//buf_dlug = 0;
			//buf_pomoc[buf_dlug++] = *ptr++;
		}
	}
#endif
}

#define POMOC_SIM900_DODAJ	POMOC_DODAJ_TABLICE

#endif
