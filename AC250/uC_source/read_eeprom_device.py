#!/usr/bin/env python3
"""
Skrypt do odczytu EEPROM z urządzenia AC800 i wyświetlenia listy numerów.
Użycie: python3 read_eeprom_device.py [PORT]
"""

import subprocess
import sys
import os

# Konfiguracja
AVRDUDE_PATH = "/opt/homebrew/bin/avrdude"  # Systemowy avrdude
MCU = "m1284p"
PROGRAMMER = "avrisp2"
PORT = "usb"  # AVRISP mkII używa USB
EEPROM_FILE = "temp_eeprom_read.bin"

print(f"Odczyt EEPROM z urządzenia przez AVRISP mkII...")
print(f"MCU: {MCU}, Programmer: {PROGRAMMER}")
print()

# Uruchom avrdude
command = [
    AVRDUDE_PATH,
    "-c", PROGRAMMER,
    "-p", MCU,
    "-P", PORT,
    "-U", f"eeprom:r:{EEPROM_FILE}:r"
]

print("Uruchamiam avrdude...")
try:
    result = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False
    )
    
    if result.returncode != 0:
        print(f"Błąd avrdude: {result.stderr}")
        sys.exit(1)
    
    print("✅ Odczyt zakończony pomyślnie!")
    print()
    
except Exception as e:
    print(f"Błąd: {e}")
    sys.exit(1)

# Przeanalizuj dane
if not os.path.exists(EEPROM_FILE):
    print(f"Błąd: Plik {EEPROM_FILE} nie został utworzony")
    sys.exit(1)

with open(EEPROM_FILE, 'rb') as f:
    data = f.read()

print(f"📊 Rozmiar EEPROM: {len(data)} bajtów")
print()

# Kod dostępu
if len(data) >= 5:
    kod = data[1:5]
    kod_ascii = ''.join(chr(b) if 32 <= b < 127 else '.' for b in kod)
    print(f"🔑 Kod dostępu: {kod_ascii}")
else:
    print("⚠️ Brak danych kodu dostępu")

print()

# Konfiguracja
if len(data) > 4094:
    tryb_byte = data[4094]
    tryb = "Publiczny" if tryb_byte == 0x01 else "Prywatny"
    print(f"⚙️ Tryb pracy: {tryb}")
    
if len(data) > 4089:
    skryba_byte = data[4089]
    skryba = "Włączona" if skryba_byte == 0x01 else "Wyłączona"
    print(f"📝 Funkcja Skryba: {skryba}")

if len(data) > 4090:
    start_h = data[4090]
    if start_h != 0xFF:
        start_m = data[4091] if len(data) > 4091 else 0
        stop_h = data[4092] if len(data) > 4092 else 0
        stop_m = data[4093] if len(data) > 4093 else 0
        print(f"⏰ Harmonogram: {start_h:02d}:{start_m:02d} - {stop_h:02d}:{stop_m:02d}")
    else:
        print(f"⏰ Harmonogram: Wyłączony")

print()
print("=" * 60)
print("📞 NUMERY TELEFONÓW")
print("=" * 60)

# Numery telefonów
start_addr = 8
entry_size = 5
max_entries = 800

numery_znalezione = []

for i in range(max_entries):
    addr = start_addr + i * entry_size
    if addr + entry_size > len(data):
        break
    
    chunk = data[addr:addr + entry_size]
    
    # Sprawdź czy numer nie jest pusty
    if chunk != b'\xff\xff\xff\xff\xff':
        # Odwróć bajty i skonwertuj na hex
        hex_str = ''.join(f'{b:02X}' for b in chunk)
        hex_str = hex_str[::-1]  # Odwróć (Little-Endian)
        hex_str = hex_str.replace('F', '')  # Usuń padding
        
        if hex_str:
            numery_znalezione.append((i+1, hex_str))

print(f"\n✅ Znaleziono {len(numery_znalezione)} numerów\n")

# Pokaż pierwsze 20
for pos, numer in numery_znalezione[:20]:
    print(f"  Pozycja {pos:3d}: {numer}")

if len(numery_znalezione) > 20:
    print(f"\n  ... i {len(numery_znalezione) - 20} więcej numerów ...\n")
    
    # Pokaż ostatnie 5
    print("  Ostatnie numery:")
    for pos, numer in numery_znalezione[-5:]:
        print(f"  Pozycja {pos:3d}: {numer}")

print()
print("=" * 60)

# Usuń tymczasowy plik
try:
    os.remove(EEPROM_FILE)
except:
    pass

print(f"\n✅ Gotowe!")
