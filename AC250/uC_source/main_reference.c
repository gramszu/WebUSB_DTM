
#ifndef INCLUDE
#include "narzedzia.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/crc16.h>
#include <util/delay.h>

#ifndef TEST_ATMEGA128
#include "pin_ATmega328.h"
#else
#include "pin_ATmega128.h"
#endif
#include "bufpomoc.h"
#include "data_sim900.h"
#include "interpretacjaSMS.h"
#include "komendy.h"
#include "konfiguracja.h"
#include "pdu.h"
#include "sim900.h"
#include "wewy.h"
#include "zapiseeprom.h"

#include "test_pcb.h"

#endif

static const char nazwa_urzadzenia[12 + 1] PROGMEM = "BRAMA";

volatile uint32_t g_czas_systemowy_100ms = 0;

#define INFORMACJA_W_RAPORCIE "www.sonfy.pl"

uchar licznik_100ms_dioda_led;
uchar liczba_blyskow_led = 0;
uchar stan_cyklu_blysku =
    0; // 0-5: 0-1=ON (200ms), 2-5=OFF (400ms), 1 blysk=600ms

#define zapal_diode_led(czas_100ms) (licznik_100ms_dioda_led = (czas_100ms) + 1)
#define zapal_diode_led_blyski(liczba)                                         \
  (liczba_blyskow_led = (liczba), stan_cyklu_blysku = 0)
// #define zapal_diode_led(czas_100ms)

volatile uchar wykonaj_zdarzenie_timer = FALSE;
#define CZY_WYKONAC_ZDARZENIE_TIMER() wykonaj_zdarzenie_timer

#define WYKONAJ_WATKI_BRAK 0
#define WYKONAJ_WATKI_10MS BIT(0)
#define WYKONAJ_WATKI_100MS BIT(1)
volatile uchar wykonac_watki_10MS = FALSE;
uchar wykonac_watki = WYKONAJ_WATKI_BRAK;
#define CZY_WYKONAC_WATKI_10MS() (wykonac_watki & WYKONAJ_WATKI_10MS)
#define CZY_WYKONAC_WATKI_100MS() (wykonac_watki & WYKONAJ_WATKI_100MS)
#define RESETUJ_WYKONANIE_WATKOW() (wykonac_watki = 0)

volatile uchar licznik_wybudz_watki_10MS = 0;
#define OPOZNIENIE_TIMERA_0 18
#define czy_pozostal_czas(procent)                                             \
  (licznik_wybudz_watki_10MS < (100 - (procent)) * OPOZNIENIE_TIMERA_0 / 100)

przerwanie_timer() {
  if (++licznik_wybudz_watki_10MS >= OPOZNIENIE_TIMERA_0) {
    licznik_wybudz_watki_10MS = 0;
    wykonac_watki_10MS = TRUE;
  }

  if (STATUS_WLACZONY_SIM900()) {
    if (!CZY_ODBIERANIE_DANYCH_SIM900() && czy_jest_bezczynny_SIM900()) {
      ustaw_odbior_SIM900();
    }
    if (CZY_HANDSHAKING_CTS_ZEZWALA_NA_TRANSMISJE_SIM900()) {
      cli();
      if (CZY_WYSYLANIE_DANYCH_SIM900()) {
        WYLACZ_PRZERWANIE_WYSYLANIA_DANYCH_SIM900();
        sei();
        uchar p = sprawdzaj_wejscie_CTS_SIM900;
        if (p && (podlaczony_modul_gsm_SIM900 ||
                  aktualnie_wysylane_polecenie_SIM900 ==
                      KOMENDA_KOLEJKI_WYSLIJ_DO_SIM900_AT)) {
          p = FALSE;
          sprawdzaj_wejscie_CTS_SIM900 = p;
          WSTAW_ZNAK_DO_BUFORA_SIM900();
        }
        cli();
        WLACZ_PRZERWANIE_WYSYLANIA_DANYCH_SIM900();
      }
      sei();
    }
  } else {
    zakoncz_przesylanie_SIM900();
  }

  wykonaj_zdarzenie_timer =
      TRUE; // uruchamia zdarzenie_timer, ktra korzysta z pozostaych zmiennych
}

uchar modul_zalogowany_w_sieci = FALSE;

uchar licznik_reset_modulu_SIM900 = 0;
#define START_LICZNIK_RESET_MODULU_SIM900 30

uchar licznik_reset_urzadzenia = 0;
#define LICZNIK_RESET_URZADZENIA_RESET_SIM900 200
#define START_LICZNIK_RESET_URZADZENIA 220

uint opoznienie_wysylania_clipow_100MS = 0;

void generuj_raport_sieci(uchar **buf_sms) {
  static const char tekst_gsm[] PROGMEM = "AC800 DTM-F1 T";
  uchar *ptr = *buf_sms;

  memcpy_R(ptr, tekst_gsm);
  ptr += sizeof tekst_gsm - 1;
  *ptr++ = '\n';

  strcpy_P((char *)ptr, PSTR("Czas: "));
  ptr += strlen((char *)ptr);
  strcat((char *)ptr, rtc_czas);
  ptr += strlen((char *)ptr);
  *ptr++ = '\n';

  static const char text_sygnal[] PROGMEM = "Sygnal GSM ";
  memcpy_R(ptr, text_sygnal);
  ptr += sizeof text_sygnal - 1;

  if (poziom_sieci_gsm <= 31 && modul_zalogowany_w_sieci) {
    utoa(poziom_sieci_gsm * 100 / 31, ptr, 10);
    ptr += strlen(ptr);
    *ptr++ = '%';
  } else {
    *ptr++ = '-';
    *ptr++ = '-';
    *ptr++ = '-';
  }
  *ptr = 0;
  *buf_sms = ptr;
}

void generuj_raport_uzytkownikow_1(uchar **buf_sms) {
  static const char tekst_gsm[] PROGMEM = "Uzytkownicy ";
  uchar *ptr = *buf_sms;

  memcpy_R(ptr, tekst_gsm);
  ptr += sizeof tekst_gsm - 1;
  uint aktywne_numery = 0;
  uint wolne_numery = 0;
  // for (uchar nr_uzyt_clip = 0; nr_uzyt_clip <
  // MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip)
  for (uint nr_uzyt_clip = 0; nr_uzyt_clip < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA;
       ++nr_uzyt_clip) {
    if (czy_aktywny_numer_telefonu_brama(nr_uzyt_clip))
      ++aktywne_numery;
    else
      ++wolne_numery;
  }
  utoa(aktywne_numery, ptr, 10);
  ptr += strlen(ptr);
  *ptr++ = '/';
  utoa(wolne_numery, ptr, 10);
  ptr += strlen(ptr);
  *buf_sms = ptr;
}

