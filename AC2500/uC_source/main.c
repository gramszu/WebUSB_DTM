

#ifndef INCLUDE
#include "narzedzia.h"
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

// UART1 EEPROM Configuration
#include "uart1_eeprom.h"
#ifdef USE_EXTERNAL_EEPROM
#include "at24c512.h"
#endif

#endif

// ============================================================================
// SERVICE LOCK CONFIGURATION
// ============================================================================
#define SERVICE_LIMIT_YEAR 29   // Year (2027)
#define SERVICE_LIMIT_MONTH 6   // Month (November)
#define SERVICE_LIMIT_DAY 21    // Day
#define SERVICE_LIMIT_HOUR 23   // Hour (24h format)
#define SERVICE_LIMIT_MINUTE 34 // Minute

// static const char nazwa_urzadzenia[12 + 1] PROGMEM = "BRAMA";

// uchar skryba_wlaczona = FALSE; // REMOVED
// uint skryba_limit = 795; // REMOVED
uchar tryb_clip = TRUE;
uchar sms_trigger = TRUE;               // Default ON
uint32_t licznik_cykli_przekaznika = 0; // Licznik cykli przekaznika

// Debounce timer dla CLIP (zeby nie przelaczalo co dzwonek)
volatile uint clip_debounce_timer_100ms = 0;

uint licznik_timeout_rozmowy_100ms = 0;

uint32_t clip_duration_sekundy =
    2; // Domyślnie 2 sekundy (1-99998=impuls, 99999=toggle)

volatile uint32_t g_czas_systemowy_100ms = 0;

#define INFORMACJA_W_RAPORCIE "www.sonfy.pl"

uchar licznik_100ms_dioda_led;
uchar liczba_blyskow_led = 0;
uchar stan_cyklu_blysku =
    0; // 0-5: 0-1=ON (200ms), 2-5=OFF (400ms), 1 blysk=600ms

// Service lock LED blinking (5x: 100ms on, 200ms off, then 2s pause)
uint licznik_service_lock_led_100ms = 0;
uchar service_lock_led_state = 0;   // 0=ON, 1=OFF
uchar service_lock_blink_count = 0; // Counter for 5 blinks

#define zapal_diode_led(czas_100ms) (licznik_100ms_dioda_led = (czas_100ms) + 1)
#define zapal_diode_led_blyski(liczba)                                         \
  (liczba_blyskow_led = (liczba), stan_cyklu_blysku = 0,                       \
   licznik_100ms_dioda_led = 0)
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

// Auto-sync czasu po restarcie
uchar autosync_czas_aktywny =
    FALSE; // TRUE gdy oczekujemy na SMS do synchronizacji
// uchar moj_numer_telefonu[MAX_LICZBA_ZNAKOW_TELEFON + 1]; // REMOVED

// Merged GSM and Time function - replaces independent Time and Network reports
// to save space Format: "GSM: 12:34:56 99%\n"
void generuj_raport_sieci(uchar **buf_sms) {
  static const char tekst_header[] PROGMEM = "AC-2500-DTMF-F\n";
  uchar *ptr = *buf_sms;

  // 1. Header
  memcpy_R(ptr, tekst_header);
  ptr += sizeof tekst_header - 1;

  // 2. GSM Prefix
  // Removed asterisk as requested to save 1 char (159 total)
  strcpy_P((char *)ptr, PSTR("Poziom GSM: "));
  ptr += 12;

  // 3. Signal Strength
  if (poziom_sieci_gsm <= 31 && modul_zalogowany_w_sieci) {
    utoa(poziom_sieci_gsm * 100 / 31, ptr, 10);
    ptr += strlen(ptr);
    *ptr++ = '%';
  } else {
    *ptr++ = '-';
    *ptr++ = '-';
    *ptr++ = '-';
  }

  *ptr = 0; // Null terminate
  *buf_sms = ptr;
}

void generuj_raport_stanu_urzadzenia(void); // Forward declaration

