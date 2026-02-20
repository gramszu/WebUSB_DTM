#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t uchar;
#define LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM 5
#define ZNAK_NUMERU_TELEFONU_PLUS 0x0A
#define ZNAK_NUMERU_TELEFONU_GWIAZDKA 0x0B
#define ZNAK_NUMERU_TELEFONU_KRZYZ 0x0C
#define ZNAK_NUMERU_TELEFONU_NIEZNANY 0x0F

uchar konwersja_znaku_telefonu(const uchar znak) {
  switch (znak) {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    return znak - '0';
  case '+':
    return ZNAK_NUMERU_TELEFONU_PLUS;
  case '*':
    return ZNAK_NUMERU_TELEFONU_GWIAZDKA;
  case '#':
    return ZNAK_NUMERU_TELEFONU_KRZYZ;
  default:
    return ZNAK_NUMERU_TELEFONU_NIEZNANY;
  }
}

void konwertuj_telefon_na_blok_eeprom(const uchar *telefon_ptr_begin,
                                      const uchar *telefon_ptr_end,
                                      uchar *blok_ptr) {
  memset(blok_ptr, 0xff, LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM);
  int l = telefon_ptr_end -
          telefon_ptr_begin; // Changed uchar to int for safety in test
  if (l < 2)
    return;
  uchar pos_buf = 0;
  telefon_ptr_end--;
  while (l--) {
    uchar s1 = konwersja_znaku_telefonu(*telefon_ptr_end--);
    s1 <<= 4;
    s1 |= konwersja_znaku_telefonu(*telefon_ptr_end);
    if (l == 0)
      s1 |= 0x0f;
    else {
      --telefon_ptr_end;
      --l;
    }
    blok_ptr[pos_buf++] = s1;
  }
}

int main() {
  uchar number[] = "123456789";
  uchar buffer[LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM];

  printf("Testing with number: %s\n", number);
  konwertuj_telefon_na_blok_eeprom(number, number + strlen((char *)number),
                                   buffer);

  printf("EEPROM Buffer: ");
  for (int i = 0; i < LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM; i++) {
    printf("%02X ", buffer[i]);
  }
  printf("\n");

  uchar number2[] = "12";
  printf("Testing with number: %s\n", number2);
  konwertuj_telefon_na_blok_eeprom(number2, number2 + strlen((char *)number2),
                                   buffer);

  printf("EEPROM Buffer: ");
  for (int i = 0; i < LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM; i++) {
    printf("%02X ", buffer[i]);
  }
  printf("\n");

  return 0;
}
