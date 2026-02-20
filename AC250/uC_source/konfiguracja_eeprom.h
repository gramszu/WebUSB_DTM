
#include "adresyeeprom.h"

#define USTAWIENIE_WYJSCIA_MASKA_STEROWANIE_DOWOLNY_UZYTKOWNIK		BIT(15)

#define ustawienie_wyjscia_czy_steruje_dowolny_uzytkownik(par)		FALSE
#define ustawienie_wyjscia_czas_wlaczenia_wyjscia(par)						2
#define ustawienie_wyjscia_czy_ustawione_wyjscie_czasowe(par)			TRUE

#define PARAMETRY_WEJSCIA_MASKA_TRYB								0x01
#define PARAMETRY_WEJSCIA_TRYB_NORMALNIE_OTWARTY		0x00
#define PARAMETRY_WEJSCIA_TRYB_NORMALNIE_ZAMKNIETY	0x01
#define TRYB_WEJSCIA(PAR)														((PAR) & PARAMETRY_WEJSCIA_MASKA_TRYB)

#define PARAMETRY_WEJSCIA_MASKA_WYZWALANIE					0x02
#define PARAMETRY_WEJSCIA_WYZWALANIE_PLUSEM					0x00
#define PARAMETRY_WEJSCIA_WYZWALANIE_MINUSEM				0x02
#define CZY_WYZWALANIE_PLUSEM(PAR)									(((PAR) & PARAMETRY_WEJSCIA_MASKA_WYZWALANIE) == PARAMETRY_WEJSCIA_WYZWALANIE_PLUSEM)

#define PARAMETRY_WEJSCIA_MASKA_KONTROLOWANIE_WEJSCIA			0x04
#define PARAMETRY_WEJSCIA_WYLACZONE_KONTROLOWANIE_WEJSCIA	0x00
#define PARAMETRY_WEJSCIA_WLACZONE_KONTROLOWANIE_WEJSCIA	0x04
#define CZY_WLACZONE_KONTROLOWANIE_WEJSCIA(PAR)						((PAR) & PARAMETRY_WEJSCIA_MASKA_KONTROLOWANIE_WEJSCIA)

#define czy_aktywny_numer_telefonu_brama(POS)			(eeprom_read_byte((void*) EEPROM_NUMER_TELEFONU_BRAMA(POS)) != 0xff)
#define kopiuj_numer_telefonu_brama(POS, S)				kopiuj_blok_eeprom_na_telefon((void*) EEPROM_NUMER_TELEFONU_BRAMA(POS), (S), MAX_LICZBA_ZNAKOW_TELEFON + 1)
#define porownaj_numer_telefonu_brama(POS, S)			porownaj_numer_telefonu((S), (void*) EEPROM_NUMER_TELEFONU_BRAMA(POS))
