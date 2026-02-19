
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
