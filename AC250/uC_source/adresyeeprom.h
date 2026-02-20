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
// SEKCJA 2: NUMERY TELEFONÓW
// ============================================================================

#define MAX_LICZBA_ZNAKOW_TELEFON 16
#define LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM 5 // BCD format

#define EEPROM_NUMER_TELEFONU_BRAMA_0 8 // 0x0008
#define EEPROM_NUMER_TELEFONU_BRAMA(NR)                                        \
  (EEPROM_NUMER_TELEFONU_BRAMA_0 +                                             \
   ((NR) * LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM))

// 250 numerów: 250 * 5 = 1250 bajtów.
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA 250
#define MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA_USER 250

#define ADRES_KONCA_NUMEROW                                                    \
  (EEPROM_NUMER_TELEFONU_BRAMA_0 +                                             \
   (MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA *                                       \
    LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM) -                                  \
   1)

// Konfiguracja zaczyna się zaraz po bloku numerów.
#define ADRES_EEPROM_CLIP_DURATION (ADRES_KONCA_NUMEROW + 1) // 4 bajty
#define ADRES_EEPROM_CLIP_TOGGLE (ADRES_EEPROM_CLIP_DURATION + 4) // 1 bajt

// ============================================================================
// SEKCJA 3: KONFIGURACJA SYSTEMU
// ============================================================================

#define ADRES_EEPROM_TRYB_PRACY (ADRES_EEPROM_CLIP_TOGGLE + 1) // 1 bajt
#define ADRES_EEPROM_TRYB_CLIP_DTMF                                           \
  (ADRES_EEPROM_TRYB_PRACY + 1) // 1 bajt (0=DTMF, 1=CLIP)
#define ADRES_EEPROM_BLOKADA_SYSTEMU                                           \
  (ADRES_EEPROM_TRYB_CLIP_DTMF + 1) // 1 B (RESERVED)
#define ADRES_EEPROM_LICZNIK_CYKLI                                            \
  (ADRES_EEPROM_BLOKADA_SYSTEMU + 1) // 4 bajty Little Endian
#define ADRES_EEPROM_CON_NUMER (ADRES_EEPROM_LICZNIK_CYKLI + 4) // 5 bajtów

// ============================================================================
// SEKCJA 4: USTAWIENIA (rezerwa na przyszłe opcje, powiadomienia, itp.)
// ============================================================================
#define ADRES_EEPROM_USTAWIENIA_START (ADRES_EEPROM_CON_NUMER + 5)
#define ADRES_EEPROM_INIT_FLAG ADRES_EEPROM_USTAWIENIA_START // 1 bajt (0xA5)
#define ROZMIAR_EEPROM_USTAWIENIA 1024 // 1 KB
#define ADRES_EEPROM_USTAWIENIA_KONIEC                                         \
  (ADRES_EEPROM_USTAWIENIA_START + ROZMIAR_EEPROM_USTAWIENIA - 1)
// Razem: numery 1.25 KB + konfig 20 B + ustawienia 1 KB.

// ============================================================================
// MAKRA POMOCNICZE
// ============================================================================

#define NUMER_W_ZAKRESIE(nr)                                                   \
  ((nr) >= 0 && (nr) < MAX_LICZBA_NUMEROW_TELEFONOW_BRAMA)

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
