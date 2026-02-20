#ifndef INCLUDE

#include "interpretacjaSMS.h"
#include "ctype.h"
#include "komendy.h"
#include "pin_ATmega328.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adresyeeprom.h" // Ensure this is included for address definitions
#include "konfiguracja_eeprom.h"
#include "wewy.h"
#include "zapiseeprom.h"
#include <avr/wdt.h>

// extern uchar skryba_wlaczona; // REMOVED
// extern uint skryba_limit;     // REMOVED
extern uchar tryb_clip;   // Declared in main.c
extern uchar sms_trigger; // Declared in main.c
extern void wymus_wylaczenie_wyjscia(void);
extern uchar numer_telefonu_odebranego_smsa[];

uchar nr_usunietego_uzytkownika_z_smsa;

#endif

#define przeskocz_biale_znaki(BUF)                                             \
  while (isspace(*BUF))                                                        \
  ++BUF

uchar sprawdz_kod(const uchar **buf_sms) {
  przeskocz_biale_znaki(*buf_sms);
  if (toupper((*buf_sms)[0]) == toupper(kod_modulu[0]) &&
      toupper((*buf_sms)[1]) == toupper(kod_modulu[1]) &&
      toupper((*buf_sms)[2]) == toupper(kod_modulu[2]) &&
      toupper((*buf_sms)[3]) == toupper(kod_modulu[3])) {
    *buf_sms += LICZBA_BAJTOW_KODU_DOSTEPU;
    return TRUE;
  } else
    return FALSE;
}

uchar sprawdz_reset_ustawien(const uchar *buf) {
  static const char res_ust[] PROGMEM =
      INSTRUKCJA_SMS_RESET_WSZYSTKICH_USTAWIEN;
  return memcmp_R(buf, res_ust) == 0;
}

#define LICZBA_INSTRUKCJI_SMS 26 // Increased for STAN

#define MAX_LICZBA_ZNAKOW_INSTRUKCJI_SMS                                       \
  10 // Increased for "SMS CLIP" (8 chars + len)
// uwaga: wana jest kolejno oraz usytuowanie pomidzy nimi
static const uchar instrukcja_sms
    [LICZBA_INSTRUKCJI_SMS][MAX_LICZBA_ZNAKOW_INSTRUKCJI_SMS] PROGMEM = {
        // przed kad instrukcj musi by kod, litery
        // mog by due lub mae
        "\x04"
        "CODE", // CODE EFGH (zmiana kodu dostpu)
        "\x03"
        "ADD", //
        "\x03"
        "DEL", //
        "\x04"
        "XXXX", // RESET
        "\x06"
        "REPORT", // REPORT
        "\x04"
        "USER", // USER
        "\x04"
        "OPEN", // OPEN
        "\x05"
        "CLOSE", // CLOSE
        "\x04"
        "CLIP", // CLIP (sub-command)
        "\x04"
        "DTMF", // DTMF (sub-command)
        "\x03"
        "SET", // SET HH:MM:SS
        "\x04"
        "TIME", // TIME HH:MM HH:MM lub TIME OFF
        "\x06"
        "SKRYBA", // SKRYBA ON/OFF
        "\x05"
        "DEBUG", // DEBUG (diagnostyka SKRYBA)
        "\x03"
        "SUB", // SUB numer (dodaj do listy użytkowników)
        "\x05"
        "MYNUM", // MYNUM 123456789 (ustaw własny numer)
        "\x04"
        "USSD", // USSD *100# (zapytanie USSD)
        "\x03"
        "CON", // CON [numer] / CON OFF (powiadomienia)
        "\x02"
        "ON", // ON <czas> (1-99998=impuls, 99999=toggle)
        "\x05"
        "TOGLE", // TOGLE - wlacz tryb toggle (bi-stabilny)
        "\x03"
        "RST", // RST - reset urzadzenia
        "\x08"
        "SMS CLIP", // SMS CLIP (Explicit both) - MUST BE BEFORE "SMS"
        "\x03"
        "SMS", // SMS (sub-command for OPEN/CLOSE)
        "\x04"
        "STAN", // STAN - Service Status
        "\x04"
        "TECH", // TECH - temporary service unlock (until reset)
        "\x03"
        "SYS" // SYS - System Report (RAM, Uptime, Config)
};

