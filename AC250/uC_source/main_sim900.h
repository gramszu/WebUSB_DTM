
#ifdef MAIN_SIM900_H
#error Dwa razy wlaczany plik
#endif

void wyzerowanie_danych_SIM900(void) {
  POMOC_DODAJ2('*', 't');
  problem_z_wyslaniem_powiadomienia();
  licznik_wysylane_polecenie_SIM900 = 0;
  aktualnie_wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_BRAK_KOMENDY;
  liczba_komend_w_kolejce_SIM900 = 0;
  licznik_oczekiwanie_na_potwierdzenie_wyslania_znakow = 0;
  czekanie_na_odebranie_zachety = FALSE;
  flaga_odebrany_znak_zachety = FALSE;
  trwa_rozmowa_przychodzaca_od_uzytkownika = FALSE;
  wysylanie_smsa_clipa = WYSYLANIE_SMSA_CLIPA_BRAK;
  flaga_odczytywanie_smsa = FALSE;
}

void reset_modulu_SIM900(void) {
  wyzerowanie_danych_SIM900();
  POMOC_DODAJ2('*', 'm');
  HARDRESET_SIM900();
  podlaczony_modul_gsm_SIM900 = FALSE;
  podlaczona_karta_SIM_SIM900 = FALSE;
  licznik_blad_stanu_karty_SIM = 0;
  licznik_blad_zalogowania_u_operatora = 0;
  nazwa_operatora[0] = '\0';
  licznik_ogolny_blad_zalogowania = 0;
  blokada_clip = FALSE;
  nastepne_wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_BRAK_KOMENDY;
  poziom_sieci_gsm = POZIOM_SIECI_BLAD;
  filtruj_komendy_z_przedzialu(KOMENDA_KOLEJKI_USUN_SMSA_1,
                               KOMENDA_KOLEJKI_USUN_SMSA_20);
  filtruj_komendy_z_przedzialu(
      KOMENDA_KOLEJKI_KOMENDA_SIM900,
      KOMENDA_KOLEJKI_WYSLIJ_DO_SIM900_ODCZYTAJ_GODZINE);
  filtruj_komendy_z_przedzialu(
      KOMENDA_KOLEJKI_SPRAWDZ_PIN,
      KOMENDA_KOLEJKI_WPISZ_POZYCJE_1_W_KSIAZCE_TELEFONICZNEJ);
  filtruj_komendy_z_przedzialu(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT,
                               KOMENDA_KOLEJKI_WYSLIJ_PDU);
  modul_zalogowany_w_sieci = FALSE;
}

static uchar czy_polecenie_SIM900(PGM_P polecenie, const uchar liczba_znakow) {
  return strncmp_P(odebrany_blok_SIM900 + 2, polecenie, liczba_znakow) == 0;
}

static uchar *polozenie_polecenia_SIM900(PGM_P polecenie) {
  return strstr_P(odebrany_blok_SIM900, polecenie);
}

void obsluga_wysylania_sms(void) {
  POMOC_DODAJ2('#', 'M');
  if (czy_jest_komenda_wyslano_sms()) {
    POMOC_DODAJ2('#', 'T');
    liczba_prob_wyslania_smsa = 0;
    zakonczono_wysylanie_smsa(zakonczenie_wysylania_sms_ok);
    flaga_wysylanie_smsa = 0;
    opoznienie_SIM900_100MS = 70;
    return;
  } else if (
      czy_jest_komenda_blad_CMS()) // spawdzi
                                   // czy_modul_byl_polaczony_z_internetem
  {
    ++liczba_prob_wyslania_smsa;
    if (numer_bledu_cms == CMS_SM_BL_NOT_READY ||
        numer_bledu_cms == CMS_SIM_BUSY ||
        numer_bledu_cms == CMS_PC_BUSY // powinien by reset
        || numer_bledu_cms == CMS_INVALID_CHARS_IN_PDU ||
        numer_bledu_cms == CMS_INCORECT_PDU_LENGTH) {
      opoznienie_SIM900_100MS = 60;
      if (liczba_prob_wyslania_smsa < max_liczba_prob_wyslania_smsa) {
        POMOC_DODAJ2('#', 'C');
        POMOC_DODAJ_HEX(numer_bledu_cms >> 8);
        POMOC_DODAJ_HEX(numer_bledu_cms & 0xff);
        dodaj_komende(flaga_wysylanie_smsa);
      } else {
        POMOC_DODAJ2('#', 'D');
        POMOC_DODAJ_HEX(numer_bledu_cms >> 8);
        POMOC_DODAJ_HEX(numer_bledu_cms & 0xff);
        liczba_prob_wyslania_smsa = 0;
        zakonczono_wysylanie_smsa(zakonczenie_wysylania_sms_blad_zakonczenie);
      }
    } else {
      POMOC_DODAJ2('#', 'F');
      POMOC_DODAJ_HEX(numer_bledu_cms >> 8);
      POMOC_DODAJ_HEX(numer_bledu_cms & 0xff);
      opoznienie_SIM900_100MS = 60;
      liczba_prob_wyslania_smsa = 0;
      zakonczono_wysylanie_smsa(zakonczenie_wysylania_sms_blad_zakonczenie);
    }
    flaga_wysylanie_smsa = 0;
  } else if (czy_jest_komenda_ERROR()) {
    POMOC_DODAJ2('#', 'G');
    opoznienie_SIM900_100MS = 60;
    czekanie_na_odebranie_zachety = FALSE;
    if ((odebrany_blok_SIM900[2] == '>' &&
         liczba_odebranych_znakow_SIM900 < 10) ||
        flaga_wysylanie_smsa == 0) // SIM900 nie odpowiada
    {
      POMOC_DODAJ2('#', 'H');
      liczba_prob_wyslania_smsa = 0;
      zakonczono_wysylanie_smsa(zakonczenie_wysylania_sms_blad_zakonczenie);
      reset_modulu_SIM900();
    } else {
      POMOC_DODAJ2('#', 'I');
      if (++liczba_prob_wyslania_smsa <
          max_liczba_prob_wyslania_smsa) // musi by sta wartoci
        dodaj_komende(flaga_wysylanie_smsa);
      else {
        liczba_prob_wyslania_smsa = 0;
        zakonczono_wysylanie_smsa(zakonczenie_wysylania_sms_blad_zakonczenie);
      }
    }
    flaga_wysylanie_smsa = 0;
  } else if (czy_jest_komenda_blad_CME()) {
    POMOC_DODAJ2('#', 'J');
    POMOC_DODAJ_HEX(numer_bledu_cme >> 8);
    POMOC_DODAJ_HEX(numer_bledu_cme & 0xff);
    if (numer_bledu_cme == CME_SIM_NOT_INSERTED ||
        numer_bledu_cme == CME_SIM_FAILURE ||
        numer_bledu_cme == CME_SIM_WRONG ||
        numer_bledu_cme == CME_NO_NETWORK_SERVICE) {
      POMOC_DODAJ2('#', 'K');
      liczba_prob_wyslania_smsa = 0;
      zakonczono_wysylanie_smsa(zakonczenie_wysylania_sms_blad_zakonczenie);
      reset_modulu_SIM900();
    }
  } else {
    POMOC_DODAJ2('#', 'L');
    POMOC_DODAJ_HEX(komenda_SIM900 >> 8);
    POMOC_DODAJ_HEX(komenda_SIM900 & 0xff);
  }
}

