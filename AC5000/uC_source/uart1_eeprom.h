#ifndef UART1_EEPROM_H
#define UART1_EEPROM_H

#include "eeprom_wrapper.h"
#include <avr/io.h>

/* Rozmiar EEPROM - AT24C512 = 64KB przy USE_EXTERNAL_EEPROM */
#ifdef USE_EXTERNAL_EEPROM
#define UART_EEPROM_SIZE 65536
#else
#define UART_EEPROM_SIZE 1024
#endif

// Komendy
#define CMD_WRITE_EEPROM 'W'  // Zapis EEPROM (adresowany bajt po bajcie)
#define CMD_READ_EEPROM 'R'   // Odczyt EEPROM (adresowany bajt po bajcie)
#define CMD_VERIFY_EEPROM 'V' // Weryfikacja EEPROM
#define CMD_POLL_DEVICE 'P'   // Polling identyfikacji urządzenia (v5000)
#define CMD_GET_ID 'I'        // Jawny odczyt identyfikacji (v5000)

/* Protokół konfiguratora */
#define CONFIG_PROTOCOL_VERSION 5000
#define CONFIG_PROTOCOL_VERSION_STR "5000"
#define CONFIG_DEVICE_ID "AC5000"

// Prototypy funkcji
void uart1_init(void);
void uart1_putc(unsigned char c);
unsigned char uart1_getc(void);
unsigned char uart1_data_available(void);
void uart1_puts(const char *str);
void uart1_process_commands(void);

#endif // UART1_EEPROM_H
