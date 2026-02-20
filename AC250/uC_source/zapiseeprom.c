
#include "zapiseeprom.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <ctype.h>

#ifdef USE_EXTERNAL_EEPROM
#include "at24c512.h"
#endif

uchar bufor_eeprom[MAX_BUFOR_EEPROM];
uchar liczba_znakow_do_zapisu = 0;
uchar nr_zapisanego_znaku = 0;
uint pozycja_w_eeprom;

uchar bufor_pomoc_liczba_znakow_do_zapisu;
uint bufor_pomoc_adres_eeprom;

uchar czy_zajety_bufor_eeprom;

#ifndef USE_EXTERNAL_EEPROM
#define EEPROM_WRITE_ATMEGA8(uiAddress, ucData)	\
{																								\
	const uint p = uiAddress;											\
	EEARH = HIGH(p);															\
	EEARL = LOW(p);																\
	EECR |= BIT(EERE);														\
	if ( EEDR != (ucData) )													\
	{																							\
		EEDR = (ucData);														\
		cli();																			\
		EECR |= BIT(EEMPE);													\
		EECR |= BIT(EEPE);													\
		sei();																			\
	}																							\
}
#define EEPROM_READ_ATMEGA8(uiAddress, data)				\
{																										\
	const uint p = uiAddress;													\
	EEARH = HIGH(p);																	\
	EEARL = LOW(p);																		\
	EECR |= BIT(EERE);																\
	(data) = EEDR;																		\
}
#endif

	// uwaga, w??cza przerwania sei()
void zapisz_bajt_w_EEPROM(void)
{
	if ( !eeprom_is_ready() 
		|| (bufor_pomoc_liczba_znakow_do_zapisu == 0 && liczba_znakow_do_zapisu == 0 && !czy_zajety_bufor_eeprom) )
		return;
	uchar pom = nr_zapisanego_znaku;
#ifdef USE_EXTERNAL_EEPROM
	if ( at24c512_read_byte((uint16_t)(pozycja_w_eeprom + pom)) == bufor_eeprom[pom] )
#else
	EECR |= BIT(EERE);
	if ( EEDR == bufor_eeprom[pom] )
#endif
		++pom;

	if ( pom < liczba_znakow_do_zapisu )
	{
		const uint poz = pozycja_w_eeprom + pom;
#ifdef USE_EXTERNAL_EEPROM
		at24c512_write_byte((uint16_t)poz, bufor_eeprom[pom]);
#else
		EEPROM_WRITE_ATMEGA8(poz, bufor_eeprom[pom]);
#endif
	}
	else
	{
		const uchar k = bufor_pomoc_liczba_znakow_do_zapisu;
		bufor_pomoc_liczba_znakow_do_zapisu = 0;
		liczba_znakow_do_zapisu = k;
		if ( k != 0 )
		{
			for (uchar i = 0; i < k; ++i)
				bufor_eeprom[i] = bufor_eeprom[POLOZENIE_BUFOR_EEPROM_POMOC_BUFOR_START+i];
			pozycja_w_eeprom = bufor_pomoc_adres_eeprom;
			pom = 0;
#ifdef USE_EXTERNAL_EEPROM
			at24c512_write_byte((uint16_t)pozycja_w_eeprom, bufor_eeprom[0]);
#else
			EEPROM_WRITE_ATMEGA8(pozycja_w_eeprom, bufor_eeprom[0]);
#endif
		}
#ifndef USE_EXTERNAL_EEPROM
		else
		{
			EEARH = 0x0f;
			EEARL = 0xff;
		}
#endif
	}
	nr_zapisanego_znaku = pom;
}

	// uwaga, w-?cza przerwania sei()
void zapisz_znaki_w_eeprom(const void* buf, const uint pozycja_w_eeprom_, 
	const uchar liczba_znakow)
{
	// wcze?niej sprawdzono, czy mo?na zapisywa?
	if ( liczba_znakow == 0 )
		return;
	memcpy(bufor_eeprom, buf, liczba_znakow);
	pozycja_w_eeprom = pozycja_w_eeprom_;
	nr_zapisanego_znaku = 0;
	liczba_znakow_do_zapisu = liczba_znakow;
#ifdef USE_EXTERNAL_EEPROM
	at24c512_write_byte((uint16_t)pozycja_w_eeprom_, bufor_eeprom[0]);
#else
	EEPROM_WRITE_ATMEGA8(pozycja_w_eeprom_, bufor_eeprom[0]);
#endif
}

void zapisz_znaki_w_eeprom_bez_kopiowania(const uint pozycja_w_eeprom_, 
	const uchar liczba_znakow)
{
	// wcze?niej sprawdzono, czy mo?na zapisywa?
	if ( liczba_znakow == 0 )
		return;
	pozycja_w_eeprom = pozycja_w_eeprom_;
	nr_zapisanego_znaku = 0;
	liczba_znakow_do_zapisu = liczba_znakow;
#ifdef USE_EXTERNAL_EEPROM
	at24c512_write_byte((uint16_t)pozycja_w_eeprom_, bufor_eeprom[0]);
#else
	EEPROM_WRITE_ATMEGA8(pozycja_w_eeprom_, bufor_eeprom[0]);
#endif
}

	// uwaga, w-?cza przerwania sei()
