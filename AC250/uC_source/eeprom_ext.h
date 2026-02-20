/*
 * Warstwa EEPROM zewnętrznego - kompatybilna z avr/eeprom.h
 * Implementacja dla AT24C512 zamiast wewnętrznego EEPROM
 * NIE includuj <avr/eeprom.h> tam gdzie używamy tej warstwy.
 */
#ifndef EEPROM_EXT_H
#define EEPROM_EXT_H

#include "narzedzia.h"
#include <stddef.h>
#include <stdint.h>
#include "at24c512.h"

#define EEPROM_EXT_SIZE 65536U

/* API zgodne z avr/eeprom.h - nasze implementacje nadpisują libc */
uint8_t eeprom_read_byte(const uint8_t *__p);
void eeprom_write_byte(uint8_t *__p, uint8_t __value);
void eeprom_update_byte(uint8_t *__p, uint8_t __value);
void eeprom_read_block(void *__dst, const void *__src, size_t __n);
void eeprom_update_block(const void *__src, void *__dst, size_t __n);
uint32_t eeprom_read_dword(const uint32_t *__p);
uint8_t eeprom_is_ready(void);

#endif /* EEPROM_EXT_H */
