
#ifndef SIM900C_H
#define SIM900C_H

#include "narzedzia.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "poleceniagsm.h"
//#include "konfiguracja.h"
#include "pamiec_ram.h"

#ifndef TEST_ATMEGA128
#include "pin_ATmega328.h"
#else
#include "pin_ATmega128.h"
#endif

#define CZY_ODBIERANIE_DANYCH_SIM900()												IS_HI(ACSR_SIM900, ACIS1)
#define WYLACZ_PRZERWANIE_ODBIORU_DANYCH_SIM900()							SET_LO(UCSRB_SIM900, RXCIE)	// korzysta z STS i ANDI
#define WLACZ_PRZERWANIE_ODBIORU_DANYCH_SIM900()							SET_HI(UCSRB_SIM900, RXCIE)	// korzysta z STS i ANDI
#define CZY_WYSYLANIE_DANYCH_SIM900()													IS_HI(UCSRB_SIM900, TXCIE)	// przerwanie nigdy nie jest wy³aczane w trakcie wysy³ania
#define WYLACZ_PRZERWANIE_WYSYLANIA_DANYCH_SIM900()						SET_LO(UCSRB_SIM900, TXCIE)
#define WLACZ_PRZERWANIE_WYSYLANIA_DANYCH_SIM900()						SET_HI(UCSRB_SIM900, TXCIE)
#define CZY_PRZESYLANIE_DANYCH_SIM900()												( CZY_ODBIERANIE_DANYCH_SIM900() || CZY_WYSYLANIE_DANYCH_SIM900() )
#define CZY_MOZNA_WYSYLAC_DANE_SIM900()												(!CZY_WYSYLANIE_DANYCH_SIM900() && podlaczony_modul_gsm_SIM900)
#define CZY_HANDSHAKING_CTS_ZEZWALA_NA_TRANSMISJE_SIM900() 		IS_LO(PIN_CTS_SIM900, CTS_WEJSCIE_SIM900)
#define USTAW_HANDSHAKING_RTS_NA_ODBIOR_SIM900()							SET_LO(PORT_RTS_SIM900, RTS_WYJSCIE_SIM900)
#define USTAW_HANDSHAKING_RTS_ZABLOKUJ_ODBIOR_SIM900()				SET_HI(PORT_RTS_SIM900, RTS_WYJSCIE_SIM900)
#define WLACZ_PWRKEY_SIM900()																	SET_HI(DDR_PWRKEY_SIM900, PWRKEY_WYJSCIE_SIM900)
#define WYLACZ_PWRKEY_SIM900()																SET_LO(DDR_PWRKEY_SIM900, PWRKEY_WYJSCIE_SIM900)
#define STATUS_WLACZONY_SIM900()															IS_HI(PIN_STATUS_SIM900, STATUS_WEJSCIE_SIM900)
//#define WLACZ_RESET_SIM900()																	SET_HI(DDR_NRESET_SIM900, NRESET_WYJSCIE_SIM900)
//#define WYLACZ_RESET_SIM900()																	SET_LO(DDR_NRESET_SIM900, NRESET_WYJSCIE_SIM900)
//#define ROZPOCZNIJ_WYLACZENIE_INTERNETU_SIM900()							SET_LO(PORT_DTR_SIM900, DTR_WYJSCIE_SIM900)
//#define ZAKONCZ_WYLACZENIE_INTERNETU_SIM900()									SET_HI(PORT_DTR_SIM900, DTR_WYJSCIE_SIM900)
//#define CZY_WLACZONY_TRYB_PRZEZROCZYSTY_INTERNETU()						IS_LO(PIN_DCD_SIM900, DCD_WEJSCIE_SIM900)
//#define CZY_OTRZYMANO_POLACZENIE()														IS_LO(PIN_RI_SIM900, RI_WEJSCIE_SIM900)

