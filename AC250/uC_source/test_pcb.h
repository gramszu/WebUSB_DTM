void test_dioda_wyjscia(void)
{
	static uchar opoznienie_1s;
	if ( ++opoznienie_1s < 100 )
		return;
	opoznienie_1s = 0;
	static uchar stan;
	stan = not stan;
	if ( stan )
	{
		ustaw_stan_led(TRUE);
	}
	else
	{
		ustaw_stan_led(FALSE);
	}
}

void test_wejscie(void)
{
	ustaw_stan_led(stan_wejscia(0));
}
