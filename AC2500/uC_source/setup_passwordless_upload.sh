#!/bin/bash

# Skrypt konfigurujący sudo bez hasła dla avrdude
# Uruchom: sudo ./setup_passwordless_upload.sh

set -e

echo "=========================================="
echo "Konfiguracja sudo bez hasła dla avrdude"
echo "=========================================="
echo ""

# Sprawdź czy skrypt jest uruchomiony jako root
if [ "$EUID" -ne 0 ]; then
    echo "❌ Błąd: Skrypt musi być uruchomiony jako root (sudo)"
    echo "Użyj: sudo ./setup_passwordless_upload.sh"
    exit 1
fi

# Znajdź ścieżkę do avrdude
AVRDUDE_PATH=$(which avrdude)
if [ -z "$AVRDUDE_PATH" ]; then
    echo "❌ Błąd: Nie znaleziono avrdude w PATH"
    exit 1
fi

echo "✅ Znaleziono avrdude: $AVRDUDE_PATH"

# Pobierz nazwę użytkownika (tego, kto wywołał sudo)
if [ -n "$SUDO_USER" ]; then
    USERNAME="$SUDO_USER"
else
    echo "❌ Błąd: Nie można określić nazwy użytkownika"
    exit 1
fi

echo "✅ Użytkownik: $USERNAME"
echo ""

# Utwórz plik konfiguracyjny sudoers
SUDOERS_FILE="/etc/sudoers.d/avrdude"
echo "Tworzę plik: $SUDOERS_FILE"

cat > "$SUDOERS_FILE" << EOF
# Zezwól użytkownikowi $USERNAME na uruchamianie avrdude bez hasła
$USERNAME ALL=(ALL) NOPASSWD: $AVRDUDE_PATH
EOF

# Ustaw odpowiednie uprawnienia (WYMAGANE dla bezpieczeństwa!)
chmod 0440 "$SUDOERS_FILE"

echo "✅ Plik utworzony i zabezpieczony"
echo ""

# Weryfikacja składni
if visudo -c -f "$SUDOERS_FILE" > /dev/null 2>&1; then
    echo "✅ Składnia pliku sudoers poprawna"
else
    echo "❌ Błąd: Nieprawidłowa składnia pliku sudoers!"
    rm -f "$SUDOERS_FILE"
    exit 1
fi

echo ""
echo "=========================================="
echo "✓ Konfiguracja zakończona pomyślnie!"
echo "=========================================="
echo ""
echo "Teraz możesz uruchomić upload.sh bez hasła:"
echo "  ./upload.sh"
echo ""
echo "Test (powinno działać bez hasła):"
echo "  sudo avrdude -?"
echo ""