void odpowiedz_na_polecenie(void) {
#define czy_polecenie_sim(POL) czy_polecenie_SIM900(POL, sizeof POL - 1)
  const uchar kom = komenda_SIM900;

  switch (aktualnie_wysylane_polecenie_SIM900) {
  case KOMENDA_KOLEJKI_WYSLIJ_DO_SIM900_AT: {
    dodaj_komende(KOMENDA_KOLEJKI_ODEBRANO_POLECENIE_ROZPOCZYNAJACE);
    max_oczekiwanie_na_odpowiedz_at = 0;
    break;
  }
  case KOMENDA_KOLEJKI_WYSLIJ_DO_SIM900_ODCZYTAJ_GODZINE: {
    static const char polecenie_zegar[] PROGMEM = "+CCLK:";
    if (kom == OK_KOMENDA_SIM900 &&
        polozenie_polecenia_SIM900(polecenie_zegar) != NULL) {
      const uchar *p1 =
          strchr(polozenie_polecenia_SIM900(polecenie_zegar), '\"');
      if (p1++ != NULL) {
        const uchar *p2 = strchr(p1, '\"');
        const uchar *p3 = strchr(p1, ',');
        if (p2 != NULL && p3++ != NULL && p2 > p3) {
          // const uchar g = atoi(p3);
          if ((p3 = strchr(p3, ':')) != NULL && p2 > p3++) {
            // Pobierz czas z odpowiedzi +CCLK: "yy/MM/dd,hh:mm:ss+zz"
            // p3 wskazuje na minuty po pierwszym ':'
            // cofnijmy si do pocztku godziny (p3 wskazuje na mm, wic -3 to hh)
            // ale w kodzie powyej p3 byo przesuwane.

            // Restart parsowania dla pewnoci:
            // "24/01/01,12:34:56+00"
            const char *ptr_time = strchr(p1, ',');
            if (ptr_time) {
              ptr_time++; // skip comma
              if (strlen(ptr_time) >= 8) {
                memcpy(rtc_czas, ptr_time, 8);
                rtc_czas[8] = '\0';

                // Aktualizacja blokady czasowej - Logic removed
                // blokada_sterowania_czasowa = FALSE; // REMOVED
              }
            }
          }
        }
      }
    }
    break;
  }
  case KOMENDA_KOLEJKI_SPRAWDZ_PIN: {
    static const char polecenie_pin_ready[] PROGMEM = "+CPIN: READY";
    static uchar nr_zapytania_o_pin;
    if (kom == OK_KOMENDA_SIM900) {
      if (polozenie_polecenia_SIM900(polecenie_pin_ready) != NULL) {
        podlaczona_karta_SIM_SIM900 = TRUE;
        bledny_PIN = FALSE;
        dodaj_komende(KOMENDA_KOLEJKI_USUN_WSZYSTKIE_SMSY);
      } else
        dodaj_komende(KOMENDA_KOLEJKI_PODAJ_PIN);
      nr_zapytania_o_pin = 0;
    } else if (kom == CME_ERROR_KOMENDA_SIM900 &&
               numer_bledu_cme == CME_SIM_BUSY)
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
    else if (kom == ERROR_KOMENDA_SIM900 || czy_jest_blad_SIM900()) {
      if (++nr_zapytania_o_pin >= 20) {
        reset_modulu_SIM900();
        nr_zapytania_o_pin = 0;
      } else {
        opoznienie_SIM900_100MS = 20;
        dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
      }
    } else
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
    break;
  }
  case KOMENDA_KOLEJKI_PODAJ_PIN: {
    if (kom == OK_KOMENDA_SIM900) {
      podlaczona_karta_SIM_SIM900 = TRUE;
      bledny_PIN = FALSE;
      dodaj_komende(KOMENDA_KOLEJKI_USUN_WSZYSTKIE_SMSY);
      opoznienie_SIM900_100MS = 50;
    } else if (kom == CME_ERROR_KOMENDA_SIM900 &&
               numer_bledu_cme == CME_SIM_INCORECT_PASWORD)
      bledny_PIN = TRUE;
    else if (kom == CME_ERROR_KOMENDA_SIM900 && numer_bledu_cme == CME_SIM_BUSY)
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
    else if (kom == ERROR_KOMENDA_SIM900 || czy_jest_blad_SIM900())
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
    else
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
    break;
  }
  case KOMENDA_KOLEJKI_USUN_WSZYSTKIE_SMSY: {
    if (kom == OK_KOMENDA_SIM900)
      dodaj_komende(KOMENDA_KOLEJKI_USTAW_ODBIERANIE_SMSOW);
    else if (kom == CMS_ERROR_KOMENDA_SIM900) {
      if (numer_bledu_cms == CMS_SIM_BUSY ||
          numer_bledu_cms == CMS_SM_BL_NOT_READY) {
        opoznienie_SIM900_100MS = 60;
        dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
      }
    } else if (kom == ERROR_KOMENDA_SIM900 || czy_jest_blad_SIM900())
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
    break;
  }
  case KOMENDA_KOLEJKI_USTAW_ODBIERANIE_SMSOW: {
    if (kom == OK_KOMENDA_SIM900)
      dodaj_komende(KOMENDA_KOLEJKI_USTAW_PAMIEC_SM_DLA_SMSOW);
    else if (kom == CMS_ERROR_KOMENDA_SIM900) {
      if (numer_bledu_cms == CMS_SIM_BUSY ||
          numer_bledu_cms == CMS_SM_BL_NOT_READY) {
        opoznienie_SIM900_100MS = 20;
        dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
      }
    } else {
      opoznienie_SIM900_100MS = 10;
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
    }
    break;
  }
  case KOMENDA_KOLEJKI_USTAW_WYSYLANIE_INFO_O_DZWONIACYM: {
    if (kom == OK_KOMENDA_SIM900 ||
        (kom == CME_ERROR_KOMENDA_SIM900 &&
         (numer_bledu_cme == CME_OPERATION_NOT_ALLOWED ||
          numer_bledu_cme == CME_OPERATION_NOT_SUPPORTED))) {
      dodaj_komende(nastepne_wysylane_polecenie_SIM900);
      nastepne_wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_BRAK_KOMENDY;
      liczba_wykonanych_komend_identycznego_polecenia = 0;
    } else if (kom != OK_KOMENDA_SIM900 &&
               ++liczba_wykonanych_komend_identycznego_polecenia <
                   MAX_LICZBA_WYKONANYCH_KOMEND_IDENTYCZNEGO_POLECENIA)
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
    opoznienie_SIM900_100MS = 2;
    break;
  }
  case KOMENDA_KOLEJKI_SPRAWDZ_POZIOM_SIECI: {
    if (kom == OK_KOMENDA_SIM900) {
      static const uchar urc[] PROGMEM = "+CSQ:";
      const uchar *p1 = polozenie_polecenia_SIM900(urc);
      if (p1 != NULL) {
        p1 += strlen_R(urc) + 1;
        poziom_sieci_gsm = strtol(p1, NULL, 10);
        // if ( poziom_sieci_gsm != POZIOM_SIECI_BLAD )
        //	POMOC_DODAJ2('*', '0' + poziom_sieci_gsm / 4);
        // else
        //{
        //	POMOC_DODAJ2('*', 'c');
        // }
      }
    } else
      poziom_sieci_gsm = POZIOM_SIECI_BLAD;
    break;
  }
  case KOMENDA_KOLEJKI_SPRAWDZ_ZAREJESTROWANIE: {
    if (kom == OK_KOMENDA_SIM900) {
      static const uchar urc[] PROGMEM = "+CREG:";
      const uchar *p1 = polozenie_polecenia_SIM900(urc);
      if (p1 != NULL) {
        p1 += strlen_R(urc) + 1;
        if (*p1 != ',')
          ++p1;
        if (*p1 != ',')
          ++p1;
        if (*p1 != ',')
          ++p1;
        ++p1;
        uchar stan_zarejestrowania = (uchar)strtol(p1, NULL, 10);
        if (stan_zarejestrowania == 0 || stan_zarejestrowania == 3 ||
            stan_zarejestrowania == 4) {
          reset_modulu_SIM900();
        } else if (stan_zarejestrowania == 1 || stan_zarejestrowania == 5) {
          modul_zalogowany_w_sieci = TRUE;
        }
      }
    }
    break;
  }
  case KOMENDA_KOLEJKI_SPRAWDZ_STAN_KARTY_SIM: {
    if (kom == OK_KOMENDA_SIM900 || (kom == CME_ERROR_KOMENDA_SIM900 &&
                                     (numer_bledu_cme == CME_INVALID_INDEX ||
                                      numer_bledu_cme == CME_NOT_FOUND))) {
      licznik_blad_stanu_karty_SIM = 0;
    } else {
      ++licznik_blad_stanu_karty_SIM;
    }
    break;
  }
  case KOMENDA_KOLEJKI_SPRAWDZ_ZALOGOWANIE_U_OPERATORA: {
    static const char polecenie_operator[] PROGMEM = "+COPS:";
    uchar w = FALSE;
    if (kom == OK_KOMENDA_SIM900 && czy_polecenie_sim(polecenie_operator)) {
      const uchar *p1 =
          strchr(polozenie_polecenia_SIM900(polecenie_operator), ',');
      if (p1 != NULL) {
        p1 = strchr(p1, ',');
        if (p1 != NULL) {
          p1 = strchr(p1, '\"');
          if (p1++ != NULL) {
            const uchar *p2 = strchr(p1, '\"');
            if (p2 != NULL && p1 != p2) {
              const uchar m = min(p2 - p1, ROZMIAR_NAZWA_OPERATORA);
              memcpy(nazwa_operatora, p1, m);
              nazwa_operatora[m] = '\0';
              w = TRUE;
            }
          }
        }
      }
    }
    if (w)
      licznik_blad_zalogowania_u_operatora = 0;
    else {
      ++licznik_blad_zalogowania_u_operatora;
    }
    // POMOC_DODAJ2('*', w ? '+' : '-');
    break;
  }
  case KOMENDA_KOLEJKI_RESETUJ_CELL_BROADCAST_SMS: {
    if (kom ==
        CMS_ERROR_KOMENDA_SIM900 /*&& numer_bledu_cms == CMS_SMS_ME_RESERVED*/) // co le interpretuje numer_bledu_cms
      ; // nic nie robi
    else if (kom != OK_KOMENDA_SIM900)
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
    opoznienie_SIM900_100MS = 2;
    break;
  }
  case KOMENDA_KOLEJKI_ODBIERZ_ROZMOWE: {
    if (kom == OK_KOMENDA_SIM900 ||
        czy_jest_komenda_uzytkownik_odebral_rozmowe()) {
      trwa_rozmowa_przychodzaca_od_uzytkownika = TRUE;
      liczba_wykonanych_komend_identycznego_polecenia = 0;

      // DTMF START
      if (!tryb_clip) {
        licznik_timeout_rozmowy_100ms = MAX_LICZNIK_TIMEOUT_ROZMOWY_100MS;
        opoznienie_SIM900_100MS =
            20; // 2 sekundy opoznienia przed wyslaniem tonu
        dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_TON_DTMF); // Beep potwierdzenia
      }
    } else if ((kom == ERROR_KOMENDA_SIM900 || czy_jest_blad_SIM900()) &&
               ++liczba_wykonanych_komend_identycznego_polecenia <
                   MAX_LICZBA_WYKONANYCH_KOMEND_IDENTYCZNEGO_POLECENIA)
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
    else if (kom == NO_CARRIER_KOMENDA_SIM900) {
      trwa_rozmowa_przychodzaca_od_uzytkownika = FALSE;
      liczba_wykonanych_komend_identycznego_polecenia = 0;
    } else
      liczba_wykonanych_komend_identycznego_polecenia = 0;
    break;
  }
  case KOMENDA_KOLEJKI_ODRZUC_ROZMOWE: {
    if (kom != OK_KOMENDA_SIM900)
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
    else {
      zakonczono_rozmowe_telefoniczna(powod_zakonczenia_rozmowy_zakonczenie);
      trwa_rozmowa_przychodzaca_od_uzytkownika = FALSE;
      wykonywanie_rozmowy_telefonicznej = FALSE;
    }
    break;
  }
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_1:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_2:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_3:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_4:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_5:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_6:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_7:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_8:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_9:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_10:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_11:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_12:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_13:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_14:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_15:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_16:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_17:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_18:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_19:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_20: {
    if (kom == OK_KOMENDA_SIM900) {
      // odczytany sms
      const uchar *ptr = polozenie_polecenia_SIM900(PSTR("+CMGR:")); // (1)
      ptr = strchr(ptr, ZNAK_KONCA_ODBIERANIA_ZNAKOW_LF);            // (1)
      // const uchar* ptr = strchr(&odebrany_blok_SIM900[7],
      // ZNAK_KONCA_ODBIERANIA_ZNAKOW_LF);
      if (ptr != NULL) {
        ++ptr;
        const uchar *ptr2 = strchr(ptr, ZNAK_KONCA_WYSYLANIA_ZNAKOW_CR);
        if (ptr2 != NULL) {
          dlugosc_pdu = (ptr2 - ptr) / 2;
          if (dlugosc_pdu > MAX_DLUGOSC_PDU)
            dlugosc_pdu = MAX_DLUGOSC_PDU;
          konwertuj_blok_dwa_znaki_na_znak_pdu(ptr, dlugosc_pdu, bufor_pdu);
          dodaj_komende(KOMENDA_KOLEJKI_INTERPRETUJ_PDU);
          const uchar nr_smsa = aktualnie_wysylane_polecenie_SIM900 -
                                KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_1;
          dodaj_komende(KOMENDA_KOLEJKI_USUN_SMSA_1 + nr_smsa);
          flaga_odczytywanie_smsa = TRUE;
          POMOC_DODAJ2('#', 'O');
        } else
          flaga_odczytywanie_smsa = FALSE;
      } else
        flaga_odczytywanie_smsa = FALSE;
    } else if (kom == CME_ERROR_KOMENDA_SIM900) {
      flaga_odczytywanie_smsa = FALSE;
      opoznienie_SIM900_100MS = 60;
    } else if (kom == CMS_ERROR_KOMENDA_SIM900) {
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
      flaga_odczytywanie_smsa = FALSE;
      opoznienie_SIM900_100MS = 60;
      POMOC_DODAJ2('#', 'P');
      POMOC_DODAJ_HEX(numer_bledu_cms >> 8);
      POMOC_DODAJ_HEX(numer_bledu_cms & 0xff);
    } else {
      flaga_odczytywanie_smsa = FALSE;
      opoznienie_SIM900_100MS = 60;
    }
    break;
  }
  case KOMENDA_KOLEJKI_USUN_SMSA_1:
  case KOMENDA_KOLEJKI_USUN_SMSA_2:
  case KOMENDA_KOLEJKI_USUN_SMSA_3:
  case KOMENDA_KOLEJKI_USUN_SMSA_4:
  case KOMENDA_KOLEJKI_USUN_SMSA_5:
  case KOMENDA_KOLEJKI_USUN_SMSA_6:
  case KOMENDA_KOLEJKI_USUN_SMSA_7:
  case KOMENDA_KOLEJKI_USUN_SMSA_8:
  case KOMENDA_KOLEJKI_USUN_SMSA_9:
  case KOMENDA_KOLEJKI_USUN_SMSA_10:
  case KOMENDA_KOLEJKI_USUN_SMSA_11:
  case KOMENDA_KOLEJKI_USUN_SMSA_12:
  case KOMENDA_KOLEJKI_USUN_SMSA_13:
  case KOMENDA_KOLEJKI_USUN_SMSA_14:
  case KOMENDA_KOLEJKI_USUN_SMSA_15:
  case KOMENDA_KOLEJKI_USUN_SMSA_16:
  case KOMENDA_KOLEJKI_USUN_SMSA_17:
  case KOMENDA_KOLEJKI_USUN_SMSA_18:
  case KOMENDA_KOLEJKI_USUN_SMSA_19:
  case KOMENDA_KOLEJKI_USUN_SMSA_20: {
    if (kom == CMS_ERROR_KOMENDA_SIM900 &&
        numer_bledu_cms != CMS_INVALID_MEMORY_INDEX) {
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
    } else if (kom == ERROR_KOMENDA_SIM900)
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
    else if (czy_jest_blad_SIM900())
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
    POMOC_DODAJ2('#', 'Q');
    opoznienie_SIM900_100MS = 25;
    break;
  }
  case KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT: {
    // czekanie_na_odebranie_zachety = FALSE; byo
    break;
  }
  case KOMENDA_KOLEJKI_WYKONAJ_ROZMOWE: {
    if (kom == ERROR_KOMENDA_SIM900 || czy_jest_blad_SIM900()) {
      wykonywanie_rozmowy_telefonicznej = FALSE;
      if (++liczba_wykonanych_komend_identycznego_polecenia <
          MAX_LICZBA_WYKONANYCH_KOMEND_IDENTYCZNEGO_POLECENIA)
        dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
      else {
        liczba_wykonanych_komend_identycznego_polecenia = 0;
        zakonczono_rozmowe_telefoniczna(powod_zakonczenia_rozmowy_odrzucenie);
        trwa_rozmowa_przychodzaca_od_uzytkownika = FALSE;
        wykonywanie_rozmowy_telefonicznej = FALSE;
        licznik_bezpieczenstwa_wykonywana_rozmowa = 0;
      }
    } else
      liczba_wykonanych_komend_identycznego_polecenia = 0;
    break;
  }
  case KOMENDA_KOLEJKI_USTAW_PAMIEC_SM_DLA_SMSOW:
    if (kom != OK_KOMENDA_SIM900) {
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
      opoznienie_SIM900_100MS = 20;
      break;
    } // bez break
    goto obsluga_standardowej_instrukcji_at;
  case KOMENDA_KOLEJKI_ODZYSKIWANIE_URZADZENIA_1: {
    if (polozenie_polecenia_SIM900(PSTR(">")) != 0) {
      reset_modulu_SIM900();
      break;
    }
  }
  case KOMENDA_KOLEJKI_WSTAWIENIE_MORING_GDY_DZWONI:
  case KOMENDA_KOLEJKI_USTAW_WZMOCNIENIE_MIKROFONU:
  case KOMENDA_KOLEJKI_USTAW_CICHY_TRYB:
  case KOMENDA_KOLEJKI_WYCISZ_DZWONKI:
  case KOMENDA_KOLEJKI_ODBIERAJ_SMS_FLASH:
  case KOMENDA_KOLEJKI_WPISZ_POZYCJE_1_W_KSIAZCE_TELEFONICZNEJ:
  case KOMENDA_KOLEJKI_WLACZ_CZAS_Z_SIECI: {
  obsluga_standardowej_instrukcji_at:
    if (kom != OK_KOMENDA_SIM900 &&
        ++liczba_wykonanych_komend_identycznego_polecenia <
            MAX_LICZBA_WYKONANYCH_KOMEND_IDENTYCZNEGO_POLECENIA)
      dodaj_komende(aktualnie_wysylane_polecenie_SIM900);
    else {
      dodaj_komende(nastepne_wysylane_polecenie_SIM900);
      if (nastepne_wysylane_polecenie_SIM900 >=
              KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_1 &&
          nastepne_wysylane_polecenie_SIM900 <=
              KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_20) {
        flaga_odczytywanie_smsa = FALSE;
      }
      nastepne_wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_BRAK_KOMENDY;
      liczba_wykonanych_komend_identycznego_polecenia = 0;
    }
    opoznienie_SIM900_100MS = 2;
    break;
  }
  }
