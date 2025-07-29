#!/bin/bash

ENV_NAME="python/venv"

# 1. Existiert venv schon?
if [ ! -d "$ENV_NAME" ]; then
    echo "📦 Erstelle virtuelle Umgebung: $ENV_NAME" >&2
    python3 -m venv "$ENV_NAME"
else
    echo "✅ Umgebung $ENV_NAME existiert bereits." >&2
fi

# 2. Aktivieren und pip aktualisieren
source "$ENV_NAME/bin/activate"

# 3. pip aktualisieren
echo "⬆️  Aktualisiere pip..." >&2
python -m pip install --upgrade pip

# 4. Sicherstellen, dass CMake installiert ist (erforderlich für sentencepiece)
if ! command -v cmake &> /dev/null; then
    echo "🔧 Installiere CMake (für sentencepiece)..." >&2
    python -m pip install cmake
else
    echo "✅ CMake ist bereits installiert." >&2
fi

# 5. Anforderungen installieren
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REQ_FILE="$SCRIPT_DIR/requirements.txt"

if [ -f "$REQ_FILE" ]; then
    echo "📥 Installiere Anforderungen aus requirements.txt..." >&2
    python -m pip install -r "$REQ_FILE"
else
    echo "⚠️  requirements.txt nicht gefunden unter: $REQ_FILE" >&2
fi

# 5. SpaCy-Sprachmodell herunterladen
if ! python -c "import spacy" 2>/dev/null; then
    echo "📦 Installiere spaCy..." >&2
    python -m pip install spacy
fi

# 6. Download deutsches Sprachmodell
echo "🧠 Lade deutsches spaCy-Modell 'de_core_news_lg' herunter..." >&2
python -m spacy download de_core_news_lg

# 7. Erfolgsmeldung
echo "🎉 Umgebung $ENV_NAME ist bereit!" >&2

# 8.  Aktuellen Pythonpfad aus aktiver Umgebung bestimmen
CURRENT_PYTHON="$(which python)"
echo "$CURRENT_PYTHON" > python/python_path.txt
