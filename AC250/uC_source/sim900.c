#include "sim900.h"
#include "pin_ATmega328.h" // Dodano
#include <avr/interrupt.h>
#include <avr/io.h>
#include <string.h>

#define FCPU_SIM900 F_CPU
#define FABRYCZNA_PREDKOSC_STARTOWA_SIM900 115200
#define VUBRR_SIM900(VUART) (FCPU_SIM900 / (VUART * 16L) - 1)
#define VUBRR_FABRYCZNA_PREDKOSC_STARTOWA_SIM900                               \
  VUBRR_SIM900(FABRYCZNA_PREDKOSC_STARTOWA_SIM900)

uchar odebrany_blok_SIM900[MAX_LICZBA_ODBIERANYCH_ZNAKOW_SIM900 + 1];
volatile uint liczba_odebranych_znakow_SIM900;
volatile uchar licznik_opoznienie_oczekiwania_na_bajt_SIM900;

// SMS timestamp synchronization variables (Date)
uchar sms_timestamp_rok = 0;
uchar sms_timestamp_miesiac = 0;
uchar sms_timestamp_dzien = 0;
uchar sms_timestamp_godzina = 0;
uchar sms_timestamp_minuta = 0;
uchar sms_timestamp_sekunda = 0;

volatile uint nr_wyslanego_znaku_SIM900;
volatile uchar sprawdzaj_wejscie_CTS_SIM900 = FALSE;
volatile uint liczba_wysylanych_znakow_SIM900;

volatile uchar komenda_SIM900 = BRAK_KOMENDY_SIM900;
uchar podlaczony_modul_gsm_SIM900 = FALSE;
uchar podlaczona_karta_SIM_SIM900 = FALSE;
uchar wykonywanie_rozmowy_telefonicznej;

volatile uint liczba_znakow_pdu_do_odebrania;

volatile uint zakonczenie_polecenia_SIM900;
volatile uint poczatek_polecenia_SIM900;
volatile uchar czy_jest_nowe_polecenie_SIM900;
volatile uchar czekanie_na_odebranie_zachety;
volatile uchar flaga_odebrany_znak_zachety;
volatile uint polozenie_otrzymanych_komend_SIM900
    [LICZBA_POLOZEN_OTRZYMANYCH_KOMEND_SIM900];

void wyslij_znaki_SIM900(const uint liczba_znakow) {
  liczba_wysylanych_znakow_SIM900 = liczba_znakow;
  nr_wyslanego_znaku_SIM900 = 0;
  UCSRA_SIM900 |= BIT(TXC);
  WYKONAJ_CLI_SEI(WLACZ_PRZERWANIE_WYSYLANIA_DANYCH_SIM900());
  sprawdzaj_wejscie_CTS_SIM900 = TRUE;
}

void wyslij_polecenie_ROM_SIM900(PGM_P polecenie) {
  wysylany_blok_SIM900[0] = 'A';
  wysylany_blok_SIM900[1] = 'T';
  uchar *w = &wysylany_blok_SIM900[2];
  uchar l = 3;
  while ((*w = pgm_read_byte(polecenie)))
    ++w, ++polecenie, ++l;
  *w = ZNAK_KONCA_WYSYLANIA_ZNAKOW_CR;
  wyslij_znaki_SIM900(l);
}

void wyslij_polecenie_RAM_SIM900(void) {
  wysylany_blok_SIM900[0] = 'A';
  wysylany_blok_SIM900[1] = 'T';
  const uchar l = strlen(wysylany_blok_SIM900);
  wysylany_blok_SIM900[l] = ZNAK_KONCA_WYSYLANIA_ZNAKOW_CR;
  wyslij_znaki_SIM900(l + 1);
}

ISR(USART_TX_vect_SIM900, ISR_NOBLOCK) {
  if (nr_wyslanego_znaku_SIM900 >= liczba_wysylanych_znakow_SIM900) {
    zakoncz_wysylanie_SIM900();
    return;
  }
  if (CZY_HANDSHAKING_CTS_ZEZWALA_NA_TRANSMISJE_SIM900()) {
    WSTAW_ZNAK_DO_BUFORA_SIM900();
  } else
    sprawdzaj_wejscie_CTS_SIM900 = TRUE;
}

#ifdef WLACZ_PODGLAD_TRANSMISJI_ODBIORU_SIMCOM_NA_USB
#warning "GSM: Wlaczony podglad odbioru danych SIM900 po USB"
#endif

#ifdef WLACZ_PODGLAD_TRANSMISJI_WYSYLANIA_SIMCOM_NA_USB
#warning "GSM: Wlaczony podglad transmisji danych SIM900 po USB"
#endif