#define MAX_LICZBA_ODBIERANYCH_ZNAKOW_SIM900    380
extern uchar odebrany_blok_SIM900[MAX_LICZBA_ODBIERANYCH_ZNAKOW_SIM900+1];
extern volatile uint liczba_odebranych_znakow_SIM900;
extern volatile uchar licznik_opoznienie_oczekiwania_na_bajt_SIM900;
#define MAX_OPOZNIENIE_OCZEKIWANIA_NA_BAJT_SIM900_500_MS		50

//#define wysylany_blok_SIM900	(&pamiec_ram[START_wysylany_blok_leon])
extern volatile uint nr_wyslanego_znaku_SIM900;
extern volatile uchar sprawdzaj_wejscie_CTS_SIM900;
extern volatile uint liczba_wysylanych_znakow_SIM900;

//extern uchar tekst_odebranego_smsa[MAX_LICZBA_ZNAKOW_SMS+1];

//extern uchar tekst_wysylanego_smsa[MAX_LICZBA_ZNAKOW_SMS+1];
//extern uchar tekst_wysylanego_echa[MAX_LICZBA_ZNAKOW_SMS+1];
//extern uchar bufor_dane_bloku_internetu[MAX_LICZBA_DANYCH_DO_INTERNETU];

	// Uwaga: wykonuje sei()
#define ustaw_odbior_SIM900()													\
{																											\
	liczba_odebranych_znakow_SIM900 = 0;								\
	licznik_opoznienie_oczekiwania_na_bajt_SIM900 = 0;	\
	SET_HI(ACSR_SIM900, ACIS1);													\
	WYKONAJ_CLI_SEI(SET_HI(UCSRB_SIM900, RXCIE));				\
	USTAW_HANDSHAKING_RTS_NA_ODBIOR_SIM900();						\
}

	// Uwaga: wykonuje sei()
#define zakoncz_odbieranie_SIM900()											\
{																												\
	USTAW_HANDSHAKING_RTS_ZABLOKUJ_ODBIOR_SIM900();				\
	WYKONAJ_CLI_SEI(SET_LO(UCSRB_SIM900, RXCIE); SET_LO(ACSR_SIM900, ACIS1));	\
}

	// Uwaga: wykonuje sei()
#define zakoncz_wysylanie_SIM900()							\
{																								\
	WYKONAJ_CLI_SEI(SET_LO(UCSRB_SIM900, TXCIE));	\
}

	// Uwaga: wykonuje sei()
#define zakoncz_przesylanie_SIM900()										\
{																												\
	USTAW_HANDSHAKING_RTS_ZABLOKUJ_ODBIOR_SIM900();				\
	WYKONAJ_CLI_SEI(SET_LO2(UCSRB_SIM900, RXCIE, TXCIE); SET_LO(ACSR_SIM900, ACIS1));	\
}

#ifdef WLACZ_PODGLAD_TRANSMISJI_WYSYLANIA_SIMCOM_NA_USB
#define WSTAW_ZNAK_DO_BUFORA_SIM900()					\
{																							\
	uint l = nr_wyslanego_znaku_SIM900;					\
	uchar znak = wysylany_blok_SIM900[l];				\
	nr_wyslanego_znaku_SIM900 = ++l;						\
	UDR_USB = znak;														  \
	UDR_SIM900 = znak;													\
}
#else
#define WSTAW_ZNAK_DO_BUFORA_SIM900()					\
{																						\
	uint l = nr_wyslanego_znaku_SIM900;					\
	uchar znak = wysylany_blok_SIM900[l];				\
	nr_wyslanego_znaku_SIM900 = ++l;						\
	UDR_SIM900 = znak;													\
}
#endif