#undef czy_polecenie_sim
  wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_BRAK_KOMENDY;
}

void obsluga_komendy_SIM900(void) {
  if (!czy_jest_bezczynny_SIM900())
    POMOC_SIM900_DODAJ(odebrany_blok_SIM900);

  if (czy_jest_blad_SIM900() || oproznij_bufor_SIM900_po_bledzie) {
    komenda_SIM900 = BLAD_SIM900;
    odpowiedz_na_polecenie();
    opoznienie_SIM900_100MS = 50;
    oproznij_bufor_SIM900_po_bledzie = TRUE;
    POMOC_DODAJ2('*', 'd');
    return;
  }

  if (czy_jest_komenda_wyslane_polecenie_SIM900()) {
    if ((liczba_wysylanych_znakow_SIM900 != liczba_odebranych_znakow_SIM900 ||
         memcmp(odebrany_blok_SIM900, wysylany_blok_SIM900,
                liczba_wysylanych_znakow_SIM900) != 0) &&
        !czekanie_na_odebranie_zachety) {
      komenda_SIM900 = BLAD_SIM900;
      oproznij_bufor_SIM900_po_bledzie = TRUE;
      opoznienie_SIM900_100MS = 50;
      POMOC_DODAJ2('*', 'e');
    }
    return;
  }

  if (czy_jest_komenda_rozmowa_telefoniczna()) {
    POMOC_DODAJ2('#', 'b');
    // Zawsze czyść bufor numeru przed parsowaniem nowego +CLIP.
    // Bez tego może zostać poprzedni numer i dać fałszywą autoryzację.
    numer_telefonu_ktory_dzwoni[0] = '\0';
    const uchar *p1 = strchr(polozenie_polecenia_SIM900(PSTR("+CLIP:")), '\"');
    if (p1++ != NULL) {
      const uchar *p2 = strchr(p1, '\"');
      if (p2 != NULL) {
        const uchar l = p2 - p1;
        if (l < MAX_LICZBA_ZNAKOW_TELEFON) {
          memcpy(numer_telefonu_ktory_dzwoni, p1, l);
          numer_telefonu_ktory_dzwoni[l] = '\0';
          POMOC_DODAJ_HEX(l);
          if (licznik_blad_zalogowania_u_operatora != 0)
            licznik_blad_zalogowania_u_operatora = 0;
          if (poziom_sieci_gsm == 0 || poziom_sieci_gsm == POZIOM_SIECI_BLAD)
            poziom_sieci_gsm = 16;
        }
      }
    }
    filtruj_i_dodaj_komende(KOMENDA_KOLEJKI_SPRAWDZ_PRZYCHODZACA_ROZMOWE);
    if (aktualnie_wysylane_polecenie_SIM900 >=
            KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_1 &&
        aktualnie_wysylane_polecenie_SIM900 <=
            KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_20) {
      flaga_odczytywanie_smsa = FALSE;
    }
    wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_BRAK_KOMENDY;
    return;
  }

  if (czy_jest_komenda_zakonczenie_rozmowy_telefonicznej()) {
    POMOC_DODAJ2('#', 'c');
    zakonczono_rozmowe_telefoniczna(powod_zakonczenia_rozmowy_odrzucenie);
    trwa_rozmowa_przychodzaca_od_uzytkownika = FALSE;
    wykonywanie_rozmowy_telefonicznej = FALSE;
    licznik_bezpieczenstwa_wykonywana_rozmowa = 0;
    if (aktualnie_wysylane_polecenie_SIM900 ==
        KOMENDA_KOLEJKI_WYKONAJ_ROZMOWE) {
      wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_BRAK_KOMENDY;
    }
    return;
  }

  if (czy_jest_komenda_uzytkownik_odebral_dzwonek()) {
    POMOC_DODAJ2('#', 'd');
    if (aktualnie_wysylane_polecenie_SIM900 == KOMENDA_KOLEJKI_WYKONAJ_ROZMOWE)
      odpowiedz_na_polecenie();
    if (aktualnie_wysylane_polecenie_SIM900 == KOMENDA_KOLEJKI_WYKONAJ_ROZMOWE) {
      if (ustaw_maksymalny_czas_dzwonienia == 0)
        maksymalny_czas_dzwonienia = 90; // standardowy czas czekania 9 sekund
      else {
        maksymalny_czas_dzwonienia = ustaw_maksymalny_czas_dzwonienia;
        ustaw_maksymalny_czas_dzwonienia = 0;
      }
      wykonywanie_rozmowy_telefonicznej = TRUE;
      licznik_bezpieczenstwa_wykonywana_rozmowa = 0;
    } else {
      // Dla połączeń przychodzących nie przechodź w tryb rozmowy:
      // wymuś decyzję i ewentualne odrzucenie.
      // Jeśli nie było +CLIP, nie wolno użyć starego numeru z poprzedniego
      // połączenia.
      numer_telefonu_ktory_dzwoni[0] = '\0';
      filtruj_i_dodaj_komende(KOMENDA_KOLEJKI_SPRAWDZ_PRZYCHODZACA_ROZMOWE);
    }
    return;
  }

  // Obsluga +DTMF:
  const uchar *ptr_dtmf = polozenie_polecenia_SIM900(PSTR("+DTMF:"));
  if (ptr_dtmf != NULL) {
    POMOC_DODAJ2('#', 'D');
    ptr_dtmf += 7; // Skip "+DTMF: "
    while (*ptr_dtmf == ' ')
      ++ptr_dtmf;
    const uchar dtmf_key = *ptr_dtmf;
    POMOC_DODAJ(dtmf_key);

    if (!tryb_clip) // Tylko w trybie DTMF
    {
      if (dtmf_key == '1') {
        // Use the same Toggle/Time logic as CLIP mode
        ustaw_wyjscie_clip();
        zapal_diode_led(10);
      }
    }
    resetuj_komende_SIM900(); // Krytyczne dla kolejnych znakow
    return;
  }

  if (czy_jest_komenda_uzytkownik_odebral_rozmowe()) {
    POMOC_DODAJ2('#', 'e');
    if (tryb_clip) {
      trwa_rozmowa_przychodzaca_od_uzytkownika = FALSE;
      dodaj_komende(KOMENDA_KOLEJKI_ODRZUC_ROZMOWE);
    }
    return;
  }
  if (czy_jest_komenda_brak_sygnalu_tonowego()) {
    POMOC_DODAJ2('#', 'f');
    if (aktualnie_wysylane_polecenie_SIM900 == KOMENDA_KOLEJKI_WYKONAJ_ROZMOWE)
      odpowiedz_na_polecenie();
    zakonczono_rozmowe_telefoniczna(powod_zakonczenia_rozmowy_odrzucenie);
    wykonywanie_rozmowy_telefonicznej = FALSE;
    licznik_bezpieczenstwa_wykonywana_rozmowa = 0;
    return;
  }

  if (czy_jest_komenda_telefon_zajety()) {
    POMOC_DODAJ2('#', 'g');
    if (aktualnie_wysylane_polecenie_SIM900 == KOMENDA_KOLEJKI_WYKONAJ_ROZMOWE)
      odpowiedz_na_polecenie();
    zakonczono_rozmowe_telefoniczna(powod_zakonczenia_rozmowy_odrzucenie);
    wykonywanie_rozmowy_telefonicznej = FALSE;
    licznik_bezpieczenstwa_wykonywana_rozmowa = 0;
    return;
  }

  if (czy_jest_komenda_nowy_SMS()) {
    const uchar *ptr = strchr(polozenie_polecenia_SIM900(PSTR("+CMTI:")), ',');
    if (ptr != NULL) {
      uint numer_smsa = (uint)strtoul(ptr + 1, NULL, 10);
      if (numer_smsa > 0 && numer_smsa <= 20) // Hardcoded 20
      {
        dodaj_komende(KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_1 + numer_smsa - 1);
        opoznienie_SIM900_100MS = 1; // 20 (1)
      }
    }
    if (aktualnie_wysylane_polecenie_SIM900 >=
            KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_1 &&
        aktualnie_wysylane_polecenie_SIM900 <=
            KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_20) {
      flaga_odczytywanie_smsa = FALSE;
      wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_BRAK_KOMENDY;
    }
    return;
  }

  if (czy_jest_komenda_otrzymano_sms_flash()) {
    opoznienie_SIM900_100MS = 20;
    if (aktualnie_wysylane_polecenie_SIM900 >=
            KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_1 &&
        aktualnie_wysylane_polecenie_SIM900 <=
            KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_20) {
      flaga_odczytywanie_smsa = FALSE;
      wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_BRAK_KOMENDY;
    }
    const uchar *p1 = strchr(polozenie_polecenia_SIM900(PSTR("+CUSD:")), '\"');
    if (p1++ != NULL) {
      uchar *p2 = strchr(p1, '\"');
      if (p2 != NULL && oczekiwanie_na_ussd) {
        // Parsuj odpowiedź USSD i wyślij SMS
        size_t len = p2 - p1;
        if (len > MAX_LICZBA_ZNAKOW_SMS)
          len = MAX_LICZBA_ZNAKOW_SMS;

        strncpy((char *)tekst_wysylanego_smsa, (char *)p1, len);
        tekst_wysylanego_smsa[len] = '\0';

        // Wyślij SMS z odpowiedzią do nadawcy oryginalnego SMS
        strcpy((char *)numer_telefonu_wysylanego_smsa,
               (char *)numer_telefonu_odebranego_smsa);
        dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT);

        // Wyczyść flagę oczekiwania
        oczekiwanie_na_ussd = FALSE;
        licznik_timeout_ussd_100ms = 0;
      }
    }
    return;
  }

  if (flaga_wysylanie_smsa)
    obsluga_wysylania_sms();

  if (liczba_odebranych_znakow_SIM900 != 0 &&
      komenda_SIM900 != BRAK_KOMENDY_SIM900)
    odpowiedz_na_polecenie();
}

