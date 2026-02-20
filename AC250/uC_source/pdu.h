
#ifndef PDU_H
#define PDU_H

#include "narzedzia.h"

#define LICZBA_ZNAKOW_WIADOMOSCI_W_PDU(PDU_PTR) (PDU_PTR[0])

void pdu_to_ascii(const uchar *pdu_ptr, uchar *bufor, const uchar dim_bufor);
#define ascii_to_pdu(bufor_pdu, ascii)                                         \
  ascii_to_pdu_blok(bufor_pdu, ascii, ascii + strlen((const char *)ascii))
uchar *ascii_to_pdu_blok(uchar *bufor_pdu, const uchar *ascii_beg,
                         const uchar *ascii_end);
void konwertuj_blok_dwa_znaki_na_znak_pdu(const uchar *blok_danych,
                                          const uchar liczba_znakow,
                                          uchar *ptr);
uchar *pobierz_numer_telefonu_nadawcy_z_PDU(const uchar *pdu_ptr,
                                            uchar *bufor_numer_telefonu,
                                            uchar *nietypowy_sms);
const uchar *pobierz_date_z_PDU(const uchar *ptr, uchar *rok, uchar *miesiac,
                                uchar *dzien);
const uchar *pobierz_czas_z_PDU(const uchar *ptr, uchar *godzina, uchar *minuta,
                                uchar *sekunda);
uchar *zapisz_naglowek_pdu(uchar *ptr, const uchar *telefon_odbiorcy,
                           const uchar *telefon_centrum_sms, const uchar TP_PID,
                           const uchar TP_DCS);
void konwertuj_pdu_na_blok_wysylany(uchar *buf, const uchar *pdu_ptr,
                                    const uchar dlugosc_pdu);

#endif