ISR(USART_RX_vect_SIM900, ISR_NAKED) {
  volatile register uchar tmp asm("r24");
  register uchar znak_UDR asm("r20");
  register uint l asm("r18");
  register uchar *ptr asm("r30");
  PUSH(tmp);
  tmp = SREG;
  PUSH(tmp);
  WYLACZ_PRZERWANIE_ODBIORU_DANYCH_SIM900();
  sei();
#ifdef RAMPZ
  tmp = RAMPZ;
  PUSH(tmp);
#endif
  asm volatile("lds %0, " ADRES_UDR_SIM900 " " : "=r"(tmp) :);
#ifdef WLACZ_PODGLAD_TRANSMISJI_ODBIORU_SIMCOM_NA_USB
  asm volatile("sts " ADRES_UDR_USB ", %0 " : "=r"(tmp) :);
#endif
  PUSH_TXT("r0");
  PUSH(znak_UDR);
  znak_UDR = tmp;
  PUSH_TXT("__zero_reg__");
  CLEAR_TXT("__zero_reg__");
  PUSH_TXT("r18");
  PUSH_TXT("r19");
  PUSH_TXT("r30");
  PUSH_TXT("r31");
  PUSH_TXT("r26");
  PUSH_TXT("r27");
  PUSH_TXT("r25");
  l = liczba_odebranych_znakow_SIM900;
  if (tmp != 0xFF) {
    ptr = odebrany_blok_SIM900;
    if (l < MAX_LICZBA_ODBIERANYCH_ZNAKOW_SIM900) {
      if (l != 0) {
        tmp = *ptr;
        if (znak_UDR == ZNAK_KONCA_WYSYLANIA_ZNAKOW_CR &&
            tmp != ZNAK_KONCA_WYSYLANIA_ZNAKOW_CR &&
            tmp != ZNAK_KONCA_ODBIERANIA_ZNAKOW_LF) {
          if (!czy_jest_nowe_polecenie_SIM900) {
            komenda_SIM900 = KOMENDA_SIM900_WYSLANE_POLECENIE;
            zakoncz_odbieranie_SIM900();
            poczatek_polecenia_SIM900 = 0;
            zakonczenie_polecenia_SIM900 = 0;
            czy_jest_nowe_polecenie_SIM900 = FALSE;
          }
        }
      }
      if ((l == 0 || ptr[0] != ZNAK_KONCA_WYSYLANIA_ZNAKOW_CR) &&
          znak_UDR == ZNAK_KONCA_ODBIERANIA_ZNAKOW_LF) {
        if (!czy_jest_nowe_polecenie_SIM900) {
          komenda_SIM900 = KOMENDA_SIM900_WYSLANE_POLECENIE;
          zakoncz_odbieranie_SIM900();
          poczatek_polecenia_SIM900 = 0;
          zakonczenie_polecenia_SIM900 = 0;
          czy_jest_nowe_polecenie_SIM900 = FALSE;
        }
      }
      if (CZY_ODBIERANIE_DANYCH_SIM900() && l > 3 &&
          znak_UDR == ZNAK_KONCA_ODBIERANIA_ZNAKOW_LF &&
          ptr[l - 1] == ZNAK_KONCA_WYSYLANIA_ZNAKOW_CR &&
          ptr[l - 2] != ZNAK_KONCA_ODBIERANIA_ZNAKOW_LF &&
          ptr[l - 3] != ZNAK_KONCA_WYSYLANIA_ZNAKOW_CR &&
          ptr[1] == ZNAK_KONCA_ODBIERANIA_ZNAKOW_LF &&
          ptr[0] == ZNAK_KONCA_WYSYLANIA_ZNAKOW_CR) {
        poczatek_polecenia_SIM900 = zakonczenie_polecenia_SIM900;
        zakonczenie_polecenia_SIM900 = l + 1;
        ++czy_jest_nowe_polecenie_SIM900;
        polozenie_otrzymanych_komend_SIM900[4] =
            polozenie_otrzymanych_komend_SIM900[3];
        polozenie_otrzymanych_komend_SIM900[3] =
            polozenie_otrzymanych_komend_SIM900[2];
        polozenie_otrzymanych_komend_SIM900[2] =
            polozenie_otrzymanych_komend_SIM900[1];
        polozenie_otrzymanych_komend_SIM900[1] = poczatek_polecenia_SIM900;
        polozenie_otrzymanych_komend_SIM900[0] = zakonczenie_polecenia_SIM900;
      }
      if (czekanie_na_odebranie_zachety != FALSE && l == 3 && znak_UDR == ' ' &&
          odebrany_blok_SIM900[2] == '>') {
        flaga_odebrany_znak_zachety = TRUE;
      }
      ptr[l] = znak_UDR;
      l = l + 1;
      ptr[l] = '\0';
    } else {
      l = 0;
      komenda_SIM900 = BLAD_SIM900;
      poczatek_polecenia_SIM900 = 0;
      zakonczenie_polecenia_SIM900 = 0;
      czy_jest_nowe_polecenie_SIM900 = FALSE;
    }
    liczba_odebranych_znakow_SIM900 = l;
  }
  licznik_opoznienie_oczekiwania_na_bajt_SIM900 = 0;
  POP_TXT("r25");
  POP_TXT("r27");
  POP_TXT("r26");
  POP_TXT("r31");
  POP_TXT("r30");
  POP_TXT("r19");
  POP_TXT("r18");
  POP_TXT("__zero_reg__");
  POP(znak_UDR);
  if (IS_HI(ACSR_SIM900, ACIS1)) {
    cli();
    LDS(tmp, ADRES_UCSRB_SIM900);
    ORI(tmp, BIT(RXCIE));
    STS(tmp, ADRES_UCSRB_SIM900);
    sei();
    NOP();
  }
  POP_TXT("r0");
#ifdef RAMPZ
  POP(tmp);
  RAMPZ = tmp;
#endif
  POP(tmp);
  SREG = tmp;
  POP(tmp);
  reti();
}