uchar wyslanie_polecenia_ROM(const uchar czy_mozna_wysylac_dane_do_SIM900,
                             const komenda_typ wykonywana_komenda,
                             const komenda_typ nastepne_wysylane_polecenie,
                             PGM_P instrukcja) {
  if (!czy_mozna_wysylac_dane_do_SIM900) {
    dodaj_komende(wykonywana_komenda);
    return FALSE;
  }
  wysylane_polecenie_SIM900 = wykonywana_komenda;
  nastepne_wysylane_polecenie_SIM900 = nastepne_wysylane_polecenie;
  wyslij_polecenie_ROM_SIM900(instrukcja);
  return TRUE;
}

#ifndef memcpy_E
#define memcpy_E(sink, source, l)                                              \
  eeprom_read_block((sink), (void *)(source), (l))
#endif

void wyslij_sms(const uchar wyslij_pdu) {
  czekanie_na_odebranie_zachety = TRUE;
  flaga_odebrany_znak_zachety = FALSE;
  if (!wyslij_pdu) {
    // Zmiana: 5 blyskow LED przy wysylaniu SMS
    zapal_diode_led_blyski(5);
    flaga_wysylanie_smsa = KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT;
    bufor_pdu[0] = 0xff; // memcpy_E(bufor_pdu, (void*)EEPROM_ADRES_CENTRUM_SMS,
                         // LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM);
    konwertuj_blok_eeprom_na_telefon(
        bufor_pdu, bufor_eeprom, 2 * LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM);
    uchar *ptr = zapisz_naglowek_pdu(bufor_pdu, numer_telefonu_wysylanego_smsa,
                                     bufor_eeprom, 0x00, 0x00);
    dlugosc_pdu = ascii_to_pdu(ptr, tekst_wysylanego_smsa) - bufor_pdu;
#ifdef WLACZ_TESTY_OGOLNE
    uchar l = strlen(numer_telefonu_wysylanego_smsa);
    for (uchar i = 0; i < l; ++i)
      POMOC_DODAJ(numer_telefonu_wysylanego_smsa[i]);
    POMOC_DODAJ(' ');
    l = strlen(tekst_wysylanego_smsa);
    if (l > 20)
      l = 20;
    for (uchar i = 0; i < l; ++i)
      POMOC_DODAJ(tekst_wysylanego_smsa[i]);
#endif
  } else {
    // Zmiana: 5 blyskow LED przy wysylaniu PDU
    zapal_diode_led_blyski(5);
    flaga_wysylanie_smsa = KOMENDA_KOLEJKI_WYSLIJ_SMSA_PDU;
    const uchar TP_DCS = tekst_wysylanego_smsa[0]; // message class
    bufor_pdu[0] = 0xff; // memcpy_E(bufor_pdu, (void*)EEPROM_ADRES_CENTRUM_SMS,
                         // LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM);
    konwertuj_blok_eeprom_na_telefon(
        bufor_pdu, bufor_eeprom, 2 * LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM);
    uchar *ptr = zapisz_naglowek_pdu(bufor_pdu, numer_telefonu_wysylanego_smsa,
                                     bufor_eeprom, 0x00, TP_DCS);
    memcpy(ptr, &tekst_wysylanego_smsa[2], tekst_wysylanego_smsa[1]);
    dlugosc_pdu = (ptr + tekst_wysylanego_smsa[1]) - bufor_pdu;
#ifdef WLACZ_TESTY_OGOLNE
    uchar l = strlen(numer_telefonu_wysylanego_smsa);
    for (uchar i = 0; i < l; ++i)
      POMOC_DODAJ(numer_telefonu_wysylanego_smsa[i]);
    POMOC_DODAJ(' ');
    POMOC_DODAJ('P');
    POMOC_DODAJ('D');
    POMOC_DODAJ('U');
#endif
  }
  const uchar oktety = dlugosc_pdu - 1 - bufor_pdu[0];
  static const char instrukcja_wyslij_smsa[] PROGMEM = "+cmgs=";
  memcpy_R(wysylane_dane_RAM_SIM900, instrukcja_wyslij_smsa);
  utoa(oktety, wysylane_dane_RAM_SIM900 + strlen_R(instrukcja_wyslij_smsa), 10);
  max_oczekiwanie_na_odpowiedz = 250;
  wyslij_polecenie_RAM_SIM900();
  POMOC_DODAJ2('#', 'S');
}

