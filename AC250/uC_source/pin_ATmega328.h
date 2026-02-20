// Led
#define PIN_LED PORTC0
#define ustaw_stan_led(stan) SET_STATE(PORTC, PIN_LED, not(stan))

// wej�cia
#define WEJ_DEFAULT PINB0
#define STAN_WEJSCIE_1() IS_HI(PINB, WEJ_DEFAULT)

// wyj�cia
#define OUT0_WYJSCIE PIND5
#define WLACZ_OUT0() SET_HI(PORTD, OUT0_WYJSCIE)
#define WYLACZ_OUT0() SET_LO(PORTD, OUT0_WYJSCIE)
#define CZY_WLACZONE_WYJSCIE0() IS_HI(PORTD, OUT0_WYJSCIE)

// SIM900
#define RXD_WEJSCIE_SIM900 PIND0
#define TXD_WYJSCIE_SIM900 PIND1
#define PORT_UART_SIM900 PORTD
#define DDR_UART_SIM900 DDRD

#define CTS_WEJSCIE_SIM900 PINC2
#define PORT_CTS_SIM900 PORTC
#define PIN_CTS_SIM900 PINC

#define RTS_WYJSCIE_SIM900 PINC3
#define PORT_RTS_SIM900 PORTC
#define DDR_RTS_SIM900 DDRC

#define PWRKEY_WYJSCIE_SIM900 PIND4
#define PORT_PWRKEY_SIM900 PORTD
#define DDR_PWRKEY_SIM900 DDRD

#define STATUS_WEJSCIE_SIM900 PIND3
#define PIN_STATUS_SIM900 PIND

#define NUMER_BUFORA_SIM900 0

#define UCSRA_SIM900 UCSR0A
#define UCSRB_SIM900 UCSR0B
#define UBRRL_SIM900 UBRR0L
#define UBRRH_SIM900 UBRR0H
#define UDR_SIM900 UDR0
#define USART_TX_vect_SIM900 USART0_TX_vect
#define USART_RX_vect_SIM900 USART0_RX_vect
#define ACSR_SIM900 ACSR

#define ADRES_UDR_SIM900 "0x00C6"
#define ADRES_UCSRB_SIM900 "0x00C1"

#define RXCIE RXCIE0
#define TXCIE TXCIE0
#define TXC TXC0
#define TXEN TXEN0
#define RXEN RXEN0

#define inicjalizacja_portow()                                                 \
  DDRC = BIT(PIN_LED);                                                         \
  PORTC = BIT(PIN_LED);                                                        \
  DDRD = BIT(OUT0_WYJSCIE);

#define przerwanie_timer() ISR(TIMER0_COMPA_vect, ISR_NOBLOCK)

#define ustaw_parametry_dla_bezpieczenstwa_rejestry()                          \
  TCCR0A = BIT(WGM01);                                                         \
  TCCR0B = BIT(CS01) | BIT(CS00);                                              \
  OCR0A = 63;                                                                  \
  TIMSK0 |= BIT(OCIE0A);

#define WLACZ_PRZERWANIE_TIMER() SET_HI(TIMSK0, OCIE0A)
#define WYLACZ_PRZERWANIE_TIMER() SET_LO(TIMSK0, OCIE0A)