void generuj_raport_uzytkownikow(uchar **buf_sms) {
  static const char tekst_gsm[] PROGMEM = "Uzytkownicy ";
  uchar *ptr = *buf_sms;

  memcpy_R(ptr, tekst_gsm);
  ptr += sizeof tekst_gsm - 1;
  uchar aktywne_numery = 0;
  uchar wolne_numery = 0;
  for (uchar nr_uzyt_clip = 0;
       nr_uzyt_clip < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip) {
    if (czy_aktywny_numer_telefonu_brama(nr_uzyt_clip))
      ++aktywne_numery;
    else
      ++wolne_numery;
  }
  utoa(aktywne_numery, ptr, 10);
  ptr += strlen(ptr);
  *ptr++ = '/';
  utoa(wolne_numery, ptr, 10);
  ptr += strlen(ptr);
  *buf_sms = ptr;
}

void generuj_raport_stanu_urzadzenia(void) {
  uchar *sms = (char *)tekst_wysylanego_smsa;
  *sms++ = '*';
  *sms++ = '\n';
  generuj_raport_sieci(&sms);
  *sms++ = '\n';
  generuj_raport_uzytkownikow_1(&sms);
  *sms++ = '\n';

  if (eeprom_read_byte((const uint8_t *)ADRES_EEPROM_TRYB_PRACY) == 0) {
    strcpy_P((char *)sms, PSTR("Tryb: Prywatny"));
  } else {
    strcpy_P((char *)sms, PSTR("Tryb: Publiczny"));
  }
  sms += strlen((char *)sms);
  *sms++ = '\n';

  if (czas_start_h == 0xFF) {
    strcpy_P((char *)sms, PSTR("Harmonogram: Wylaczony"));
  } else {
    sprintf((char *)sms, "Harmonogram: %02d:%02d %02d:%02d", (int)czas_start_h,
            (int)czas_start_m, (int)czas_stop_h, (int)czas_stop_m);
  }
  sms += strlen((char *)sms);
  *sms++ = '\n';

  static const char tekst_demo[] PROGMEM = INFORMACJA_W_RAPORCIE;
  strcpy_P((char *)sms, tekst_demo);
}

void ustaw_wyjscie_clip(void) {
  stan_wyjscie[0] = TRUE;
  licznik_przelacznik_wyjscia[0] = 2 * 10ul; // 2 sekundy
}

uchar kod_modulu[LICZBA_BAJTOW_KODU_DOSTEPU];

uchar nie_wysylaj_echa_z_powodu_nietypowego_smsa;

uchar numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama;

// Blokada komend REPORT/USER - maksymalnie 8 w ciągu 30 sekund
static uchar licznik_report_user = 0;
static uint timer_report_user_100ms = 0;
#define MAX_LICZBA_KOMEND_REPORT_USER_W_OKNIE 8
#define OKNO_CZASOWE_REPORT_USER_100MS (30 * 10) // 30 sekund

// Mechanizm wykrywania zablokowanej kolejki i czyszczenia
static komenda_typ ostatnia_pierwsza_komenda = KOMENDA_KOLEJKI_BRAK_KOMENDY;
static uint licznik_kolejka_stoi_100ms = 0;
#define MAX_CZAS_KOLEJKA_STOI_100MS                                            \
  (15 * 10) // 15 sekund - jesli kolejka stoi tyle czasu, wyczysc

static uint licznik_usunietych_sms_przez_limit = 0;
static uint licznik_awaryjnych_resetow_kolejki = 0;

#define WATCHDOG_WYSYLANIA_SMS_100MS (30 * 10) // 30 sekund
static uint licznik_watchdog_wysylanie_smsa_100ms = 0;
static uchar liczba_kolejnych_watchdogow_wysylania = 0;
#define WATCHDOG_SMS_TIMEOUT_100MS (10 * 10) // 10 sekund
#define WATCHDOG_SMS_SAFE_MODE_100MS                                           \
  (5 * 10) // 5 sekund po resecie - nie przyjmuj SMS

static inline void watchdog_sms_arm(void) {
  watchdog_sms_aktywny = TRUE;
  watchdog_sms_licznik_100ms = 0;
}

static inline void watchdog_sms_disarm(void) {
  watchdog_sms_aktywny = FALSE;
  watchdog_sms_licznik_100ms = 0;
}

static inline void sygnalizuj_pelny_system(void) {
  // 10 szybkich blyskow informuje, ze kolejka/limit SMS jest zapelniony
  zapal_diode_led_blyski(10);
}

// Funkcja usuwania zablokowanego SMS z modulu SIM900
static void usun_zablokowany_sms(void) {
  // Usun SMS z modulu SIM900 (numer jest w aktualnie_wysylane_polecenie_SIM900)
  // Sprawdzamy czy to byla komenda odczytu SMS
  if (aktualnie_wysylane_polecenie_SIM900 >= KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_1 &&
      aktualnie_wysylane_polecenie_SIM900 <= KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_20) {
    const uchar nr_smsa =
        aktualnie_wysylane_polecenie_SIM900 - KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_1;
    // Uzywamy filtruj_i_dodaj_komende zamiast dodaj_komende
    filtruj_i_dodaj_komende(KOMENDA_KOLEJKI_USUN_SMSA_1 + nr_smsa);
  } else {
    // Fallback: Usun wszystkie
    filtruj_i_dodaj_komende(KOMENDA_KOLEJKI_USUN_WSZYSTKIE_SMSY);
  }
}

static void zareaguj_na_usuniety_sms_z_powodu_limitu(void) {
  if (licznik_usunietych_sms_przez_limit < 0xFFFF)
    ++licznik_usunietych_sms_przez_limit;
  sygnalizuj_pelny_system();
  watchdog_sms_disarm();
  // Uzywamy filtruj_i_dodaj
  filtruj_i_dodaj_komende(KOMENDA_KOLEJKI_USUN_WSZYSTKIE_SMSY);
}