uchar wykonanie_komend_SIM900(void) {
  const uchar czy_gsm_zajety =
      flaga_odczytywanie_smsa || flaga_wysylanie_smsa ||
      wykonywanie_rozmowy_telefonicznej ||
      trwa_rozmowa_przychodzaca_od_uzytkownika ||
      aktualnie_wysylane_polecenie_SIM900 != KOMENDA_KOLEJKI_BRAK_KOMENDY ||
      opoznienie_SIM900_100MS || czekanie_na_odebranie_zachety;

  const uchar czy_mozna_wysylac_dane_do_SIM900 =
      !czy_gsm_zajety && CZY_MOZNA_WYSYLAC_DANE_SIM900() &&
      czy_jest_bezczynny_SIM900();

  const komenda_typ wykonywana_komenda = komendy_kolejka[0];
  switch (wykonywana_komenda) {
  case KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT:
  case KOMENDA_KOLEJKI_WYSLIJ_SMSA_PDU: {
    POWTORZ_JESLI(flaga_trwa_rozmowa_wychodzaca || opoznienie_SIM900_100MS);
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    wyslij_sms(wykonywana_komenda == KOMENDA_KOLEJKI_WYSLIJ_SMSA_PDU);
    break;
  }
  case KOMENDA_KOLEJKI_WYSLIJ_PDU: {
    czekanie_na_odebranie_zachety = FALSE;
    konwertuj_pdu_na_blok_wysylany(wysylany_blok_SIM900, bufor_pdu,
                                   dlugosc_pdu);
    const uint liczba_znakow = 2 * dlugosc_pdu + 1;
    wysylany_blok_SIM900[liczba_znakow - 1] = ZNAK_CTRL_Z;
    wyslij_znaki_SIM900(liczba_znakow);
    break;
  }
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_1:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_2:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_3:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_4:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_5:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_6:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_7:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_8:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_9:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_10:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_11:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_12:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_13:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_14:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_15:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_16:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_17:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_18:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_19:
  case KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_20: {
    POWTORZ_JESLI(flaga_odczytywanie_smsa || !modul_zalogowany_w_sieci ||
                  flaga_trwa_rozmowa_wychodzaca || opoznienie_SIM900_100MS ||
                  wysylanie_smsa_clipa != WYSYLANIE_SMSA_CLIPA_BRAK ||
                  flaga_wysylanie_smsa ||
                  aktualnie_wysylane_polecenie_SIM900 ==
                      KOMENDA_KOLEJKI_USUN_WSZYSTKIE_SMSY);
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
    POWTORZ_JESLI(czy_sa_komendy_z_przedzialu(
                      KOMENDA_KOLEJKI_WYSLIJ_SMSA_I_CLIP_DO_UZYTKOWNIKOW,
                      KOMENDA_KOLEJKI_WYSLIJ_SMSA_I_CLIP_DO_UZYTKOWNIKOW) ||
                  czy_sa_komendy_z_przedzialu(KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT,
                                              KOMENDA_KOLEJKI_WYSLIJ_PDU) ||
                  czy_sa_komendy_z_przedzialu(
                      KOMENDA_KOLEJKI_ODBIERZ_ROZMOWE,
                      KOMENDA_KOLEJKI_SPRAWDZ_PRZYCHODZACA_ROZMOWE));
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    flaga_odczytywanie_smsa = TRUE;
    static const char instrukcja_odczytaj_smsa[] PROGMEM = "+cmgr=";
    memcpy_R(wysylane_dane_RAM_SIM900, instrukcja_odczytaj_smsa);
    const uint nr_smsa =
        wykonywana_komenda - KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_1 + 1;
    utoa(nr_smsa, wysylane_dane_RAM_SIM900 + strlen_R(instrukcja_odczytaj_smsa),
         10);
    wyslij_polecenie_RAM_SIM900();
    POMOC_DODAJ2('#', 'X');
    break;
  }
  case KOMENDA_KOLEJKI_INTERPRETUJ_PDU: {
    const uchar *ptr = pobierz_numer_telefonu_nadawcy_z_PDU(
        bufor_pdu, numer_telefonu_odebranego_smsa,
        &nie_wysylaj_echa_z_powodu_nietypowego_smsa);
    if (ptr == 0) // (1)
    {
      flaga_odczytywanie_smsa = FALSE;
      break;
    }
    {
      uchar rok, miesiac, dzien;
      ptr = pobierz_date_z_PDU(ptr, &rok, &miesiac, &dzien);
      sms_timestamp_rok = rok;
      sms_timestamp_miesiac = miesiac;
      sms_timestamp_dzien = dzien;
    }
    {
      uchar godzina, minuta, sekunda;
      ptr = pobierz_czas_z_PDU(ptr, &godzina, &minuta, &sekunda);

      // Zapisz timestamp z SMS do późniejszego użycia
      // (zostanie użyty w wykonanie_polecenia_sms jeśli kod ABCD jest poprawny)
      sms_timestamp_godzina = godzina;
      sms_timestamp_minuta = minuta;
      extern uchar sms_timestamp_sekunda;
      sms_timestamp_sekunda = sekunda;
    }
    // ptr_start_pdu_z_wiadomoscia = (uchar *)ptr; // REMOVED
    pdu_to_ascii(ptr, tekst_odebranego_smsa, MAX_LICZBA_ZNAKOW_SMS_ODBIOR + 1);
#ifdef WLACZ_TESTY_OGOLNE
    const uchar l = min(strlen(tekst_odebranego_smsa), 20);
    for (uchar i = 0; i < l; ++i)
      POMOC_DODAJ(tekst_odebranego_smsa[i]);
#endif
    dodaj_komende(KOMENDA_KOLEJKI_INTERPRETUJ_ODEBRANEGO_SMSA);
    break;
  }
  case KOMENDA_KOLEJKI_INTERPRETUJ_ODEBRANEGO_SMSA: {
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
    // SMS: krótki impuls LED, bez długiego podtrzymania.
    liczba_blyskow_led = 0;
    zapal_diode_led(1);
    wykonanie_polecenia_sms();
    flaga_odczytywanie_smsa = FALSE;
    break;
  }
  case KOMENDA_KOLEJKI_USUN_SMSA_1:
  case KOMENDA_KOLEJKI_USUN_SMSA_2:
  case KOMENDA_KOLEJKI_USUN_SMSA_3:
  case KOMENDA_KOLEJKI_USUN_SMSA_4:
  case KOMENDA_KOLEJKI_USUN_SMSA_5:
  case KOMENDA_KOLEJKI_USUN_SMSA_6:
  case KOMENDA_KOLEJKI_USUN_SMSA_7:
  case KOMENDA_KOLEJKI_USUN_SMSA_8:
  case KOMENDA_KOLEJKI_USUN_SMSA_9:
  case KOMENDA_KOLEJKI_USUN_SMSA_10:
  case KOMENDA_KOLEJKI_USUN_SMSA_11:
  case KOMENDA_KOLEJKI_USUN_SMSA_12:
  case KOMENDA_KOLEJKI_USUN_SMSA_13:
  case KOMENDA_KOLEJKI_USUN_SMSA_14:
  case KOMENDA_KOLEJKI_USUN_SMSA_15:
  case KOMENDA_KOLEJKI_USUN_SMSA_16:
  case KOMENDA_KOLEJKI_USUN_SMSA_17:
  case KOMENDA_KOLEJKI_USUN_SMSA_18:
  case KOMENDA_KOLEJKI_USUN_SMSA_19:
  case KOMENDA_KOLEJKI_USUN_SMSA_20: {
    POWTORZ_JESLI(flaga_odczytywanie_smsa || flaga_trwa_rozmowa_wychodzaca ||
                  opoznienie_zatrzymaj_odpytywanie_urzadzenia ||
                  czekanie_na_odebranie_zachety)
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    static const char instrukcja_usun_smsa[] PROGMEM = "+cmgd=";
    memcpy_R(wysylane_dane_RAM_SIM900, instrukcja_usun_smsa);
    const uint nr_smsa = wykonywana_komenda - KOMENDA_KOLEJKI_USUN_SMSA_1 + 1;
    utoa(nr_smsa, wysylane_dane_RAM_SIM900 + strlen_R(instrukcja_usun_smsa),
         10);
    wyslij_polecenie_RAM_SIM900();
    POMOC_DODAJ2('#', 'Y');
    break;
  }
  case KOMENDA_KOLEJKI_SPRAWDZ_PRZYCHODZACA_ROZMOWE: {
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
    liczba_blyskow_led = 0;
    zapal_diode_led(1);

    uchar status_ok =
        sprawdz_przychodzaca_rozmowe(); // Returns TRUE if number authorized
    const uchar mozna_odebrac = (!tryb_clip && !sms_trigger);

    if (status_ok) {
      if (!mozna_odebrac) {
        // Tryb CLIP: wlacz wyjscie i ODRZUC polaczenie
        if (polozenie_polecenia_SIM900(PSTR("+CMTI:")) == NULL) {
          ustaw_wyjscie_clip();
        }
        dodaj_komende(KOMENDA_KOLEJKI_ODRZUC_ROZMOWE);
      } else {
        // Tryb DTMF: ODBIERZ polaczenie
        dodaj_komende(KOMENDA_KOLEJKI_ODBIERZ_ROZMOWE);
      }
    } else {
      // Status NIE OK - odrzuc
      dodaj_komende(KOMENDA_KOLEJKI_ODRZUC_ROZMOWE);
    }
    blokada_clip = TRUE;
    break;
  }
  case KOMENDA_KOLEJKI_ODRZUC_ROZMOWE: {
    // Nie odkładaj ATH "za długo" – to psuje odrzucanie CLIP.
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    // Po ATH: tylko krótki błysk LED (200 ms), bez długiego świecenia.
    liczba_blyskow_led = 0;
    zapal_diode_led(1);
    static const char instrukcja_odrzuc_rozmowe[] PROGMEM = "h";
    wyslij_polecenie_ROM_SIM900(instrukcja_odrzuc_rozmowe);
    break;
  }
  case KOMENDA_KOLEJKI_ODBIERZ_ROZMOWE: {
    // Odbieranie rozmowy dozwolone tylko w czystym DTMF.
    if (tryb_clip || sms_trigger) {
      dodaj_komende(KOMENDA_KOLEJKI_ODRZUC_ROZMOWE);
      break;
    }
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    static const char instrukcja_odbierz_rozmowe[] PROGMEM = "a";
    wyslij_polecenie_ROM_SIM900(instrukcja_odbierz_rozmowe);
    POMOC_DODAJ2('#', 'p');
    break;
  }
  case KOMENDA_KOLEJKI_WYSLIJ_TON_DTMF: {
    // Czekaj na koncowke inicjalizacji polaczenia/opoznienia?
    POWTORZ_JESLI(opoznienie_SIM900_100MS);
    if (!CZY_MOZNA_WYSYLAC_DANE_SIM900() || !czy_jest_bezczynny_SIM900() ||
        aktualnie_wysylane_polecenie_SIM900 != KOMENDA_KOLEJKI_BRAK_KOMENDY) {
      dodaj_komende(wykonywana_komenda);
      break;
    }
    wysylane_polecenie_SIM900 = wykonywana_komenda;
    static const char instrukcja_vts[] PROGMEM = "+vts=1";
    wyslij_polecenie_ROM_SIM900(instrukcja_vts);
    POMOC_DODAJ2('#', 'V');
    break;
  }
  case KOMENDA_KOLEJKI_WYKONAJ_ROZMOWE: {
    POWTORZ_JESLI(!modul_zalogowany_w_sieci || opoznienie_SIM900_100MS);
    if (wykonywanie_rozmowy_telefonicznej)
      break;
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    zapal_diode_led(20);
    wysylane_dane_RAM_SIM900[0] = 'd';
    // static const char doladowanie[] PROGMEM = "*100*59484869838559#";
    // strcpy_P(numer_telefonu_do_ktorego_dzwonic, doladowanie);
    strcpy(wysylane_dane_RAM_SIM900 + 1, numer_telefonu_do_ktorego_dzwonic);
    uchar l = strlen(numer_telefonu_do_ktorego_dzwonic) + 1;
    wysylane_dane_RAM_SIM900[l] = ';';
    wysylane_dane_RAM_SIM900[l + 1] = '\0';
    wyslij_polecenie_RAM_SIM900();
    licznik_bezpieczenstwa_wykonywana_rozmowa = 300;
    POMOC_DODAJ2('#', 'h');
#ifdef WLACZ_TESTY_OGOLNE
    for (uchar i = 0; i < l; ++i)
      POMOC_DODAJ(numer_telefonu_do_ktorego_dzwonic[i]);
#endif
    break;
  }
  case KOMENDA_KOLEJKI_SPRAWDZ_POZIOM_SIECI: {
    if (flaga_trwa_rozmowa_wychodzaca ||
        opoznienie_zatrzymaj_odpytywanie_urzadzenia ||
        czekanie_na_odebranie_zachety || czy_gsm_zajety ||
        wysylanie_smsa_clipa != WYSYLANIE_SMSA_CLIPA_BRAK) {
      if (poziom_sieci_gsm == POZIOM_SIECI_BLAD ||
          poziom_sieci_gsm == 0) // poziom sieci bez zmian
        poziom_sieci_gsm = 16;
      break;
    }
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    static const char instrukcja_poziom_sieci[] PROGMEM = "+csq";
    wyslij_polecenie_ROM_SIM900(instrukcja_poziom_sieci);
    break;
  }
  case KOMENDA_KOLEJKI_SPRAWDZ_ZAREJESTROWANIE: {
    // Usunięto warunek !modul_zalogowany_w_sieci - musimy sprawdzać ZAWSZE!
    if (flaga_trwa_rozmowa_wychodzaca ||
        opoznienie_zatrzymaj_odpytywanie_urzadzenia ||
        czekanie_na_odebranie_zachety || czy_gsm_zajety ||
        wysylanie_smsa_clipa != WYSYLANIE_SMSA_CLIPA_BRAK) {
      break;
    }
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    static const char instrukcja_creg[] PROGMEM = "+creg?";
    wyslij_polecenie_ROM_SIM900(instrukcja_creg);
    break;
  }
  case KOMENDA_KOLEJKI_SPRAWDZ_STAN_KARTY_SIM: {
    if (flaga_trwa_rozmowa_wychodzaca ||
        opoznienie_zatrzymaj_odpytywanie_urzadzenia ||
        czekanie_na_odebranie_zachety || czy_gsm_zajety ||
        wysylanie_smsa_clipa != WYSYLANIE_SMSA_CLIPA_BRAK) {
      licznik_blad_stanu_karty_SIM = 0;
      break;
    }
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    static const char instrukcja_odczyt_ksiazki[] PROGMEM = "+cpbr=26";
    wyslij_polecenie_ROM_SIM900(instrukcja_odczyt_ksiazki);
    break;
  }
  case KOMENDA_KOLEJKI_SPRAWDZ_ZALOGOWANIE_U_OPERATORA: {
    if (flaga_trwa_rozmowa_wychodzaca ||
        opoznienie_zatrzymaj_odpytywanie_urzadzenia ||
        czekanie_na_odebranie_zachety || czy_gsm_zajety ||
        wysylanie_smsa_clipa != WYSYLANIE_SMSA_CLIPA_BRAK) {
      licznik_blad_zalogowania_u_operatora = 0;
      break;
    }
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    static const char instrukcja_sprawdz_operatora[] PROGMEM = "+cops?";
    wyslij_polecenie_ROM_SIM900(instrukcja_sprawdz_operatora);
    break;
  }
  case KOMENDA_KOLEJKI_WYSLIJ_DO_SIM900_ODCZYTAJ_GODZINE: {
    if (flaga_trwa_rozmowa_wychodzaca ||
        opoznienie_zatrzymaj_odpytywanie_urzadzenia ||
        czekanie_na_odebranie_zachety || czy_gsm_zajety ||
        wysylanie_smsa_clipa != WYSYLANIE_SMSA_CLIPA_BRAK)
      break;
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    static const char instrukcja_cclk[] PROGMEM = "+cclk?";
    wyslij_polecenie_ROM_SIM900(instrukcja_cclk);
    break;
  }
  case KOMENDA_KOLEJKI_RESETUJ_CELL_BROADCAST_SMS: {
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    static const char instrukcja_ustawienie_pamieci[] PROGMEM = "+cdscb";
    wyslij_polecenie_ROM_SIM900(instrukcja_ustawienie_pamieci);
    break;
  }
  case KOMENDA_KOLEJKI_USTAW_PAMIEC_SM_DLA_SMSOW: {
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    static const char instrukcja_ustawienie_pamieci[] PROGMEM =
        "+cpms=\"SM\",\"SM\",\"SM\"";
    nastepne_wysylane_polecenie_SIM900 =
        KOMENDA_KOLEJKI_WSTAWIENIE_MORING_GDY_DZWONI;
    wyslij_polecenie_ROM_SIM900(instrukcja_ustawienie_pamieci);
    break;
  }
  case KOMENDA_KOLEJKI_PODLACZONY_MODUL_GSM: {
    dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_DO_SIM900_AT);
    opoznienie_zatrzymaj_odpytywanie_urzadzenia = 20;
    ustaw_odbior_SIM900();
    break;
  }
  case KOMENDA_KOLEJKI_WYSLIJ_DO_SIM900_AT: { // po wczeniu zasilania wysya 2
                                              // razy, bo za pierwszym razem nie
                                              // dziaa kontrola RTS / CTS w
                                              // SIM800
    if (max_oczekiwanie_na_odpowiedz_at)
      break;
    if (opoznienie_zatrzymaj_odpytywanie_urzadzenia) {
      dodaj_komende(wykonywana_komenda);
      break;
    }
    if (CZY_WYSYLANIE_DANYCH_SIM900()) {
      dodaj_komende(wykonywana_komenda);
      break;
    }
    ustaw_odbior_SIM900();
    wysylane_polecenie_SIM900 = wykonywana_komenda;
    max_oczekiwanie_na_odpowiedz_at = 20;
    static const char instrukcja_at[] PROGMEM = "+ifc=2,2";
    wyslij_polecenie_ROM_SIM900(instrukcja_at);
    break;
  }
  case KOMENDA_KOLEJKI_ODEBRANO_POLECENIE_ROZPOCZYNAJACE: {
    podlaczony_modul_gsm_SIM900 = TRUE;
    dodaj_komende(KOMENDA_KOLEJKI_SPRAWDZ_PIN);
    opoznienie_SIM900_100MS = 40;
    break;
  }
  case KOMENDA_KOLEJKI_PODAJ_PIN: {
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    static const char instrukcja_podaj_pin[] PROGMEM = "+cpin=\"1976\"";
    strcpy_P(wysylane_dane_RAM_SIM900, instrukcja_podaj_pin);
    wyslij_polecenie_RAM_SIM900();
    break;
  }
  case KOMENDA_KOLEJKI_USUN_WSZYSTKIE_SMSY: {
    static uchar drugi_raz = FALSE;
    if (drugi_raz) {
      dodaj_komende(KOMENDA_KOLEJKI_USTAW_ODBIERANIE_SMSOW);
      break;
    }
    drugi_raz = TRUE;
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    static const char instrukcja_usun_wszystkie_smsy[] PROGMEM = "+cmgda=6";
    wyslij_polecenie_ROM_SIM900(instrukcja_usun_wszystkie_smsy);
    break;
  }
  case KOMENDA_KOLEJKI_KOMENDA_SIM900: {
    obsluga_komendy_SIM900();
    if (komenda_SIM900 != KOMENDA_SIM900_WYSLANE_POLECENIE)
      max_oczekiwanie_na_odpowiedz = 20;
    resetuj_komende_SIM900();
    break;
  }
  case KOMENDA_KOLEJKI_USTAW_WZMOCNIENIE_MIKROFONU: {
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    static const char instrukcja[] PROGMEM =
        "+cmic=0,15"; // ustawienie od 0 ... 15
    strcpy_P(wysylane_dane_RAM_SIM900, instrukcja);
    nastepne_wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_ROZPOCZNIJ_PRACE;
    wyslij_polecenie_RAM_SIM900();
    break;
  }
  case KOMENDA_KOLEJKI_ROZPOCZNIJ_PRACE: {
    POMOC_DODAJ2('*', 'h');
    if (podlaczona_karta_SIM_SIM900) {
      dodaj_komende(KOMENDA_KOLEJKI_RESETUJ_CELL_BROADCAST_SMS);
      dodaj_komende(KOMENDA_KOLEJKI_MODUL_ZALOGOWANY);
    }
    break;
  }
  case KOMENDA_KOLEJKI_MODUL_ZALOGOWANY: {
    // Nie ustawiamy modul_zalogowany_w_sieci tutaj!
    // Flaga jest ustawiana tylko przez +CREG gdy faktycznie zalogowany
    break;
  }
  case KOMENDA_KOLEJKI_USTAW_ZEGAR_SIM900: {
    JESLI_EEPROM_ZAJETY_WYKONAJ_POZNIEJ();
    JESLI_SIM900_WYSYLA_DANE_WYKONAJ_POZNIEJ();
    strcpy(wysylane_dane_RAM_SIM900, bufor_ustaw_czas);
    wyslij_polecenie_RAM_SIM900();
    break;
  }
  case KOMENDA_KOLEJKI_SPRAWDZ_PIN:
  case KOMENDA_KOLEJKI_USTAW_ODBIERANIE_SMSOW:
  case KOMENDA_KOLEJKI_WSTAWIENIE_MORING_GDY_DZWONI:
  case KOMENDA_KOLEJKI_USTAW_WYSYLANIE_INFO_O_DZWONIACYM:
  case KOMENDA_KOLEJKI_WLACZ_DETEKCJE_DTMF:
  case KOMENDA_KOLEJKI_USTAW_CICHY_TRYB:
  case KOMENDA_KOLEJKI_WYCISZ_DZWONKI:
  case KOMENDA_KOLEJKI_ODBIERAJ_SMS_FLASH:
  case KOMENDA_KOLEJKI_WPISZ_POZYCJE_1_W_KSIAZCE_TELEFONICZNEJ:
  case KOMENDA_KOLEJKI_WLACZ_CZAS_Z_SIECI: {

#define MAX_ROZMIAR_POLECENIA_GSM 14

    static const char instrukcje[10][MAX_ROZMIAR_POLECENIA_GSM] PROGMEM = {
        "+cpin?",         "+cnmi=2,1,2,1", "+moring=1", "+clip=1",
        "+ddet=1,10,1",   "+calm=1",       "+crsl=1",   "+cusd=1",
        "+cpbw=26,\"1\"", "+clts=0",
    };
    static const komenda_typ nastepna_komenda[10] PROGMEM = {
        KOMENDA_KOLEJKI_BRAK_KOMENDY,
        KOMENDA_KOLEJKI_BRAK_KOMENDY,
        KOMENDA_KOLEJKI_USTAW_WYSYLANIE_INFO_O_DZWONIACYM,
        KOMENDA_KOLEJKI_WLACZ_DETEKCJE_DTMF,
        KOMENDA_KOLEJKI_USTAW_CICHY_TRYB,
        KOMENDA_KOLEJKI_WYCISZ_DZWONKI,
        KOMENDA_KOLEJKI_ODBIERAJ_SMS_FLASH,
        KOMENDA_KOLEJKI_WPISZ_POZYCJE_1_W_KSIAZCE_TELEFONICZNEJ,
        KOMENDA_KOLEJKI_WLACZ_CZAS_Z_SIECI,
        KOMENDA_KOLEJKI_USTAW_WZMOCNIENIE_MIKROFONU,
    };
    // RE-DOING THIS CHUNK TO BE SAFE AND CORRECT

    const uchar nr = wykonywana_komenda - KOMENDA_KOLEJKI_SPRAWDZ_PIN;
    if (!wyslanie_polecenia_ROM(
            czy_mozna_wysylac_dane_do_SIM900, wykonywana_komenda,
            pgm_read_word(&nastepna_komenda[nr]), &instrukcje[nr][0]))
      break;
    POMOC_DODAJ2('*', 's');
    POMOC_DODAJ_HEX(nr);
    break;
  }
  default:
    return FALSE;
  }
  return TRUE;
}

