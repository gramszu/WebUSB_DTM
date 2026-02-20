
#ifndef POLECENIA_GSM_H
#define POLECENIA_GSM_H

uchar czy_jest_nowa_komenda_SIM900(void);
extern uint numer_bledu_cme;
extern uint numer_bledu_cms;

extern uchar bledny_PIN;
#define POZIOM_SIECI_BLAD	99
#define STAN_MODUL_NIEZALOGOWANY	100	// w³asne
extern uchar poziom_sieci_gsm;

void resetuj_polecenie_konczace(void);

extern uchar liczba_komend_w_kolejce_SIM900;

#define CME_OPERATION_NOT_ALLOWED		3
#define CME_OPERATION_NOT_SUPPORTED	4
#define CME_SIM_NOT_INSERTED		10
#define CME_SIM_FAILURE					13
#define CME_SIM_BUSY						14
#define CME_SIM_WRONG						15
#define CME_SIM_INCORECT_PASWORD	16
#define CME_INVALID_INDEX				21
#define CME_NOT_FOUND						22
#define CME_NO_NETWORK_SERVICE	30

#define CMS_SMS_ME_RESERVED			301
#define CMS_SIM_BUSY						314
#define CMS_SM_BL_NOT_READY			517
#define CMS_INVALID_MEMORY_INDEX	321
#define CMS_PC_BUSY								515
#define CMS_INVALID_CHARS_IN_PDU	528
#define CMS_INCORECT_PDU_LENGTH		529

#define MIN_LICZBA_ODEBRANYCH_ZNAKOW_KONCZACYCH_ROZMOWE_W_TRAKCIE_PODSLUCHU		10

#endif
