/*
 * Implementacja API avr/eeprom.h dla zewnętrznego AT24C512
 * Te symbole nadpiszą wersje z libc przy linkowaniu
 */

#include "eeprom_ext.h"
#include <stddef.h>

uint8_t eeprom_read_byte(const uint8_t *__p) {
  uint16_t addr = (uint16_t)(uintptr_t)__p;
  if (addr >= AT24C512_SIZE)
    return 0xFF;
  return at24c512_read_byte(addr);
}

void eeprom_write_byte(uint8_t *__p, uint8_t __value) {
  uint16_t addr = (uint16_t)(uintptr_t)__p;
  if (addr < AT24C512_SIZE) {
    at24c512_write_byte(addr, __value);
    while (!at24c512_is_ready())
      ; /* cykl zapisu AT24C512 ~5 ms – bez tego następny bajt może się zepsuć */
  }
}

void eeprom_update_byte(uint8_t *__p, uint8_t __value) {
  uint8_t old = eeprom_read_byte(__p);
  if (old != __value)
    eeprom_write_byte(__p, __value);
}

void eeprom_read_block(void *__dst, const void *__src, size_t __n) {
  uint16_t addr = (uint16_t)(uintptr_t)__src;
  if (addr >= AT24C512_SIZE || __n == 0)
    return;
  if (addr + __n > AT24C512_SIZE)
    __n = AT24C512_SIZE - addr;
  at24c512_read_block(__dst, addr, (uint16_t)__n);
}

void eeprom_update_block(const void *__src, void *__dst, size_t __n) {
  uint16_t addr = (uint16_t)(uintptr_t)__dst;
  if (addr >= AT24C512_SIZE || __n == 0)
    return;
  if (addr + __n > AT24C512_SIZE)
    __n = AT24C512_SIZE - addr;
  at24c512_write_block(__src, addr, (uint16_t)__n);
  while (!at24c512_is_ready())
    ;
}

uint32_t eeprom_read_dword(const uint32_t *__p) {
  uint16_t addr = (uint16_t)(uintptr_t)__p;
  uint32_t v;
  uint8_t *vp = (uint8_t *)&v;
  if (addr + 4 > AT24C512_SIZE)
    return 0xFFFFFFFFUL;
  vp[0] = at24c512_read_byte(addr);
  vp[1] = at24c512_read_byte(addr + 1);
  vp[2] = at24c512_read_byte(addr + 2);
  vp[3] = at24c512_read_byte(addr + 3);
  return v;
}

uint8_t eeprom_is_ready(void) {
  return at24c512_is_ready();
}
