// Led
#define PIN_LED									PG4
#define ustaw_stan_led(stan)		SET_STATE(PORTG, PIN_LED, not (stan))

// wejœcia
#define WEJ_1	PF7
#define WEJ_2	PF5
#define WEJ_DEFAULT	PB6
#define STEROWANIE_NO_NC_WEJ_1	PA1
#define STEROWANIE_NO_NC_WEJ_2	PA2

#define STAN_WEJSCIE_1()		IS_HI(PINF, WEJ_1)
#define STAN_WEJSCIE_2()		IS_HI(PINF, WEJ_2)
#define STAN_WEJSCIE_3()		IS_HI(PINB, WEJ_DEFAULT)

#define USTAW_NO_WEJSCIE_1(WYZWALANIE_PLUSEM)	SET_STATE(PORTA, STEROWANIE_NO_NC_WEJ_1, !(WYZWALANIE_PLUSEM))
#define USTAW_NO_WEJSCIE_2(WYZWALANIE_PLUSEM)	SET_STATE(PORTA, STEROWANIE_NO_NC_WEJ_2, !(WYZWALANIE_PLUSEM))

#define USTAW_NC_WEJSCIE_1(WYZWALANIE_PLUSEM)	SET_STATE(PORTA, STEROWANIE_NO_NC_WEJ_1, !(WYZWALANIE_PLUSEM))
#define USTAW_NC_WEJSCIE_2(WYZWALANIE_PLUSEM)	SET_STATE(PORTA, STEROWANIE_NO_NC_WEJ_2, !(WYZWALANIE_PLUSEM))

// wyjœcia
#define OUT0_WYJSCIE							PA6
#define OUT1_WYJSCIE							PG2
#define WLACZ_OUT0()							SET_HI(PORTA, OUT0_WYJSCIE)
#define WYLACZ_OUT0()							SET_LO(PORTA, OUT0_WYJSCIE)
#define CZY_WLACZONE_WYJSCIE0()		IS_HI(PORTA, OUT0_WYJSCIE)
#define WLACZ_OUT1()							SET_HI(PORTG, OUT1_WYJSCIE)
#define WYLACZ_OUT1()							SET_LO(PORTG, OUT1_WYJSCIE)
#define CZY_WLACZONE_WYJSCIE1()		IS_HI(PORTG, OUT1_WYJSCIE)

// 1-wire
#define pin_1wire																PE2
#define ddrx_1wire 															DDRE	// przypisywaæ ddrx = DDRA
#define pinx_1wire 															PINE	// przypisywaæ ddrx = PINA

// USB uart na przerwaniach
#define TX_PIN_GPS           PE1               //!< Transmit data pin
#define RX_PIN_GPS           PE4               //!< Receive data pin, must be INT0
#define PORT_TX_PIN_GPS			 PORTE
#define PIN_RX_PIN_GPS			 PINE
#define przerwanie_int_uart()		ISR(INT4_vect, ISR_NAKED)
#define EXT_IFR          EIFR              //!< External Interrupt Flag Register
#define EXT_ICR          EICRB             //!< External Interrupt Control Register
#define ENABLE_EXTERNAL_INTERRUPT( )   	( EIMSK |= ( 1 << INT4 ) )
#define DISABLE_EXTERNAL_INTERRUPT( )  	( EIMSK &= ~( 1 << INT4 ) )
#define CLEAR_EXTERNAL_INTERRUPT( )        ( EXT_IFR |= (1 << INTF4 ) )
#define SET_EDGE_INTERRUPT() 								( EXT_ICR |= (1 << ISC41))        // Interrupt sense control: falling edge.
#define OCR              OCR2              //!< Output Compare Register
#define TCNT						 TCNT2
#define TCCR_P           TCCR2             //!< Timer/Counter Control (Prescaler) Register
#define STOP_TIMER()										SET_LO(TCCR_P, CS21)
#define RUN_TIMER()											SET_HI(TCCR_P, CS21)
#define IS_RUNNING_TIMER()							IS_HI(TCCR_P, CS21)
#define TCCR             								TCCR2             //!< Timer/Counter Control Register
#define SET_CTC_MODE()									(TCCR |= (1 << WGM21))
#define ENABLE_TIMER_INTERRUPT( )       ( TIMSK |= ( 1 << OCIE2 ) )
#define DISABLE_TIMER_INTERRUPT( )      ( TIMSK &= ~( 1 << OCIE2 ) )
#define CLEAR_TIMER_INTERRUPT( )        ( TIFR |= ( (1 << OCF2) ) )
#define TIMER_COMP_VECT  TIMER2_COMP_vect  //!< Timer Compare Interrupt Vector

