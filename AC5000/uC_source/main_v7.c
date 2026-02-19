
#ifndef INCLUDE
#include "narzedzia.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <ctype.h>
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
#include "enumkomendy.h"
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

// static const char nazwa_urzadzenia[12+1] PROGMEM = "BRAMA";

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
      TRUE; // uruchamia zdarzenie_timer, kt�ra korzysta z pozosta�ych zmiennych
}

uchar modul_zalogowany_w_sieci = FALSE;

uchar licznik_reset_modulu_SIM900 = 0;
#define START_LICZNIK_RESET_MODULU_SIM900 30

uchar licznik_reset_urzadzenia = 0;
#define LICZNIK_RESET_URZADZENIA_RESET_SIM900 200
#define START_LICZNIK_RESET_URZADZENIA 220

uint opoznienie_wysylania_clipow_100MS = 0;

void generuj_raport_sieci(uchar **buf_sms) {
  static const char tekst_gsm[] PROGMEM =
      "AC200 DTM-F1\nS/N: 2.0-4725\nSygnal GSM ";
  uchar *ptr = *buf_sms;

  memcpy_R(ptr, tekst_gsm);
  ptr += sizeof tekst_gsm - 1;
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

void generuj_raport_uzytkownikow(uchar **buf_sms) {
  static const char tekst_gsm[] PROGMEM = "Uzytkownicy ";
  uchar *ptr = *buf_sms;

  memcpy_R(ptr, tekst_gsm);
  ptr += sizeof tekst_gsm - 1;
  uchar aktywne_numery = 0;
  for (uchar nr_uzyt_clip = 0;
       nr_uzyt_clip < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip) {
    if (czy_aktywny_numer_telefonu_brama(nr_uzyt_clip))
      ++aktywne_numery;
  }
  utoa(aktywne_numery, ptr, 10);
  ptr += strlen(ptr);
  *ptr++ = '/';
  utoa(MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA, ptr, 10);
  ptr += strlen(ptr);
  *buf_sms = ptr;
}

void generuj_raport_status(uchar **buf_sms) {
  uchar *ptr = *buf_sms;

  static const char tekst_status[] PROGMEM = "Status: ";
  memcpy_R(ptr, tekst_status);
  ptr += sizeof tekst_status - 1;

  if (blokada_sterowania) {
    static const char tekst_blokada[] PROGMEM = "Blokada";
    memcpy_R(ptr, tekst_blokada);
    ptr += sizeof tekst_blokada - 1;
  } else {
    static const char tekst_aktywny[] PROGMEM = "Aktywny";
    memcpy_R(ptr, tekst_aktywny);
    ptr += sizeof tekst_aktywny - 1;
  }
  *ptr = 0;
  *buf_sms = ptr;
}

void generuj_raport_tryb(uchar **buf_sms) {
  uchar *ptr = *buf_sms;

  static const char tekst_tryb[] PROGMEM = "Tryb: ";
  memcpy_R(ptr, tekst_tryb);
  ptr += sizeof tekst_tryb - 1;

  if (tryb_publiczny) {
    static const char tekst_publiczny[] PROGMEM = "Publiczny";
    memcpy_R(ptr, tekst_publiczny);
    ptr += sizeof tekst_publiczny - 1;
  } else {
    static const char tekst_prywatny[] PROGMEM = "Prywatny";
    memcpy_R(ptr, tekst_prywatny);
    ptr += sizeof tekst_prywatny - 1;
  }
  *ptr = 0;
  *buf_sms = ptr;
}

void generuj_raport_sterowanie(uchar **buf_sms) {
  uchar *ptr = *buf_sms;

  static const char tekst_ster[] PROGMEM = "Sterowanie: ";
  memcpy_R(ptr, tekst_ster);
  ptr += sizeof tekst_ster - 1;

  if (tryb_clip) {
    static const char tekst_clip[] PROGMEM = "CLIP";
    memcpy_R(ptr, tekst_clip);
    ptr += sizeof tekst_clip - 1;
  } else {
    static const char tekst_dtmf[] PROGMEM = "DTMF";
    memcpy_R(ptr, tekst_dtmf);
    ptr += sizeof tekst_dtmf - 1;
  }
  *ptr = 0;
  *buf_sms = ptr;
}

void generuj_raport_stanu_urzadzenia(void) {
  uchar *sms = (char *)tekst_wysylanego_smsa;
  *sms++ = '*';
  *sms++ = '\n';
  generuj_raport_sieci(&sms);
  *sms++ = '\n';
  generuj_raport_uzytkownikow(&sms);
  *sms++ = '\n';
  generuj_raport_status(&sms);
  *sms++ = '\n';
  generuj_raport_tryb(&sms);
  *sms++ = '\n';
  generuj_raport_sterowanie(&sms);
  *sms++ = '\n';
  static const char tekst_demo[] PROGMEM = INFORMACJA_W_RAPORCIE;
  strcpy_P(sms, tekst_demo);
}

static uchar blokada_po_odebraniu_polaczenia = FALSE;
static uchar oczekiwanie_na_sms_info_con = FALSE;

static inline void ustaw_blokade_po_odebraniu_polaczenia(void) {
  blokuj_odbior_do_bufora_SIM900();
  blokada_po_odebraniu_polaczenia = TRUE;
  oczekiwanie_na_sms_info_con = FALSE;
}

static inline void zwolnij_blokade_po_odebraniu_polaczenia(void) {
  if (blokada_po_odebraniu_polaczenia)
    odblokuj_odbior_do_bufora_SIM900();
  blokada_po_odebraniu_polaczenia = FALSE;
  oczekiwanie_na_sms_info_con = FALSE;
}

void wyslij_powiadomienie_con(const uchar *numer_uzywajacy) {
  if (numer_con[0] == '\0') // brak aktywnego CON
  {
    zwolnij_blokade_po_odebraniu_polaczenia();
    return;
  }
  // KRYTYCZNE: Sprawdz czy SMS nie jest juz w trakcie wysylania LUB w kolejce
  if (flaga_wysylanie_smsa != 0 ||
      czy_sa_komendy_z_przedzialu(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT,
                                  KOMENDA_KOLEJKI_WYSLIJ_PDU)) {
    zwolnij_blokade_po_odebraniu_polaczenia();
    return; // Pomiń powiadomienie - inny SMS w trakcie/kolejce
  }

  strcpy((char *)numer_telefonu_wysylanego_smsa, (char *)numer_con);
  // Przygotuj tresc SMS: "Użyto: +numer"
  strcpy((char *)tekst_wysylanego_smsa, "Uzyto: ");
  strcat((char *)tekst_wysylanego_smsa, (char *)numer_uzywajacy);
  oczekiwanie_na_sms_info_con = TRUE;
  dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
}

void ustaw_wyjscie_clip(void) {
  stan_wyjscie[0] = TRUE;
  licznik_przelacznik_wyjscia[0] = 2 * 10ul; // 2 sekundy
  // Wyslij powiadomienie CON
  wyslij_powiadomienie_con(numer_telefonu_ktory_dzwoni);
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
    dodaj_komende(KOMENDA_KOLEJKI_USUN_SMSA_1 + nr_smsa);
  }
}

