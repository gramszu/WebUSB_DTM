#ifndef NARZEDZIA_LEN_H
#define NARZEDZIA_LEN_H

#ifndef BIT
#define BIT(NR) (1ull << (NR))
#endif

#ifndef __BV
#define __BV BIT
#endif

#ifndef BITC
#define BITC(NR)	((uchar)(1 << (uchar)(NR)))
#endif

#define not	!

#define IS_HI(P, NR) ( (P) & BIT(NR) )
#define IS_LO(P, NR) ( !IS_HI(P, NR) )
#define IS_HI2(P, NR1, NR2)	( (P) & (BIT(NR1) | BIT(NR2)) )
#define IS_LO2(P, NR1, NR2) ( !IS_HI2(P, NR1, NR2) )
#define SET_HI(P, NR) ( (P) |= BIT(NR) )
#define SET_LO(P, NR) ( (P) &= ~BIT(NR) )
#define SET_HI1(P, NR1) ( (P) |= BIT(NR1) )
#define SET_LO1(P, NR1) ( (P) &= ~BIT(NR1) )
#define SET_HI2(P, NR1, NR2) ( (P) |= BIT(NR1) | BIT(NR2) )
#define SET_LO2(P, NR1, NR2) ( (P) &= ~(BIT(NR1) | BIT(NR2)) )
#define SET_HI3(P, NR1, NR2, NR3) ( (P) |= BIT(NR1) | BIT(NR2) | BIT(NR3) )
#define SET_LO3(P, NR1, NR2, NR3) ( (P) &= ~(BIT(NR1) | BIT(NR2) | BIT(NR3)) )
#define SET_HI4(P, NR1, NR2, NR3, NR4) ( (P) |= BIT(NR1) | BIT(NR2) | BIT(NR3) | BIT(NR4) )
#define SET_LO4(P, NR1, NR2, NR3, NR4) ( (P) &= ~(BIT(NR1) | BIT(NR2) | BIT(NR3) | BIT(NR4)) )
#define SET_HI5(P, NR1, NR2, NR3, NR4, NR5) ( (P) |= BIT(NR1) | BIT(NR2) | BIT(NR3) | BIT(NR4) | BIT(NR5) )
#define SET_LO5(P, NR1, NR2, NR3, NR4, NR5) ( (P) &= ~(BIT(NR1) | BIT(NR2) | BIT(NR3) | BIT(NR4) | BIT(NR5)) )
#define SET_HI6(P, NR1, NR2, NR3, NR4, NR5, NR6) ( (P) |= BIT(NR1) | BIT(NR2) | BIT(NR3) | BIT(NR4) | BIT(NR5) | BIT(NR5) )
#define SET_LO6(P, NR1, NR2, NR3, NR4, NR5, NR6) ( (P) &= ~(BIT(NR1) | BIT(NR2) | BIT(NR3) | BIT(NR4) | BIT(NR5) | BIT(NR5)) )
#define SET_HI7(P, NR1, NR2, NR3, NR4, NR5, NR6, NR7) ( (P) |= BIT(NR1) | BIT(NR2) | BIT(NR3) | BIT(NR4) | BIT(NR5) | BIT(NR6) | BIT(NR7) )
#define SET_LO7(P, NR1, NR2, NR3, NR4, NR5, NR6, NR7) ( (P) &= ~(BIT(NR1) | BIT(NR2) | BIT(NR3) | BIT(NR4) | BIT(NR5) | BIT(NR6) | BIT(NR7)) )

#define SET_STATE(byte, NR, state)	do { if ( state ) SET_HI(byte, NR); else SET_LO(byte, NR); } while ( 0 )

#define FIRST_SET_BIT_POMOC(z, n, maska)	((maska) & BIT(n)) ? n : 
#define BYTE(zmienna, nr)									*(((uchar*)(&(zmienna))) + (nr))
#define NO_BYTE_OF_BIT(maska)							(FSBIT(maska) / 8)
#define NBBIT															NO_BYTE_OF_BIT

// korzystaæ w case CSET( Enum1, Maska1): ... break;
#define CSET(maska, ustawienie)						((ustawienie) << (FSBIT(maska) - (NBBIT(maska) * 8)))

// korzystaæ tylko dla ustawieñ jednobitowych (dla wielobitowych generuje d³u¿szy kod o 18 bajtów)
#define SETTING(maska, zmienna)						( BYTE(zmienna, NBBIT(maska)) & ((maska) >> (NBBIT(maska) * 8)) )

