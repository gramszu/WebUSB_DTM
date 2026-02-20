#include "wewy.h"
#include "adresyeeprom.h"
#include "zapiseeprom.h"
#include "eeprom_wrapper.h"

#ifndef TEST_ATMEGA128
#include "pin_ATmega328.h"
#else
#include "pin_ATmega128.h"
#endif

uchar licznik_wejscie[LICZBA_WEJSC];

ulong czas_trwania_impulsu_na_wejsciu[LICZBA_WEJSC];
ulong czas_trwania_impulsu_off_na_wejsciu[LICZBA_WEJSC];
uchar stan_logiczny_na_wejsciu[LICZBA_WEJSC];

ulong czas_trwania_impulsu[LICZBA_WEJSC];
ulong czas_trwania_impulsu_off[LICZBA_WEJSC];

uchar licznik_wstrzymanie_alarmow_po_inicjalizacji_100ms;

uchar poprzedni_stan_logiczny_na_wejsciu[LICZBA_WEJSC];

uchar aktualny_stan_logiczny_na_wejsciu(const uchar nr_wej) {
  const uchar s = stan_wejscia(nr_wej) != 0;
  const uchar par = PARAMETRY_WEJSCIA_TRYB_NORMALNIE_OTWARTY |
                    PARAMETRY_WEJSCIA_WYZWALANIE_MINUSEM |
                    PARAMETRY_WEJSCIA_MASKA_KONTROLOWANIE_WEJSCIA;
  if (!((TRYB_WEJSCIA(par) != PARAMETRY_WEJSCIA_TRYB_NORMALNIE_ZAMKNIETY) ^
        CZY_WYZWALANIE_PLUSEM(par)))
    return s ? STAN_LOGICZNY_NA_WEJSCIU_ON : STAN_LOGICZNY_NA_WEJSCIU_OFF;
  else
    return s ? STAN_LOGICZNY_NA_WEJSCIU_OFF : STAN_LOGICZNY_NA_WEJSCIU_ON;
}

void aktualizuj_stan_wyzwolenia_wejsc_100ms(void) {
  for (uchar nr_wej = 0; nr_wej < LICZBA_WEJSC; ++nr_wej) {
    stan_logiczny_na_wejsciu[nr_wej] &=
        ~STAN_LOGICZNY_NA_WEJSCIU_WYZWOLENIE; // pozostaje stary stan logiczny
    uchar chwilowy_stan_logiczny = aktualny_stan_logiczny_na_wejsciu(nr_wej);
    if (CZY_AKTUALNY_STAN_LOGICZNY_OFF(nr_wej)) {
      if (chwilowy_stan_logiczny == STAN_LOGICZNY_NA_WEJSCIU_ON) {
        if (czas_trwania_impulsu[nr_wej] <
            czas_trwania_impulsu_na_wejsciu[nr_wej]) {
          if (++czas_trwania_impulsu[nr_wej] ==
              czas_trwania_impulsu_na_wejsciu[nr_wej])
            stan_logiczny_na_wejsciu[nr_wej] = STAN_LOGICZNY_NA_WEJSCIU_ON;
        }
      } else {
        czas_trwania_impulsu[nr_wej] = 0; // zaczyna odliczac od nowa, poniewaz
                                          // nie byl to zbyt dlugi impuls
      }
    } else // STAN_LOGICZNY_NA_WEJSCIU_ON
    {
      if (chwilowy_stan_logiczny == STAN_LOGICZNY_NA_WEJSCIU_OFF) {
        if (czas_trwania_impulsu_off[nr_wej] <
            czas_trwania_impulsu_off_na_wejsciu[nr_wej]) {
          if (++czas_trwania_impulsu_off[nr_wej] ==
              czas_trwania_impulsu_off_na_wejsciu[nr_wej])
            stan_logiczny_na_wejsciu[nr_wej] = STAN_LOGICZNY_NA_WEJSCIU_OFF;
        }
      } else {
        czas_trwania_impulsu_off[nr_wej] =
            0; // zaczyna odliczac od nowa, poniewaz nie byl to zbyt dlugi
               // impuls
      }
    }
    if (stan_logiczny_na_wejsciu[nr_wej] !=
        poprzedni_stan_logiczny_na_wejsciu[nr_wej]) {
      poprzedni_stan_logiczny_na_wejsciu[nr_wej] =
          stan_logiczny_na_wejsciu[nr_wej]; // tutaj nie ma stanu wyzwolenia
      stan_logiczny_na_wejsciu[nr_wej] |= STAN_LOGICZNY_NA_WEJSCIU_WYZWOLENIE;
      if (CZY_AKTUALNY_STAN_LOGICZNY_OFF(nr_wej))
        czas_trwania_impulsu[nr_wej] = 0;
      else
        czas_trwania_impulsu_off[nr_wej] = 0;
    }
    if (licznik_wstrzymanie_alarmow_po_inicjalizacji_100ms)
      stan_logiczny_na_wejsciu[nr_wej] &= ~STAN_LOGICZNY_NA_WEJSCIU_WYZWOLENIE;
  }

  if (licznik_wstrzymanie_alarmow_po_inicjalizacji_100ms)
    --licznik_wstrzymanie_alarmow_po_inicjalizacji_100ms;
}