static void zareaguj_na_usuniety_sms_z_powodu_limitu(void) {
  if (licznik_usunietych_sms_przez_limit < 0xFFFF)
    ++licznik_usunietych_sms_przez_limit;
  sygnalizuj_pelny_system();
  watchdog_sms_disarm();
  if (!czy_sa_komendy_z_przedzialu(KOMENDA_KOLEJKI_USUN_WSZYSTKIE_SMSY,
                                   KOMENDA_KOLEJKI_USUN_WSZYSTKIE_SMSY))
    dodaj_komende(KOMENDA_KOLEJKI_USUN_WSZYSTKIE_SMSY);
}

// Funkcja zapisu znacznikow debugowania do EEPROM
static void zapisz_debug_do_eeprom(uchar komenda, uchar akcja) {
  // Uzywamy bezposredniego zapisu do EEPROM bez sprawdzania czy_wolny_eeprom()
  // bo to moze blokowac zapis (EEPROM moze byc zajety przez inne operacje)
  // eeprom_update_byte czeka az EEPROM bedzie gotowy automatycznie

  // ZMIANA: Wersja nieblokujaca. Jesli EEPROM zajety, pomijamy zapis debug.
  if (!czy_wolny_eeprom())
    return;

  uchar buf[8];
  buf[0] = licznik_report_user;
  buf[1] = (uchar)(timer_report_user_100ms & 0xFF);
  buf[2] = (uchar)((timer_report_user_100ms >> 8) & 0xFF);
  buf[3] = (uchar)flaga_wysylanie_smsa;

  // Policz SMS w kolejce
  uchar liczba_sms_w_kolejce = 0;
  for (uchar i = 0; i < LICZBA_KOMEND; ++i) {
    if (komendy_kolejka[i] >= KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT &&
        komendy_kolejka[i] <= KOMENDA_KOLEJKI_WYSLIJ_SMSA_PDU)
      ++liczba_sms_w_kolejce;
  }
  buf[4] = liczba_sms_w_kolejce;
  buf[5] = komenda;
  buf[6] = akcja;

  // Zapisz 7 bajtow od adresu EEPROM_DEBUG_START (1008)
  // 1008: LICZNIK_REPORT_USER
  // 1009: TIMER_L
  // 1010: TIMER_H
  // 1011: FLAGA_WYSYLANIE
  // 1012: LICZBA_SMS
  // 1013: OSTATNIA_KOMENDA
  // 1014: OSTATNIA_AKCJA
  zapisz_znaki_w_eeprom(buf, EEPROM_DEBUG_START, 7);
}

