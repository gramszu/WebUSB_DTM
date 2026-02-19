# Funkcja USSD - Dokumentacja

## Opis
Funkcja USSD umożliwia wysyłanie zapytań USSD (np. sprawdzenie salda, numeru telefonu) przez SMS i otrzymywanie odpowiedzi również przez SMS.

## Użycie

### Składnia
```
ABCD USSD <kod_ussd>
```

### Przykłady
- Sprawdzenie salda: `ABCD USSD *100#`
- Sprawdzenie numeru: `ABCD USSD *101#`
- Pakiety internetowe: `ABCD USSD *111#`

## Parametry
- **Timeout**: 30 sekund
- **Maksymalna długość odpowiedzi**: 160 znaków

## Implementacja

### Pliki zmodyfikowane
1. **data_sim900.h** - Flagi USSD (timeout 30s)
2. **interpretacjaSMS.c** - Parser komendy USSD
3. **main_sim900.h** - Obsługa odpowiedzi +CUSD:
4. **main.c** - Timeout logic

### Rozmiar firmware
- Przed USSD: 25,170 bytes
- Po USSD: 25,540 bytes
- Wzrost: +370 bytes

## Bezpieczeństwo
- Kod dostępu wymagany (ABCD)
- Timeout 30s zapobiega blokowaniu
- Rate limiting: 3 REPORT/USER w 30s
- Kolejka: max 30 komend

## Historia zmian
- Step 1 (db99c2d): Flagi USSD
- Step 2 (c911052): Parser + handler + timeout 30s
- Step 3: Dokumentacja

## Autor
Implementacja: Antigravity AI
Data: 2025-12-23