// napiêcie zasilania
#define WEJSCIE_NAPIECIE_ZASILANIA							PF1
#define LICZBA_POMIAROW_ANALOGOWYCH							1

// SIM900
#define RXD_WEJSCIE_SIM900											PD2
#define TXD_WYJSCIE_SIM900											PD3
#define PORT_UART_SIM900												PORTD
#define DDR_UART_SIM900													DDRD
#define CTS_WEJSCIE_SIM900											PD4
#define PORT_CTS_SIM900													PORTD
#define PIN_CTS_SIM900													PIND
#define RTS_WYJSCIE_SIM900											PD5
#define PORT_RTS_SIM900													PORTD
#define DDR_RTS_SIM900													DDRD
#define PWRKEY_WYJSCIE_SIM900										PG0
#define PORT_PWRKEY_SIM900											PORTG
#define DDR_PWRKEY_SIM900												DDRG
#define STATUS_WEJSCIE_SIM900										PC0
#define PIN_STATUS_SIM900												PINC
//#define NRESET_WYJSCIE_SIM900										PC1
//#define DDR_NRESET_SIM900												DDRC
//#define RI_WEJSCIE_SIM900												PD6
//#define PORT_RI_SIM900													PORTD
//#define DDR_RI_SIM900														DDRD
//#define PIN_RI_SIM900														PIND
//#define DTR_WYJSCIE_SIM900 											PG1
//#define PORT_DTR_SIM900													PORTG
//#define DDR_DTR_SIM900													DDRG
//#define DCD_WEJSCIE_SIM900											PD7
//#define PIN_DCD_SIM900													PIND
//#define DSR_WEJSCIE_SIM900										brak
#define NUMER_BUFORA_SIM900											1

#define UCSRA_SIM900						UCSR1A
#define UCSRB_SIM900						UCSR1B
#define UBRRL_SIM900						UBRR1L
#define UBRRH_SIM900						UBRR1H
#define UDR_SIM900							UDR1
#define USART_TX_vect_SIM900		USART1_TX_vect
#define USART_RX_vect_SIM900		USART1_RX_vect
#define ACSR_SIM900							ACSR

#if     NUMER_BUFORA_SIM900 == 0
#define   ADRES_UDR_SIM900				"0x002C"
#define   ADRES_UCSRB_SIM900			"0x002A"
#elif   NUMER_BUFORA_SIM900 == 1
#define   ADRES_UDR_SIM900				"0x009C"
#define   ADRES_UCSRB_SIM900			"0x009A"
#endif


//#define LICZBA_UZYTKOWNIKOW					8
//#define LICZBA_WEJSC_URZADZENIA			6
//#define LICZBA_WEJSC_ANALOGOWYCH_URZADZENIA		2
//#define LICZBA_WYJSC_URZADZENIA			6
//#define LICZBA_CZUJNIKOW_TEMPERATURY_URZADZENIA		6
//#define LICZBA_ZNAKOW_WIADOMOSCI			20
//#define LICZBA_ZNAKOW_OPISU_PARAMETRU	10

#define inicjalizacja_portow()		\
		DDRA = BIT(OUT0_WYJSCIE) | BIT(STEROWANIE_NO_NC_WEJ_1) | BIT(STEROWANIE_NO_NC_WEJ_2);		\
		PORTA = 0;		\
		DDRB = BIT(PB7) /*na potrzeby wejœcia DEFAULT na p³ycie integrala*/;		\
		PORTB = BIT(PB7);		\
		DDRC = 0;		\
		PORTC = 0;		\
		DDRD = 0;		\
		PORTD = 0;		\
		DDRE = BIT(TX_PIN_GPS);		\
		PORTE = BIT(TX_PIN_GPS) | BIT(RX_PIN_GPS);		\
		DDRF = 0;		\
		PORTF = 0;		\
		DDRG = BIT(PIN_LED) | BIT(OUT1_WYJSCIE);		\
		PORTG = BIT(PIN_LED);		\
		ADCSRA = BIT(ADIE) | BIT(ADPS2) | BIT(ADPS1);		\
		ADMUX = BIT(REFS0);

#define przerwanie_timer()		ISR(TIMER0_COMP_vect, ISR_NOBLOCK)

#define ustaw_parametry_dla_bezpieczenstwa_rejestry()		\
	TCCR0 = BIT(CS02) | BIT(WGM01);			\
	OCR0 = 63;		\
	TIMSK |= BIT(OCIE0);		\

#define WLACZ_PRZERWANIE_TIMER()		SET_HI(TIMSK, OCIE0)
#define WYLACZ_PRZERWANIE_TIMER()		SET_LO(TIMSK, OCIE0)