void zapisz_znak_w_eeprom(const uchar znak, const uint pozycja_w_eeprom_)
{
	// wcze?niej sprawdzono, czy mo?na zapisywa?
	if ( eeprom_read_byte((void*) pozycja_w_eeprom_) == znak )
		return;
	bufor_eeprom[0] = znak;
	zapisz_znaki_w_eeprom_bez_kopiowania(pozycja_w_eeprom_, 1);
}

//int memcmp_E(const void* buf, const void* eeprom_ptr, size_t size)
//{
//	if ( size == 0 )
//		return 0;
//	while ( *(uchar*) buf == eeprom_read_byte(eeprom_ptr) && --size )
//	{
//		++buf;
//		++eeprom_ptr;
//	}
//	return *(uchar*)buf - eeprom_read_byte(eeprom_ptr);
//}

//void* memchr_E(void* eeprom_ptr, const char ch, size_t size)
//{
//	if ( size == 0 )
//		return 0;
//	while ( ch != eeprom_read_byte(eeprom_ptr) && --size )
//	{
//		++eeprom_ptr;
//	}
//	return ch != eeprom_read_byte(eeprom_ptr) ? NULL : eeprom_ptr;
//}
//
//int strncpy_E(const void* buf, const void* eeprom_ptr, size_t size)
//{
//	const void* buf_beg = buf;
//	while ( size-- && (*(uchar*) buf = eeprom_read_byte(eeprom_ptr)) != 0 )
//	{
//		++buf;
//		++eeprom_ptr;
//	}
//	return buf - buf_beg;
//}

//int strncmp_E(const void* buf, const void* eeprom_ptr, size_t size)
//{
//	if ( size == 0 )
//		return 0;
//	while ( (*(uchar*) buf == eeprom_read_byte(eeprom_ptr)) && --size )
//	{
//		if ( *(uchar*) buf == 0 )
//			break;
//		++buf;
//		++eeprom_ptr;
//	}
//	return *(uchar*)buf - eeprom_read_byte(eeprom_ptr);
//}

//int strncmp_any_char_E(const void* buf, const void* eeprom_ptr, size_t size)
//{
//	if ( size == 0 )
//		return 0;
//	while ( (toupper(*(uchar*) buf) == toupper(eeprom_read_byte(eeprom_ptr))) && --size )
//	{
//		if ( *(uchar*) buf == 0 )
//			break;
//		++buf;
//		++eeprom_ptr;
//	}
//	return *(uchar*)buf - eeprom_read_byte(eeprom_ptr);
//}

//int strnlen_E(const void* eeprom_ptr, size_t size)
//{
//	int i = 0;
//	while ( eeprom_read_byte(eeprom_ptr++) != 0 && size-- )
//		++i;
//	return i;
//}

//uint32_t eeprom_read_long(const uint32_t *addr)
//{
//	ulong l;
//	const uchar* ptr = (uchar*) addr;
//	uchar* ptr2 = (uchar*) &l;
//	*ptr2++ = eeprom_read_byte(ptr++);
//	*ptr2++ = eeprom_read_byte(ptr++);
//	*ptr2++ = eeprom_read_byte(ptr++);
//	*ptr2++ = eeprom_read_byte(ptr++);
//	return l;
//}

uint32_t eeprom_read_3char(const uint32_t *addr)
{
	ulong l = 0;
	const uchar* ptr = (uchar*) addr;
	uchar* ptr2 = (uchar*) &l;
	*ptr2++ = eeprom_read_byte(ptr++);
	*ptr2++ = eeprom_read_byte(ptr++);
	*ptr2++ = eeprom_read_byte(ptr++);
	return l;
}


// pozosta-o na przysz-o??
/*
void odczytaj_znaki_z_eeprom(const uint pozycja_w_eeprom_, const uchar liczba_znakow)
{
	// wcze?niej sprawdzono, czy mo?na odczytywa?
	for (uchar i = 0; i < liczba_znakow; ++i)
	{
		EEPROM_READ_ATMEGA8(pozycja_w_eeprom_ + i, bufor_eeprom[i]);
	}
}
*/

/*	case KOMENDA_KOLEJKI_TEST_EEPROM1:
	{
		if ( czy_wolny_eeprom() )
		{
			for (uchar i = 0; i < MAX_BUFOR_EEPROM; ++i)
				bufor_eeprom[i] = i;
			zapisz_znaki_w_eeprom(0, bufor_eeprom, MAX_BUFOR_EEPROM);
			dodaj_komende(KOMENDA_KOLEJKI_TEST_EEPROM2);
		}
		else
			dodaj_komende(aktualnie_wykonywana_komenda);
		break;
	}
	case KOMENDA_KOLEJKI_TEST_EEPROM2:
	{
		if ( czy_wolny_eeprom() )
		{
			ZAPAL_DIODE_CZERWONA();
			for (uchar i = 0; i < MAX_BUFOR_EEPROM; ++i)
				bufor_eeprom[i] = 0x00;
			odczytaj_znaki_z_eeprom(0, MAX_BUFOR_EEPROM);
			zapisz_znaki_w_eeprom(MAX_BUFOR_EEPROM, bufor_eeprom, MAX_BUFOR_EEPROM);
		}
		else
			dodaj_komende(aktualnie_wykonywana_komenda);
		break;
	}
*/
