#include "uart1_eeprom.h"
#include <avr/io.h>
#include <util/delay.h>

// Inicjalizacja UART1 na pinach PB3 (TXD1) i PB4 (RXD1)
// Prędkość: 115200 baud
void uart1_init(void) {
#define UART_BAUD 115200
#define UBRR_VAL ((F_CPU + 4UL * UART_BAUD) / (8UL * UART_BAUD) - 1)

  UBRR1H = (unsigned char)(UBRR_VAL >> 8);
  UBRR1L = (unsigned char)UBRR_VAL;

  UCSR1A = (1 << U2X1);
  UCSR1B = (1 << RXEN1) | (1 << TXEN1);
  UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);

  DDRB |= (1 << PORTB3);   // TXD1 output
  DDRB &= ~(1 << PORTB4);   // RXD1 input
  PORTB |= (1 << PORTB4);   // Pull-up on RXD1
}

void uart1_putc(unsigned char c) {
  while (!(UCSR1A & (1 << UDRE1)))
    ;
  UDR1 = c;
}

unsigned char uart1_getc(void) {
  while (!(UCSR1A & (1 << RXC1)))
    ;
  return UDR1;
}

unsigned char uart1_data_available(void) {
  return (UCSR1A & (1 << RXC1)) != 0;
}

void uart1_puts(const char *str) {
  while (*str)
    uart1_putc(*str++);
}

extern volatile uint8_t poziom_sieci_gsm;

#ifdef USE_EXTERNAL_EEPROM
/* Protokół zgodny z konfiguratorem: R+addr(2B)->1B, W+addr(2B)+data->OK na końcu */
#define EEPROM_UART_SIZE 65536
#else
#define EEPROM_UART_SIZE 1024
#endif

#if EEPROM_UART_SIZE == 65536
#define EEPROM_UART_SIZE_STR "65536"
#define EEPROM_ADDR_BYTES_STR "2"
#else
#define EEPROM_UART_SIZE_STR "1024"
#define EEPROM_ADDR_BYTES_STR "2"
#endif

static void uart1_send_device_identity(void) {
  uart1_puts("ID=" CONFIG_DEVICE_ID ";VER=" CONFIG_PROTOCOL_VERSION_STR ";EEPROM=");
  uart1_puts(EEPROM_UART_SIZE_STR);
  uart1_puts(";ADDR_BYTES=");
  uart1_puts(EEPROM_ADDR_BYTES_STR);
  uart1_puts("\n");
}

#ifdef USE_EXTERNAL_EEPROM
/* Stan maszyny dla R: czekamy na addr_hi, addr_lo */
static uint8_t uart_r_state = 0;
static uint8_t uart_r_addr_h = 0, uart_r_addr_l = 0;

/* Stan maszyny dla W: czekamy na addr_hi, addr_lo, data */
static uint8_t uart_w_state = 0;
static uint8_t uart_w_addr_h = 0, uart_w_addr_l = 0;
static uint32_t uart_w_count = 0;
#endif

void uart1_process_commands(void) {
#ifdef USE_EXTERNAL_EEPROM
  /* Obsługa R - adresowany odczyt bajt po bajcie */
  if (uart_r_state == 1 && uart1_data_available()) {
    uart_r_addr_h = uart1_getc();
    uart_r_state = 2;
    return;
  }
  if (uart_r_state == 2 && uart1_data_available()) {
    uart_r_addr_l = uart1_getc();
    {
      uint16_t addr = ((uint16_t)uart_r_addr_h << 8) | uart_r_addr_l;
      if (addr < EEPROM_UART_SIZE) {
        unsigned char data = eeprom_read_byte((const uint8_t *)addr);
        uart1_putc(data);
      } else {
        uart1_putc(0xFF);
      }
    }
    uart_r_state = 0;
    return;
  }

  /* Obsługa W - adresowany zapis bajt po bajcie */
  if (uart_w_state == 1 && uart1_data_available()) {
    uart_w_addr_h = uart1_getc();
    uart_w_state = 2;
    return;
  }
  if (uart_w_state == 2 && uart1_data_available()) {
    uart_w_addr_l = uart1_getc();
    uart_w_state = 3;
    return;
  }
  if (uart_w_state == 3 && uart1_data_available()) {
    unsigned char data = uart1_getc();
    uint16_t addr = ((uint16_t)uart_w_addr_h << 8) | uart_w_addr_l;
    if (addr < EEPROM_UART_SIZE)
      eeprom_write_byte((uint8_t *)addr, data);
    if (addr == 0)
      uart_w_count = 0;
    uart_w_count++;
    if (addr == (EEPROM_UART_SIZE - 1)) {
      uart_w_state = 0;
      uart_w_count = 0;
      uart1_puts("OK\n");
    } else {
      uart_w_state = 0; /* czekaj na następne 'W' od konfiguratora */
    }
    return;
  }
#endif

  if (!uart1_data_available())
    return;

  unsigned char cmd = uart1_getc();

  switch (cmd) {
  case 'C': {
    for (uint8_t i = 0; i < 4; i++) {
      unsigned char data = uart1_getc();
      eeprom_write_byte((uint8_t *)(1 + i), data);
      _delay_ms(5);
    }
    uart1_puts("OK\n");
    break;
  }

  case 'c': {
    for (uint8_t i = 0; i < 4; i++) {
      unsigned char data = eeprom_read_byte((uint8_t *)(1 + i));
      uart1_putc(data);
    }
    break;
  }

#ifdef USE_EXTERNAL_EEPROM
  case CMD_READ_EEPROM: {
    /* Rozpocznij odczyt - czekamy na 2 bajty adresu */
    uart_r_state = 1;
    break;
  }

  case CMD_WRITE_EEPROM: {
    /* Rozpocznij zapis - czekamy na addr_hi, addr_lo, data.
     * Konfigurator wysyła W dla każdego bajtu, więc W = pierwszy bajt
     * następnych 3 to addr_h, addr_l, data. */
    uart_w_state = 1;
    break;
  }
#else
  case CMD_READ_EEPROM: {
    /* Stary protokół: strumieniowy odczyt 1024 B */
    for (uint16_t i = 0; i < EEPROM_UART_SIZE; i++) {
      unsigned char data = eeprom_read_byte((const uint8_t *)i);
      uart1_putc(data);
    }
    break;
  }

  case CMD_WRITE_EEPROM: {
    /* Stary protokół: strumieniowy zapis 1024 B */
    for (uint16_t i = 0; i < EEPROM_UART_SIZE; i++) {
      while (!uart1_data_available())
        ;
      unsigned char data = uart1_getc();
      eeprom_write_byte((uint8_t *)i, data);
    }
    uart1_puts("OK\n");
    break;
  }
#endif

  case 'T': {
    uart1_puts("TEST_OK\n");
    break;
  }

  case CMD_POLL_DEVICE:
  case CMD_GET_ID: {
    uart1_send_device_identity();
    break;
  }

  case 'S': {
    uart1_putc(poziom_sieci_gsm);
    break;
  }

  default:
    uart1_putc(cmd);
    break;
  }
}
