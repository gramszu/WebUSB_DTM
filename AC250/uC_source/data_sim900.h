
#ifdef DATA_SIM900_H
#error Dwa razy wlaczany plik
#endif

#include "adresyeeprom.h"
#include "enumkomendy.h"

uchar opoznienie_SIM900_100MS = 0;
uchar oproznij_bufor_SIM900_po_bledzie = FALSE;

#define ROZMIAR_NAZWA_OPERATORA 11
uchar nazwa_operatora[ROZMIAR_NAZWA_OPERATORA];

#define czy_pobrany_operator() (nazwa_operatora[0] != 0)

komenda_typ aktualnie_wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_BRAK_KOMENDY;
uint licznik_wysylane_polecenie_SIM900 = 0;
#define MAX_LICZNIK_WYSYLANE_POLECENIE_SIM900 (10 * 50)

komenda_typ nastepne_wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_BRAK_KOMENDY;

uchar liczba_wykonanych_komend_identycznego_polecenia;
#define MAX_LICZBA_WYKONANYCH_KOMEND_IDENTYCZNEGO_POLECENIA 3

uchar max_oczekiwanie_na_odpowiedz = 3;
#define wysylane_polecenie_SIM900                                              \
  licznik_wysylane_polecenie_SIM900 = 0;                                       \
  aktualnie_wysylane_polecenie_SIM900

uchar trwa_rozmowa_przychodzaca_od_uzytkownika = FALSE;

#define flaga_trwa_rozmowa_wychodzaca                                          \
  (wykonywanie_rozmowy_telefonicznej ||                                        \
   licznik_bezpieczenstwa_wykonywana_rozmowa)

uint licznik_bezpieczenstwa_wykonywana_rozmowa = 0;

#define MAX_DLUGOSC_PDU 180
uchar bufor_pdu[MAX_DLUGOSC_PDU];
uchar dlugosc_pdu = 0;

uchar numer_telefonu_odebranego_smsa[MAX_LICZBA_ZNAKOW_TELEFON + 1];
// #define tekst_odebranego_smsa
// (&pamiec_ram[START_tekst_odebranego_smsa])
uchar flaga_odczytywanie_smsa = FALSE;

// uchar tekst_wysylanego_smsa[MAX_LICZBA_ZNAKOW_SMS+1];
uchar numer_telefonu_wysylanego_smsa[MAX_LICZBA_ZNAKOW_TELEFON + 1];
komenda_typ flaga_wysylanie_smsa = FALSE;

uchar liczba_prob_wyslania_smsa = 0;
#define max_liczba_prob_wyslania_smsa 2

uchar numer_telefonu_ktory_dzwoni[MAX_LICZBA_ZNAKOW_TELEFON + 1];

uchar numer_telefonu_do_ktorego_dzwonic[MAX_LICZBA_ZNAKOW_TELEFON + 1];
uchar maksymalny_czas_dzwonienia = 0;
uchar ustaw_maksymalny_czas_dzwonienia = 0;

uchar liczba_smsow_ech_do_wyslania = 0;

uint licznik_awaria_brak_zasiegu = 0;
#define MAX_LICZNIK_AWARIA_BRAK_ZASIEGU 600 // 60 sek.

uchar licznik_blad_stanu_karty_SIM = 0;
#define LICZNIK_BLAD_KARTY_SIM 6
#define CZY_BLAD_KARTY_SIM()                                                   \
  (licznik_blad_stanu_karty_SIM > LICZNIK_BLAD_KARTY_SIM - 1)

uchar licznik_blad_zalogowania_u_operatora = 0;
#define LICZNIK_BLAD_ZALOGOWANIA_U_OPERATORA 20
#define CZY_BLAD_ZALOGOWANIA_U_OPERATORA()                                     \
  (licznik_blad_zalogowania_u_operatora >                                      \
   LICZNIK_BLAD_ZALOGOWANIA_U_OPERATORA - 1)

uint licznik_ogolny_blad_zalogowania = 0;
#define MAX_LICZNIK_OGOLNY_BLAD_ZALOGOWANIA (10 * 60 * 2)

uint licznik_bezpieczenstwa_opoznienie_wysylania_sms = 0;
#define MAX_LICZNIK_BEZPIECZENSTWA_OPOZNIENIE_WYSYLANIA_SMS 10

#define czy_pierwsza_komenda_odrzucenia_rozmowy()                              \
  (liczba_wysylanych_znakow_SIM900 == 2 && odebrany_blok_SIM900[0] == 'y')

uchar licznik_oczekiwanie_na_potwierdzenie_wyslania_znakow;
#define MAX_LICZNIK_OCZEKIWANIE_NA_POTWIERDZENIE_WYSLANIA_ZNAKOW 150

uchar max_oczekiwanie_na_odpowiedz_at = 0;

uchar blokada_clip = FALSE;

uchar opoznienie_zatrzymaj_odpytywanie_urzadzenia;

volatile uchar watchdog_sms_aktywny = FALSE;
volatile uint watchdog_sms_licznik_100ms = 0;
volatile uchar watchdog_sms_trwa_reset = FALSE;
volatile uint watchdog_sms_safe_mode_100ms = 0;

extern uchar tryb_clip;
extern uchar tryb_pracy;
extern uint licznik_timeout_rozmowy_100ms;
#define MAX_LICZNIK_TIMEOUT_ROZMOWY_100MS 300 // 30 sekund

// SMS timestamp synchronization
// DATE COMPONENTS (Added for Service Lock)
extern uchar sms_timestamp_rok;
extern uchar sms_timestamp_miesiac;
extern uchar sms_timestamp_dzien;

extern uchar sms_timestamp_godzina;
extern uchar sms_timestamp_minuta;
extern uchar sms_timestamp_sekunda;
uchar sms_pomijaj_aktualizacje_czasu = FALSE; // TRUE dla komendy SET

// USSD - minimalistyczna implementacja (10s timeout)
uchar oczekiwanie_na_ussd = FALSE;
uint licznik_timeout_ussd_100ms = 0;