// Funkcja zapisu znacznikow debugowania do EEPROM
static void zapisz_debug_do_eeprom(uchar komenda, uchar akcja) {
  if (!czy_wolny_eeprom())
    return;

  uchar buf[8];
  buf[0] = licznik_report_user;
  buf[1] = (uchar)(timer_report_user_100ms & 0xFF);
  buf[2] = (uchar)((timer_report_user_100ms >> 8) & 0xFF);
  buf[3] = (uchar)flaga_wysylanie_smsa;

  uchar liczba_sms_w_kolejce = 0;
  for (uchar i = 0; i < LICZBA_KOMEND; ++i) {
    if (komendy_kolejka[i] >= KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT &&
        komendy_kolejka[i] <= KOMENDA_KOLEJKI_WYSLIJ_SMSA_PDU)
      ++liczba_sms_w_kolejce;
  }
  buf[4] = liczba_sms_w_kolejce;
  buf[5] = komenda;
  buf[6] = akcja;

  zapisz_znaki_w_eeprom(buf, EEPROM_DEBUG_START, 7);
}

void wykonanie_polecenia_sms(void) {
  tekst_odebranego_smsa[MAX_LICZBA_ZNAKOW_SMS] = 0; // (1) dla pewnoci
  uchar komenda = interpretuj_wiadomosc_sms(tekst_odebranego_smsa);

  // --- WARSTWA 1: Limit czasowy (Rate Limiting) - V7 style ---
  if (komenda == INTERPRETACJA_SMS_RAPORT ||
      komenda == INTERPRETACJA_SMS_USER) {
    if (licznik_report_user >= MAX_LICZBA_KOMEND_REPORT_USER_W_OKNIE) {
      zapisz_debug_do_eeprom(1, 1);
      usun_zablokowany_sms();
      zareaguj_na_usuniety_sms_z_powodu_limitu();
      watchdog_sms_disarm();
      return;
    }
  }

  // --- WARSTWA 2: Sprawdzenie zajętości modemu (Busy Check) ---
  if (flaga_wysylanie_smsa) {
    // Same number check
    if (strcmp((char *)numer_telefonu_odebranego_smsa,
               (char *)numer_telefonu_wysylanego_smsa) == 0) {
      zapisz_debug_do_eeprom(1, 1);
      usun_zablokowany_sms();
      zareaguj_na_usuniety_sms_z_powodu_limitu();
      watchdog_sms_disarm();
      return;
    }
    // Different number but busy
    zapisz_debug_do_eeprom(1, 1);
    usun_zablokowany_sms();
    zareaguj_na_usuniety_sms_z_powodu_limitu();
    watchdog_sms_disarm();
    return;
  }

  // --- WARSTWA 3 i 4: Ochrona kolejki ---
  uchar liczba_sms_w_kolejce = 0;
  uchar liczba_wszystkich_komend = 0;
  for (uchar i = 0; i < LICZBA_KOMEND; ++i) {
    if (komendy_kolejka[i] != KOMENDA_KOLEJKI_BRAK_KOMENDY) {
      ++liczba_wszystkich_komend;
      if (komendy_kolejka[i] >= KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT &&
          komendy_kolejka[i] <= KOMENDA_KOLEJKI_WYSLIJ_SMSA_PDU)
        ++liczba_sms_w_kolejce;
    }
  }

  if (liczba_wszystkich_komend >= 35) {
    zapisz_debug_do_eeprom(1, 1);
    usun_zablokowany_sms();
    zareaguj_na_usuniety_sms_z_powodu_limitu();
    watchdog_sms_disarm();
    return;
  }

  if (liczba_sms_w_kolejce >= 6) {
    zapisz_debug_do_eeprom(1, 1);
    usun_zablokowany_sms();
    zareaguj_na_usuniety_sms_z_powodu_limitu();
    watchdog_sms_disarm();
    return;
  }

  switch (komenda) {
  case INTERPRETACJA_SMS_POPRAWNY:
    zapal_diode_led_blyski(2); // Odbior poprawnej komendy - 2 blyski
    break;
  case INTERPRETACJA_SMS_BRAK_KODU: {
    zapal_diode_led_blyski(1); // Odbior zwyklego SMS (bez kodu) - 1 blysk
    zapisz_debug_do_eeprom(0, 3);
    break;
  }
  case INTERPRETACJA_SMS_RAPORT: {
    ++licznik_report_user;
    timer_report_user_100ms = OKNO_CZASOWE_REPORT_USER_100MS;

    strcpy((char *)numer_telefonu_wysylanego_smsa,
           (char *)numer_telefonu_odebranego_smsa);
    generuj_raport_stanu_urzadzenia();
    dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
    zapal_diode_led_blyski(2); // Komenda REPORT - 2 blyski
    zapisz_debug_do_eeprom(1, 0);
    break;
  }
  case INTERPRETACJA_SMS_USER: {
    ++licznik_report_user;
    timer_report_user_100ms = OKNO_CZASOWE_REPORT_USER_100MS;

    strcpy((char *)numer_telefonu_wysylanego_smsa,
           (char *)numer_telefonu_odebranego_smsa);

    // Konwertuj numer na format EEPROM
    konwertuj_telefon_na_blok_eeprom(
        &numer_telefonu_do_ktorego_dzwonic[0],
        &numer_telefonu_do_ktorego_dzwonic[strlen(
            (char *)numer_telefonu_do_ktorego_dzwonic)],
        &bufor_eeprom[0]);
    // Sprawdz czy numer jest na liscie
    uchar znaleziono = FALSE;
    for (uchar nr_uzyt_clip = 0;
         nr_uzyt_clip < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip) {
      if (porownaj_numer_telefonu_blok(
              &bufor_eeprom[0],
              (void *)EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt_clip))) {
        znaleziono = TRUE;
        break;
      }
    }
    // Przygotuj odpowiedz: "OK" lub "Brak"
    if (znaleziono) {
      strcpy((char *)tekst_wysylanego_smsa, "OK");
    } else {
      strcpy((char *)tekst_wysylanego_smsa, "Brak");
    }

    dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
    zapal_diode_led_blyski(2); // Komenda USER - 2 blyski
    zapisz_debug_do_eeprom(2, 0);
    break;
  }
  case INTERPRETACJA_SMS_USER_BEZ_NUMERU: {
    strcpy((char *)numer_telefonu_wysylanego_smsa,
           (char *)numer_telefonu_odebranego_smsa);
    strcpy((char *)tekst_wysylanego_smsa,
           "Wpisz numer jaki sprawdzasz, np: USER 793557357");
    dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
    zapal_diode_led_blyski(2);
    break;
  }
  case INTERPRETACJA_SMS_USER_LIST: {
    strcpy((char *)numer_telefonu_wysylanego_smsa,
           (char *)numer_telefonu_odebranego_smsa);
    numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama = 0;
    dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_RAPORT_UZYTKOWNIKOW);
    zapal_diode_led_blyski(2);
    break;
  }
  case INTERPRETACJA_SMS_RESET_WSZYSTKICH_USTAWIEN: {
    if (not czy_sa_komendy_z_przedzialu(
            KOMENDA_KOLEJKI_RESET_KOD_DOSTEPU,
            KOMENDA_KOLEJKI_RESET_USTAWIEN_INICJALIZACJA)) {
      zapal_diode_led_blyski(25);
      dodaj_komende(KOMENDA_KOLEJKI_RESET_USTAWIEN_0);
    }
    break;
  }
  default:
    break;
  }
  watchdog_sms_disarm();
}