void wykonanie_polecenia_sms(void) {
  tekst_odebranego_smsa[MAX_LICZBA_ZNAKOW_SMS] = 0; // (1) dla pewno�ci
  uchar komenda = interpretuj_wiadomosc_sms(tekst_odebranego_smsa);
  switch (komenda) {
  case INTERPRETACJA_SMS_POPRAWNY:
    zapal_diode_led_blyski(2);
    break; // 2 szybkie blyski
  case INTERPRETACJA_SMS_BRAK_KODU: {
    // SMS bez kodu - zapisz debug (komenda=0 oznacza brak kodu)
    zapisz_debug_do_eeprom(0, 3); // Brak kodu (akcja=3 oznacza brak kodu)
    break;
  }
  case INTERPRETACJA_SMS_RAPORT: {
    // Sprawdz blokade - maksymalnie 8 komend REPORT/USER w ciągu 30 sekund
    if (licznik_report_user >= MAX_LICZBA_KOMEND_REPORT_USER_W_OKNIE) {
      // Ignoruj nadmiarowe komendy - usun SMS z modulu SIM900
      zapisz_debug_do_eeprom(1, 1); // REPORT, zablokowane przez limit czasowy
      usun_zablokowany_sms();
      zareaguj_na_usuniety_sms_z_powodu_limitu();
      break;
    }

    // Sprawdz czy SMS nie jest juz w trakcie wysylania
    // (flaga_wysylanie_smsa jest zadeklarowana w data_sim900.h)
    if (flaga_wysylanie_smsa != 0) {
      // Sprawdz czy to ten sam numer - jesli tak, ignoruj aby nie duplikowac
      if (strcmp((char *)numer_telefonu_odebranego_smsa,
                 (char *)numer_telefonu_wysylanego_smsa) == 0) {
        // Ta sama komenda z tego samego numeru juz jest w trakcie wysylania -
        // ignoruj
        zapisz_debug_do_eeprom(1, 1); // REPORT, zablokowane - ten sam numer
        usun_zablokowany_sms();
        zareaguj_na_usuniety_sms_z_powodu_limitu();
        break;
      }
      // SMS jest juz w trakcie wysylania (ale inny numer) - ignoruj komende aby
      // nie blokowac systemu
      zapisz_debug_do_eeprom(1, 1); // REPORT, zablokowane - SMS w trakcie
      usun_zablokowany_sms();
      zareaguj_na_usuniety_sms_z_powodu_limitu();
      break;
    }

    // Sprawdz czy w kolejce nie ma juz zbyt wielu komend wysylania SMS
    // Licz ile jest komend SMS w kolejce
    uchar liczba_sms_w_kolejce = 0;
    for (uchar i = 0; i < LICZBA_KOMEND; ++i) {
      if (komendy_kolejka[i] >= KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT &&
          komendy_kolejka[i] <= KOMENDA_KOLEJKI_WYSLIJ_SMSA_PDU)
        ++liczba_sms_w_kolejce;
    }

    // Sprawdz czy kolejka ogolnie nie jest pelna (zabezpieczenie przed
    // zawieszeniem)
    uchar liczba_wszystkich_komend = 0;
    for (uchar i = 0; i < LICZBA_KOMEND; ++i) {
      if (komendy_kolejka[i] != KOMENDA_KOLEJKI_BRAK_KOMENDY)
        ++liczba_wszystkich_komend;
    }

    // Jesli kolejka ogolnie jest pelna (powyzej 35 z 40), blokuj aby nie
    // zawiesic systemu
    if (liczba_wszystkich_komend >= 35) {
      zapisz_debug_do_eeprom(1,
                             1); // REPORT, zablokowane - kolejka ogolnie pelna
      usun_zablokowany_sms();
      zareaguj_na_usuniety_sms_z_powodu_limitu();
      break;
    }

    // Maksymalnie 6 komend SMS w kolejce (zostawiamy miejsce na 8 z limitu
    // czasowego) Uwaga: jesli jest w trakcie wysylania to juz blokuje, wiec max
    // 6 w kolejce + 1 w trakcie = 7
    if (liczba_sms_w_kolejce >= 6) {
      // Kolejka SMS jest pelna - ignoruj komende
      zapisz_debug_do_eeprom(1, 1); // REPORT, zablokowane - kolejka pelna
      usun_zablokowany_sms();
      zareaguj_na_usuniety_sms_z_powodu_limitu();
      break;
    }

    ++licznik_report_user;
    timer_report_user_100ms = OKNO_CZASOWE_REPORT_USER_100MS;

    strcpy((char *)numer_telefonu_wysylanego_smsa,
           (char *)numer_telefonu_odebranego_smsa);
    generuj_raport_stanu_urzadzenia();
    dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
    zapal_diode_led_blyski(2);    // 2 blyski - odbior komendy REPORT
    zapisz_debug_do_eeprom(1, 0); // REPORT, OK
    break;
  }
  case INTERPRETACJA_SMS_USER: {
    // Sprawdz blokade - maksymalnie 8 komend REPORT/USER w ciągu 30 sekund
    if (licznik_report_user >= MAX_LICZBA_KOMEND_REPORT_USER_W_OKNIE) {
      // Ignoruj nadmiarowe komendy - usun SMS z modulu SIM900
      zapisz_debug_do_eeprom(2, 1); // USER, zablokowane przez limit czasowy
      usun_zablokowany_sms();
      zareaguj_na_usuniety_sms_z_powodu_limitu();
      break;
    }

    // Sprawdz czy SMS nie jest juz w trakcie wysylania
    // (flaga_wysylanie_smsa jest zadeklarowana w data_sim900.h)
    if (flaga_wysylanie_smsa != 0) {
      // Sprawdz czy to ten sam numer - jesli tak, ignoruj aby nie duplikowac
      if (strcmp((char *)numer_telefonu_odebranego_smsa,
                 (char *)numer_telefonu_wysylanego_smsa) == 0) {
        // Ta sama komenda z tego samego numeru juz jest w trakcie wysylania -
        // ignoruj
        zapisz_debug_do_eeprom(2, 1); // USER, zablokowane - ten sam numer
        usun_zablokowany_sms();
        zareaguj_na_usuniety_sms_z_powodu_limitu();
        break;
      }
      // SMS jest juz w trakcie wysylania (ale inny numer) - ignoruj komende aby
      // nie blokowac systemu
      zapisz_debug_do_eeprom(2, 1); // USER, zablokowane - SMS w trakcie
      usun_zablokowany_sms();
      zareaguj_na_usuniety_sms_z_powodu_limitu();
      break;
    }

    // Sprawdz czy w kolejce nie ma juz zbyt wielu komend wysylania SMS
    // Licz ile jest komend SMS w kolejce
    uchar liczba_sms_w_kolejce = 0;
    for (uchar i = 0; i < LICZBA_KOMEND; ++i) {
      if (komendy_kolejka[i] >= KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT &&
          komendy_kolejka[i] <= KOMENDA_KOLEJKI_WYSLIJ_SMSA_PDU)
        ++liczba_sms_w_kolejce;
    }

    // Sprawdz czy kolejka ogolnie nie jest pelna (zabezpieczenie przed
    // zawieszeniem)
    uchar liczba_wszystkich_komend = 0;
    for (uchar i = 0; i < LICZBA_KOMEND; ++i) {
      if (komendy_kolejka[i] != KOMENDA_KOLEJKI_BRAK_KOMENDY)
        ++liczba_wszystkich_komend;
    }

    // Jesli kolejka ogolnie jest pelna (powyzej 35 z 40), blokuj aby nie
    // zawiesic systemu
    if (liczba_wszystkich_komend >= 35) {
      zapisz_debug_do_eeprom(2, 1); // USER, zablokowane - kolejka ogolnie pelna
      usun_zablokowany_sms();
      zareaguj_na_usuniety_sms_z_powodu_limitu();
      break;
    }

    // Maksymalnie 6 komend SMS w kolejce (zostawiamy miejsce na 8 z limitu
    // czasowego) Uwaga: jesli jest w trakcie wysylania to juz blokuje, wiec max
    // 6 w kolejce + 1 w trakcie = 7
    if (liczba_sms_w_kolejce >= 6) {
      // Kolejka SMS jest pelna - ignoruj komende
      zapisz_debug_do_eeprom(2, 1); // USER, zablokowane - kolejka pelna
      usun_zablokowany_sms();
      zareaguj_na_usuniety_sms_z_powodu_limitu();
      break;
    }

    ++licznik_report_user;
    timer_report_user_100ms = OKNO_CZASOWE_REPORT_USER_100MS;

    strcpy((char *)numer_telefonu_wysylanego_smsa,
           (char *)numer_telefonu_odebranego_smsa);
    // Konwertuj numer na format EEPROM
    konwertuj_telefon_na_blok_eeprom(&numer_telefonu_do_ktorego_dzwonic[0],
                                     &numer_telefonu_do_ktorego_dzwonic[strlen(
                                         numer_telefonu_do_ktorego_dzwonic)],
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
    zapal_diode_led_blyski(2);    // 2 blyski - odbior komendy USER
    zapisz_debug_do_eeprom(2, 0); // USER, OK
    break;
  }
  case INTERPRETACJA_SMS_USER_BEZ_NUMERU: {
    // USER bez numeru – wyślij informację jak poprawnie użyć komendy
    strcpy((char *)numer_telefonu_wysylanego_smsa,
           (char *)numer_telefonu_odebranego_smsa);
    strcpy((char *)tekst_wysylanego_smsa,
           "Wpisz numer jaki sprawdzasz, np: USER 793557357");
    dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
    zapal_diode_led_blyski(2); // 2 blyski - otrzymano błędną komendę USER
    break;
  }
  case INTERPRETACJA_SMS_USER_LIST: {
    // USER bez numeru – wyślij pełną listę użytkowników na numer nadawcy
    strcpy((char *)numer_telefonu_wysylanego_smsa,
           (char *)numer_telefonu_odebranego_smsa);
    numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama = 0;
    dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_RAPORT_UZYTKOWNIKOW);
    zapal_diode_led_blyski(2); // 2 blyski - odbior komendy USER (lista)
    break;
  }
  case INTERPRETACJA_SMS_RESET_WSZYSTKICH_USTAWIEN: {
    if (not czy_sa_komendy_z_przedzialu(
            KOMENDA_KOLEJKI_RESET_KOD_DOSTEPU,
            KOMENDA_KOLEJKI_RESET_USTAWIEN_INICJALIZACJA)) {
      zapal_diode_led_blyski(25); // 25 szybkich blyskow dla resetu calkowitego
      dodaj_komende(KOMENDA_KOLEJKI_RESET_USTAWIEN_0);
    }
    break;
  }
  default: // bez potwierdzenia
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

uchar odbierz_przychodzace_polaczenie = FALSE;

uchar sprawdz_przychodzaca_rozmowe(void) // wysy�a TRUE, gdy nale�y odebra�
{
  odbierz_przychodzace_polaczenie = FALSE;

  if (numer_telefonu_ktory_dzwoni[0] == 0)
    return FALSE;

  // Sprawdz czy EEPROM jest wolny
  if (!czy_wolny_eeprom()) {
    dodaj_komende(KOMENDA_KOLEJKI_SPRAWDZ_UZYTKOWNIKOW_BRAMA);
    return odbierz_przychodzace_polaczenie;
  }

  // Jesli Blokada - nie reaguj na rozmowy
  if (blokada_sterowania)
    return FALSE;

  // Jesli tryb Publiczny - odbierz dla kazdego
  if (tryb_publiczny) {
    odbierz_przychodzace_polaczenie = TRUE;
    return TRUE;
  }

  // Tryb Prywatny - sprawdz liste uzytkownikow
  konwertuj_telefon_na_blok_eeprom(
      &numer_telefonu_ktory_dzwoni[0],
      &numer_telefonu_ktory_dzwoni[strlen(numer_telefonu_ktory_dzwoni)],
      &bufor_eeprom[0]);

  for (uchar nr_uzyt_clip = 0;
       nr_uzyt_clip < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip) {
    if (porownaj_numer_telefonu_blok(
            &bufor_eeprom[0],
            (void *)EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt_clip))) {
      odbierz_przychodzace_polaczenie = TRUE;
      return TRUE;
    }
  }

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

static void awaryjne_odblokowanie_wysylania_smsa(void) {
  watchdog_sms_disarm();
  flaga_wysylanie_smsa = 0;
  liczba_prob_wyslania_smsa = 0;
  czekanie_na_odebranie_zachety = FALSE;
  wysylanie_smsa_clipa = WYSYLANIE_SMSA_CLIPA_BRAK;
}

enum PowodZakonczeniaRozmowyTelefonicznej {
  powod_zakonczenia_rozmowy_odrzucenie,
  powod_zakonczenia_rozmowy_zakonczenie,
  powod_zakonczenia_rozmowy_przekroczony_czas,
  powod_zakonczenia_rozmowy_otrzymana_wiadomosc
};

void zakonczono_rozmowe_telefoniczna(
    const enum PowodZakonczeniaRozmowyTelefonicznej powod) {
  POMOC_DODAJ2('#', 'a');
  opoznienie_SIM900_100MS = 60; // by�o 60
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
  if (oczekiwanie_na_sms_info_con)
    zwolnij_blokade_po_odebraniu_polaczenia();
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

void wyslij_powiadomienie_con(const uchar *numer_uzywajacy);

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
    zapal_diode_led_blyski(4); // 4 blyski dla resetu kodu
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
      bufor_eeprom[0] = 0x00;                // adres 0 - wolny/suma kontrolna
      kod_modulu[0] = bufor_eeprom[1] = 'A'; // adres 1
      kod_modulu[1] = bufor_eeprom[2] = 'B'; // adres 2
      kod_modulu[2] = bufor_eeprom[3] = 'C'; // adres 3
      kod_modulu[3] = bufor_eeprom[4] = 'D'; // adres 4
      bufor_eeprom[5] = 0; // adres 5 - EEPROM_USTAWIENIE_STANOW_WYJSC
      bufor_eeprom[6] = 0; // adres 6 - EEPROM_USTAWIENIE_WYJSCIA (bajt L)
      bufor_eeprom[7] = 0; // adres 7 - EEPROM_USTAWIENIE_WYJSCIA (bajt H)
      stan_wyjscie[0] = 0;
      licznik_przelacznik_wyjscia[0] = 0;
    }
    if (nr_bloku == 31) {
      // Adresy 1016-1020, 1021, 1022 i 1023 znajdują się w bloku 31 (992-1023)
      // 1016 - 992 = 24 (EEPROM_NUMER_CON), 1021 - 992 = 29, 1022 - 992 = 30,
      // 1023 - 992 = 31 Reset CON na OFF
      numer_con[0] = '\0';
      bufor_eeprom[24] = 0xff; // numer_con pusty (CON OFF)
      tryb_clip = TRUE;
      bufor_eeprom[29] = TRUE; // tryb_clip (domyslnie CLIP - automatyczne)
      blokada_sterowania = FALSE;
      bufor_eeprom[30] = FALSE; // blokada_sterowania (domyslnie Aktywny)
      tryb_publiczny = TRUE;
      bufor_eeprom[31] = TRUE; // tryb_publiczny (domyslnie Publiczny)
    }
    zapisz_znaki_w_eeprom_bez_kopiowania(
        nr_bloku * LICZBA_BAJTOW_ZAPISYWANA_W_JEDNEJ_KOMENDZIE,
        LICZBA_BAJTOW_ZAPISYWANA_W_JEDNEJ_KOMENDZIE);
    dodaj_komende(KOMENDA_KOLEJKI_RESET_USTAWIEN_0 + nr_bloku + 1);
    // Blyski LED tylko na poczatku resetu (w
    // INTERPRETACJA_SMS_RESET_WSZYSTKICH_USTAWIEN)
    break;
  }
  case KOMENDA_KOLEJKI_RESET_USTAWIEN_INICJALIZACJA: {
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
    uchar bledny_zapis = FALSE;
    static const uchar tab_eeprom_fabryczny[7] PROGMEM = {
        0x00, 'A', 'B', 'C', 'D', 0x00, 0x00,
    };
    for (uint i = 0; i < 7; ++i) {
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

    // KRYTYCZNE: Reset flagi przed sprawdzeniem (Problem 1)
    odbierz_przychodzace_polaczenie = FALSE;

    // KRYTYCZNE: Odrzuc polaczenia z ukrytym numerem ZAWSZE (Problem 2)
    if (numer_telefonu_ktory_dzwoni[0] == 0)
      break;

    if (blokada_sterowania) // jesli Blokada - nie reaguj na rozmowy
      break;
    if (tryb_publiczny) // jesli tryb Publiczny - odbierz dla kazdego
    {
      odbierz_przychodzace_polaczenie = TRUE;
      break;
    }
    // tryb Prywatny - sprawdz liste uzytkownikow
    konwertuj_telefon_na_blok_eeprom(
        &numer_telefonu_ktory_dzwoni[0],
        &numer_telefonu_ktory_dzwoni[strlen(numer_telefonu_ktory_dzwoni)],
        &bufor_eeprom[0]);
    for (uchar nr_uzyt_clip = 0;
         nr_uzyt_clip < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip) {
      if (porownaj_numer_telefonu_blok(
              &bufor_eeprom[0],
              (void *)EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt_clip))) {
        odbierz_przychodzace_polaczenie = TRUE;
        break;
      }
    }
    break;
  }
  case KOMENDA_KOLEJKI_DODAJ_UZYTKOWNIKA_BRAMA: {
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
    for (uchar nr_uzyt_clip = 0;
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
    for (uchar nr_uzyt_clip = 0;
         nr_uzyt_clip < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip) {
      if (porownaj_numer_telefonu_blok(
              &bufor_eeprom[0], // 1 por�wnanie zajmuje ~10us, czyli 170 numer�w
                                // ~ 2ms
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
            sms, MAX_LICZBA_ZNAKOW_TELEFON);
        *sms++ = '#';
        *sms++ = '\n';
        ++numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama;
        if ((sms + MAX_LICZBA_ZNAKOW_TELEFON + 1 >=
             &tekst_wysylanego_smsa[MAX_LICZBA_ZNAKOW_SMS]) ||
            (numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama >=
             MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA)) {
          if ((numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama <
               MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA))
            dodaj_komende(aktualnie_wykonywana_komenda);
          *sms = '\0';
          dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
          wysylanie_smsa_clipa = WYSYLANIE_SMSA_WYSYLANIE;
          break;
        }
      } else {
        if (++numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama >=
            MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA) {
          if (sms != &tekst_wysylanego_smsa[0]) {
            // Jeśli w buforze jest tylko '*' (brak użytkowników) – podmień na
            // komunikat tekstowy
            if (sms == &tekst_wysylanego_smsa[1] &&
                tekst_wysylanego_smsa[0] == '*') {
              strcpy((char *)tekst_wysylanego_smsa, "Brak uzytkownikow");
              sms =
                  &tekst_wysylanego_smsa[strlen((char *)tekst_wysylanego_smsa)];
            }
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
  // Nie blokuj zapisu stanu wyjść jeśli trwa reset (reset ma priorytet)
  if (czy_sa_komendy_z_przedzialu(KOMENDA_KOLEJKI_RESET_USTAWIEN_0,
                                  KOMENDA_KOLEJKI_RESET_USTAWIEN_INICJALIZACJA))
    return;
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
          // wznow dzialanie To zapobiega zapychaniu kolejki przez zablokowane
          // komendy
          filtruj_komendy_z_przedzialu(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT,
                                       KOMENDA_KOLEJKI_WYSLIJ_SMSA_PDU);
          if (licznik_awaryjnych_resetow_kolejki < 0xFFFF)
            ++licznik_awaryjnych_resetow_kolejki;
          if (!czy_sa_komendy_z_przedzialu(KOMENDA_KOLEJKI_USUN_WSZYSTKIE_SMSY,
                                           KOMENDA_KOLEJKI_USUN_WSZYSTKIE_SMSY))
            dodaj_komende(KOMENDA_KOLEJKI_USUN_WSZYSTKIE_SMSY);

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
      awaryjne_odblokowanie_wysylania_smsa();
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

  // Obsluga szybkich blyskow LED - timing 200ms ON / 350ms OFF
  if (liczba_blyskow_led > 0) {
    ++stan_cyklu_blysku;
    // Cykl: 2x ON (200ms), 3.5x OFF (350ms) = 550ms na blysk
    if (stan_cyklu_blysku >=
        6) // 2 + 4 = 6 (zaokraglone 350ms do 400ms dla prostoty)
    {
      stan_cyklu_blysku = 0;
      --liczba_blyskow_led;
    }
    ustaw_stan_led(stan_cyklu_blysku < 2); // LED ON gdy stan 0-1 (200ms)
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
      ustaw_stan_led(TRUE); // LED ON caly czas gdy aktywny licznik (np. DTMF)
    else
      ustaw_stan_led(licznik_dioda_led_poziom_sieci == 40 ||
                     licznik_dioda_led_poziom_sieci == 43 ||
                     (licznik_dioda_led_poziom_sieci >= 46 &&
                      licznik_dioda_led_poziom_sieci <= 49));
  }
  zapis_w_eeprom_stanu_wyjsc();
  // test_sms_clip_100ms();
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
  // Weryfikacja czy kod został poprawnie odczytany - jeśli nie, ustaw domyślny
  // ABCD
  if (kod_modulu[0] == 0xFF || kod_modulu[0] == 0x00) {
    kod_modulu[0] = 'A';
    kod_modulu[1] = 'B';
    kod_modulu[2] = 'C';
    kod_modulu[3] = 'D';
  }
  kopiuj_parametry_we_wy_z_eeprom();
  inicjalizuj_parametry_we_wy();

  // Reset blokady komend REPORT/USER po inicjalizacji/resecie
  licznik_report_user = 0;
  timer_report_user_100ms = 0;

  // Reset flagi wysylania SMS po resecie (ochrona przed blokada)
  flaga_wysylanie_smsa = 0;

  // Zwieksz licznik resetow i zapisz debug
  static uchar licznik_resetow = 0;
  if (licznik_resetow < 255)
    ++licznik_resetow;
  while (!eeprom_is_ready())
    ;
  eeprom_update_byte((void *)EEPROM_DEBUG_LICZNIK_RESETOW, licznik_resetow);
  while (!eeprom_is_ready())
    ; // ZMIANA: Czekaj na zakonczenie zapisu licznika, aby
      // zapisz_debug_do_eeprom nie pominelo zapisu
  zapisz_debug_do_eeprom(0, 2); // Reset (komenda=0 oznacza reset)

  uchar par = eeprom_read_byte((void *)EEPROM_USTAWIENIE_STANOW_WYJSC);
  if (IS_HI(par, 0))
    stan_wyjscie[0] = TRUE;
  blokada_sterowania = eeprom_read_byte((void *)EEPROM_BLOKADA_STEROWANIA);
  if (blokada_sterowania != FALSE && blokada_sterowania != TRUE)
    blokada_sterowania = FALSE; // domyslnie Aktywny (odblokowane)
  tryb_publiczny = eeprom_read_byte((void *)EEPROM_TRYB_PUBLICZNY);
  if (tryb_publiczny != FALSE && tryb_publiczny != TRUE)
    tryb_publiczny = TRUE; // domyslnie Publiczny (kazdy moze otworzyc)
  tryb_clip = eeprom_read_byte((void *)EEPROM_TRYB_CLIP_DTMF);
  if (tryb_clip != FALSE && tryb_clip != TRUE)
    tryb_clip = TRUE; // domyslnie CLIP (automatyczne wlaczanie wyjscia)

  // Wczytaj numer CON z EEPROM
  uchar con_blok[LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM];
  memcpy_E(con_blok, EEPROM_NUMER_CON, LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM);
  if (con_blok[0] == 0xff)
    numer_con[0] = '\0'; // Brak numeru CON
  else
    konwertuj_blok_eeprom_na_telefon(con_blok, numer_con,
                                     MAX_LICZBA_ZNAKOW_TELEFON);
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
    // wykonanie watk�w mo�e trwa� maksymalnie 10 ms
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
      // Podczas resetu wykonuj komendy resetu, ale nie inne funkcje, które mogą
      // ustawiać LED Reset ma priorytet - błyski LED są obsługiwane w
      // steruj_urzadzeniem_100MS()
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