static void ustaw_piny_SIM900(void) {
  SET_HI(DDR_UART_SIM900, TXD_WYJSCIE_SIM900);
  SET_HI2(PORT_UART_SIM900, TXD_WYJSCIE_SIM900, RXD_WEJSCIE_SIM900);
  SET_HI(DDR_RTS_SIM900, RTS_WYJSCIE_SIM900);
  SET_HI(PORT_RTS_SIM900, RTS_WYJSCIE_SIM900);
  SET_HI(PORT_CTS_SIM900, CTS_WEJSCIE_SIM900);
}

static void wylacz_piny_SIM900(void) {
  SET_LO2(PORT_UART_SIM900, TXD_WYJSCIE_SIM900, RXD_WEJSCIE_SIM900);
  SET_LO(DDR_UART_SIM900, TXD_WYJSCIE_SIM900);
  SET_LO(PORT_CTS_SIM900, CTS_WEJSCIE_SIM900);
  SET_LO(PORT_RTS_SIM900, RTS_WYJSCIE_SIM900);
  SET_LO(DDR_RTS_SIM900, RTS_WYJSCIE_SIM900);
}

void inicjalizacja_SIM900(void) {
  UBRRL_SIM900 = VUBRR_FABRYCZNA_PREDKOSC_STARTOWA_SIM900 % 256;
  UBRRH_SIM900 = VUBRR_FABRYCZNA_PREDKOSC_STARTOWA_SIM900 / 256;
  UCSRB_SIM900 |= BIT(TXEN) | BIT(RXEN);
  wylacz_piny_SIM900();
}

uchar licznik_100ms_procedura_inicjalizacyjna_SIM900 = 1;

uchar zakonczenie_procedury_inicjalizacyjnej(void) {
  if (STATUS_WLACZONY_SIM900()) {
    licznik_100ms_procedura_inicjalizacyjna_SIM900 = 0;
    liczba_odebranych_znakow_SIM900 = 0;
    ustaw_piny_SIM900();
    return TRUE;
  }
  return FALSE;
}

uchar procedura_inicjalizacyjna_SIM900_100MS(void) {
  if (licznik_100ms_procedura_inicjalizacyjna_SIM900) {
    ++licznik_100ms_procedura_inicjalizacyjna_SIM900;
    switch (licznik_100ms_procedura_inicjalizacyjna_SIM900) {
    case 2: {
      WYLACZ_PWRKEY_SIM900();
      if (zakonczenie_procedury_inicjalizacyjnej())
        return TRUE;
    }
    case 3:
      WLACZ_PWRKEY_SIM900();
      break;
    case 15:
      WYLACZ_PWRKEY_SIM900();
      break;
    case 40:
    case 41:
    case 42:
    case 43: {
      if (zakonczenie_procedury_inicjalizacyjnej())
        return TRUE;
      break;
    }
    case 161: {
      podlaczony_modul_gsm_SIM900 = FALSE;
      podlaczona_karta_SIM_SIM900 = FALSE;
      break;
    }
    case 163:
      break;
    case 200:
      WLACZ_PWRKEY_SIM900();
      break;
    case 215:
      WYLACZ_PWRKEY_SIM900();
      break;
    case 245:
    case 246:
    case 247:
    case 248:
    case 249:
    case 250:
    case 251:
    case 252:
    case 253:
    case 254: {
      if (zakonczenie_procedury_inicjalizacyjnej())
        return TRUE;
      break;
    }
    case 255:
      WYKONAJ_PROCEDURE_INICJALIZACYJNA_SIM900();
      break;
    }
  }
  return FALSE;
}