void steruj_wejsciem_reset_100ms(void) {
  static uchar licznik_reset;
  if (CZY_AKTUALNY_STAN_LOGICZNY_ON(0)) {
    if (licznik_reset < 20 * 10)
      ++licznik_reset;
  } else {
    if (licznik_reset >= 20 * 10) {
      if (not czy_sa_komendy_z_przedzialu(
              KOMENDA_KOLEJKI_RESET_KOD_DOSTEPU,
              KOMENDA_KOLEJKI_RESET_USTAWIEN_INICJALIZACJA))
        dodaj_komende(KOMENDA_KOLEJKI_RESET_USTAWIEN_0);
    } else if (licznik_reset >= 5 * 10) {
      if (not czy_sa_komendy_z_przedzialu(
              KOMENDA_KOLEJKI_RESET_KOD_DOSTEPU,
              KOMENDA_KOLEJKI_RESET_USTAWIEN_INICJALIZACJA))
        dodaj_komende(KOMENDA_KOLEJKI_RESET_KOD_DOSTEPU);
    }
    licznik_reset = 0;
  }
}

uchar sprawdz_przychodzaca_rozmowe(void) // wysya TRUE, gdy naley odebra
{
  if (numer_telefonu_ktory_dzwoni[0] != 0)
    dodaj_komende(KOMENDA_KOLEJKI_SPRAWDZ_UZYTKOWNIKOW_BRAMA);
  return FALSE;
}

uchar wysylanie_echa_przez_pdu = FALSE;

uchar *ptr_start_pdu_z_wiadomoscia; // pierwszy znak

#define WYSYLANIE_SMSA_CLIPA_BRAK 0
#define WYSYLANIE_SMSA_WYSYLANIE 1
#define WYSYLANIE_SMSA_POWTARZANIE 2
#define WYSYLANIE_CLIPA_WYSYLANIE 4
#define WYSYLANIE_CLIPA_KONCZENIE 5

uchar wysylanie_smsa_clipa = WYSYLANIE_SMSA_CLIPA_BRAK;

enum PowodZakonczeniaRozmowyTelefonicznej {
  powod_zakonczenia_rozmowy_odrzucenie,
  powod_zakonczenia_rozmowy_zakonczenie,
  powod_zakonczenia_rozmowy_przekroczony_czas,
  powod_zakonczenia_rozmowy_otrzymana_wiadomosc
};

void zakonczono_rozmowe_telefoniczna(
    const enum PowodZakonczeniaRozmowyTelefonicznej powod) {
  POMOC_DODAJ2('#', 'a');
  opoznienie_SIM900_100MS = 60; // byo 60
  licznik_bezpieczenstwa_wykonywana_rozmowa = 0;
  ustaw_maksymalny_czas_dzwonienia = 0;
  maksymalny_czas_dzwonienia = 0;
  blokada_clip = FALSE;
}

enum PowodZakonczeniaWysylaniaSMS {
  zakonczenie_wysylania_sms_ok,
  zakonczenie_wysylania_sms_blad_powtarzanie,
  zakonczenie_wysylania_sms_blad_zakonczenie,
  zakonczenie_wysylania_sms_blad_powtorz_sms,
};

void zakonczono_wysylanie_smsa(const enum PowodZakonczeniaWysylaniaSMS powod) {
  POMOC_DODAJ2('#', 'A');
  opoznienie_SIM900_100MS = 60;

  wysylanie_smsa_clipa = WYSYLANIE_SMSA_CLIPA_BRAK;
}

void problem_z_wyslaniem_powiadomienia(void) {
  wykonywanie_rozmowy_telefonicznej = FALSE;
  licznik_bezpieczenstwa_wykonywana_rozmowa = 0;
  wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_BRAK_KOMENDY;
  czekanie_na_odebranie_zachety = FALSE;
  opoznienie_SIM900_100MS = 60;
  flaga_wysylanie_smsa = 0;
}

#define JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ()                             \
  if (!czy_mozna_wysylac_dane_do_SIM900) {                                     \
    dodaj_komende(wykonywana_komenda);                                         \
    break;                                                                     \
  }                                                                            \
  wysylane_polecenie_SIM900 = wykonywana_komenda;

#define POWTORZ_JESLI(WAR)                                                     \
  {                                                                            \
    if (WAR) {                                                                 \
      dodaj_komende(wykonywana_komenda);                                       \
      break;                                                                   \
    }                                                                          \
  }

#define JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ() POWTORZ_JESLI(!czy_wolny_eeprom())

#define JESLI_SIM900_ZAJETY_WYKONAJ_POZNIEJ() POWTORZ_JESLI(czy_gsm_zajety)

#include "main_sim900.h"

#define aktualnie_wykonywana_komenda wykonywana_komenda

void inicjalizuj_parametry_modulu(void);

#define MAX_LICZBA_KOMEND_DLA_ALARMOW (LICZBA_KOMEND / 4)

