#!/bin/bash

# Jeśli skrypt nie ma uprawnień roota (potrzebne do dostępu do AVRISP2 na macOS),
# spróbuj uruchomić go ponownie przez sudo – unikamy pętli dzięki znacznikowi.
if [ "$EUID" -ne 0 ] && [ -z "$UPLOAD_SH_ROOT" ]; then
    echo "Brak uprawnień do urządzenia USB – proszę o hasło administratora..."
    sudo -v  # Cache hasło na ~5-15 minut
    UPLOAD_SH_ROOT=1 exec sudo -E UPLOAD_SH_ROOT=1 "$0" "$@"
fi

# Skrypt do wgrywania flash, EEPROM, bootloadera i fuse bits do ATmega328PB przez AVRISP2

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

MCU=m328pb
PROGRAMMER=avrisp2
FLASH_FILE="${SCRIPT_DIR}/AC200-DTM-FS-UART-WWW.hex"
EEPROM_FILE="${SCRIPT_DIR}/default_eeprom_AC200.hex"
BOOTLOADER_FILE="${SCRIPT_DIR}/urboot.hex"

echo "=========================================="
echo "Wgrywanie do ATmega328PB przez AVRISP2"
echo "=========================================="
echo ""

# Sprawdz czy pliki istnieja
if [ ! -f "$FLASH_FILE" ]; then
    echo "BŁĄD: Nie znaleziono pliku $FLASH_FILE"
    exit 1
fi

if [ ! -f "$BOOTLOADER_FILE" ]; then
    echo "BŁĄD: Nie znaleziono pliku bootloadera $BOOTLOADER_FILE"
    exit 1
fi

# EEPROM opcjonalny
EEPROM_OPTION=""
if [ -f "$EEPROM_FILE" ]; then
    EEPROM_OPTION="-U eeprom:w:${EEPROM_FILE}:i"
    echo "Wgrywanie:"
    echo "  1. Flash programu: $FLASH_FILE"
    echo "  2. EEPROM: $EEPROM_FILE"
    echo "  3. Bootloader: $BOOTLOADER_FILE"
else
    echo "UWAGA: Nie znaleziono pliku $EEPROM_FILE - wgrywam bez EEPROM"
    echo "Wgrywanie:"
    echo "  1. Flash programu: $FLASH_FILE"
    echo "  3. Bootloader: $BOOTLOADER_FILE"
fi

echo "  4. Fuse bits (lfuse=0xFD, hfuse=0xDE, efuse=0xF5)"
echo "  5. Lock bits (0x3C)"
echo ""

# Wgraj wszystko - BEZ flagi -D aby kasować EEPROM przy każdym upload
# Stary HFUSE: 0xDA (BOOTSZ=10, 2KB bootloader od 0x3C00) - nieprawidłowy dla urboot 512B
# Nowy HFUSE: 0xDE (BOOTSZ=11, 512B bootloader od 0x7E00) - prawidłowy dla urboot
avrdude -c ${PROGRAMMER} -p ${MCU} -B 5 -F -e -v \
    -U flash:w:${FLASH_FILE}:i \
    ${EEPROM_OPTION} \
    -U flash:w:${BOOTLOADER_FILE}:i \
    -U lfuse:w:0xFD:m \
    -U hfuse:w:0xDE:m \
    -U efuse:w:0xF5:m \
    -U lock:w:0x3C:m


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