enum KomendySIM900
{
	BRAK_KOMENDY_SIM900,
	OK_KOMENDA_SIM900,
	MO_RING_KOMENDA_SIM900,
	NO_CARRIER_KOMENDA_SIM900,
	ERROR_KOMENDA_SIM900,
	CME_ERROR_KOMENDA_SIM900,
	CMS_ERROR_KOMENDA_SIM900,
	BUSY_KOMENDA_SIM900,
	CMTI_KOMENDA_SIM900,
	NO_DIALTONE_KOMENDA_SIM900,
	CLIP_KOMENDA_SIM900,
	WYSLIJ_PDU_KOMENDA_SIM900,
	MO_CONNECTED_KOMENDA_SIM900,
	CMGS_KOMENDA_SIM900,
	CUSD_KOMENDA_SIM900,
	CONNECT_KOMENDA_SIM900,
	CONNECT_FAIL_KOMENDA_SIM900,
	SHUT_OK_KOMENDA_SIM900,
	CLOSED_KOMENDA_SIM900,
	DANE_ZE_STRONY_INTERNETOWEJ_KOMENDA_SIM900,
	TCP_ERROR_KOMENDA_SIM900,
	//NO_ANSWER_KOMENDA_SIM900,
	DTMF_KOMENDA_SIM900,
	//PROCEEDING_KOMENDA_SIM900,
	//CMT_KOMENDA_SIM900,
	//BMT_KOMENDA_SIM900,
	//CDS_KOMENDA_SIM900,

	BLAD_SIM900 = 128,
	KOMENDA_SIM900_WYSLANE_POLECENIE = 129
};

extern volatile uchar komenda_SIM900;

extern volatile uint poczatek_polecenia_SIM900;
extern volatile uint zakonczenie_polecenia_SIM900;
extern volatile uchar czy_jest_nowe_polecenie_SIM900;
extern volatile uint liczba_znakow_pdu_do_odebrania;
#define LICZBA_POLOZEN_OTRZYMANYCH_KOMEND_SIM900	5
extern volatile uint polozenie_otrzymanych_komend_SIM900[LICZBA_POLOZEN_OTRZYMANYCH_KOMEND_SIM900];

#define czy_jest_blad_SIM900()																	( komenda_SIM900 == BLAD_SIM900 )
#define czy_jest_bezczynny_SIM900()															( komenda_SIM900 == BRAK_KOMENDY_SIM900 )
#define czy_jest_komenda_SIM900()																( komenda_SIM900 != BRAK_KOMENDY_SIM900 || czy_jest_nowa_komenda_SIM900() )
#define czy_jest_komenda_OK()																		( komenda_SIM900 == OK_KOMENDA_SIM900 )
#define czy_jest_komenda_ERROR()																( komenda_SIM900 == ERROR_KOMENDA_SIM900 )
#define czy_jest_komenda_blad_CMS()															( komenda_SIM900 == CMS_ERROR_KOMENDA_SIM900 )
#define czy_jest_komenda_blad_CME()															( komenda_SIM900 == CME_ERROR_KOMENDA_SIM900 )
#define czy_jest_komenda_TCP_ERROR()														( komenda_SIM900 == TCP_ERROR_KOMENDA_SIM900 )
#define czy_jest_komenda_wyslane_polecenie_SIM900()							( komenda_SIM900 == KOMENDA_SIM900_WYSLANE_POLECENIE )
#define czy_jest_komenda_rozmowa_telefoniczna()									( komenda_SIM900 == CLIP_KOMENDA_SIM900 )
#define czy_jest_komenda_zakonczenie_rozmowy_telefonicznej()		( komenda_SIM900 == NO_CARRIER_KOMENDA_SIM900 )
#define czy_jest_komenda_wyslij_PDU()														( komenda_SIM900 == WYSLIJ_PDU_KOMENDA_SIM900 )
#define czy_jest_komenda_wyslano_sms()													( komenda_SIM900 == CMGS_KOMENDA_SIM900 )
#define czy_jest_komenda_nowy_SMS()															( komenda_SIM900 == CMTI_KOMENDA_SIM900 )
#define czy_jest_komenda_otrzymano_sms_flash()									( komenda_SIM900 == CUSD_KOMENDA_SIM900 )
#define czy_jest_komenda_uzytkownik_odebral_dzwonek()						( komenda_SIM900 == MO_RING_KOMENDA_SIM900 )
#define czy_jest_komenda_uzytkownik_odebral_rozmowe()						( komenda_SIM900 == MO_CONNECTED_KOMENDA_SIM900 )
#define czy_jest_komenda_brak_sygnalu_tonowego()								( komenda_SIM900 == NO_DIALTONE_KOMENDA_SIM900 )
#define czy_jest_komenda_telefon_zajety()												( komenda_SIM900 == BUSY_KOMENDA_SIM900 )
#define czy_jest_komenda_polaczony_z_internetem()								( komenda_SIM900 == CONNECT_KOMENDA_SIM900 )
#define czy_jest_komenda_odrzucenie_polaczenia_internetowego()	( komenda_SIM900 == CONNECT_FAIL_KOMENDA_SIM900 )
#define czy_jest_komenda_odlaczenie_serwera()										( komenda_SIM900 == CLOSED_KOMENDA_SIM900 )
#define czy_jest_komenda_odlaczenie_internetu()									( komenda_SIM900 == SHUT_OK_KOMENDA_SIM900 )
#define czy_jest_komenda_dtmf()( komenda_SIM900 == DTMF_KOMENDA_SIM900 )
#define czy_jest_komenda_otrzymano_dane_z_serwera()							( komenda_SIM900 == DANE_ZE_STRONY_INTERNETOWEJ_KOMENDA_SIM900 )
#define resetuj_komende_SIM900()																( komenda_SIM900 = BRAK_KOMENDY_SIM900 )
//#define czy_jest_komenda_dzwonek()														( komenda_SIM900 == RING_KOMENDA_SIM900 )
//#define czy_jest_komenda_nowy_CBS()															( komenda_SIM900 == BMT_KOMENDA_SIM900 )
//#define czy_jest_komenda_nowy_REPORT()													( komenda_SIM900 == CDS_KOMENDA_SIM900 )