uchar wykonanie_komend_ukladow(void) {
  const komenda_typ aktualnie_wykonywana_komenda = komendy_kolejka[0];

  switch (aktualnie_wykonywana_komenda) {
  case KOMENDA_KOLEJKI_RESET_KOD_DOSTEPU: {
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
    kod_modulu[0] = bufor_eeprom[0] = 'A';
    kod_modulu[1] = bufor_eeprom[1] = 'B';
    kod_modulu[2] = bufor_eeprom[2] = 'C';
    kod_modulu[3] = bufor_eeprom[3] = 'D';
    zapisz_znaki_w_eeprom_bez_kopiowania(ADRES_EEPROM_KOD_DOSTEPU,
                                         LICZBA_BAJTOW_KODU_DOSTEPU);
    zapal_diode_led(50);
    break;
  }
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_0:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_1:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_2:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_3:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_4:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_5:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_6:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_7:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_8:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_9:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_10:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_11:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_12:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_13:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_14:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_15:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_16:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_17:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_18:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_19:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_20:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_21:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_22:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_23:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_24:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_25:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_26:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_27:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_28:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_29:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_30:
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_31: {
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
    const uchar nr_bloku =
        aktualnie_wykonywana_komenda - KOMENDA_KOLEJKI_RESET_USTAWIEN_0;
#define LICZBA_BAJTOW_ZAPISYWANA_W_JEDNEJ_KOMENDZIE 32
    memset(bufor_eeprom, 0xff, LICZBA_BAJTOW_ZAPISYWANA_W_JEDNEJ_KOMENDZIE);
    if (nr_bloku == 0) {
      kod_modulu[0] = bufor_eeprom[1] = 'A';
      kod_modulu[1] = bufor_eeprom[2] = 'B';
      kod_modulu[2] = bufor_eeprom[3] = 'C';
      kod_modulu[3] = bufor_eeprom[4] = 'D';
      bufor_eeprom[5] = 0;
      stan_wyjscie[0] = 0;
      licznik_przelacznik_wyjscia[0] = 0;
    }
    zapisz_znaki_w_eeprom_bez_kopiowania(
        nr_bloku * LICZBA_BAJTOW_ZAPISYWANA_W_JEDNEJ_KOMENDZIE,
        LICZBA_BAJTOW_ZAPISYWANA_W_JEDNEJ_KOMENDZIE);
    dodaj_komende(KOMENDA_KOLEJKI_RESET_USTAWIEN_0 + nr_bloku + 1);
    zapal_diode_led(50);
    break;
  }
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_INICJALIZACJA: {
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
    uchar bledny_zapis = FALSE;
    static const uchar tab_eeprom_fabryczny[6] PROGMEM = {
        0xff, 'A', 'B', 'C', 'D', 0x00,
    };
    for (uint i = 0; i < 6; ++i) {
      if (eeprom_read_byte((void *)i) !=
          pgm_read_byte(tab_eeprom_fabryczny + i))
        bledny_zapis = TRUE;
    }
    for (uint i = EEPROM_NUMER_TELEFONU_BRAMA_0;
         i < EEPROM_NUMER_TELEFONU_BRAMA_0 +
                 MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA *
                     LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM;
         ++i) {
      if (eeprom_read_byte((void *)i) != 0xff) {
        bledny_zapis = TRUE;
        break;
      }
    }
    if (bledny_zapis) {
      dodaj_komende(KOMENDA_KOLEJKI_RESET_USTAWIEN_0);
      break;
    }
    inicjalizuj_parametry_modulu();
    // zapal_diode_led(5);
    break;
  }
  case KOMENDA_KOLEJKI_SPRAWDZ_UZYTKOWNIKOW_BRAMA: {
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();

    if (blokada_sterowania_czasowa) {
      // Poza dozwolonym czasem - ignoruj
      break;
    }

    if (eeprom_read_byte((const uint8_t *)ADRES_EEPROM_TRYB_PRACY) == 1) {
      // Tryb publiczny - wysteruj wyjscie dla kazdego
      ustaw_wyjscie_clip();
      break;
    }

    konwertuj_telefon_na_blok_eeprom(
        &numer_telefonu_ktory_dzwoni[0],
        &numer_telefonu_ktory_dzwoni[strlen(numer_telefonu_ktory_dzwoni)],
        &bufor_eeprom[0]);
    // for (uchar nr_uzyt_clip = 0; nr_uzyt_clip <
    // MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip)
    for (uint nr_uzyt_clip = 0;
         nr_uzyt_clip < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip) {
      if (porownaj_numer_telefonu_blok(
              &bufor_eeprom[0],
              (void *)EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt_clip))) {
        ustaw_wyjscie_clip();
        break;
      }
    }
    break;
  }
  case KOMENDA_KOLEJKI_DODAJ_UZYTKOWNIKA_BRAMA: {
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
    // for (uchar nr_uzyt_clip = 0; nr_uzyt_clip <
    // MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip)
    for (uint nr_uzyt_clip = 0;
         nr_uzyt_clip < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip) {
      if (not czy_aktywny_numer_telefonu_brama(nr_uzyt_clip)) {
        konwertuj_telefon_na_blok_eeprom(
            &numer_telefonu_do_ktorego_dzwonic[0],
            &numer_telefonu_do_ktorego_dzwonic[strlen(
                numer_telefonu_do_ktorego_dzwonic)],
            &bufor_eeprom[0]);
        zapisz_znaki_w_eeprom_bez_kopiowania(
            EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt_clip),
            LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM);
        break;
      }
    }
    break;
  }
  case KOMENDA_KOLEJKI_USUN_UZYTKOWNIKA_BRAMA: {
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
    konwertuj_telefon_na_blok_eeprom(&numer_telefonu_do_ktorego_dzwonic[0],
                                     &numer_telefonu_do_ktorego_dzwonic[strlen(
                                         numer_telefonu_do_ktorego_dzwonic)],
                                     &bufor_eeprom[0]);
    // for (uchar nr_uzyt_clip = 0; nr_uzyt_clip <
    // MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip)
    for (uint nr_uzyt_clip = 0;
         nr_uzyt_clip < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip) {
      if (porownaj_numer_telefonu_blok(
              &bufor_eeprom[0], // 1 porwnanie zajmuje ~10us, czyli 170 numerw ~
                                // 2ms
              (void *)EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt_clip))) {
        numer_telefonu_do_ktorego_dzwonic[0] = '\0';
        konwertuj_telefon_na_blok_eeprom(&numer_telefonu_do_ktorego_dzwonic[0],
                                         &numer_telefonu_do_ktorego_dzwonic[1],
                                         &bufor_eeprom[0]);
        zapisz_znaki_w_eeprom_bez_kopiowania(
            EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt_clip),
            LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM);
        break;
      }
    }
    break;
  }
  default:
    return FALSE;
  }
  return TRUE;
}