// korzystaæ dla ustawieñ wielobitowych
#define IS_SETTING(maska, ustawienie, zmienna)		(SETTING(maska, zmienna) == CSET(maska, ustawienie))

#define SET_SETTING(zmienna, maska, ustawienie)		BYTE(zmienna, NBBIT(maska)) \
	= (( BYTE(zmienna, NBBIT(maska)) & (uchar)(~((maska) >> (NBBIT(maska) * 8)))) | CSET(maska, ustawienie))
	
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;
typedef signed char schar;
typedef signed int sint;
typedef signed long slong;
typedef signed long long slonglong;
typedef const unsigned char cuchar;

#ifndef WIN32
#define TRUE 		255
#define FALSE 	0

#define HI		255
#define LO			0

#ifndef NULL
#define NULL		0
#endif


#define max(VALUE1, VALUE2) ((VALUE1) > (VALUE2) ? (VALUE1) : (VALUE2))
#define min(VALUE1, VALUE2) ((VALUE1) < (VALUE2) ? (VALUE1) : (VALUE2))
#define inside(left, value, right)		((value) > (left) && (value) < (right))
#define inside_eq(left, value, right)		((value) >= (left) && (value) <= (right))
#define outside(left, value, right)		((value) <= (left) || (value) >= (right))

#endif

typedef union _UINT_UCHAR
{
	uint l;
	uchar ch[2];
} UINT_UCHAR;

#define LOW(liczba_uint) (((UINT_UCHAR)(liczba_uint)).ch[0]) //((uchar)(liczba_uint))
#define HIGH(liczba_uint) (((UINT_UCHAR)(liczba_uint)).ch[1])

typedef union _ULONG_UCHAR
{
	ulong l;
	uchar ch[4];
} ULONG_UCHAR;

#define LONGCH(liczba_ulong, CH)	(((ULONG_UCHAR)(liczba_ulong)).ch[CH])
#define LONG0(liczba_ulong)				LONGCH((liczba_ulong), 0)
#define LONG1(liczba_ulong)				LONGCH((liczba_ulong), 1)
#define LONG2(liczba_ulong)				LONGCH((liczba_ulong), 2)
#define LONG3(liczba_ulong)				LONGCH((liczba_ulong), 3)

typedef union _PTR_UCHAR
{
	void* ptr;
	uint l;
	uchar ch[sizeof (void*)];
} PTR_UCHAR;

// REGISTER - mo¿e byæ zmienn¹ w C (jednego z typów char) znajduj¹c¹ siê w rejestrze
#define PUSH(REGISTER)	asm volatile ("push %0" "\n\t" : "=r" (REGISTER) : )
#define POP(REGISTER)		asm volatile ("pop %0" "\n\t" : "=r" (REGISTER) : )
#define PUSH_TXT(REGISTER_TXT) asm volatile ("push " REGISTER_TXT "\n\t" :: )
#define POP_TXT(REGISTER_TXT) asm volatile ("pop " REGISTER_TXT "\n\t" :: )
#define NOP()		asm volatile ("nop" :: )
#define SWAP(REGISTER) asm volatile ("swap %0" "\n\t" : "=r" (REGISTER) : )
#define CLEAR(REGISTER)	asm volatile ("eor %0, %0" "\n\t" : "=r" (REGISTER) : )
#define CLEAR_UINT(VALUE)	asm volatile ("eor %a0, %a0" "\n\t" "eor %b0, %b0" "\n\t" : "=r" (VALUE) : "0" (VALUE) )
#define CLEAR_TXT(REGISTER_TXT)	asm volatile ("eor " REGISTER_TXT " , " REGISTER_TXT "\n\t" :: )
#define LDS(REGISTER, ADDRESS)	asm volatile ("lds %0, " ADDRESS "\n\t" : "=r" (REGISTER) : );	// ADDRESS = 0x009C
#define STS(REGISTER, ADDRESS)	asm volatile ("sts " ADDRESS ", %0" "\n\t" : "=r" (REGISTER) : );	// ADDRESS = 0x009C
#define SBRS(REGISTER, NOBIT)		asm volatile ("sbrs %0, %1" "\n\t" : : "r" (REGISTER), "I"(NOBIT))
#define SBRC(REGISTER, NOBIT)		asm volatile ("sbrc %0, %1" "\n\t" : : "r" (REGISTER), "I"(NOBIT))
#define ORI(REGISTER, MASK)			asm volatile ("ori %0, %1" "\n\t" : "=r" (REGISTER) : "M"(MASK))
#define LSR(REGISTER)						asm volatile ("lsr %0" "\n\t" : "=r" (REGISTER) : )
#define CLC()										asm volatile ("clc" "\n\t" : : )
#define SEC()										asm volatile ("sec" "\n\t" : : )