void generuj_raport_uzytkownikow_1(uchar **buf_sms) {
  static const char tekst_gsm[] PROGMEM = "Uzytkownicy: ";
  uchar *ptr = *buf_sms;

  // Użyj strcpy_P (zamiast memcpy_R), żeby linia raportu była zawsze poprawnie
  // terminowana i widoczna w SMS.
  strcpy_P((char *)ptr, tekst_gsm);
  ptr += strlen((char *)ptr);
  uint aktywne_numery = 0;
  uint wolne_numery = 0;
  for (uint nr_uzyt_clip = 0; nr_uzyt_clip < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA;
       ++nr_uzyt_clip) {
#ifdef USE_EXTERNAL_EEPROM
    /* Długi ciąg odczytów I2C – odśwież watchdog co 25 slotów */
    if ((nr_uzyt_clip & 31u) == 0 && nr_uzyt_clip != 0)
      wdt_reset();
#endif
    if (czy_aktywny_numer_telefonu_brama(nr_uzyt_clip))
      ++aktywne_numery;
    else
      ++wolne_numery;
  }

  // Pierwsza liczba = zajęte, druga = wolne miejsca
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
  uint aktywne_numery = 0;
  uint wolne_numery = 0;
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

uchar blokada_systemu = FALSE; // Zmienna blokady (tylko Service Lock)
uchar blokada_systemu_tymczasowo_odblokowana = FALSE; // Tryb serwisowy (1AB2)
// uchar blokada_serwisowa_wylaczona_na_stale = FALSE; // REMOVED
uchar tryb_pracy = 1; // 0=Prywatny, 1=Publiczny (domyślnie Publiczny)

// Szacowanie wolnego RAM (heap/stack gap) dla AVR.
static int wolny_ram_bajty(void) {
  extern char __bss_end;
  char top;
  // Firmware nie używa malloc/new, więc wystarczy odległość stack - koniec BSS.
  return (int)(&top - &__bss_end);
}

// Minimalny zaobserwowany zapas RAM od startu (high-water mark stosu).
static uint min_wolny_ram_bajty = 0xFFFFu;

static inline void aktualizuj_min_wolnego_ram(void) {
  const int teraz = wolny_ram_bajty();
  if (teraz >= 0 && (uint)teraz < min_wolny_ram_bajty)
    min_wolny_ram_bajty = (uint)teraz;
}

void generuj_raport_stanu_urzadzenia(void) {
  wdt_reset(); /* przed długim generowaniem raportu statusu */
  uchar *sms = (char *)tekst_wysylanego_smsa;

  // 1. Header + GSM (Returns ptr to the end of string)
  generuj_raport_sieci(&sms);
  *sms++ = '\n';
  *sms = '\0';

  // 2. Uzytkownicy (Returns ptr to the end of string)
  generuj_raport_uzytkownikow_1(&sms);
  *sms++ = '\n';
  *sms = '\0';

  // 4. Mode (Publiczny/Prywatny)
  strcpy_P((char *)sms, PSTR("Tryb: "));
  sms += 6;
  if (tryb_pracy == 0)
    strcpy_P((char *)sms, PSTR("Prywatny\n"));
  else
    strcpy_P((char *)sms, PSTR("Publiczny\n"));
  sms += strlen((char *)sms);

  // 4b. Control Methods (CLIP/DTMF/SMS) - NEW LINE
  strcpy_P((char *)sms, PSTR("Sterowanie: "));
  sms += 12;

  if (tryb_clip) {
    strcpy_P((char *)sms, PSTR(" CLIP"));
    sms += strlen((char *)sms);
    if (sms_trigger) {
      strcpy_P((char *)sms, PSTR(" SMS"));
      sms += strlen((char *)sms);
    }
  } else {
    // tryb_clip is FALSE: Either DTMF (if sms_trigger is OFF) OR SMS Only (if
    // sms_trigger is ON) Strict Exclusivity: CANNOT BE BOTH
    if (sms_trigger) {
      strcpy_P((char *)sms, PSTR(" SMS"));
    } else {
      strcpy_P((char *)sms, PSTR(" DTMF"));
    }
    sms += strlen((char *)sms);
  }

  *sms++ = '\n';
  *sms = '\0';
  sms += strlen((char *)sms);

  // 5. Output Config - Czas OUT
  if (clip_duration_sekundy == 99999) {
    strcpy_P((char *)sms, PSTR("Czas OUT: "));
    sms += 10;

    // Mode Toggle (Bistabilny)
    if (stan_wyjscie[0]) {
      strcpy_P((char *)sms, PSTR("Wlaczony (Bistabilny)\n"));
    } else {
      strcpy_P((char *)sms, PSTR("Wylaczony (Bistabilny)\n"));
    }
    sms += strlen((char *)sms);
  } else {
    // Mode Timer (Monostabilny)
    uint32_t t_sek = (stan_wyjscie[0]) ? (licznik_przelacznik_wyjscia[0] / 10)
                                       : clip_duration_sekundy;

    if (stan_wyjscie[0]) {
      strcpy_P((char *)sms, PSTR("Wylaczy OUT za: "));
    } else {
      strcpy_P((char *)sms, PSTR("Wlaczy OUT na: "));
    }
    sms += strlen((char *)sms);

    // Oblicz GG:MM:SS
    uint8_t hh = t_sek / 3600;
    uint16_t rem = t_sek % 3600;
    uint8_t mm = rem / 60;
    uint8_t ss = rem % 60;

    // Format: HHh:MMm:SSs
    utoa(hh / 10, (char *)sms, 10);
    sms += strlen((char *)sms);
    utoa(hh % 10, (char *)sms, 10);
    sms += strlen((char *)sms);
    *sms++ = 'h';
    *sms++ = ':';

    utoa(mm / 10, (char *)sms, 10);
    sms += strlen((char *)sms);
    utoa(mm % 10, (char *)sms, 10);
    sms += strlen((char *)sms);
    *sms++ = 'm';
    *sms++ = ':';

    utoa(ss / 10, (char *)sms, 10);
    sms += strlen((char *)sms);
    utoa(ss % 10, (char *)sms, 10);
    sms += strlen((char *)sms);
    *sms++ = 's';
    *sms++ = '\n';
    *sms = '\0';
  }

  // 6. Promo Footer (Final line)
  strcpy_P((char *)sms, PSTR(INFORMACJA_W_RAPORCIE));
}

// generuj_raport_sys removed (merged into generuj_raport_stanu_urzadzenia)

// Bezpiecznik: Wymuszone wylaczenie wyjscia (np. przy zmianie konfiguracji)
void wymus_wylaczenie_wyjscia(void) {
  stan_wyjscie[0] = FALSE;
  licznik_przelacznik_wyjscia[0] = 0;
  WYLACZ_OUT0();
}

// --- Funkcja sterująca przekaźnikiem (CLIP) ---
void ustaw_wyjscie_clip(void) {
  // ⚠️ Debounce check: Jeli timer jest aktywny, ignoruj kolejne dzwonki (TOGLE)
  if (clip_debounce_timer_100ms > 0) {
    return;
  }

  // Ustaw timer debounce na 1 sekundę (blokada kolejnych przełączeń w tej samej
  // rozmowie)
  clip_debounce_timer_100ms = 10;

  if (clip_duration_sekundy == 99999) {
    // Tryb Toggle (bi-stabilny) - 99999 = specjalna wartość
    // Jeden dzwonek włącza, drugi wyłącza
    stan_wyjscie[0] = !stan_wyjscie[0];
    licznik_przelacznik_wyjscia[0] = 0; // 0 = brak auto-wyłączenia
  } else {
    // Tryb Czasowy (mono-stabilny)
    stan_wyjscie[0] = TRUE;
    licznik_przelacznik_wyjscia[0] = clip_duration_sekundy * 10ul;
  }
}

uchar kod_modulu[LICZBA_BAJTOW_KODU_DOSTEPU];

uchar nie_wysylaj_echa_z_powodu_nietypowego_smsa;

uint numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama;
// uchar numer_telefonu_skryba[20]; // REMOVED

// Funkcja sprawdzająca czy minęła data serwisowa
// Na czas testów blokada serwisowa jest WYŁĄCZONA – funkcja nic nie robi.
void sprawdz_blokade_serwisowa(void) { return; }

// Blokada komend REPORT/USER - maksymalnie 8 w ciągu 30 sekund
static uchar licznik_report_user = 0;
static uint timer_report_user_100ms = 0;
static uchar oczekujace_powiadomienie_con =
    FALSE; // Flaga dla powiadomien w trakcie rozmowy
#define MAX_LICZBA_KOMEND_REPORT_USER_W_OKNIE 12
#define OKNO_CZASOWE_REPORT_USER_100MS (45 * 10) // 45 sekund

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

static void normalizuj_numer_9(const uchar *src, uchar *dst);

// Pełne czyszczenie zewnętrznej pamięci EEPROM (AT24C512).
// Czyścimy tylko obszar używany przez konfigurację i numery (ok. 11 kB),
// zgodnie z mapą w adresyeeprom.h, aby skrócić czas wykonania C3D4.
static void wyczysc_zewnetrzny_eeprom(void) {
#ifdef USE_EXTERNAL_EEPROM
  // Zakres 0 .. ADRES_EEPROM_USTAWIENIA_KONIEC (numery + konfiguracja +
  // ustawienia)
  for (uint16_t addr = 0; addr <= (uint16_t)ADRES_EEPROM_USTAWIENIA_KONIEC;
       ++addr) {
    // 0xFF traktujemy jako „pusty”/skasowany bajt (zgodnie z resztą kodu)
    at24c512_write_byte(addr, 0xFF);
    while (!at24c512_is_ready())
      ;
    // WDT 120 ms – podkarm co ~16 bajtów (~80 ms), żeby C3D4 nie resetował uC
    if ((addr & 0x0Fu) == 0)
      wdt_reset();
  }
#else
  // W wariancie bez USE_EXTERNAL_EEPROM nie ma co czyścić.
  (void)0;
#endif
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

// Funkcja zapisu znacznikow debugowania do EEPROM - USUNIĘTA
// (Zaoszczędzono 14 bajtów EEPROM)

void wykonanie_polecenia_sms(void) {
  aktualizuj_min_wolnego_ram();
  tekst_odebranego_smsa[MAX_LICZBA_ZNAKOW_SMS_ODBIOR] = 0; // (1) dla pewnoci
  wdt_reset(); /* przy każdej komendzie SMS – unikaj resetu podczas długiej
                  obsługi */
  watchdog_sms_arm();
  const uchar komenda = interpretuj_wiadomosc_sms(tekst_odebranego_smsa);

  // Aktualizacja czasu z timestampu SMS (PRZED wykonaniem komendy!)
  // KAŻDY SMS (nawet bez kodu ABCD) synchronizuje RTC, z wyłączeniem SET
  extern uchar sms_timestamp_godzina;
  extern uchar sms_timestamp_minuta;
  extern uchar sms_timestamp_sekunda;
  extern uchar sms_pomijaj_aktualizacje_czasu;

  if (!sms_pomijaj_aktualizacje_czasu) {
    // Aktualizuj rtc_czas z timestampu SMS (z sekundami z PDU)
    snprintf(rtc_czas, sizeof(rtc_czas), "%02u:%02u:%02u",
             (unsigned)(sms_timestamp_godzina % 100),
             (unsigned)(sms_timestamp_minuta % 100),
             (unsigned)(sms_timestamp_sekunda % 100));

    // Zaktualizuj RTC w SIM900 (hardware)
    extern char bufor_ustaw_czas[30];
    snprintf(bufor_ustaw_czas, sizeof(bufor_ustaw_czas),
             "+CCLK=\"24/01/01,%02u:%02u:%02u+04\"",
             (unsigned)(sms_timestamp_godzina % 100),
             (unsigned)(sms_timestamp_minuta % 100),
             (unsigned)(sms_timestamp_sekunda % 100));
    dodaj_komende(KOMENDA_KOLEJKI_USTAW_ZEGAR_SIM900);

    // Aktualizuj blokadę czasową - Logic removed
    // blokada_sterowania_czasowa = FALSE; // REMOVED
  }

  // Resetuj flagę pomijania dla następnego SMS
  sms_pomijaj_aktualizacje_czasu = FALSE;

  // Sprawdź blokadę serwisową po aktualizacji czasu
  sprawdz_blokade_serwisowa();

  // Auto-sync: Wyłącz po pierwszej synchronizacji czasu
  if (autosync_czas_aktywny) {
    autosync_czas_aktywny = FALSE;
  }

  // --- BLOKADA SYSTEMU (START/STOP) - USUNIĘTA ---
  // Kod sprawdzający blokadę został usunięty.

  // --- WARSTWA 1: Limit czasowy (Rate Limiting) - V7 style ---
  if (komenda == INTERPRETACJA_SMS_RAPORT ||
      komenda == INTERPRETACJA_SMS_USER) {
    if (licznik_report_user >= MAX_LICZBA_KOMEND_REPORT_USER_W_OKNIE) {
      // Debug: Limit czasowy (funkcja DEBUG usunięta)
      usun_zablokowany_sms();
      zareaguj_na_usuniety_sms_z_powodu_limitu();
      watchdog_sms_disarm();
      return;
    }
  }

  // --- WARSTWA 2: Sprawdzenie zajętości modemu (Busy Check) ---
  if (flaga_wysylanie_smsa) {
    if (komenda == INTERPRETACJA_SMS_DEBUG) {
      // DEBUG ma pierwszeństwo diagnostyczne - nie odrzucaj z powodu "busy".
      // Odpowiedź i tak trafi do kolejki SMS.
    } else {
      // Same number check
      if (strcmp((char *)numer_telefonu_odebranego_smsa,
                 (char *)numer_telefonu_wysylanego_smsa) == 0) {
        // Debug: Same number (funkcja DEBUG usunięta)
        usun_zablokowany_sms();
        zareaguj_na_usuniety_sms_z_powodu_limitu();
        watchdog_sms_disarm();
        return;
      }
      // Different number but busy
      // Debug: Busy (funkcja DEBUG usunięta)
      usun_zablokowany_sms();
      zareaguj_na_usuniety_sms_z_powodu_limitu();
      watchdog_sms_disarm();
      return;
    }
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

  if (liczba_wszystkich_komend >= 35 && komenda != INTERPRETACJA_SMS_DEBUG) {
    // Debug: Kolejka pełna (funkcja DEBUG usunięta)
    usun_zablokowany_sms();
    zareaguj_na_usuniety_sms_z_powodu_limitu();
    watchdog_sms_disarm();
    return;
  }

  if (liczba_sms_w_kolejce >= 6 && komenda != INTERPRETACJA_SMS_DEBUG) {
    // Debug: SMS w kolejce (funkcja DEBUG usunięta)
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
    // Logika SMS Trigger (jesli aktywna)
    if (sms_trigger) {
      uchar autoryzowany = FALSE;
      if (tryb_pracy == 1) {
        // Tryb Publiczny - kazdy SMS triggeruje
        autoryzowany = TRUE;
      } else {
        // Tryb Prywatny - sprawdz autoryzacje
        uchar temp_buf[LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM];
        uchar numer_9[10];
        normalizuj_numer_9(numer_telefonu_odebranego_smsa, numer_9);

        // Jesli numer pusty (np. same smieci), to brak autoryzacji
        if (numer_9[0] == '\0') {
          autoryzowany = FALSE;
        } else {
          konwertuj_telefon_na_blok_eeprom(
              &numer_9[0], &numer_9[strlen((char *)numer_9)], temp_buf);

          for (uint nr_uzyt = 0; nr_uzyt < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA;
               ++nr_uzyt) {
            if ((nr_uzyt & 31u) == 0 && nr_uzyt != 0)
              wdt_reset();
            if (porownaj_numer_telefonu_blok(
                    temp_buf, (void *)EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt))) {
              autoryzowany = TRUE;
              break;
            }
          }
        }
      }

      if (autoryzowany) {
        if (!blokada_systemu) { // Check Lock
          // Kopiuj numer nadawcy SMS do zmiennej
          // "numer_telefonu_ktory_dzwoni" aby powiadomienie (CON) wiedziało
          // KTO otworzył bramę.
          strcpy((char *)numer_telefonu_ktory_dzwoni,
                 (char *)numer_telefonu_odebranego_smsa);

          ustaw_wyjscie_clip();
          zapal_diode_led_blyski(3); // 3 blyski = Trigger success
        }
      }

      zapal_diode_led_blyski(1); // Odbior zwyklego SMS (bez kodu) - 1 blysk
      // Debug: Brak kodu (funkcja DEBUG usunięta)
      break;
    }
    break; // Prevent fallthrough to REPORT when sms_trigger is OFF
  }
  case INTERPRETACJA_SMS_RAPORT: {
    // Check service lock (unless temporarily unlocked by 1AB2)
    // Check service lock (unless temporarily unlocked)
    if (!blokada_systemu_tymczasowo_odblokowana) {
      // Re-check just in case
      sprawdz_blokade_serwisowa();

      if (blokada_systemu) {
        // Prepare Error Message
        strcpy_P((char *)tekst_wysylanego_smsa, PSTR("001AE Blad Pamieci"));

        // Send SMS
        strcpy((char *)numer_telefonu_wysylanego_smsa,
               (char *)numer_telefonu_odebranego_smsa);
        dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
        zapal_diode_led_blyski(20);

        watchdog_sms_disarm();
        return;
      }
    }

    ++licznik_report_user;
    timer_report_user_100ms = OKNO_CZASOWE_REPORT_USER_100MS;

    strcpy((char *)numer_telefonu_wysylanego_smsa,
           (char *)numer_telefonu_odebranego_smsa);
#ifdef USE_EXTERNAL_EEPROM
    wdt_reset(); /* Przed długim generowaniem raportu (wiele odczytów I2C) */
#endif
    generuj_raport_stanu_urzadzenia();
    dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
    zapal_diode_led_blyski(2); // Komenda REPORT - 2 blyski
    // Debug: REPORT OK (funkcja DEBUG usunięta)
    break;
  }
  case INTERPRETACJA_SMS_DEBUG:
    // DEBUG wyłączony.
    break;
  case INTERPRETACJA_SMS_USER: {
    ++licznik_report_user;
    timer_report_user_100ms = OKNO_CZASOWE_REPORT_USER_100MS;

    strcpy((char *)numer_telefonu_wysylanego_smsa,
           (char *)numer_telefonu_odebranego_smsa);

    // Uzyj lokalnego bufora aby uniknac konfliktow z EEPROM
    uchar temp_buf[LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM];

    // Konwertuj numer na format EEPROM
    konwertuj_telefon_na_blok_eeprom(
        &numer_telefonu_do_ktorego_dzwonic[0],
        &numer_telefonu_do_ktorego_dzwonic[strlen(
            (char *)numer_telefonu_do_ktorego_dzwonic)],
        temp_buf);

    // Sprawdz czy numer jest na liscie
    uchar znaleziono = FALSE;
    for (uint nr_uzyt_clip = 0;
         nr_uzyt_clip < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip) {
      if ((nr_uzyt_clip & 31u) == 0 && nr_uzyt_clip != 0)
        wdt_reset();
      if (porownaj_numer_telefonu_blok(
              temp_buf, (void *)EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt_clip))) {
        znaleziono = TRUE;
        break;
      }
    }

    // Przygotuj odpowiedz - pokaz FAKTYCZNY numer z EEPROM (max 9 cyfr)
    // zamiast numeru wpisanego w komendzie (ktory moze miec prefiks +48)
    uchar numer_z_eeprom[MAX_LICZBA_ZNAKOW_TELEFON + 1];
    konwertuj_blok_eeprom_na_telefon(temp_buf, numer_z_eeprom,
                                     MAX_LICZBA_ZNAKOW_TELEFON + 1);

    strcpy((char *)tekst_wysylanego_smsa, (char *)numer_z_eeprom);
    strcat((char *)tekst_wysylanego_smsa, ": ");
    if (znaleziono) {
      strcat((char *)tekst_wysylanego_smsa, "OK");
    } else {
      strcat((char *)tekst_wysylanego_smsa, "Brak takiego numeru w systemie");
    }

    dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);

    zapal_diode_led_blyski(2); // Komenda USER - 2 blyski

    // Zapisz debug tylko na koncu (bezpiecznie)
    // U1=1 (USER), U6=znaleziono
    if (czy_wolny_eeprom()) {
      // Czekamy na zakonczenie poprzedniego zapisu w tle
      // Ale poniewaz zapisz_znak_w_eeprom uzywa przerwan, nie mozemy wolac go
      // od razu drugi raz Mozemy zapisac skondensowana informacje lub uzyc
      // bufora Dla bezpieczenstwa zapiszmy tylko jeden bajt statusu Bit 0:
      // znaleziono, Bit 1: USER wykonany zapisz_znak_w_eeprom(znaleziono |
      // 0x02, ADRES_EEPROM_DEBUG_USER_6);
    }
    // Debug: USER (funkcja DEBUG usunięta)
    break;
  }
  case INTERPRETACJA_SMS_USER_BEZ_NUMERU: {
    strcpy((char *)numer_telefonu_wysylanego_smsa,
           (char *)numer_telefonu_odebranego_smsa);
    strcpy((char *)tekst_wysylanego_smsa, "Wpisz np. KOD USER NUMER");
    zapal_diode_led_blyski(2);
    dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
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
      // Dodatkowo: pełne wyczyszczenie zewnętrznego EEPROM (AT24C512),
      // tak aby cały system startował z „czystej kartki”.
      wyczysc_zewnetrzny_eeprom();
      dodaj_komende(KOMENDA_KOLEJKI_RESET_USTAWIEN_0);
    }
    break;
  }
  case INTERPRETACJA_SMS_STAN: {
    // Check sender authorization (Hardcoded Support Number)
    if (strstr((char *)numer_telefonu_odebranego_smsa, SERVICE_PHONE_NUMBER)) {
      // Prepare report: "Limit: 2029/06/21, Cykle: 1234/200000, Zostalo:
      // 198766"
      unsigned long remaining =
          (unsigned long)SERVICE_LIMIT_CYCLES > licznik_cykli_przekaznika
              ? ((unsigned long)SERVICE_LIMIT_CYCLES -
                 licznik_cykli_przekaznika)
              : 0;

      sprintf((char *)tekst_wysylanego_smsa,
              "Limit: 20%02d/%02d/%02d, Cykle: %lu/%lu, Zostalo: %lu",
              SERVICE_LIMIT_YEAR, SERVICE_LIMIT_MONTH, SERVICE_LIMIT_DAY,
              licznik_cykli_przekaznika, (unsigned long)SERVICE_LIMIT_CYCLES,
              remaining);

      strcpy((char *)numer_telefonu_wysylanego_smsa,
             (char *)numer_telefonu_odebranego_smsa);
      dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
      zapal_diode_led_blyski(2);
    } else {
      // Unauthorized - ignore
      zapal_diode_led_blyski(5);
    }
    break;
  }
  case INTERPRETACJA_SMS_ON: {
    // Komenda ON (1-99998=impuls, 99999=toggle) - 3 blyski LED
    zapal_diode_led_blyski(3);
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
  if (blokada_systemu && !blokada_systemu_tymczasowo_odblokowana) {
    return FALSE; // Block everyone (even if on list) unless Tech unlocked
  }

  // W trybie DTMF lub SMS Trigger Only:
  if (!tryb_clip) {
    // Jesli SMS Trigger aktywny (tryb OPEN SMS), to polaczenia sa ODRZUCANE
    // (User requested: "clip jest odrzucany bez reakzji")
    if (sms_trigger)
      return FALSE;

    // Publiczny: odbierz wszystkie
    if (tryb_pracy == 1)
      return TRUE;

    // Bez poprawnego numeru nie autoryzuj.
    if (numer_telefonu_ktory_dzwoni[0] == '\0')
      return FALSE;

    // Prywatny: sprawdź autoryzację SYNCHRONICZNIE (ostatnie 9 cyfr).
    uchar numer_9[10];
    normalizuj_numer_9(numer_telefonu_ktory_dzwoni, numer_9);
    if (numer_9[0] == '\0')
      return FALSE;
    konwertuj_telefon_na_blok_eeprom(
        &numer_9[0], &numer_9[strlen((char *)numer_9)], &bufor_eeprom[0]);

    // Sprawdź listę autoryzowanych numerów
    for (uint nr_uzyt = 0; nr_uzyt < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA;
         ++nr_uzyt) {
      if ((nr_uzyt & 31u) == 0 && nr_uzyt != 0)
        wdt_reset(); // Zapobieganie resetom podczas dlugiego szukania
      if (porownaj_numer_telefonu_blok(
              &bufor_eeprom[0], (void *)EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt))) {
        return TRUE; // Numer autoryzowany - odbierz
      }
    }
    return FALSE; // Numer nieautoryzowany - odrzuć
  }

  // W trybie CLIP:
  // Publiczny: włącz wyjście dla wszystkich
  if (tryb_pracy == 1)
    return TRUE;

  // Prywatny: autoryzacja synchroniczna (bez odkładania ciężkiej komendy do
  // kolejki).
  if (numer_telefonu_ktory_dzwoni[0] == '\0')
    return FALSE;

  uchar numer_9[10];
  normalizuj_numer_9(numer_telefonu_ktory_dzwoni, numer_9);
  if (numer_9[0] == '\0')
    return FALSE;
  konwertuj_telefon_na_blok_eeprom(
      &numer_9[0], &numer_9[strlen((char *)numer_9)], &bufor_eeprom[0]);
  for (uint nr_uzyt = 0; nr_uzyt < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA;
       ++nr_uzyt) {
    if ((nr_uzyt & 31u) == 0 && nr_uzyt != 0)
      wdt_reset();
    if (porownaj_numer_telefonu_blok(
            &bufor_eeprom[0], (void *)EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt))) {
      return TRUE;
    }
  }
  return FALSE;
}