uchar wykonanie_komend_powiadomien(void) {
  const komenda_typ aktualnie_wykonywana_komenda = komendy_kolejka[0];

  const uchar czy_gsm_zajety =
      flaga_odczytywanie_smsa || flaga_wysylanie_smsa ||
      wykonywanie_rozmowy_telefonicznej ||
      trwa_rozmowa_przychodzaca_od_uzytkownika ||
      aktualnie_wysylane_polecenie_SIM900 != KOMENDA_KOLEJKI_BRAK_KOMENDY ||
      opoznienie_SIM900_100MS || czekanie_na_odebranie_zachety;

  const uchar czy_mozna_wysylac_dane_do_SIM900 =
      !czy_gsm_zajety && CZY_MOZNA_WYSYLAC_DANE_SIM900() &&
      czy_jest_bezczynny_SIM900();

  switch (aktualnie_wykonywana_komenda) {
  case KOMENDA_KOLEJKI_WYSLIJ_RAPORT_UZYTKOWNIKOW: {
    if (not czy_mozna_wysylac_dane_do_SIM900 ||
        wysylanie_smsa_clipa != WYSYLANIE_SMSA_CLIPA_BRAK) {
      dodaj_komende(aktualnie_wykonywana_komenda);
      break;
    }
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
    uchar *sms = &tekst_wysylanego_smsa[0];
    *sms++ = '*';
    for (;;) {
      if (czy_aktywny_numer_telefonu_brama(
              numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama)) {
        sms += kopiuj_blok_eeprom_na_telefon(
            (void *)EEPROM_NUMER_TELEFONU_BRAMA(
                numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama),
            sms, MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA_USER);
        *sms++ = '#';
        *sms++ = '\n';
        ++numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama;
        if ((sms + MAX_LICZBA_ZNAKOW_TELEFON + 1 >=
             &tekst_wysylanego_smsa[MAX_LICZBA_ZNAKOW_SMS]) ||
            (numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama >=
             MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA_USER)) {
          if ((numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama <
               MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA_USER))
            dodaj_komende(aktualnie_wykonywana_komenda);
          *sms = '\0';
          dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
          wysylanie_smsa_clipa = WYSYLANIE_SMSA_WYSYLANIE;
          break;
        }
      } else {
        if (++numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama >=
            MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA_USER) {
          if (sms != &tekst_wysylanego_smsa[0]) {
            *sms = '\0';
            dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
            wysylanie_smsa_clipa = WYSYLANIE_SMSA_WYSYLANIE;
          }
          break;
        }
      }
    }
    break;
  }
  default:
    return FALSE;
  }
  return TRUE;
}

void wykonanie_komend(void) {
  if (wykonanie_komend_SIM900())
    ;
  else if (wykonanie_komend_ukladow())
    ;
  else if (wykonanie_komend_powiadomien())
    ;
  else
    ;
  usun_komende();
}

void test_sms_clip_100ms(void) {
  // static ulong licznik_sms;
  // if ( ++licznik_sms > 60 * 10 )
  //{
  //	licznik_sms = 0;

  // tekst_wysylanego_smsa[0] = 'A';
  // tekst_wysylanego_smsa[1] = 'B';
  // tekst_wysylanego_smsa[2] = 'C';
  // tekst_wysylanego_smsa[3] = 'D';
  // tekst_wysylanego_smsa[4] = 0;
  // static const char tel[] PROGMEM = "731314727";
  ////static const char tel[] PROGMEM = "505691117";
  // memcpy_R(numer_telefonu_wysylanego_smsa, tel);
  // dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);

  // memcpy_R(numer_telefonu_do_ktorego_dzwonic, tel);
  // dodaj_komende(KOMENDA_KOLEJKI_WYKONAJ_ROZMOWE);
  //}
}

void steruj_wejscia_wyjscia_100ms(void) {
  aktualizuj_stan_wyzwolenia_wejsc_100ms();
  steruj_wejsciem_reset_100ms();
  steruj_wyjscia_100ms();
}

void zapis_w_eeprom_stanu_wyjsc(void) {
  if (not czy_wolny_eeprom())
    return;
  uchar par = 0;
  if ((stan_wyjscie[0] != 0) ^ (licznik_przelacznik_wyjscia[0] != 0))
    par |= BIT(0);
  if (par != eeprom_read_byte((void *)EEPROM_USTAWIENIE_STANOW_WYJSC))
    zapisz_znaki_w_eeprom(&par, (uint)EEPROM_USTAWIENIE_STANOW_WYJSC, 1);
}

// void test_clip_na_101(void)
//{
//	if ( not modul_zalogowany_w_sieci )
//		return;
//	static uint licznik_101;
//	if ( ++licznik_101 < 2 * 60 * 10 )
//		return;
//	licznik_101 = 0;
//	static const char doladowanie[] PROGMEM = "*101#";
//	strcpy_P(numer_telefonu_do_ktorego_dzwonic, doladowanie);
//	dodaj_komende(KOMENDA_KOLEJKI_WYKONAJ_ROZMOWE);
// }