void steruj_wejscia_10ms(void) {
#define TEST_LICZNIKA_WEJSCIA(NR_WEJSCIA)                                      \
  licznik = licznik_wejscie[NR_WEJSCIA - 1];                                   \
  if (!STAN_WEJSCIE_##NR_WEJSCIA())                                            \
    licznik >>= 1;                                                             \
  licznik |= 0x01;                                                             \
  if (STAN_WEJSCIE_##NR_WEJSCIA())                                             \
    licznik <<= 1;                                                             \
  licznik_wejscie[NR_WEJSCIA - 1] = licznik;

  uchar licznik;
  TEST_LICZNIKA_WEJSCIA(1);
}

ulong licznik_przelacznik_wyjscia[LICZBA_WYJSC];

uchar stan_wyjscie[LICZBA_WYJSC];

// Licznik cykli przekaźnika (zdefiniowany w main.c)
extern uint32_t licznik_cykli_przekaznika;
static uchar poprzedni_stan_wyjscia_0 = 0;

void steruj_wyjscia_100ms(void) {
  for (uchar i = 0; i < LICZBA_WYJSC; ++i) {
    if (licznik_przelacznik_wyjscia[i] != 0) {
      if (--licznik_przelacznik_wyjscia[i] == 0)
        stan_wyjscie[i] = not stan_wyjscie[i];
    }
  }

  // Cycle counting logic (Rising Edge)
  if (stan_wyjscie[0] && !poprzedni_stan_wyjscia_0) {
    if (licznik_cykli_przekaznika < 4294967295UL) {
      licznik_cykli_przekaznika++;
      // Save to EEPROM every 10 cycles to reduce wear
      if ((licznik_cykli_przekaznika % 10) == 0) {
        zapisz_znak_w_eeprom((uchar)(licznik_cykli_przekaznika & 0xFF),
                             ADRES_EEPROM_LICZNIK_CYKLI);
        zapisz_znak_w_eeprom((uchar)((licznik_cykli_przekaznika >> 8) & 0xFF),
                             ADRES_EEPROM_LICZNIK_CYKLI + 1);
        zapisz_znak_w_eeprom((uchar)((licznik_cykli_przekaznika >> 16) & 0xFF),
                             ADRES_EEPROM_LICZNIK_CYKLI + 2);
        zapisz_znak_w_eeprom((uchar)((licznik_cykli_przekaznika >> 24) & 0xFF),
                             ADRES_EEPROM_LICZNIK_CYKLI + 3);
      }
    }
  }
  poprzedni_stan_wyjscia_0 = stan_wyjscie[0];

  // Blokada serwisowa po przekroczeniu 500,000 cykli
  if (stan_wyjscie[0]) {
    if (licznik_cykli_przekaznika >= 500000) {
      WYLACZ_OUT0(); // Service Lock
    } else {
      WLACZ_OUT0();
    }
  } else {
    WYLACZ_OUT0();
  }
}

void kopiuj_parametry_we_wy_z_eeprom(void) {
  czas_trwania_impulsu_na_wejsciu[0] = czas_trwania_impulsu_off_na_wejsciu[0] =
      20;

  // Odczyt licznika cykli
  licznik_cykli_przekaznika =
      eeprom_read_dword((uint32_t *)ADRES_EEPROM_LICZNIK_CYKLI);
  if (licznik_cykli_przekaznika == 0xFFFFFFFF) {
    licznik_cykli_przekaznika = 0;
  }
}

void inicjalizuj_parametry_we_wy(void) {
  for (uchar i = 0; i < LICZBA_WEJSC; ++i)
    poprzedni_stan_logiczny_na_wejsciu[i] = 0;
  for (uchar i = 0; i < LICZBA_WYJSC; ++i)
    licznik_przelacznik_wyjscia[i] = 0;
  licznik_wstrzymanie_alarmow_po_inicjalizacji_100ms = 30;
}
