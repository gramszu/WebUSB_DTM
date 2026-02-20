/*
 * AT24C512 I2C EEPROM driver dla ATmega328PB
 * Piny: PC4 = SDA (TWD), PC5 = SCL (TWC)
 * TWI na tych pinach jest domyślny w 32-pin package
 */

#include "at24c512.h"
#include <avr/io.h>
#include <util/delay.h>

#define F_SCL 400000UL  /* 400 kHz Fast Mode – AT24C512 to obsługuje, szybszy odczyt/zapis */
#define TWI_BAUD(FREQ, FSCL) (((FREQ) / (FSCL)) - 16) / 2

/* ATmega328PB ma TWI0 (TWCR0, TWSR0, TWDR0, TWBR0) */
#define TWI_CR  TWCR0
#define TWI_SR  TWSR0
#define TWI_DR  TWDR0
#define TWI_BR  TWBR0

/* Ochrona przed zawieszeniem na uszkodzonej magistrali I2C. */
#define TWI_OP_TIMEOUT_LOOPS 60000U
#define EEPROM_READY_TIMEOUT_LOOPS 20000U

static uint8_t twi_wait_int(void) {
  uint16_t timeout = TWI_OP_TIMEOUT_LOOPS;
  while (!(TWI_CR & (1 << TWINT)) && --timeout)
    ;
  return (timeout != 0);
}

static uint8_t twi_start(void) {
  TWI_CR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
  if (!twi_wait_int())
    return 0xFF;
  return TWI_SR & 0xF8;
}

static void twi_stop(void) {
  TWI_CR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
  uint16_t timeout = TWI_OP_TIMEOUT_LOOPS;
  while ((TWI_CR & (1 << TWSTO)) && --timeout)
    ;
}

static uint8_t twi_send_byte(uint8_t data) {
  TWI_DR = data;
  TWI_CR = (1 << TWINT) | (1 << TWEN);
  if (!twi_wait_int())
    return 0xFF;
  return TWI_SR & 0xF8;
}

static uint8_t twi_recv_byte_ack(void) {
  TWI_CR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN);
  if (!twi_wait_int())
    return 0xFF;
  return TWI_DR;
}

static uint8_t twi_recv_byte_nack(void) {
  TWI_CR = (1 << TWINT) | (1 << TWEN);
  if (!twi_wait_int())
    return 0xFF;
  return TWI_DR;
}

void at24c512_init(void) {
  /* PC4 (SDA), PC5 (SCL) - pull-up włączone przez PORT */
  PORTC |= (1 << PC4) | (1 << PC5);
  DDRC &= ~((1 << PC4) | (1 << PC5));

  TWI_BR = (uint8_t)TWI_BAUD(F_CPU, F_SCL);
  TWI_SR = 0; /* prescaler 1 */
}

uint8_t at24c512_read_byte(uint16_t addr) {
  uint8_t data;
  if (twi_start() != 0x08)
    return 0xFF;
  if (twi_send_byte((AT24C512_I2C_ADDR << 1) | 0) != 0x18)
    goto fail;
  twi_send_byte((uint8_t)(addr >> 8));
  twi_send_byte((uint8_t)(addr & 0xFF));
  if (twi_start() != 0x10)
    goto fail;
  if (twi_send_byte((AT24C512_I2C_ADDR << 1) | 1) != 0x40)
    goto fail;
  data = twi_recv_byte_nack();
  twi_stop();
  return data;
fail:
  twi_stop();
  return 0xFF;
}

void at24c512_read_block(void *buf, uint16_t addr, uint16_t len) {
  uint8_t *p = (uint8_t *)buf;
  if (len == 0)
    return;
  if (twi_start() != 0x08)
    goto fail;
  if (twi_send_byte((AT24C512_I2C_ADDR << 1) | 0) != 0x18)
    goto fail;
  twi_send_byte((uint8_t)(addr >> 8));
  twi_send_byte((uint8_t)(addr & 0xFF));
  if (twi_start() != 0x10)
    goto fail;
  if (twi_send_byte((AT24C512_I2C_ADDR << 1) | 1) != 0x40)
    goto fail;
  while (len-- > 1) {
    *p++ = twi_recv_byte_ack();
  }
  *p = twi_recv_byte_nack();
  twi_stop();
  return;
fail:
  twi_stop();
  while (len--)
    *p++ = 0xFF;
}

void at24c512_write_byte(uint16_t addr, uint8_t data) {
  if (twi_start() != 0x08)
    goto fail;
  if (twi_send_byte((AT24C512_I2C_ADDR << 1) | 0) != 0x18)
    goto fail;
  twi_send_byte((uint8_t)(addr >> 8));
  twi_send_byte((uint8_t)(addr & 0xFF));
  twi_send_byte(data);
  twi_stop();
  return;
fail:
  twi_stop();
}

void at24c512_write_block(const void *buf, uint16_t addr, uint16_t len) {
  const uint8_t *p = (const uint8_t *)buf;
  while (len-- > 0) {
    at24c512_write_byte(addr++, *p++);
    uint16_t timeout = EEPROM_READY_TIMEOUT_LOOPS;
    while (!at24c512_is_ready() && --timeout)
      ;
    if (timeout == 0)
      break;
  }
}

uint8_t at24c512_is_ready(void) {
  if (twi_start() != 0x08)
    return 0;
  if (twi_send_byte((AT24C512_I2C_ADDR << 1) | 0) == 0x18) {
    twi_stop();
    return 1;
  }
  twi_stop();
  return 0;
}