void steruj_urzadzeniem_100MS(void) {
  steruj_wejscia_wyjscia_100ms();
  steruj_SIM900_100MS();
  if (licznik_100ms_dioda_led)
    --licznik_100ms_dioda_led;

  // Aktualizacja timera blokady komend REPORT/USER
  if (timer_report_user_100ms > 0) {
    --timer_report_user_100ms;
    if (timer_report_user_100ms == 0) {
      // Okno czasowe minelo - resetuj licznik
      licznik_report_user = 0;
    }
  } else {
    // Timer jest 0 - upewnij sie ze licznik tez jest 0 (ochrona przed bledami)
    licznik_report_user = 0;
  }

  // Mechanizm wykrywania zablokowanej kolejki i czyszczenia starych komend SMS
  const komenda_typ pierwsza_komenda = komendy_kolejka[0];
  if (pierwsza_komenda != KOMENDA_KOLEJKI_BRAK_KOMENDY) {
    // Sprawdz czy pierwsza komenda to SMS - jesli tak i stoi zbyt dlugo,
    // wyczysc
    if (pierwsza_komenda >= KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT &&
        pierwsza_komenda <= KOMENDA_KOLEJKI_WYSLIJ_SMSA_PDU) {
      // To jest komenda SMS - sprawdz czy stoi zbyt dlugo
      if (pierwsza_komenda == ostatnia_pierwsza_komenda) {
        // Ta sama komenda SMS jest na poczatku kolejki - kolejka moze stac
        if (licznik_kolejka_stoi_100ms < MAX_CZAS_KOLEJKA_STOI_100MS)
          ++licznik_kolejka_stoi_100ms;
        else {
          // Komenda SMS stoi juz 30 sekund - wyczysc wszystkie komendy SMS i
          // wznow dzialanie
          filtruj_komendy_z_przedzialu(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT,
                                       KOMENDA_KOLEJKI_WYSLIJ_SMSA_PDU);
          if (licznik_awaryjnych_resetow_kolejki < 0xFFFF)
            ++licznik_awaryjnych_resetow_kolejki;

          filtruj_i_dodaj_komende(KOMENDA_KOLEJKI_USUN_WSZYSTKIE_SMSY);

          // Resetuj blokade komend REPORT/USER aby wznowic dzialanie
          licznik_report_user = 0;
          timer_report_user_100ms = 0;

          // Resetuj liczniki kolejki
          licznik_kolejka_stoi_100ms = 0;
          ostatnia_pierwsza_komenda = KOMENDA_KOLEJKI_BRAK_KOMENDY;

          // Resetuj flage wysylania SMS (na wypadek zablokowania)
          flaga_wysylanie_smsa = 0;
        }
      } else {
        // Pierwsza komenda SMS sie zmienila - kolejka dziala
        ostatnia_pierwsza_komenda = pierwsza_komenda;
        licznik_kolejka_stoi_100ms = 0;
      }
    } else {
      // To nie jest komenda SMS - resetuj licznik (nie interesuje nas)
      ostatnia_pierwsza_komenda = pierwsza_komenda;
      licznik_kolejka_stoi_100ms = 0;
    }
  } else {
    // Kolejka jest pusta
    ostatnia_pierwsza_komenda = KOMENDA_KOLEJKI_BRAK_KOMENDY;
    licznik_kolejka_stoi_100ms = 0;
  }

  // Watchdog pilnujacy zakleszczonego wysylania SMS
  if (flaga_wysylanie_smsa) {
    if (licznik_watchdog_wysylanie_smsa_100ms < WATCHDOG_WYSYLANIA_SMS_100MS) {
      ++licznik_watchdog_wysylanie_smsa_100ms;
    } else {
      licznik_watchdog_wysylanie_smsa_100ms = 0;
      // Awaryjne odblokowanie
      watchdog_sms_disarm();
      flaga_wysylanie_smsa = 0;
      liczba_prob_wyslania_smsa = 0;
      czekanie_na_odebranie_zachety = FALSE;
      wysylanie_smsa_clipa = WYSYLANIE_SMSA_CLIPA_BRAK;

      if (liczba_kolejnych_watchdogow_wysylania < 255)
        ++liczba_kolejnych_watchdogow_wysylania;
      if (liczba_kolejnych_watchdogow_wysylania >= 3) {
        liczba_kolejnych_watchdogow_wysylania = 0;
        reset_modulu_SIM900();
      }
    }
  } else {
    licznik_watchdog_wysylanie_smsa_100ms = 0;
    liczba_kolejnych_watchdogow_wysylania = 0;
  }

  if (watchdog_sms_aktywny) {
    if (watchdog_sms_licznik_100ms < WATCHDOG_SMS_TIMEOUT_100MS)
      ++watchdog_sms_licznik_100ms;
    else {
      watchdog_sms_trwa_reset = TRUE;
      cli();
      wdt_enable(WDTO_15MS);
      while (1)
        ;
    }
  } else if (watchdog_sms_safe_mode_100ms) {
    --watchdog_sms_safe_mode_100ms;
  }

  // Obsluga szybkich blyskow LED
  if (liczba_blyskow_led > 0) {
    ++stan_cyklu_blysku;
    if (stan_cyklu_blysku >= 6) {
      stan_cyklu_blysku = 0;
      --liczba_blyskow_led;
    }
    ustaw_stan_led(stan_cyklu_blysku < 2);
    return;
  }

  static uchar licznik_dioda_led_poziom_sieci;
  if (poziom_sieci_gsm < 16 && licznik_100ms_dioda_led == 0) {
    if (++licznik_dioda_led_poziom_sieci >= 50)
      licznik_dioda_led_poziom_sieci = 1;
  } else
    licznik_dioda_led_poziom_sieci = 0;
  if (poziom_sieci_gsm == POZIOM_SIECI_BLAD || poziom_sieci_gsm == 0) {
    ustaw_stan_led(TRUE);
  } else {
    if (licznik_100ms_dioda_led)
      ustaw_stan_led(TRUE);
    else
      ustaw_stan_led(licznik_dioda_led_poziom_sieci == 40 ||
                     licznik_dioda_led_poziom_sieci == 43 ||
                     (licznik_dioda_led_poziom_sieci >= 46 &&
                      licznik_dioda_led_poziom_sieci <= 49));
  }
  zapis_w_eeprom_stanu_wyjsc();
}

void steruj_urzadzeniem_10MS(void) {
  steruj_wejscia_10ms();
  steruj_SIM900_10MS();
  zapisz_bajt_w_EEPROM();
  // test_dioda_wyjscia();
  // test_wejscie();
}

void wolne_zdarzenie_timer(void) {
  WYKONAJ_CLI_SEI(WYLACZ_PRZERWANIE_TIMER());
  if (!CZY_WYKONAC_ZDARZENIE_TIMER()) {
    WYKONAJ_CLI_SEI(WLACZ_PRZERWANIE_TIMER());
    return;
  }
  wykonaj_zdarzenie_timer = FALSE;
  if (wykonac_watki_10MS) {
    wykonac_watki = WYKONAJ_WATKI_10MS;
    wykonac_watki_10MS = FALSE;
  }
  WYKONAJ_CLI_SEI(WLACZ_PRZERWANIE_TIMER());
  if (CZY_WYKONAC_WATKI_10MS()) {
    static uchar licznik_wybuc_watki_100MS = 0;
#define OPOZNIENIE_100_MS 10
    if (++licznik_wybuc_watki_100MS == OPOZNIENIE_100_MS) {
      licznik_wybuc_watki_100MS = 0;
      wykonac_watki = WYKONAJ_WATKI_100MS | WYKONAJ_WATKI_10MS;
    }
    NOP();
    cli();
    if (CZY_ODBIERANIE_DANYCH_SIM900()) {
      WYLACZ_PRZERWANIE_ODBIORU_DANYCH_SIM900();
      sei();
      if (liczba_odebranych_znakow_SIM900 != 0 &&
          ++licznik_opoznienie_oczekiwania_na_bajt_SIM900 >
              MAX_OPOZNIENIE_OCZEKIWANIA_NA_BAJT_SIM900_500_MS) {
        resetuj_odbior_SIM900_po_bledzie();
      } else {
        WYKONAJ_CLI_SEI(WLACZ_PRZERWANIE_ODBIORU_DANYCH_SIM900());
      }
    }
    sei();
  }
}