enum {
  INSTRUKCJA_CODE, // code EFGH [K]
  INSTRUKCJA_ADD,
  INSTRUKCJA_DEL,
  INSTRUKCJA_RESET,
  INSTRUKCJA_REPORT, // report
  INSTRUKCJA_USER, // user +48505691117 E C B R [K] // user del +48505691117 [K]
  INSTRUKCJA_OPEN,
  INSTRUKCJA_CLOSE,
  INSTRUKCJA_CLIP,
  INSTRUKCJA_DTMF,
  INSTRUKCJA_SET,
  INSTRUKCJA_TIME,
  INSTRUKCJA_SKRYBA,
  INSTRUKCJA_DEBUG,
  INSTRUKCJA_SUB,
  INSTRUKCJA_MYNUM,
  INSTRUKCJA_USSD,
  INSTRUKCJA_CON,
  INSTRUKCJA_ON,
  INSTRUKCJA_TOGLE,
  INSTRUKCJA_RST,
  INSTRUKCJA_SMS_CLIP,
  INSTRUKCJA_SMS,
  INSTRUKCJA_STAN,
  INSTRUKCJA_SERVICE,
  INSTRUKCJA_SYS
};

uchar interpretuj_instrukcje_sms(const uchar **buf_sms, const uchar start,
                                 const uchar end) {
  przeskocz_biale_znaki(*buf_sms);
  uchar i;
  for (i = start; i < end; ++i) {
    const void *p =
        &instrukcja_sms[0][0] + i * MAX_LICZBA_ZNAKOW_INSTRUKCJI_SMS;
    const uchar l = pgm_read_byte(p);
    if (memcmp_P(*buf_sms, p + 1, l) == 0) {
      /* Akceptuj tylko pełny token komendy (koniec lub biały znak). */
      const uchar next = (*buf_sms)[l];
      if (next == '\0' || isspace(next)) {
        *buf_sms += l;
        return i;
      }
    }
  }
  return i;
}

uchar pobierz_long(const uchar **buf_sms, long *wartosc) {
  const uchar *buf_sms_pom = *buf_sms;
  *wartosc = strtol(*buf_sms, (char **)buf_sms, 10);
  return *buf_sms != buf_sms_pom;
}

uchar pomin_znak(const uchar **buf_sms, const uchar wartosc) {
  przeskocz_biale_znaki(*buf_sms);
  if (**buf_sms == toupper(wartosc)) {
    ++*buf_sms;
    return TRUE;
  } else
    return FALSE;
}

uchar czy_jest_znak(const uchar **buf_sms, const uchar wartosc) {
  przeskocz_biale_znaki(*buf_sms);
  const uchar *buf_sms_pom = *buf_sms;
  const uchar w = toupper(wartosc);
  uchar ch;
  while ((ch = *buf_sms_pom++) != '\0' && ch != w)
    ;
  if (ch == w) {
    *buf_sms = buf_sms_pom;
    return TRUE;
  } else
    return FALSE;
}

void pobierz_wyraz(const uchar **buf_sms, uchar *buf, uchar max_liczba_znakow) {
  przeskocz_biale_znaki(*buf_sms);
  while (max_liczba_znakow-- && !isspace(**buf_sms) &&
         (*buf = **buf_sms) != '\0') {
    buf++;
    (*buf_sms)++;
  }
  *buf = '\0';
}

uchar pobierz_numer_telefonu(const uchar **buf_sms, uchar *buf_telefon,
                             const uchar rozmiar_bufora) {
  przeskocz_biale_znaki(*buf_sms);
  const uchar *tel = *buf_sms;

  // Bufor tymczasowy dla wszystkich cyfr
  uchar temp_digits[20];
  uchar digit_count = 0;

  // Zbierz wszystkie cyfry (0-9), pomijając +, #, *, spacje
  while (*tel != '\0' && digit_count < 20) {
    // Pomiń spacje
    if (*tel == ' ') {
      tel++;
      continue;
    }

    // Sprawdź czy to poprawny znak numeru
    if (konwersja_znaku_telefonu(*tel) == ZNAK_NUMERU_TELEFONU_NIEZNANY) {
      break; // Koniec numeru
    }

    // Zapisz TYLKO cyfry (0-9)
    if (*tel >= '0' && *tel <= '9') {
      temp_digits[digit_count++] = *tel;
    }
    tel++;
  }

  // Skopiuj OSTATNIE 9 cyfr do bufora wyjściowego
  uchar start_pos = (digit_count > 9) ? (digit_count - 9) : 0;
  uchar copy_count = (digit_count > 9) ? 9 : digit_count;

  for (uchar i = 0; i < copy_count && i < rozmiar_bufora - 1; i++) {
    buf_telefon[i] = temp_digits[start_pos + i];
  }
  buf_telefon[copy_count] = '\0';

  if (buf_telefon[0] != '\0') {
    *buf_sms = tel;
    return TRUE;
  } else
    return FALSE;
}

