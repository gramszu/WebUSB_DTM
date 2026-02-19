#!/bin/bash

# Jeśli skrypt nie ma uprawnień roota (potrzebne do dostępu do AVRISP2 na macOS),
# spróbuj uruchomić go ponownie przez sudo – unikamy pętli dzięki znacznikowi.
if [ "$EUID" -ne 0 ] && [ -z "$UPLOAD_SH_ROOT" ]; then
    echo "Brak uprawnień do urządzenia USB – proszę o hasło administratora..."
    sudo -v  # Cache hasło na ~5-15 minut
    UPLOAD_SH_ROOT=1 exec sudo -E UPLOAD_SH_ROOT=1 "$0" "$@"
fi

# Skrypt do wgrywania Flash i fuse bits do ATmega328PB przez AVRISP2
# BEZ BOOTLOADERA. EEPROM wewnętrzny NIE jest wgrywany (konfiguracja w zewn. AT24C512).
#
# FUSE BITS:
#   lfuse: 0xFD
#   hfuse: 0xC9 (EESAVE=0?, SPIEN=0, BOOTRST=1-disabled)
#   efuse: 0xF6 (BOD 2.7V)
#   lock:  0x3F (No lock)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "${SCRIPT_DIR}" || exit 1

MCU=m328pb
PROGRAMMER=avrisp2
FLASH_FILE="AC200_DTM-F.hex"

echo "=========================================="
echo "Wgrywanie do ATmega328PB (BEZ BOOTLOADERA)"
echo "=========================================="
echo ""

if [ ! -f "$FLASH_FILE" ]; then
    echo "BŁĄD: Nie znaleziono pliku $FLASH_FILE"
    echo "Uruchom najpierw: make"
    exit 1
fi

echo "Wgrywanie:"
echo "  1. Flash: $FLASH_FILE"
echo "  2. Fuse bits (lfuse=0xFD, hfuse=0xC9, efuse=0xF6)"
echo "  3. Lock bits (0x3F)"
echo ""

avrdude -c ${PROGRAMMER} -p ${MCU} -B 5 -F -e -v \
    -U flash:w:"${FLASH_FILE}":i \
    -U lfuse:w:0xFD:m \
    -U hfuse:w:0xC9:m \
    -U efuse:w:0xF6:m \
    -U lock:w:0x3F:m

if [ $? -eq 0 ]; then
    echo ""
    echo "=========================================="
    echo "✓ Operacja zakończona pomyślnie!"
    echo "=========================================="
else
    echo ""
    echo "=========================================="
    echo "✗ Błąd podczas wgrywania!"
    echo "=========================================="
    exit 1
fi