void steruj_SIM900_100MS(void) {
  if (opoznienie_SIM900_100MS && --opoznienie_SIM900_100MS == 0)
    oproznij_bufor_SIM900_po_bledzie = FALSE;

  if (opoznienie_wysylania_clipow_100MS)
    --opoznienie_wysylania_clipow_100MS;

  // Timeout rozmowy - automatyczne rozlaczenie po 30 sekundach
  if (licznik_timeout_rozmowy_100ms) {
    if (--licznik_timeout_rozmowy_100ms == 0 &&
        trwa_rozmowa_przychodzaca_od_uzytkownika) {
      dodaj_komende(KOMENDA_KOLEJKI_ODRZUC_ROZMOWE);
      POMOC_DODAJ2('#', 'X'); // Timeout marker
    }
  }

  if (max_oczekiwanie_na_odpowiedz_at) {
    if (--max_oczekiwanie_na_odpowiedz_at == 0) {
      wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_BRAK_KOMENDY;
      dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_DO_SIM900_AT);
      sprawdzaj_wejscie_CTS_SIM900 = FALSE;
      zakoncz_wysylanie_SIM900();
    }
  }

  if (procedura_inicjalizacyjna_SIM900_100MS())
    dodaj_komende(KOMENDA_KOLEJKI_PODLACZONY_MODUL_GSM);

  if (maksymalny_czas_dzwonienia) {
    if (--maksymalny_czas_dzwonienia == 0) {
      POMOC_DODAJ2('#', 'i');
      dodaj_komende(KOMENDA_KOLEJKI_ODRZUC_ROZMOWE);
    }
  }

  if (licznik_bezpieczenstwa_wykonywana_rozmowa) {
    if (--licznik_bezpieczenstwa_wykonywana_rozmowa == 0) {
      POMOC_DODAJ2('#', 'j');
      zakonczono_rozmowe_telefoniczna(
          powod_zakonczenia_rozmowy_przekroczony_czas);
      trwa_rozmowa_przychodzaca_od_uzytkownika = FALSE;
      wykonywanie_rozmowy_telefonicznej = FALSE;
    }
  }