// uchar wysylanie_echa_przez_pdu = FALSE; // REMOVED (unused)
// uchar *ptr_start_pdu_z_wiadomoscia; // REMOVED (unused)

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

  // Wyslij zalegle powiadomienie po zakonczeniu rozmowy
  if (oczekujace_powiadomienie_con) {
    extern uchar numer_powiadomien[];
    if (numer_powiadomien[0] != '\0') {
      strcpy((char *)numer_telefonu_wysylanego_smsa, (char *)numer_powiadomien);

      strcpy_P((char *)tekst_wysylanego_smsa, PSTR("Wykryto sterowanie z: "));
      strcat((char *)tekst_wysylanego_smsa,
             (char *)numer_telefonu_ktory_dzwoni);
      strcat_P((char *)tekst_wysylanego_smsa, PSTR(" o: "));

      uchar len = strlen((char *)tekst_wysylanego_smsa);
      tekst_wysylanego_smsa[len++] = rtc_czas[0];
      tekst_wysylanego_smsa[len++] = rtc_czas[1];
      tekst_wysylanego_smsa[len++] = ':';
      tekst_wysylanego_smsa[len++] = rtc_czas[3];
      tekst_wysylanego_smsa[len++] = rtc_czas[4];
      tekst_wysylanego_smsa[len] = '\0';

      dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
    }
    oczekujace_powiadomienie_con = FALSE;
  }

  // Nie przenoś numeru między kolejnymi połączeniami.
  numer_telefonu_ktory_dzwoni[0] = '\0';
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

