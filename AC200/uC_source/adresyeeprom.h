#ifndef ADRESYEEPROM_H
#define ADRESYEEPROM_H

#include "narzedzia.h"

// ============================================================================
// SEKCJA 1: NAGŁÓWEK I PODSTAWOWA KONFIGURACJA (0x0000 - 0x0007)
// ============================================================================

#define ADRES_EEPROM_CHECKSUM 0 // 0x0000

#define ADRES_EEPROM_KOD_DOSTEPU 1 // 0x0001
#define LICZBA_BAJTOW_KODU_DOSTEPU 4

#define EEPROM_USTAWIENIE_STANOW_WYJSC 5 // 0x0005

#define EEPROM_USTAWIENIE_WYJSCIA 6 // 0x0006 (2 bajty)
// REMOVED above, reused for SMS Trigger
#define ADRES_EEPROM_SMS_TRIGGER 6 // 0x0006 (1 bajt)
#define ADRES_EEPROM_SERVICE_UNLOCK_PERMANENT                                  \
  7 // 0x0007 (1 bajt) - trwałe wyłączenie blokady serwisowej

// ============================================================================
// SEKCJA 2: NUMERY TELEFONÓW (0x0008 - ok. 1 KB przy 200 numerach)
// ============================================================================

#define MAX_LICZBA_ZNAKOW_TELEFON 16
#define LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM 5 // BCD format

#define EEPROM_NUMER_TELEFONU_BRAMA_0 8 // 0x0008
#define EEPROM_NUMER_TELEFONU_BRAMA(NR)                                        \
  (EEPROM_NUMER_TELEFONU_BRAMA_0 +                                             \
   ((NR) * LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM))

// 200 numerów: 200 * 5 = 1000 bajtów (ok. 1.0 KB). Działa też na EEPROM wewn.
// (AT24C512).
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA 200
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA_USER 200

// Koniec bloku numerów: 8 + (200 * 5) - 1 = 1007. Konfiguracja od 1008.
#define ADRES_EEPROM_CLIP_DURATION 1008 // 4 bajty: 1008-1011
#define ADRES_EEPROM_CLIP_TOGGLE 1012   // 1 bajt

// ============================================================================
// SEKCJA 3: KONFIGURACJA SYSTEMU
// ============================================================================

#define ADRES_EEPROM_TRYB_PRACY 1013      // 1 bajt
#define ADRES_EEPROM_TRYB_CLIP_DTMF 1014  // 1 bajt (0=DTMF, 1=CLIP)
#define ADRES_EEPROM_BLOKADA_SYSTEMU 1015 // 1 B (RESERVED)
#define ADRES_EEPROM_LICZNIK_CYKLI 1016   // 4 bajty Little Endian: 1016-1019
#define ADRES_EEPROM_CON_NUMER 1020       // 5 bajtów: 1020-1024

// ============================================================================
// SEKCJA 4: USTAWIENIA (rezerwa na przyszłe opcje, powiadomienia, itp.)
// ============================================================================
#define ADRES_EEPROM_USTAWIENIA_START 1025
#define ADRES_EEPROM_INIT_FLAG 1025   // 1 bajt (Magic Byte 0xA5)
#define ROZMIAR_EEPROM_USTAWIENIA 1024 // 1 KB
#define ADRES_EEPROM_USTAWIENIA_KONIEC                                         \
  (ADRES_EEPROM_USTAWIENIA_START + ROZMIAR_EEPROM_USTAWIENIA - 1)
// Razem: numery 1.0 KB + konfig 20 B + ustawienia 1 KB ≈ 2.0 KB

// ============================================================================
// MAKRA POMOCNICZE
// ============================================================================

#define NUMER_W_ZAKRESIE(nr)                                                   \
  ((nr) >= 0 && (nr) < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA)

#define ADRES_KONCA_NUMEROW                                                    \
  (EEPROM_NUMER_TELEFONU_BRAMA_0 +                                             \
   (MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA *                                       \
    LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM) -                                  \
   1)

// ============================================================================
// WERYFIKACJA POPRAWNOŚCI (compile-time checks)
// ============================================================================

// Sprawdź czy numery nie nachodzą na konfigurację
#if (ADRES_KONCA_NUMEROW >= ADRES_EEPROM_TRYB_PRACY)
#error "BŁĄD: Numery telefonów nachodzą na konfigurację!"
#endif

#if defined(USE_EXTERNAL_EEPROM)
#define EEPROM_MAX_ADDR 65535
#else
#define EEPROM_MAX_ADDR 1023
#endif
#if (ADRES_KONCA_NUMEROW >= EEPROM_MAX_ADDR ||                                 \
     ADRES_EEPROM_LICZNIK_CYKLI + 3 >= EEPROM_MAX_ADDR)
#error "BŁĄD: Adresy wykraczają poza pamięć EEPROM!"
#endif
#if (ADRES_EEPROM_USTAWIENIA_KONIEC > EEPROM_MAX_ADDR)
#error "BŁĄD: Blok ustawień wykracza poza EEPROM (wymagaj USE_EXTERNAL_EEPROM)"
#endif

#endif // ADRESYEEPROM_H
