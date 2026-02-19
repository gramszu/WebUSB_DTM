/*
 * Warstwa EEPROM - wybór wewnętrzny lub zewnętrzny AT24C512
 * Przy USE_EXTERNAL_EEPROM używamy AT24C512 na PC4/PC5
 */
#ifndef EEPROM_WRAPPER_H
#define EEPROM_WRAPPER_H

#ifdef USE_EXTERNAL_EEPROM
#include "eeprom_ext.h"
#else
#include <avr/eeprom.h>
#endif

#endif /* EEPROM_WRAPPER_H */