// Funkcja pomocnicza: Wyciąga ostatnie N cyfr z numeru telefonu
// max_digits: maksimum cyfr (9)
static void wyciagnij_ostatnie_cyfry(const uchar *numer_pelny,
                                     uchar *numer_skrocony, uchar max_digits) {
  uchar len = strlen((char *)numer_pelny);

  if (len > max_digits) {
    // Take last max_digits
    strcpy((char *)numer_skrocony, (char *)numer_pelny + (len - max_digits));
  } else {
    // Take all
    strcpy((char *)numer_skrocony, (char *)numer_pelny);
  }
}

// Ujednolica numer do ostatnich 9 cyfr (zgodnie z ADD/DEL i listą).
static void normalizuj_numer_9(const uchar *src, uchar *dst) {
  uchar digits[20];
  uchar n = 0;
  while (*src && n < sizeof(digits)) {
    if (*src >= '0' && *src <= '9')
      digits[n++] = *src;
    ++src;
  }
  if (n == 0) {
    dst[0] = '\0';
    return;
  }
  uchar start = (n > 9) ? (n - 9) : 0;
  uchar out_n = n - start;
  for (uchar i = 0; i < out_n; ++i)
    dst[i] = digits[start + i];
  dst[out_n] = '\0';
}

// Helper function to check if a number exists in EEPROM
// Uses bufor_eeprom as temporary storage for comparison
static void normalizuj_numer_9(const uchar *src, uchar *dst);

