@echo off
setlocal

set "ENV_DIR=python\venv"
set "PYTHON_EXE=%ENV_DIR%\Scripts\python.exe"
set "REQUIREMENTS=python\requirements.txt"
set "PYTHON_PATH_FILE=python\python_path.txt"

REM 1. Virtuelle Umgebung erstellen, falls sie nicht existiert
if not exist "%PYTHON_EXE%" (
    echo 📦 Erstelle virtuelle Umgebung: %ENV_DIR%
    python -m venv %ENV_DIR%
) else (
    echo ✅ Umgebung %ENV_DIR% existiert bereits.
)

REM 2. Upgrade pip
call "%PYTHON_EXE%" -m pip install --upgrade pip

REM 3. Installiere requirements
if exist "%REQUIREMENTS%" (
    echo 📥 Installiere Anforderungen aus %REQUIREMENTS%...
    call "%PYTHON_EXE%" -m pip install -r "%REQUIREMENTS%"
) else (
    echo ⚠️  Datei %REQUIREMENTS% nicht gefunden!
)

REM 4. SpaCy-Sprachmodell herunterladen
echo 🧠 Lade deutsches spaCy-Modell 'de_core_news_lg' herunter...
call "%PYTHON_EXE%" -m spacy download de_core_news_lg

REM 5. Schreibe absoluten Pfad zur python.exe in Textdatei
for %%F in ("%PYTHON_EXE%") do set "ABS_PYTHON=%%~fF"
echo %ABS_PYTHON% > "%PYTHON_PATH_FILE%"

echo 🎉 Umgebung %ENV_DIR% ist bereit!
endlocal
