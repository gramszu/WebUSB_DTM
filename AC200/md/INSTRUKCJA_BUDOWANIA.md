# Instrukcja Budowania Aplikacji AC200-DTM-F2

Poniższa instrukcja opisuje krok po kroku, jak skompilować aplikację z kodu źródłowego (`.py`) do pliku wykonywalnego (`.exe`) oraz jak stworzyć instalator (`Setup.exe`).

## Wymagania

1.  **Python 3.8+** (zainstalowany i dodany do PATH)
2.  **Zależności Python:**
    ```powershell
    pip install -r requirements.txt
    ```
3.  **Inno Setup 6** (do tworzenia instalatora)

## Krok 1: Generowanie pliku EXE

Aby zamienić kod Python na aplikację Windows:

1.  Otwórz terminal (PowerShell lub CMD) w folderze `Install`.
2.  Uruchom komendę:
    ```powershell
    pyinstaller AC200-DTM-F2.spec
    ```

**Wynik:**
Gotowa aplikacja pojawi się w folderze:
`Install/dist/AC200-DTM-F2/AC200-DTM-F2.exe`

Aplikacja w tym miejscu jest już w pełni funkcjonalna (można ją uruchomić). Jest to wersja "rozpakowana" (folderowa).

## Krok 2: Tworzenie Instalatora (Setup.exe)

Aby stworzyć jeden plik instalacyjny, który użytkownik może łatwo zainstalować:

1.  Upewnij się, że wykonałeś **Krok 1**.
2.  Kliknij prawym przyciskiem myszy na plik: `Install/AC200-DTM-F2.iss`.
3.  Wybierz opcję **"Compile"** (z menu Inno Setup).
    *   Alternatywnie z terminala (jeśli Inno Setup jest w PATH):
        ```powershell
        iscc AC200-DTM-F2.iss
        ```

**Wynik:**
Gotowy instalator pojawi się w folderze:
`Install/plik_exe/AC200-DTM-F2-Setup.exe`

## Struktura plików

*   `AC200-DTM-F2.py` - Główny kod programu.
*   `AC200-DTM-F2.spec` - Konfiguracja dla PyInstaller (ikony, ukrywanie konsoli, dołączanie plików).
*   `AC200-DTM-F2.iss` - Skrypt dla Inno Setup (tworzenie instalatora).
*   `graphics/` - Ikony i obrazki.
*   `tools/` - Program `avrdude.exe` używany do komunikacji z procesorem.