void ustaw_parametry_dla_bezpieczenstwa(void) {
  wdt_reset();
  ustaw_parametry_dla_bezpieczenstwa_rejestry();
  set_sleep_mode(SLEEP_MODE_IDLE);
}

void inicjalizuj_parametry_modulu(void) {
  memcpy_E(kod_modulu, ADRES_EEPROM_KOD_DOSTEPU, LICZBA_BAJTOW_KODU_DOSTEPU);

  // Auto-naprawa: Jeśli kod jest pusty (FF) lub zerowy, ustaw ABCD
  if (kod_modulu[0] == 0xFF || kod_modulu[0] == 0x00) {
    kod_modulu[0] = 'A';
    kod_modulu[1] = 'B';
    kod_modulu[2] = 'C';
    kod_modulu[3] = 'D';
    // Zapisz do EEPROM natychmiast
    zapisz_znaki_w_eeprom_bez_kopiowania(ADRES_EEPROM_KOD_DOSTEPU,
                                         LICZBA_BAJTOW_KODU_DOSTEPU);
  }

  // Auto-naprawa: Jeśli tryb pracy jest nieokreślony (0xFF), ustaw Publiczny
  // (1)
  if (eeprom_read_byte((const uint8_t *)ADRES_EEPROM_TRYB_PRACY) == 0xFF) {
    zapisz_znak_w_eeprom(1, ADRES_EEPROM_TRYB_PRACY);
  }

  // Inicjalizacja parametrow czasowych
  czas_start_h = eeprom_read_byte((const uint8_t *)ADRES_EEPROM_CZAS_START_H);
  czas_start_m = eeprom_read_byte((const uint8_t *)ADRES_EEPROM_CZAS_START_M);
  czas_stop_h = eeprom_read_byte((const uint8_t *)ADRES_EEPROM_CZAS_STOP_H);
  czas_stop_m = eeprom_read_byte((const uint8_t *)ADRES_EEPROM_CZAS_STOP_M);

  // Auto-naprawa Harmonogramu: Jeśli wartości są nieprawidłowe (ale nie 0xFF),
  // ustaw na OFF
  if ((czas_start_h > 23 && czas_start_h != 0xFF) ||
      (czas_start_m > 59 && czas_start_m != 0xFF) ||
      (czas_stop_h > 23 && czas_stop_h != 0xFF) ||
      (czas_stop_m > 59 && czas_stop_m != 0xFF)) {
    // Zapisz wartosci wylaczajace (0xFF)
    zapisz_znak_w_eeprom(0xFF, ADRES_EEPROM_CZAS_START_H);
    zapisz_znak_w_eeprom(0xFF, ADRES_EEPROM_CZAS_START_M);
    zapisz_znak_w_eeprom(0xFF, ADRES_EEPROM_CZAS_STOP_H);
    zapisz_znak_w_eeprom(0xFF, ADRES_EEPROM_CZAS_STOP_M);

    czas_start_h = 0xFF;
    czas_start_m = 0xFF;
    czas_stop_h = 0xFF;
    czas_stop_m = 0xFF;
  }

  blokada_sterowania_czasowa =
      FALSE; // Domyślnie odblokowane, zaktualizuje sie przy odczycie czasu

  // --- NOWE Z V7 (Ported) ---
  // Reset blokady komend REPORT/USER po inicjalizacji/resecie
  licznik_report_user = 0;
  timer_report_user_100ms = 0;

  // Reset flagi wysylania SMS po resecie
  flaga_wysylanie_smsa = 0;

  // Zwieksz licznik resetow i zapisz debug
  static uchar licznik_resetow = 0;
  if (licznik_resetow < 255)
    ++licznik_resetow;
  while (!eeprom_is_ready())
    ;
  eeprom_update_byte((void *)EEPROM_DEBUG_LICZNIK_RESETOW, licznik_resetow);
  while (!eeprom_is_ready())
    ;
  zapisz_debug_do_eeprom(0, 2); // Reset (komenda=0 oznacza reset)
}

static void opoznienie_startowe(void) {
  for (uchar i = 0; i < 5; ++i)
    _delay_ms(20);
  wdt_reset();
}

#include "test_debug.h"

int main(void) {
  // POMOC_DODAJ2('*', 'u');
  { // inicjalizacja
#ifndef DEBUG
    wdt_enable(WDTO_120MS);
    opoznienie_startowe();
    opoznienie_startowe();
    opoznienie_startowe();
    opoznienie_startowe();
#endif
    inicjalizacja_portow();
    inicjalizuj_parametry_modulu();
    inicjalizacja_SIM900();
#ifdef DEBUG
    debug_main();
#endif
    ustaw_parametry_dla_bezpieczenstwa();
  }

  sei();

  for (;;) {
    // wykonanie watkw moe trwa maksymalnie 10 ms
    if (CZY_WYKONAC_WATKI_10MS())
      ustaw_parametry_dla_bezpieczenstwa();
    if (CZY_WYKONAC_WATKI_100MS()) {
      steruj_urzadzeniem_100MS();
#ifdef TEST_PCB
      testPCB();
#endif
    }
    if (CZY_WYKONAC_WATKI_10MS()) {
      steruj_urzadzeniem_10MS();
      wykonanie_komend();

      RESETUJ_WYKONANIE_WATKOW();
    }

    { // tryb SLEEP
      cli();
      if (!wykonaj_zdarzenie_timer) {
        sleep_enable();
        sei();
        sleep_cpu();
        cli();
        sleep_disable();
      }
      sei();
      wolne_zdarzenie_timer();
    }
  }

  return 0;
}