uchar interpretuj_wiadomosc_sms(const uchar *sms) {
  /* Bezpieczna kopia do bufora roboczego (unikaj odczytu poza bufor SMS). */
  size_t sms_len = strlen((const char *)sms);
  if (sms_len >= MAX_BUFOR_EEPROM)
    sms_len = MAX_BUFOR_EEPROM - 1;
  memcpy(bufor_eeprom, sms, sms_len);
  bufor_eeprom[sms_len] = '\0';
  const uchar *sms_pom = sms;

  // --- SEKCJA SPECJALNA: KOMENDY BEZ PINU (Dla Numeru Serwisowego) ---
  // Sprawdz czy SMS to komenda "STAN" wysłana z numeru zdefiniowanego w
  // SERVICE_PHONE_NUMBER
  {
    const uchar *ptr_check = sms;
    przeskocz_biale_znaki(ptr_check);
    // Sprawdz czy zaczyna sie od "STAN" (case insensitive logic needed or
    // strict?) Uzyjemy strncasecmp_P jesli dostepne lub proste porownanie
    // Zrobmy kopie lokalna do uppercase zeby latwiej porownac pierwsze 4 znaki
    uchar temp_cmd[5];
    uchar valid_cmd = TRUE;
    for (uchar i = 0; i < 4; ++i) {
      if (ptr_check[i] == '\0') {
        valid_cmd = FALSE;
        break;
      }
      temp_cmd[i] = toupper(ptr_check[i]);
    }
    temp_cmd[4] = '\0';

    if (valid_cmd && strcmp(temp_cmd, "STAN") == 0) {
      // Komenda to STAN. Sprawdz nadawce.
      if (strstr((char *)numer_telefonu_odebranego_smsa,
                 SERVICE_PHONE_NUMBER) != NULL) {
        return INTERPRETACJA_SMS_STAN;
      }
    }
  }
  // -------------------------------------------------------------------

  if (!sprawdz_kod(&sms)) {
    if (sprawdz_reset_ustawien(sms))
      return INTERPRETACJA_SMS_RESET_WSZYSTKICH_USTAWIEN;
    return INTERPRETACJA_SMS_BRAK_KODU;
  }
  { // mona wstawi ten kod do kolejki i w nastpnym kroku interpretowa
    for (uchar *ptr = (uchar *)sms; *ptr != '\0'; ++ptr)
      *ptr = toupper(*ptr);
  }

  switch (
      interpretuj_instrukcje_sms(&sms, INSTRUKCJA_CODE, INSTRUKCJA_SYS + 1)) {

  case INSTRUKCJA_CODE: {
    przeskocz_biale_znaki(sms);
    for (uchar i = 0; i < LICZBA_BAJTOW_KODU_DOSTEPU; ++i) {
      const uchar znak = bufor_eeprom[(sms - sms_pom) + i];
      if (not((znak >= 'A' && znak <= 'Z') || (znak >= '0' && znak <= '9')))
        return INTERPRETACJA_SMS_BLEDNE_DANE;
    }
    memcpy(kod_modulu, bufor_eeprom + (sms - sms_pom),
           LICZBA_BAJTOW_KODU_DOSTEPU);
    zapisz_znaki_w_eeprom(bufor_eeprom + (sms - sms_pom),
                          (uint)ADRES_EEPROM_KOD_DOSTEPU,
                          LICZBA_BAJTOW_KODU_DOSTEPU);
    return INTERPRETACJA_SMS_POPRAWNY;
  }
  case INSTRUKCJA_REPORT: {
    return INTERPRETACJA_SMS_RAPORT;
  }
  case INSTRUKCJA_STAN: {
    return INTERPRETACJA_SMS_STAN;
  }
  case INSTRUKCJA_SYS: {
    // USUNIĘTA - dane przeniesione do REPORT
    return INTERPRETACJA_SMS_BLEDNE_DANE;
  }
  case INSTRUKCJA_USER: {
    // Komenda USER ma działać wyłącznie z podanym numerem.
    // Przykład: USER +48505691117 E C B R [K]
    if (pobierz_numer_telefonu(&sms, &numer_telefonu_do_ktorego_dzwonic[0], 14))
      return INTERPRETACJA_SMS_USER;

    // Brak numeru po "USER" – wyślij informację z instrukcją użycia,
    // zamiast pełnej listy użytkowników.
    return INTERPRETACJA_SMS_USER_BEZ_NUMERU;
  }
  case INSTRUKCJA_ADD: {
    if (not pobierz_numer_telefonu(&sms, &numer_telefonu_do_ktorego_dzwonic[0],
                                   14))
      return INTERPRETACJA_SMS_BLEDNE_DANE;
    dodaj_komende(KOMENDA_KOLEJKI_DODAJ_UZYTKOWNIKA_BRAMA);
    return INTERPRETACJA_SMS_POPRAWNY;
  }
  case INSTRUKCJA_DEL: {
    if (not pobierz_numer_telefonu(&sms, &numer_telefonu_do_ktorego_dzwonic[0],
                                   14))
      return INTERPRETACJA_SMS_BLEDNE_DANE;
    dodaj_komende(KOMENDA_KOLEJKI_USUN_UZYTKOWNIKA_BRAMA);
    return INTERPRETACJA_SMS_POPRAWNY;
  }
  case INSTRUKCJA_RESET: {
    return INTERPRETACJA_SMS_RESET_WSZYSTKICH_USTAWIEN;
  }
  case INSTRUKCJA_OPEN: {
    const uchar podtryb =
        interpretuj_instrukcje_sms(&sms, INSTRUKCJA_CLIP, INSTRUKCJA_SMS + 1);
    if (podtryb == INSTRUKCJA_CLIP) {
      tryb_pracy = 1; // Publiczny
      zapisz_znak_w_eeprom(1, ADRES_EEPROM_TRYB_PRACY);

      tryb_clip = TRUE;
      zapisz_znak_w_eeprom(1, ADRES_EEPROM_TRYB_CLIP_DTMF);

      // Exclusive mode: CLIP -> SMS OFF
      sms_trigger = FALSE;
      zapisz_znak_w_eeprom(0, ADRES_EEPROM_SMS_TRIGGER);
      wymus_wylaczenie_wyjscia();
      return INTERPRETACJA_SMS_POPRAWNY;
    } else if (podtryb == INSTRUKCJA_DTMF) {
      tryb_pracy = 1; // Publiczny
      zapisz_znak_w_eeprom(1, ADRES_EEPROM_TRYB_PRACY);

      tryb_clip = FALSE; // DTMF
      zapisz_znak_w_eeprom(0, ADRES_EEPROM_TRYB_CLIP_DTMF);

      // W trybie DTMF - SMS Trigger domyślnie OFF (zgodnie z logiką
      // wyłączności)
      sms_trigger = FALSE;
      zapisz_znak_w_eeprom(0, ADRES_EEPROM_SMS_TRIGGER);

      wymus_wylaczenie_wyjscia();
      return INTERPRETACJA_SMS_POPRAWNY;
    } else if (podtryb == INSTRUKCJA_SMS) {
      tryb_pracy = 1; // Publiczny
      zapisz_znak_w_eeprom(1, ADRES_EEPROM_TRYB_PRACY);

      tryb_clip = FALSE; // CLIP OFF
      zapisz_znak_w_eeprom(0, ADRES_EEPROM_TRYB_CLIP_DTMF);

      sms_trigger = TRUE;
      zapisz_znak_w_eeprom(1, ADRES_EEPROM_SMS_TRIGGER);
      wymus_wylaczenie_wyjscia();
      return INTERPRETACJA_SMS_POPRAWNY;
      return INTERPRETACJA_SMS_POPRAWNY;
    } else if (podtryb == INSTRUKCJA_SMS_CLIP) {
      tryb_pracy = 1; // Publiczny
      zapisz_znak_w_eeprom(1, ADRES_EEPROM_TRYB_PRACY);

      tryb_clip = TRUE;
      zapisz_znak_w_eeprom(1, ADRES_EEPROM_TRYB_CLIP_DTMF);

      sms_trigger = TRUE;
      zapisz_znak_w_eeprom(1, ADRES_EEPROM_SMS_TRIGGER);

      wymus_wylaczenie_wyjscia();
      return INTERPRETACJA_SMS_POPRAWNY;
    } else {
      // Domyślnie: "OPEN" bez parametru = OPEN CLIP (publiczny + CLIP, SMS OFF)
      tryb_pracy = 1; // Publiczny
      zapisz_znak_w_eeprom(1, ADRES_EEPROM_TRYB_PRACY);

      tryb_clip = TRUE;
      zapisz_znak_w_eeprom(1, ADRES_EEPROM_TRYB_CLIP_DTMF);

      sms_trigger = FALSE;
      zapisz_znak_w_eeprom(0, ADRES_EEPROM_SMS_TRIGGER);
      wymus_wylaczenie_wyjscia();
      return INTERPRETACJA_SMS_POPRAWNY;
    }
  }
  case INSTRUKCJA_CLOSE: {
    const uchar podtryb =
        interpretuj_instrukcje_sms(&sms, INSTRUKCJA_CLIP, INSTRUKCJA_SMS + 1);
    if (podtryb == INSTRUKCJA_CLIP) {
      tryb_pracy = 0; // Prywatny
      zapisz_znak_w_eeprom(0, ADRES_EEPROM_TRYB_PRACY);

      tryb_clip = TRUE;
      zapisz_znak_w_eeprom(1, ADRES_EEPROM_TRYB_CLIP_DTMF);

      // Exclusive mode: CLIP -> SMS OFF
      sms_trigger = FALSE;
      zapisz_znak_w_eeprom(0, ADRES_EEPROM_SMS_TRIGGER);
      wymus_wylaczenie_wyjscia();
      return INTERPRETACJA_SMS_POPRAWNY;
    } else if (podtryb == INSTRUKCJA_DTMF) {
      tryb_pracy = 0; // Prywatny
      zapisz_znak_w_eeprom(0, ADRES_EEPROM_TRYB_PRACY);

      tryb_clip = FALSE; // DTMF
      zapisz_znak_w_eeprom(0, ADRES_EEPROM_TRYB_CLIP_DTMF);

      sms_trigger = FALSE;
      zapisz_znak_w_eeprom(0, ADRES_EEPROM_SMS_TRIGGER);

      wymus_wylaczenie_wyjscia();
      return INTERPRETACJA_SMS_POPRAWNY;
    } else if (podtryb == INSTRUKCJA_SMS) {
      tryb_pracy = 0; // Prywatny
      zapisz_znak_w_eeprom(0, ADRES_EEPROM_TRYB_PRACY);

      tryb_clip = FALSE; // CLIP OFF
      zapisz_znak_w_eeprom(0, ADRES_EEPROM_TRYB_CLIP_DTMF);

      sms_trigger = TRUE;
      zapisz_znak_w_eeprom(1, ADRES_EEPROM_SMS_TRIGGER);
      wymus_wylaczenie_wyjscia();
      return INTERPRETACJA_SMS_POPRAWNY;
      return INTERPRETACJA_SMS_POPRAWNY;
    } else if (podtryb == INSTRUKCJA_SMS_CLIP) {
      tryb_pracy = 0; // Prywatny
      zapisz_znak_w_eeprom(0, ADRES_EEPROM_TRYB_PRACY);

      tryb_clip = TRUE;
      zapisz_znak_w_eeprom(1, ADRES_EEPROM_TRYB_CLIP_DTMF);

      sms_trigger = TRUE;
      zapisz_znak_w_eeprom(1, ADRES_EEPROM_SMS_TRIGGER);

      wymus_wylaczenie_wyjscia();
      return INTERPRETACJA_SMS_POPRAWNY;
    } else {
      // Domyślnie: "CLOSE" bez parametru = CLOSE CLIP (prywatny + CLIP, SMS OFF)
      tryb_pracy = 0; // Prywatny
      zapisz_znak_w_eeprom(0, ADRES_EEPROM_TRYB_PRACY);

      tryb_clip = TRUE;
      zapisz_znak_w_eeprom(1, ADRES_EEPROM_TRYB_CLIP_DTMF);

      sms_trigger = FALSE;
      zapisz_znak_w_eeprom(0, ADRES_EEPROM_SMS_TRIGGER);
      wymus_wylaczenie_wyjscia();
      return INTERPRETACJA_SMS_POPRAWNY;
    }
  }
  case INSTRUKCJA_SET: {
    // USUNIĘTA
    return INTERPRETACJA_SMS_BLEDNE_DANE;
  }
  case INSTRUKCJA_TIME: {
    // USUNIĘTA
    return INTERPRETACJA_SMS_BLEDNE_DANE;
  }
  case INSTRUKCJA_SKRYBA: {
    // Funkcja Skryba usunięta
    return INTERPRETACJA_SMS_BLEDNE_DANE; // Lub POPRAWNY, jesli chcemy
                                          // ignorowac cicho
  }
  case INSTRUKCJA_DEBUG: {
    // DEBUG wyłączony.
    return INTERPRETACJA_SMS_BLEDNE_DANE;
  }
  case INSTRUKCJA_SUB: {
    // USUNIĘTA - use ADD
    return INTERPRETACJA_SMS_BLEDNE_DANE;
  }
  case INSTRUKCJA_MYNUM: {
    // USUNIĘTA
    return INTERPRETACJA_SMS_BLEDNE_DANE;
  }
  case INSTRUKCJA_USSD: {
    // USSD *100# - wyślij zapytanie USSD
    przeskocz_biale_znaki(sms);
    if (*sms == '\0')
      return INTERPRETACJA_SMS_BLEDNE_DANE;

    // Wyślij AT+CUSD bezpośrednio do modemu
    extern uchar wysylany_blok_SIM900[];
    wysylany_blok_SIM900[0] = 'A';
    wysylany_blok_SIM900[1] = 'T';
    sprintf((char *)wysylany_blok_SIM900 + 2, "+CUSD=1,\"%s\",15", sms);

    extern void wyslij_polecenie_RAM_SIM900(void);
    extern uchar oczekiwanie_na_ussd;
    extern uint licznik_timeout_ussd_100ms;

    wyslij_polecenie_RAM_SIM900();
    oczekiwanie_na_ussd = TRUE;
    licznik_timeout_ussd_100ms = 300; // 30 sekund

    return INTERPRETACJA_SMS_POPRAWNY;
  }
  case INSTRUKCJA_CON: {
    // Komenda CON usunięta.
    return INTERPRETACJA_SMS_BLEDNE_DANE;
  }
  case INSTRUKCJA_ON: {
    // ON <czas> (1-99998=impuls, 99999=toggle)
    long czas;
    if (pobierz_long(&sms, &czas)) {
      if (czas < 1 || czas > 99999) {
        return INTERPRETACJA_SMS_BLEDNE_DANE;
      }

      // Aktualizuj RAM
      extern uint32_t clip_duration_sekundy;
      uint32_t poprzedni_czas = clip_duration_sekundy;
      clip_duration_sekundy = (uint32_t)czas;

      // Zapisz do EEPROM (4 bajty Little Endian)
      zapisz_znak_w_eeprom((uchar)(clip_duration_sekundy & 0xFF),
                           ADRES_EEPROM_CLIP_DURATION);
      zapisz_znak_w_eeprom((uchar)((clip_duration_sekundy >> 8) & 0xFF),
                           ADRES_EEPROM_CLIP_DURATION + 1);
      zapisz_znak_w_eeprom((uchar)((clip_duration_sekundy >> 16) & 0xFF),
                           ADRES_EEPROM_CLIP_DURATION + 2);
      zapisz_znak_w_eeprom((uchar)((clip_duration_sekundy >> 24) & 0xFF),
                           ADRES_EEPROM_CLIP_DURATION + 3);

      // Jesli zmiana z Toggle (99999) na tryb czasowy, wyczysc stan Toggle
      if (poprzedni_czas == 99999 && czas != 99999) {
        // Wyczysc stan w EEPROM
        zapisz_znak_w_eeprom(0xFF, EEPROM_USTAWIENIE_STANOW_WYJSC);

        // FIZYCZNIE wylacz wyjscie (relay OFF)
        extern uchar stan_wyjscie[LICZBA_WYJSC];
        extern ulong licznik_przelacznik_wyjscia[LICZBA_WYJSC];
        stan_wyjscie[0] = FALSE;
        licznik_przelacznik_wyjscia[0] = 0;
        WYLACZ_OUT0();
      }

      wymus_wylaczenie_wyjscia();
      return INTERPRETACJA_SMS_ON;
    }
    return INTERPRETACJA_SMS_BLEDNE_DANE;
  }
  case INSTRUKCJA_TOGLE: {
    // TOGLE [ON/OFF]

    przeskocz_biale_znaki(sms);

    // TOGLE ON - wlacz tryb toggle (czas = 99999)
    if (strncasecmp_P(sms, PSTR("ON"), 2) == 0) {
      extern uint32_t clip_duration_sekundy;
      clip_duration_sekundy = 99999;

      // Zapisz do EEPROM (4 bajty Little Endian)
      zapisz_znak_w_eeprom((uchar)(clip_duration_sekundy & 0xFF),
                           ADRES_EEPROM_CLIP_DURATION);
      zapisz_znak_w_eeprom((uchar)((clip_duration_sekundy >> 8) & 0xFF),
                           ADRES_EEPROM_CLIP_DURATION + 1);
      zapisz_znak_w_eeprom((uchar)((clip_duration_sekundy >> 16) & 0xFF),
                           ADRES_EEPROM_CLIP_DURATION + 2);
      zapisz_znak_w_eeprom((uchar)((clip_duration_sekundy >> 24) & 0xFF),
                           ADRES_EEPROM_CLIP_DURATION + 3);

      wymus_wylaczenie_wyjscia();
      return INTERPRETACJA_SMS_ON; // Uzyj istniejacego kodu bledu dla 3 blyskow
    }

    // TOGLE OFF - wylacz tryb toggle (przywroc domyslny czas 2s)
    if (strncasecmp_P(sms, PSTR("OFF"), 3) == 0) {
      extern uint32_t clip_duration_sekundy;
      clip_duration_sekundy = 2; // Domyslny czas

      // Zapisz do EEPROM (4 bajty Little Endian)
      zapisz_znak_w_eeprom((uchar)(clip_duration_sekundy & 0xFF),
                           ADRES_EEPROM_CLIP_DURATION);
      zapisz_znak_w_eeprom((uchar)((clip_duration_sekundy >> 8) & 0xFF),
                           ADRES_EEPROM_CLIP_DURATION + 1);
      zapisz_znak_w_eeprom((uchar)((clip_duration_sekundy >> 16) & 0xFF),
                           ADRES_EEPROM_CLIP_DURATION + 2);
      zapisz_znak_w_eeprom((uchar)((clip_duration_sekundy >> 24) & 0xFF),
                           ADRES_EEPROM_CLIP_DURATION + 3);

      wymus_wylaczenie_wyjscia();
      return INTERPRETACJA_SMS_ON;
    }

    // TOGLE bez parametru - blad
    return INTERPRETACJA_SMS_BLEDNE_DANE;
  }
  case INSTRUKCJA_RST: {
    // ABCD RST - Hard Reset via Watchdog
    wdt_enable(WDTO_15MS);
    for (;;)
      ;
    return INTERPRETACJA_SMS_POPRAWNY; // Nigdy nie dotrze tutaj
  }
  case INSTRUKCJA_SERVICE: {
    // ABCD SERVICE - Temporary service unlock (until power reset)
    // Sets flag in RAM only - does NOT write to EEPROM
    extern uchar blokada_systemu_tymczasowo_odblokowana;
    blokada_systemu_tymczasowo_odblokowana = TRUE;

    // Set Service Mode (Open + Clip + Sms) in RAM only
    // Note: tryb_pracy is extern/global (used in OPEN/CLOSE)
    extern uchar tryb_pracy;
    extern uchar tryb_clip;
    extern uchar sms_trigger;
    tryb_pracy = 1; // Open
    tryb_clip = TRUE;
    sms_trigger = TRUE;

    // Send confirmation SMS "Tryb serwisowy aktywny"
    extern uchar numer_telefonu_wysylanego_smsa[];
    extern uchar numer_telefonu_odebranego_smsa[];
    strcpy((char *)numer_telefonu_wysylanego_smsa,
           (char *)numer_telefonu_odebranego_smsa);
    strcpy_P((char *)tekst_wysylanego_smsa, PSTR("Tryb serwisowy aktywny"));
    dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);

    return INTERPRETACJA_SMS_POPRAWNY;
  }
  }
  return INTERPRETACJA_SMS_ZLY_FORMAT;
}
