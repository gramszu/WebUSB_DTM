# Komendy SMS - AC200-DTM-F3

Wszystkie komendy muszą być poprzedzone 4-znakowym kodem dostępu (domyślnie `ABCD`).
Wielkość liter nie ma znaczenia (np. `abcd open` działa tak samo jak `ABCD OPEN`).

| Komenda | Parametry | Opis | Przykład |
| :--- | :--- | :--- | :--- |
| **CODE** | `STARY NOWY` | Zmienia 4-znakowy kod dostępu. | `ABCD CODE ABCD 1234` |
| **ADD** | `NUMER` | Dodaje numer telefonu do listy uprawnionych (brama). | `ABCD ADD 500123456` |
| **DEL** | `NUMER` | Usuwa numer telefonu z listy uprawnionych. | `ABCD DEL 500123456` |
| **XXXX** | *brak* | Przywraca ustawienia fabryczne sterownika (wymaga potwierdzenia diodą). | `ABCD XXXX` |
| **REPORT** | *brak* | Wysyła raport o stanie urządzenia (zasięg, tryb, wyjścia). | `ABCD REPORT` |
| **USER** | `NUMER` | Sprawdza, czy dany numer znajduje się na liście. | `ABCD USER 500123456` |
| **USER** | `ALL` | Wysyła listę wszystkich użytkowników w SMS-ach (wieloczęściowy). | `ABCD USER ALL` |
| **USER** | `LIST` | Wysyła skrócony raport o liczbie użytkowników. | `ABCD USER LIST` |
| **OPEN** | *brak* | Otwiera bramę (zgodnie z trybem CLIP/SMS) i ustawia tryb **Publiczny**. | `ABCD OPEN` |
| **OPEN** | `CLIP` | Ustawia tryb Publiczny, sterowanie CLIP. | `ABCD OPEN CLIP` |
| **OPEN** | `DTMF` | Ustawia tryb Publiczny, sterowanie DTMF (wdzwaniane, brak CLIP). | `ABCD OPEN DTMF` |
| **OPEN** | `SMS` | Ustawia tryb Publiczny, sterowanie SMS (CLIP wyłączony). | `ABCD OPEN SMS` |
| **CLOSE** | *brak* | Ustawia tryb **Prywatny** (tylko uprawnieni). | `ABCD CLOSE` |
| **CLOSE** | `CLIP` | Ustawia tryb Prywatny, sterowanie CLIP. | `ABCD CLOSE CLIP` |
| **CLOSE** | `SMS` | Ustawia tryb Prywatny, sterowanie SMS. | `ABCD CLOSE SMS` |
| **SET** | `HH:MM:SS` | Ustawia zegar systemowy (np. 14:30:00). | `ABCD SET 14:30:00` |
| **SET** | *brak* | Zwraca aktualny czas systemowy. | `ABCD SET` |
| **TIME** | `START STOP` | Ustawia harmonogram działania (godziny aktywności). Format HH:MM. | `ABCD TIME 08:00 16:00` |
| **TIME** | `OFF` | Wyłącza harmonogram (działanie 24h). | `ABCD TIME OFF` |
| **SKRYBA** | `ON [LIMIT]` | Włącza funkcję "Skryba" (automatyczne dodawanie numerów). Opcjonalny limit. | `ABCD SKRYBA ON 50` |
| **SKRYBA** | `OFF` | Wyłącza funkcję "Skryba". | `ABCD SKRYBA OFF` |
| **START** | *brak* | Odblokowuje system (zdejmuje blokadę). | `ABCD START` |cn
| **STOP** | *brak* | Blokuje system (całkowita blokada sterowania). | `ABCD STOP` |
| **SUB** | `NUMER` | Dodaje numer jako "Super User" (omija blokady, pozycje 194-199). | `ABCD SUB 600999999` |
| **MYNUM** | `NUMER` | Ustawia własny numer telefonu urządzenia (do identyfikacji). | `ABCD MYNUM 700800900` |
| **USSD** | `KOD` | Wysyła kod USSD do operatora i zwraca odpowiedź. | `ABCD USSD *101#` |
| **CON** | `[NUMER]` | Włącza powiadomienia na podany numer (lub ostatni, jeśli brak). | `ABCD CON 500123456` |
| **CON** | `OFF` | Wyłącza powiadomienia. | `ABCD CON OFF` |
| **ON** | `CZAS` | Ustawia czas załączenia przekaźnika w sekundach (lub 99999 dla Toggle). | `ABCD ON 5` |
| **TOGLE** | `ON` | Włącza tryb bistabilny (włącz/wyłącz). | `ABCD TOGLE ON` |
| **TOGLE** | `OFF` | Wyłącza tryb bistabilny (powrót do mono 2s). | `ABCD TOGLE OFF` |
| **RST** | *brak* | Zdalny restart urządzenia. | `ABCD RST` |

> [!NOTE]
> Format numerów telefonów: zalecane 9 cyfr (np. `500123456`). System obsługuje również formaty z prefiksem `+48`.
