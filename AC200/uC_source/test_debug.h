#ifdef DEBUG

void debug_reset(void)
{
	steruj_wejscia_10ms();
	steruj_wejscia_wyjscia_100ms();
	wykonanie_komend();
	zapisz_bajt_w_EEPROM();
}

void debug_generuj_eeprom(void)
{
	eeprom_write_byte((void*) ADRES_EEPROM_KOD_DOSTEPU + 0, 'A');
	eeprom_write_byte((void*) ADRES_EEPROM_KOD_DOSTEPU + 1, 'B');
	eeprom_write_byte((void*) ADRES_EEPROM_KOD_DOSTEPU + 2, 'C');
	eeprom_write_byte((void*) ADRES_EEPROM_KOD_DOSTEPU + 3, 'D');
	for (uchar k = 0; k < 35; ++k)	// przygotowanie eeprom
	{
		static const char num[] PROGMEM = "505691117";
		strcpy_P(numer_telefonu_wysylanego_smsa, num);
		numer_telefonu_wysylanego_smsa[8] = '0' + k % 10;
		konwertuj_telefon_na_blok_eeprom(&numer_telefonu_wysylanego_smsa[0], &numer_telefonu_wysylanego_smsa[9], bufor_eeprom);
		for (uchar i = 0; i < LICZBA_BAJTOW_NUMERU_TELEFONU_W_EEPROM; ++i)
			eeprom_write_byte((void*) (EEPROM_NUMER_TELEFONU_BRAMA(k) + i), bufor_eeprom[i]);
	}
}

void debug_generowanie_smsow(void)
{
	//generuj_raport_stanu_urzadzenia();
	debug_generuj_eeprom();
	podlaczony_modul_gsm_SIM900 = TRUE;
	numer_skopiowanego_telefonu_do_raportu_uzytkownikow_brama = 0;
	dodaj_komende(KOMENDA_KOLEJKI_WYSLIJ_RAPORT_UZYTKOWNIKOW);
	
	for (;;)
		wykonanie_komend();
}

void debug_test_przychodzacej_rozmowy(void)
{
	debug_generuj_eeprom();
	eeprom_write_word((void*) EEPROM_USTAWIENIE_WYJSCIA, 10);
	static const char num[] PROGMEM = "505691117";
	strcpy_P(numer_telefonu_ktory_dzwoni, num);
	
	sprawdz_przychodzaca_rozmowe();
	for (;;)
	{
		wykonanie_komend();
		steruj_wejscia_wyjscia_100ms();
	}
}

void debug_test_przychodzacego_smsa(void)
{
	debug_generuj_eeprom();
	eeprom_write_word((void*) EEPROM_USTAWIENIE_WYJSCIA, 0);	//USTAWIENIE_WYJSCIA_MASKA_STEROWANIE_DOWOLNY_UZYTKOWNIK
	static const char num[] PROGMEM = "505691117";
	strcpy_P(numer_telefonu_odebranego_smsa, num);
	tekst_odebranego_smsa[0] = 'P';
	
	wykonanie_polecenia_sms();
	for (;;)
	{
		wykonanie_komend();
		steruj_wejscia_wyjscia_100ms();
	}
}

void debug_sms_sterujacy(void)
{
	debug_generuj_eeprom();
	static const char num[] PROGMEM = "ABCD DEL 505691117";
	strcpy_P(tekst_odebranego_smsa, num);
	wykonanie_polecenia_sms();
	for (;;)
	{
		wykonanie_komend();
		zapisz_bajt_w_EEPROM();
	}
}

void debug_main(void)
{
	debug_sms_sterujacy();
	for (;;)
		;
}

#endif