extern uchar podlaczony_modul_gsm_SIM900;
extern uchar podlaczona_karta_SIM_SIM900;
extern volatile uchar czekanie_na_odebranie_zachety;
extern volatile uchar flaga_odebrany_znak_zachety;
extern uchar wykonywanie_rozmowy_telefonicznej;

void inicjalizacja_SIM900(void);

#define resetuj_odbior_SIM900_po_bledzie()	\
{																						\
	USTAW_HANDSHAKING_RTS_ZABLOKUJ_ODBIOR_SIM900();	\
	SET_LO(UCSRB_SIM900, RXCIE); SET_LO(ACSR_SIM900, ACIS1);							\
	poczatek_polecenia_SIM900 = 0;						\
	zakonczenie_polecenia_SIM900 = 0;					\
	czy_jest_nowe_polecenie_SIM900 = FALSE;		\
}

#define ZNAK_KONCA_WYSYLANIA_ZNAKOW_CR						0x0d	// '\r'
#define ZNAK_KONCA_ODBIERANIA_ZNAKOW_LF						0x0a	// '\n'
#define ZNAK_CTRL_Z																0x1a
#define ZNAK_POCZATEK_DANYCH_Z_SERWERA						0x08
#define ZNAK_KONIEC_DANYCH_Z_SERWERA							0x07

uchar procedura_inicjalizacyjna_SIM900_100MS(void);
void wyslij_polecenie_ROM_SIM900(PGM_P polecenie);	// nale¿y wpisaæ tylko instrukcjê,
void wyslij_polecenie_RAM_SIM900(void);	// np. "+CCLK?"
#define wysylane_dane_RAM_SIM900 (wysylany_blok_SIM900+2)
void wyslij_znaki_SIM900(const uint liczba_znakow);	// trzeba wpisaæ at...\r

extern uchar licznik_100ms_procedura_inicjalizacyjna_SIM900;
#define WYKONAJ_PROCEDURE_INICJALIZACYJNA_SIM900()	do { licznik_100ms_procedura_inicjalizacyjna_SIM900 = 1; } while ( 0 )
#define HARDRESET_SIM900()													do { licznik_100ms_procedura_inicjalizacyjna_SIM900 = 150; } while ( 0 )

#endif
