#ifndef AT24C512_H
#define AT24C512_H

#include "narzedzia.h"
#include <stdint.h>

/* AT24C512 - 512Kbit (64KB) I2C EEPROM
 * Adres 7-bit: 0x50 (A2=A1=A0=0)
 * PC4 = SDA (TWD), PC5 = SCL (TWC) - ATmega328PB TWI
 */

#define AT24C512_SIZE 65536U
#define AT24C512_I2C_ADDR 0x50

/* Inicjalizacja TWI (PC4=SDA, PC5=SCL) */
void at24c512_init(void);

/* Odczyt jednego bajtu z adresu (0-65535) */
uint8_t at24c512_read_byte(uint16_t addr);

/* Zapis jednego bajtu pod adres */
void at24c512_write_byte(uint16_t addr, uint8_t data);

/* Odczyt bloku do buf */
void at24c512_read_block(void *buf, uint16_t addr, uint16_t len);

/* Zapis bloku z buf (każdy bajt osobno - wait po każdym) */
void at24c512_write_block(const void *buf, uint16_t addr, uint16_t len);

/* Zwraca 1 gdy ostatni zapis zakończony (ACK polling) */
uint8_t at24c512_is_ready(void);

#endif /* AT24C512_H */
