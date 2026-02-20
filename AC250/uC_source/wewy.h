
#include <avr/io.h>
#include "narzedzia.h"
#include "konfiguracja_eeprom.h"

#define LICZBA_WEJSC	1
extern uchar licznik_wejscie[LICZBA_WEJSC];

extern uchar parametry_wejscia[LICZBA_WEJSC];
void kopiuj_parametry_we_wy_z_eeprom(void);

extern ulong czas_trwania_impulsu_na_wejsciu[LICZBA_WEJSC];
extern ulong czas_trwania_impulsu_off_na_wejsciu[LICZBA_WEJSC];

#define STAN_LOGICZNY_NA_WEJSCIU_WYZWOLENIE	0x02
#define STAN_LOGICZNY_NA_WEJSCIU_ON					0x01
#define STAN_LOGICZNY_NA_WEJSCIU_OFF				0x00
#define CZY_WYZWOLENIE_NA_WEJSCIU(NR_WEJ)	(stan_logiczny_na_wejsciu[NR_WEJ] & STAN_LOGICZNY_NA_WEJSCIU_WYZWOLENIE)
#define CZY_AKTUALNY_STAN_LOGICZNY_ON(NR_WEJ)	(stan_logiczny_na_wejsciu[NR_WEJ] & STAN_LOGICZNY_NA_WEJSCIU_ON)
#define CZY_AKTUALNY_STAN_LOGICZNY_OFF(NR_WEJ)	(!CZY_AKTUALNY_STAN_LOGICZNY_ON(NR_WEJ))

extern uchar stan_logiczny_na_wejsciu[LICZBA_WEJSC];

void steruj_wejscia_10ms(void);
void aktualizuj_stan_wyzwolenia_wejsc_100ms(void);
void inicjalizuj_parametry_we_wy(void);

static uchar stan_wejscia(const uchar nr_wejscia) __attribute__((unused));

static uchar stan_wejscia(const uchar nr_wejscia)
{
	return licznik_wejscie[nr_wejscia] & 0xf0;
}

uchar aktualny_stan_logiczny_na_wejsciu(const uchar nr_wej);

#define LICZBA_WYJSC	1

uchar stan_wyjscie[LICZBA_WYJSC];

void steruj_wyjscia_100ms(void);

extern ulong licznik_przelacznik_wyjscia[LICZBA_WYJSC];