#define WYKONAJ_CLI_SEI(POLECENIE)	\
do {																		\
	cli();														\
	POLECENIE;												\
	sei(); 														\
	NOP(); 														\
} while ( 0 )

// skróty
#define strlen_R(rom)				(sizeof rom - 1)
#define memcpy_R(sink, rom)	memcpy_P(sink, rom, sizeof rom - 1)
//#define memcmp_P	strncmp_P
#define memcmp_R(sink, rom)	memcmp_P(sink, rom, sizeof rom - 1)
#define strcpy_move(sink, source)			while ( *source ) *sink++ = *source++	// nie kopiuje '\0'

#define PIN_LICZNIK(STAN_PIN, LICZNIK)	\
	do {	\
		if ( ! (STAN_PIN) )	\
			(LICZNIK) >>= 1;	\
		(LICZNIK) |= 0x01;	\
		if ( (STAN_PIN) )	\
			(LICZNIK) <<= 1;	\
	} while ( 0 )

#define STAN_PIN_LICZNIK(LICZNIK)						((LICZNIK) & 0xf0)

#define SPLI																	STAN_PIN_LICZNIK

// rezygnowaæ z poni¿szego
#define TEST_PIN_LICZNIK(STAN_PIN, LICZNIK)	\
	do {	\
		if ( (STAN_PIN) )	\
			(LICZNIK) >>= 1;	\
		(LICZNIK) |= 0x01;	\
		if ( ! (STAN_PIN) )	\
			(LICZNIK) <<= 1;	\
	} while ( 0 )

#define kopiuj_zmienna_do_bufora(bufor, zmienna)	\
do	\
{	\
	if ( sizeof (zmienna) == 1 )	\
		(bufor)[0] = (unsigned char) zmienna;	\
	if ( sizeof (zmienna) == 2 )	\
	{	\
		uint u = zmienna;	\
		(bufor)[0] = LOW(u);	\
		(bufor)[1] = HIGH(u);	\
	}	\
	if ( sizeof (zmienna) == 4 )	\
	{	\
		ulong u = zmienna;	\
		(bufor)[0] = LONG0(u);	\
		(bufor)[1] = LONG1(u);	\
		(bufor)[2] = LONG2(u);	\
		(bufor)[3] = LONG3(u);	\
	}	\
} while ( 0 )

#define kopiuj_bufor_do_zmiennej(bufor, zmienna)	\
do	\
{	\
	if ( sizeof (zmienna) == 1 )	\
		zmienna = (bufor)[0];	\
	if ( sizeof (zmienna) == 2 )	\
		zmienna = *((uint*) (bufor));	\
	if ( sizeof (zmienna) == 4 )	\
		zmienna = *((ulong*) (bufor));	\
} while ( 0 )

#define TEST_PCB_STANDARDOWE_WYJSCIE(LICZNIK_1S, USTAWIENIE)	\
do	\
{	\
	static uint licznik_1S;	\
	if ( ++licznik_1S >= 2 * (LICZNIK_1S) )	\
		licznik_1S = 0;	\
	USTAWIENIE(licznik_1S < (LICZNIK_1S));	\
} while ( 0 )

#define TEST_PCB_STANDARDOWE_WYJSCIE_OPCJA_1(LICZNIK_1S, USTAWIENIE, OPCJA1)	\
do	\
{	\
	static uint licznik_1S;	\
	if ( ++licznik_1S >= 2 * (LICZNIK_1S) )	\
		licznik_1S = 0;	\
	USTAWIENIE(OPCJA1, licznik_1S < (LICZNIK_1S));	\
} while ( 0 )

typedef struct _bit_struct
{
	uchar bit0: 1;
	uchar bit1: 1;
	uchar bit2: 1;
	uchar bit3: 1;
	uchar bit4: 1;
	uchar bit5: 1;
	uchar bit6: 1;
	uchar bit7: 1;
} byte_bit_struct;

typedef union _bit_union
{
	uchar ch;
	byte_bit_struct bit;
} byte_bit_union;

#if defined POKAZ_NAPRAWY

#define napraw(STR)	_Pragma(#STR)

#else

#define napraw(STR)	

#endif

int get_free_memory(void);
void generuj_raport_sys(void);
void generuj_raport_stanu_urzadzenia(void);
#endif
