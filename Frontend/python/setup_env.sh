#!/bin/bash

ENV_NAME="python/venv"

# 1. Existiert venv schon?
if [ ! -d "$ENV_NAME" ]; then
    echo "📦 Erstelle virtuelle Umgebung: $ENV_NAME"
    python3 -m venv "$ENV_NAME"
else
    echo "✅ Umgebung $ENV_NAME existiert bereits."
fi

# 2. Aktivieren und pip aktualisieren
source "$ENV_NAME/bin/activate"

echo "⬆️  Aktualisiere pip..."
pip install --upgrade pip --break-system-packages

# 3. Anforderungen installieren
if [ -f python/requirements.txt ]; then
    echo "📥 Installiere Anforderungen aus requirements.txt..."
    pip install -r python/requirements.txt --break-system-packages
else
    echo "⚠️  Keine requirements.txt gefunden!"
fi

# 4. SpaCy-Sprachmodell herunterladen
echo "🧠 Lade deutsches spaCy-Modell 'de_core_news_lg' herunter..."
python -m spacy download de_core_news_lg

# 5. Erfolgsmeldung
echo "🎉 Umgebung $ENV_NAME ist bereit!"

# 5. Aktuellen Pythonpfad aus aktiver Umgebung bestimmen
CURRENT_PYTHON="$(which python3)"
echo "$CURRENT_PYTHON" > python/python_path.txt