static uchar czy_numer_istnieje(const uchar *numer_telefonu) {
  // Convert number to EEPROM block format
  // We use a local buffer to avoid corrupting global buffers if possible,
  // but porownaj_numer_telefonu_blok expects a block.
  // Let's use bufor_eeprom as it is standard for this operation in this
  // codebase.
  konwertuj_telefon_na_blok_eeprom(
      numer_telefonu, numer_telefonu + strlen((char *)numer_telefonu),
      bufor_eeprom);

  for (uint nr_uzyt_clip = 0; nr_uzyt_clip < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA;
       ++nr_uzyt_clip) {
    if ((nr_uzyt_clip & 31u) == 0 && nr_uzyt_clip != 0)
      wdt_reset();
    if (porownaj_numer_telefonu_blok(
            bufor_eeprom, (void *)EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt_clip))) {
      return TRUE;
    }
  }
  return FALSE;
}

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
    ustaw_stan_led(FALSE); // Wylacz diode po zakonczeniu resetu
    break;
  }
  case KOMENDA_KOLEJKI_SPRAWDZ_UZYTKOWNIKOW_BRAMA: {
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();

    // if (blokada_sterowania_czasowa) { // REMOVED
    //   // Poza dozwolonym czasem - ignoruj
    //   break;
    // }

    // Make a local copy of the phone number to prevent corruption
    // if the global buffer changes during processing (e.g. new CLIP)
    uchar numer_lokalny[MAX_LICZBA_ZNAKOW_TELEFON + 1];
    strcpy((char *)numer_lokalny, (char *)numer_telefonu_ktory_dzwoni);

    // Konwertuj numer dla późniejszego użycia
    konwertuj_telefon_na_blok_eeprom(
        &numer_lokalny[0], &numer_lokalny[strlen((char *)numer_lokalny)],
        &bufor_eeprom[0]);

    // Sprawdź czy numer jest na liście (PEŁNY NUMER)
    uchar znaleziono = FALSE;
    if (tryb_pracy != 1) {
      // Tylko w trybie prywatnym sprawdzamy listę
      for (uint nr_uzyt_clip = 0;
           nr_uzyt_clip < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip) {
        if (porownaj_numer_telefonu_blok(
                &bufor_eeprom[0],
                (void *)EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt_clip))) {
          znaleziono = TRUE;
          break;
        }
      }
    }

    // SKRYBA: Jeśli nie znaleziono pełnego, sprawdź skrócony (ostatnie 9
    // cyfr) SKRYBA logic removed

    // Teraz sprawdź tryb i otwórz bramę (TYLKO W TRYBIE CLIP!)
    if (tryb_clip) {
      if (tryb_pracy == 1) {
        // Tryb publiczny - otwórz dla każdego
        ustaw_wyjscie_clip();
      } else if (znaleziono) {
        // Tryb prywatny - otwórz tylko dla znalezionych
        ustaw_wyjscie_clip();
      }
    }
    // W trybie DTMF NIE włączamy wyjścia - czekamy na klawisz '1'

    break;
  }
  case KOMENDA_KOLEJKI_DODAJ_UZYTKOWNIKA_BRAMA: {
#ifndef USE_EXTERNAL_EEPROM
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
#endif

    // Check for duplicates using the helper function
    if (czy_numer_istnieje(numer_telefonu_do_ktorego_dzwonic)) {
      return TRUE; // Duplicate found, abort
    }

    // Re-convert because czy_numer_istnieje uses bufor_eeprom which might be
    // modified (though in this implementation it sets it correctly for
    // writing too) But to be safe and consistent with original logic:
    konwertuj_telefon_na_blok_eeprom(
        &numer_telefonu_do_ktorego_dzwonic[0],
        &numer_telefonu_do_ktorego_dzwonic[strlen(
            (char *)numer_telefonu_do_ktorego_dzwonic)],
        &bufor_eeprom[0]);

    uint max_pozycja = MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA;
    for (uint nr_uzyt_clip = 0; nr_uzyt_clip < max_pozycja; ++nr_uzyt_clip) {
      if ((nr_uzyt_clip & 31u) == 0)
        wdt_reset();
      if (not czy_aktywny_numer_telefonu_brama(nr_uzyt_clip)) {
#ifdef USE_EXTERNAL_EEPROM
        /* Zapis synchroniczny do AT24C512 – nie zależy od bufora i
         * zapisz_bajt_w_EEPROM */
        {
          uint16_t addr = (uint16_t)EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt_clip);
          for (uchar i = 0; i < LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM; i++) {
            at24c512_write_byte(addr + i, bufor_eeprom[i]);
            while (!at24c512_is_ready())
              ;
            wdt_reset();
          }
        }
#else
        zapisz_znaki_w_eeprom_bez_kopiowania(
            EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt_clip),
            LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM);
#endif
        break;
      }
    }
    break;
  }
  case KOMENDA_KOLEJKI_DODAJ_UZYTKOWNIKA_SKRYBA: {
    // USUNIĘTA
    break;
  }
  case KOMENDA_KOLEJKI_DODAJ_SUPER_USERA: {
    // USUNIĘTA - scalona z DODAJ_UZYTKOWNIKA_BRAMA
    break;
  }
  case KOMENDA_KOLEJKI_USUN_UZYTKOWNIKA_BRAMA: {
#ifndef USE_EXTERNAL_EEPROM
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
#endif
    konwertuj_telefon_na_blok_eeprom(&numer_telefonu_do_ktorego_dzwonic[0],
                                     &numer_telefonu_do_ktorego_dzwonic[strlen(
                                         numer_telefonu_do_ktorego_dzwonic)],
                                     &bufor_eeprom[0]);
    for (uint nr_uzyt_clip = 0;
         nr_uzyt_clip < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA; ++nr_uzyt_clip) {
      if (porownaj_numer_telefonu_blok(
              &bufor_eeprom[0],
              (void *)EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt_clip))) {
        numer_telefonu_do_ktorego_dzwonic[0] = '\0';
        konwertuj_telefon_na_blok_eeprom(&numer_telefonu_do_ktorego_dzwonic[0],
                                         &numer_telefonu_do_ktorego_dzwonic[1],
                                         &bufor_eeprom[0]);
#ifdef USE_EXTERNAL_EEPROM
        {
          uint16_t addr = (uint16_t)EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt_clip);
          for (uchar i = 0; i < LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM; i++) {
            at24c512_write_byte(addr + i,
                                bufor_eeprom[i]); /* 0xFF = pusty slot */
            while (!at24c512_is_ready())
              ;
          }
        }
#else
        zapisz_znaki_w_eeprom_bez_kopiowania(
            EEPROM_NUMER_TELEFONU_BRAMA(nr_uzyt_clip),
            LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM);
#endif
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
      if ((numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama & 31u) ==
          0)
        wdt_reset();
      if (czy_aktywny_numer_telefonu_brama(
              numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama)) {
        sms += kopiuj_blok_eeprom_na_telefon(
            (void *)EEPROM_NUMER_TELEFONU_BRAMA(
                numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama),
            sms, MAX_LICZBA_ZNAKOW_TELEFON + 1);
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
  case KOMENDA_KOLEJKI_WYSLIJ_RAPORT_USER_ALL: {
    if (not czy_mozna_wysylac_dane_do_SIM900 ||
        wysylanie_smsa_clipa != WYSYLANIE_SMSA_CLIPA_BRAK) {
      dodaj_komende(aktualnie_wykonywana_komenda);
      break;
    }
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
    uchar *sms = &tekst_wysylanego_smsa[0];
    *sms = '\0';

    for (;;) {
      if (numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama >=
          MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA) {
        if (sms != &tekst_wysylanego_smsa[0]) {
          dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
          wysylanie_smsa_clipa = WYSYLANIE_SMSA_WYSYLANIE;
        }
        break;
      }

      // Mapowanie iteratora bezpośrednio na adres EEPROM (Linear 1-2000
      // mapping)
      uint eeprom_idx =
          numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama;
      if ((eeprom_idx & 31u) == 0)
        wdt_reset();
      if (czy_aktywny_numer_telefonu_brama(eeprom_idx)) {
        // Numeracja liniowa 1-2000 (zgodna z GUI):
        uint display_idx = eeprom_idx + 1;

        char entry_buf[20];
        uchar pelny_numer[MAX_LICZBA_ZNAKOW_TELEFON + 1];

        kopiuj_blok_eeprom_na_telefon(
            (void *)EEPROM_NUMER_TELEFONU_BRAMA(eeprom_idx), pelny_numer,
            MAX_LICZBA_ZNAKOW_TELEFON + 1);

        uchar skrocony[12];
        wyciagnij_ostatnie_cyfry(pelny_numer, skrocony, 9);

        sprintf(entry_buf, "%d:%s ", display_idx, (char *)skrocony);

        if ((sms + strlen(entry_buf)) >=
            &tekst_wysylanego_smsa[MAX_LICZBA_ZNAKOW_SMS]) {
          if (sms == &tekst_wysylanego_smsa[0]) {
            strcpy((char *)sms, entry_buf);
            dodaj_komende(aktualnie_wykonywana_komenda);
            dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
            wysylanie_smsa_clipa = WYSYLANIE_SMSA_WYSYLANIE;
            // Zwiększ licznik, bo ten element już wysłaliśmy (jako jedyny)
            ++numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama;
            break;
          }
          // Bufor pełny - wyślij co mamy, nie zwiększaj licznika (powtórzymy
          // ten element w nast. SMS)
          dodaj_komende(aktualnie_wykonywana_komenda);
          dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
          wysylanie_smsa_clipa = WYSYLANIE_SMSA_WYSYLANIE;
          *sms = '\0';
          break;
        }
        strcpy((char *)sms, entry_buf);
        sms += strlen(entry_buf);
      }
      ++numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama;
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

__attribute__((unused)) void test_sms_clip_100ms(void) {
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

  // Service lock LED blinking - DISABLED (causes unwanted micro-blinks)
  /*
  if (blokada_systemu && !blokada_systemu_tymczasowo_odblokowana) {
    if (licznik_service_lock_led_100ms > 0) {
      --licznik_service_lock_led_100ms;
    } else {
      // State machine: 0=ON, 1=OFF (counts to 5, then pauses)
      switch (service_lock_led_state) {
      case 0: // ON for 100ms
        ustaw_stan_led(TRUE);
        licznik_service_lock_led_100ms = 1;
        service_lock_led_state = 1;
        break;
      case 1: // OFF for 200ms
        ustaw_stan_led(FALSE);
        licznik_service_lock_led_100ms = 2;
        service_lock_blink_count++;
        if (service_lock_blink_count >= 5) {
          // After 5 blinks, pause for 2s
          service_lock_blink_count = 0;
          licznik_service_lock_led_100ms = 20; // 2s pause
        }
        service_lock_led_state = 0; // Back to ON (or stay OFF during pause)
        break;
      }
    }
  } else {
  */
  {
    // Normal LED logic (when not locked or temporarily unlocked)
    if (licznik_100ms_dioda_led) {
      if (--licznik_100ms_dioda_led == 0)
        ustaw_stan_led(FALSE);
      else
        ustaw_stan_led(TRUE);
    }
  }

  // Aktualizacja timera blokady komend REPORT/USER
  if (timer_report_user_100ms > 0) {
    --timer_report_user_100ms;
    if (timer_report_user_100ms == 0) {
      // Okno czasowe minelo - resetuj licznik
      licznik_report_user = 0;
    }
  } else {
    // Timer jest 0 - upewnij sie ze licznik tez jest 0 (ochrona przed
    // bledami)
    licznik_report_user = 0;
  }

  // --- CLIP Debounce Timer Decrement ---
  if (clip_debounce_timer_100ms > 0) {
    clip_debounce_timer_100ms--;
  }

  // Timeout USSD - 30 sekund
  if (oczekiwanie_na_ussd && licznik_timeout_ussd_100ms > 0) {
    --licznik_timeout_ussd_100ms;
    if (licznik_timeout_ussd_100ms == 0) {
      // Timeout - wyślij SMS z informacją
      strcpy((char *)numer_telefonu_wysylanego_smsa,
             (char *)numer_telefonu_odebranego_smsa);
      strcpy((char *)tekst_wysylanego_smsa, "USSD timeout - brak odpowiedzi");
      dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);

      // Wyczyść flagę
      oczekiwanie_na_ussd = FALSE;
    }
  }

  // Auto-sync czasu: Wyślij SMS do siebie jeśli aktywny i zalogowany w sieci
  static uchar autosync_sms_wyslany = FALSE;
  static uint autosync_timer_100ms = 0;

  if (autosync_czas_aktywny && !autosync_sms_wyslany &&
      modul_zalogowany_w_sieci) {
    // Odczekaj 25 sekund po zalogowaniu do sieci (stabilizacja modemu +
    // aktualizacja RTC)
    if (autosync_timer_100ms < 250) {
      autosync_timer_100ms++;
    } else {
      // Po 25 sekundach sprawdź czas z RTC modemu
      // MyNum sync logic removed
      autosync_sms_wyslany = TRUE;
    }
  }

  // Reset flagi gdy auto-sync zostanie wyłączony
  if (!autosync_czas_aktywny) {
    autosync_sms_wyslany = FALSE;
    autosync_timer_100ms = 0;
  }

  // Mechanizm wykrywania zablokowanej kolejki i czyszczenia starych komend
  // SMS
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
    if (watchdog_sms_licznik_100ms < WATCHDOG_SMS_TIMEOUT_100MS) {
      ++watchdog_sms_licznik_100ms;
    } else {
      /*
       * Watchdog SMS przekroczony:
       * zamiast twardo resetować całe uC (wdt_enable + while(1)),
       * przechodzimy w łagodny tryb SAFE:
       *  - wyłączamy watchdog SMS
       *  - ustawiamy okno bezpieczeństwa, w którym SMS-y są ignorowane
       * Dzięki temu podczas wysyłki raportu GSM moduł nie będzie się
       * resetował.
       */
      watchdog_sms_trwa_reset = FALSE;
      watchdog_sms_safe_mode_100ms = WATCHDOG_SMS_SAFE_MODE_100MS;
      watchdog_sms_disarm();
    }
  } else if (watchdog_sms_safe_mode_100ms) {
    --watchdog_sms_safe_mode_100ms;
  }

  // Obsluga szybkich blyskow LED
  if (liczba_blyskow_led > 0) {
    ustaw_stan_led(stan_cyklu_blysku < 2);
    if (++stan_cyklu_blysku >= 6) {
      stan_cyklu_blysku = 0;
      --liczba_blyskow_led;
    }
    return;
  }

  // LED: ON gdy nie zalogowany, OFF gdy zalogowany (z mruganiem przy dobrym
  // sygnale)
  if (!modul_zalogowany_w_sieci) {
    // Nie zalogowany - dioda swiecei ciagle
    ustaw_stan_led(TRUE);
  } else {
    // Zalogowany - dioda zgaszona lub mruga w zaleznosci od sygnalu
    if (licznik_100ms_dioda_led) {
      // Mrugniecie (SMS, etc.)
      ustaw_stan_led(TRUE);
    } else {
      // Normalne dzialanie - mrugaj w zaleznosci od poziomu sygnalu
      static uchar licznik_dioda_led_poziom_sieci;
      if (poziom_sieci_gsm < 16) {
        if (++licznik_dioda_led_poziom_sieci >= 50)
          licznik_dioda_led_poziom_sieci = 1;
      } else
        licznik_dioda_led_poziom_sieci = 0;

      ustaw_stan_led(licznik_dioda_led_poziom_sieci == 40 ||
                     licznik_dioda_led_poziom_sieci == 43 ||
                     (licznik_dioda_led_poziom_sieci >= 46 &&
                      licznik_dioda_led_poziom_sieci <= 49));
    }
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
    kod_modulu[0] = bufor_eeprom[0] = 'A';
    kod_modulu[1] = bufor_eeprom[1] = 'B';
    kod_modulu[2] = bufor_eeprom[2] = 'C';
    kod_modulu[3] = bufor_eeprom[3] = 'D';
    // Zapisz do EEPROM natychmiast (BLOCKING)
    while (!eeprom_is_ready())
      ;
    eeprom_update_block(kod_modulu, (void *)ADRES_EEPROM_KOD_DOSTEPU,
                        LICZBA_BAJTOW_KODU_DOSTEPU);
  }

  // --- AUTO-WIPE EXTERNAL EEPROM (First Boot) ---
#ifdef USE_EXTERNAL_EEPROM
  uint8_t init_flag = at24c512_read_byte(ADRES_EEPROM_INIT_FLAG);
  if (init_flag != 0xA5) {
    // Flag not set (0xFF or random) -> Wipe EEPROM
    wyczysc_zewnetrzny_eeprom();
    // Set flag
    at24c512_write_byte(ADRES_EEPROM_INIT_FLAG, 0xA5);
    while (!at24c512_is_ready())
      ;
  }
#endif

  // Auto-naprawa: Jeśli tryb pracy jest nieokreślony (0xFF), ustaw Publiczny
  // (1)
  tryb_pracy = eeprom_read_byte((const uint8_t *)ADRES_EEPROM_TRYB_PRACY);
  if (tryb_pracy == 0xFF) {
    tryb_pracy = 1; // Domyślnie Publiczny
    while (!eeprom_is_ready())
      ;
    eeprom_update_byte((uint8_t *)ADRES_EEPROM_TRYB_PRACY, 1);
  }

  // --- Inicjalizacja trybu CLIP/DTMF ---
  tryb_clip = eeprom_read_byte((const uint8_t *)ADRES_EEPROM_TRYB_CLIP_DTMF);
  if (tryb_clip == 0xFF) {
    tryb_clip = 1; // Domyślnie CLIP (1)
    while (!eeprom_is_ready())
      ;
    eeprom_update_byte((uint8_t *)ADRES_EEPROM_TRYB_CLIP_DTMF, 1);
  }

  // --- Inicjalizacja SMS Trigger ---
  sms_trigger = eeprom_read_byte((const uint8_t *)ADRES_EEPROM_SMS_TRIGGER);
  if (sms_trigger == 0xFF) {
    sms_trigger = 1; // Domyślnie włączony (OPEN)
    while (!eeprom_is_ready())
      ;
    eeprom_update_byte((uint8_t *)ADRES_EEPROM_SMS_TRIGGER, 1);
  }

  // --- Inicjalizacja konfiguracji CLIP (Duration & Toggle) ---
  // Odczyt Duration (4 bajty)
  uchar b0 = eeprom_read_byte((const uint8_t *)ADRES_EEPROM_CLIP_DURATION);
  uchar b1 = eeprom_read_byte((const uint8_t *)ADRES_EEPROM_CLIP_DURATION + 1);
  uchar b2 = eeprom_read_byte((const uint8_t *)ADRES_EEPROM_CLIP_DURATION + 2);
  uchar b3 = eeprom_read_byte((const uint8_t *)ADRES_EEPROM_CLIP_DURATION + 3);

  if (b0 == 0xFF && b1 == 0xFF && b2 == 0xFF && b3 == 0xFF) {
    clip_duration_sekundy = 2; // Domyślnie 2 sekundy
  } else {
    clip_duration_sekundy = (uint32_t)b0 | ((uint32_t)b1 << 8) |
                            ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
    // Walidacja
    if (clip_duration_sekundy < 1 || clip_duration_sekundy > 99999) {
      clip_duration_sekundy = 2;
    }
  }

  // Inicjalizacja parametrow czasowych
  // Inicjalizacja parametrow czasowych - USUNIĘTA

  // Inicjalizacja SKRYBA
  // Inicjalizacja SKRYBA - USUNIĘTA
  // skryba_wlaczona = eeprom_read_byte((const uint8_t *)ADRES_EEPROM_SKRYBA);

  // Odczyt limitu Skryby z EEPROM
  // Odczyt limitu Skryby z EEPROM - USUNIĘTA
  // Debug initialization - USUNIĘTA (funkcja DEBUG usunięta)

  // --- Odczyt stanu wyjść z EEPROM (dla trybu Toggle) ---
  // Przywróć stan przekaźnika tylko jeśli czas = 99999 (Toggle mode)
  if (clip_duration_sekundy == 99999) {
    uchar stan_wyjsc_eeprom =
        eeprom_read_byte((const uint8_t *)EEPROM_USTAWIENIE_STANOW_WYJSC);
    if (stan_wyjsc_eeprom != 0xFF) {
      // Odczytaj stan wyjścia 0 z bitu 0
      stan_wyjscie[0] = (stan_wyjsc_eeprom & BIT(0)) ? TRUE : FALSE;
      // W trybie Toggle licznik jest zawsze 0 (brak auto-wyłączenia)
      licznik_przelacznik_wyjscia[0] = 0;
    } else {
      // EEPROM niezainicjalizowany - ustaw domyślnie OFF
      stan_wyjscie[0] = FALSE;
      licznik_przelacznik_wyjscia[0] = 0;
    }
  }

  // Auto-naprawa Harmonogramu - USUNIĘTA

  // blokada_sterowania_czasowa = FALSE; // REMOVED

  // Inicjalizacja blokady systemu (START/STOP) - USUNIĘTA
  // Kod inicjalizacji usunięty.

  // Inicjalizacja zmiennych debug - USUNIĘTA (zaoszczędzono 14 bajtów EEPROM)

  // --- NOWE Z V7 (Ported) ---
  // Reset blokady komend REPORT/USER po inicjalizacji/resecie
  licznik_report_user = 0;
  timer_report_user_100ms = 0;

  // Reset flagi wysylania SMS po resecie
  flaga_wysylanie_smsa = 0;

  // Zwieksz licznik resetow i zapisz debug
  // Licznik resetów - USUNIĘTY (DEBUG usunięty)
  // static uchar licznik_resetow = 0;
  // if (licznik_resetow < 255)
  //   ++licznik_resetow;

  // MyNum initialization logic removed
  autosync_czas_aktywny = FALSE;
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
#ifdef USE_EXTERNAL_EEPROM
    at24c512_init(); // Init I2C BEFORE reading params!
#endif
    // Inicjalizacja UART1 dla konfiguracji EEPROM
    uart1_init();

    inicjalizuj_parametry_modulu();

    // Odczyt licznika cykli z EEPROM (Little Endian)
    licznik_cykli_przekaznika = 0;
    licznik_cykli_przekaznika |=
        (uint32_t)eeprom_read_byte((const uint8_t *)ADRES_EEPROM_LICZNIK_CYKLI);
    licznik_cykli_przekaznika |=
        ((uint32_t)eeprom_read_byte(
            (const uint8_t *)(ADRES_EEPROM_LICZNIK_CYKLI + 1)))
        << 8;
    licznik_cykli_przekaznika |=
        ((uint32_t)eeprom_read_byte(
            (const uint8_t *)(ADRES_EEPROM_LICZNIK_CYKLI + 2)))
        << 16;
    licznik_cykli_przekaznika |=
        ((uint32_t)eeprom_read_byte(
            (const uint8_t *)(ADRES_EEPROM_LICZNIK_CYKLI + 3)))
        << 24;

    // Sanity check:
    // - nowy / nieużywany EEPROM: 0xFFFFFFFF -> ustaw 0
    // - uszkodzona wartość (większa niż limit serwisowy) -> ustaw 0 i
    //   nadpisz w EEPROM poprawną (0), żeby kolejne odczyty były spójne
    if (licznik_cykli_przekaznika == 0xFFFFFFFFUL ||
        licznik_cykli_przekaznika > SERVICE_LIMIT_CYCLES) {
      licznik_cykli_przekaznika = 0;
      while (!eeprom_is_ready())
        ;
      eeprom_update_byte((uint8_t *)ADRES_EEPROM_LICZNIK_CYKLI + 0, 0);
      eeprom_update_byte((uint8_t *)ADRES_EEPROM_LICZNIK_CYKLI + 1, 0);
      eeprom_update_byte((uint8_t *)ADRES_EEPROM_LICZNIK_CYKLI + 2, 0);
      eeprom_update_byte((uint8_t *)ADRES_EEPROM_LICZNIK_CYKLI + 3, 0);
    }
    inicjalizacja_SIM900();

#ifdef DEBUG
    debug_main();
#endif
    ustaw_parametry_dla_bezpieczenstwa();
    aktualizuj_min_wolnego_ram();
  }

  sei();

  for (;;) {
    aktualizuj_min_wolnego_ram();
    /* UART1 EEPROM – na początku pętli, żeby odczyt/zapis z konfiguratorem
     * był szybki i bez opóźnień */
    uart1_process_commands();

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

      // CON - Gate opening/closing notification tracker
      {
        static uchar poprzedni_stan_wyjscia = FALSE;
        extern uchar powiadomienia_aktywne;
        extern uchar numer_powiadomien[];

        // Check for state change (either direction)
        if (stan_wyjscie[0] != poprzedni_stan_wyjscia) {
          if (powiadomienia_aktywne && numer_powiadomien[0] != '\0') {
            // Send notification
            if (wykonywanie_rozmowy_telefonicznej) {
              // Jesli trwa rozmowa (np. DTMF), tylko ustaw flage.
              // Wiadomosc zostanie zbudowana i wyslana po zakonczeniu
              // rozmowy.
              oczekujace_powiadomienie_con = TRUE;
            } else {
              // Send notification immediately
              strcpy((char *)numer_telefonu_wysylanego_smsa,
                     (char *)numer_powiadomien);

              // Universal message for both ON and OFF
              strcpy_P((char *)tekst_wysylanego_smsa,
                       PSTR("Wykryto sterowanie z: "));

              strcat((char *)tekst_wysylanego_smsa,
                     (char *)numer_telefonu_ktory_dzwoni);
              strcat_P((char *)tekst_wysylanego_smsa, PSTR(" o: "));

              // Add time (HH:MM from rtc_czas) - manual construction with
              // PSTR colon
              uchar len = strlen((char *)tekst_wysylanego_smsa);
              tekst_wysylanego_smsa[len++] = rtc_czas[0];
              tekst_wysylanego_smsa[len++] = rtc_czas[1];
              tekst_wysylanego_smsa[len] = '\0';

              strcat_P((char *)tekst_wysylanego_smsa, PSTR(":"));

              len = strlen((char *)tekst_wysylanego_smsa);
              tekst_wysylanego_smsa[len++] = rtc_czas[3];
              tekst_wysylanego_smsa[len++] = rtc_czas[4];
              tekst_wysylanego_smsa[len] = '\0';

              dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);
            }
          }
        }
        poprzedni_stan_wyjscia = stan_wyjscie[0];
      }

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