#ifndef WYLACZ_AUTOMATYCZNE_KONTROLOWANIE_SIMCOM

  { // poziom sieci i zalogowanie u operatora
    static uchar licznik_cyklu_8_sek = 0;
    if (++licznik_cyklu_8_sek == 48)
      filtruj_i_dodaj_komende(KOMENDA_KOLEJKI_SPRAWDZ_POZIOM_SIECI);
    else if (licznik_cyklu_8_sek == 52)
      filtruj_i_dodaj_komende(KOMENDA_KOLEJKI_SPRAWDZ_ZAREJESTROWANIE);
    else if (licznik_cyklu_8_sek == 56)
      filtruj_i_dodaj_komende(KOMENDA_KOLEJKI_SPRAWDZ_ZALOGOWANIE_U_OPERATORA);
    else if (licznik_cyklu_8_sek == 60) {
      filtruj_i_dodaj_komende(KOMENDA_KOLEJKI_SPRAWDZ_STAN_KARTY_SIM);
    } else if (licznik_cyklu_8_sek >= 80) {
      licznik_cyklu_8_sek = 0;
      if (modul_zalogowany_w_sieci)
        filtruj_i_dodaj_komende(
            KOMENDA_KOLEJKI_WYSLIJ_DO_SIM900_ODCZYTAJ_GODZINE);
    }
  }

  if (opoznienie_zatrzymaj_odpytywanie_urzadzenia)
    --opoznienie_zatrzymaj_odpytywanie_urzadzenia;

  if (modul_zalogowany_w_sieci &&
      (poziom_sieci_gsm == POZIOM_SIECI_BLAD || poziom_sieci_gsm == 0)) {
    if (licznik_awaria_brak_zasiegu < MAX_LICZNIK_AWARIA_BRAK_ZASIEGU) {
      if (++licznik_awaria_brak_zasiegu == MAX_LICZNIK_AWARIA_BRAK_ZASIEGU) {
        licznik_awaria_brak_zasiegu = 0;
        POMOC_DODAJ2('*', 'j');
        reset_modulu_SIM900();
      }
    }
  } else
    licznik_awaria_brak_zasiegu = 0;

  if (CZY_BLAD_KARTY_SIM() || CZY_BLAD_ZALOGOWANIA_U_OPERATORA()) {
    POMOC_DODAJ2('*', 'k');
    reset_modulu_SIM900();
  }
#endif

  if (licznik_reset_modulu_SIM900) {
    if (--licznik_reset_modulu_SIM900 == 0) {
      POMOC_DODAJ2('*', 'l');
      reset_modulu_SIM900();
    }
  }

  if (!modul_zalogowany_w_sieci) {
    if (++licznik_ogolny_blad_zalogowania > MAX_LICZNIK_OGOLNY_BLAD_ZALOGOWANIA)
      reset_modulu_SIM900();
  } else
    licznik_ogolny_blad_zalogowania = 0;
}

void steruj_SIM900_10MS(void) {
  if (czy_jest_komenda_SIM900())
    dodaj_komende(KOMENDA_KOLEJKI_KOMENDA_SIM900);

  if (flaga_odebrany_znak_zachety) {
    dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_PDU);
    // czekanie_na_odebranie_zachety = FALSE;	// byo
    flaga_odebrany_znak_zachety = FALSE;
    POMOC_DODAJ2('#', 'm');
  }

  { // kontrola wysyania sms-w i clipw
    static uint licznik_bezpieczenstwa = 0;
    if (modul_zalogowany_w_sieci && flaga_wysylanie_smsa) {
      if (++licznik_bezpieczenstwa >= 5000) // 50 sek
      {
        POMOC_DODAJ2('#', 'Z');
        licznik_bezpieczenstwa = 0;
        komenda_SIM900 = ERROR_KOMENDA_SIM900;
        obsluga_wysylania_sms();
      }
    } else
      licznik_bezpieczenstwa = 0;
  }

  { // kontrola programu: zbyt dugi okres wykonywania polecenia do SIM900 lub
    // jego brak
    static komenda_typ poprzednia_komenda;
    static uint licznik_poprzednia_komenda;
    static uint licznik_odebranych_znakow;
    if (aktualnie_wysylane_polecenie_SIM900 != KOMENDA_KOLEJKI_BRAK_KOMENDY &&
        aktualnie_wysylane_polecenie_SIM900 == poprzednia_komenda &&
        !trwa_rozmowa_przychodzaca_od_uzytkownika) {
      uint max_licznik_poprzednia_komenda = 5 * 100; // 5 sek;
      if (aktualnie_wysylane_polecenie_SIM900 ==
              KOMENDA_KOLEJKI_WYSLIJ_DO_SIM900_AT ||
          aktualnie_wysylane_polecenie_SIM900 == KOMENDA_KOLEJKI_WYSLIJ_PDU ||
          aktualnie_wysylane_polecenie_SIM900 ==
              KOMENDA_KOLEJKI_WYKONAJ_ROZMOWE)
        max_licznik_poprzednia_komenda = 2 * 60 * 100; // 2 minuty

      if (aktualnie_wysylane_polecenie_SIM900 ==
              KOMENDA_KOLEJKI_WYSLIJ_SMSA_TEXT // zmienione (1)
          || aktualnie_wysylane_polecenie_SIM900 ==
                 KOMENDA_KOLEJKI_WYSLIJ_SMSA_PDU)
        max_licznik_poprzednia_komenda = 15 * 100;

      if (++licznik_poprzednia_komenda >= max_licznik_poprzednia_komenda) {
        licznik_poprzednia_komenda = 0;
        POMOC_DODAJ2('#', 'r');
        POMOC_DODAJ_HEX(aktualnie_wysylane_polecenie_SIM900 >> 8);
        POMOC_DODAJ_HEX(aktualnie_wysylane_polecenie_SIM900);
        if (aktualnie_wysylane_polecenie_SIM900 !=
            KOMENDA_KOLEJKI_ODZYSKIWANIE_URZADZENIA_1) {
          if (aktualnie_wysylane_polecenie_SIM900 ==
                  KOMENDA_KOLEJKI_ODRZUC_ROZMOWE ||
              aktualnie_wysylane_polecenie_SIM900 ==
                  KOMENDA_KOLEJKI_PODAJ_PIN ||
              aktualnie_wysylane_polecenie_SIM900 ==
                  KOMENDA_KOLEJKI_USTAW_PAMIEC_SM_DLA_SMSOW ||
              aktualnie_wysylane_polecenie_SIM900 ==
                  KOMENDA_KOLEJKI_USTAW_WZMOCNIENIE_MIKROFONU ||
              aktualnie_wysylane_polecenie_SIM900 ==
                  KOMENDA_KOLEJKI_RESETUJ_CELL_BROADCAST_SMS ||
              (aktualnie_wysylane_polecenie_SIM900 >=
                   KOMENDA_KOLEJKI_SPRAWDZ_PIN &&
               aktualnie_wysylane_polecenie_SIM900 <=
                   KOMENDA_KOLEJKI_WPISZ_POZYCJE_1_W_KSIAZCE_TELEFONICZNEJ) ||
              (aktualnie_wysylane_polecenie_SIM900 >=
                   KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_1 &&
               aktualnie_wysylane_polecenie_SIM900 <=
                   KOMENDA_KOLEJKI_ODCZYTAJ_SMSA_20) ||
              (aktualnie_wysylane_polecenie_SIM900 >=
                   KOMENDA_KOLEJKI_USUN_SMSA_1 &&
               aktualnie_wysylane_polecenie_SIM900 <=
                   KOMENDA_KOLEJKI_USUN_SMSA_20)) {
            nastepne_wysylane_polecenie_SIM900 =
                aktualnie_wysylane_polecenie_SIM900;
          } else
            nastepne_wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_BRAK_KOMENDY;
          wyzerowanie_danych_SIM900();
          wysylane_polecenie_SIM900 = KOMENDA_KOLEJKI_ODZYSKIWANIE_URZADZENIA_1;
          wysylane_dane_RAM_SIM900[0] = '\0';
          wyslij_polecenie_RAM_SIM900();
        } else
          reset_modulu_SIM900();
      }
      licznik_odebranych_znakow = 0;
    } else if (aktualnie_wysylane_polecenie_SIM900 !=
                   KOMENDA_KOLEJKI_BRAK_KOMENDY &&
               trwa_rozmowa_przychodzaca_od_uzytkownika &&
               czy_jest_bezczynny_SIM900()) {
      {
        cli();
        const uint l = liczba_odebranych_znakow_SIM900;
        sei();
        if (licznik_odebranych_znakow < l)
          licznik_odebranych_znakow = l;
      }
      if (licznik_odebranych_znakow >=
          MIN_LICZBA_ODEBRANYCH_ZNAKOW_KONCZACYCH_ROZMOWE_W_TRAKCIE_PODSLUCHU) {
        if (++licznik_poprzednia_komenda >= 5 * 100) // 5 sek
        {
          licznik_poprzednia_komenda = 0;
          zakonczono_rozmowe_telefoniczna(
              powod_zakonczenia_rozmowy_zakonczenie);
          licznik_odebranych_znakow = 0;
          POMOC_DODAJ2('#', 'q');
          reset_modulu_SIM900();
        }
      }
    } else {
      licznik_poprzednia_komenda = 0;
      if (!trwa_rozmowa_przychodzaca_od_uzytkownika)
        licznik_odebranych_znakow = 0;
    }

    poprzednia_komenda = aktualnie_wysylane_polecenie_SIM900;
  }
}
