# AudioTranskriptor — KI-gestützte Meeting-Transkription

Plattformübergreifende Desktop-App (Qt/C++) zur Aufnahme von **System-** und **Mikrofon-Audio**, Speicherung als **WAV** (HQ + ASR-optimiert), anschließender **ASR-Transkription** (Python) sowie **Tag-Generierung** (z. B. via spaCy), **PDF-Export** und optionaler **Datenbank-Persistenz**.

> Zielplattformen: **Windows** (WASAPI Loopback), **Linux** (PulseAudio).
> **macOS (CoreAudio)** und **Echtzeit-Transkription (Streaming-ASR)** sind **implementiert auf separaten Branches**; siehe Abschnitt *Branches & Varianten*.

---

## Features

- **Dual Capture**: gleichzeitige Erfassung von System- und Mikrofon-Audio
  - Windows: WASAPI Loopback
  - Linux: PulseAudio (`module-null-sink` + `module-loopback`)
  - macOS: **CoreAudio** — *verfügbar auf separatem Branch* (s. u.)
- **Robuste Persistenz**:
  - HQ-WAV (Float32, 48 kHz, Stereo)
  - ASR-WAV (PCM16, 16 kHz, Mono)
- **ASR-Pipeline (Python)**: Start/Überwachung per `QProcess`, segmentweise Übergabe ins UI
- **Tag-Generator (Python)**: Tags aus Transkripten (z. B. spaCy)
- **GUI-Workflow**: Start/Stop, Fortschritt, Sprecher-Editor, Text-Editor, Suche, Multi-Suche
- **Export**: PDF-Export des Transkripts
- **DB-Integration (optional)**: Speichern/Aktualisieren (z. B. Supabase) über Einstellungsdialog
- **Streaming-ASR (Echtzeit)** — *verfügbar auf separatem Branch* (s. u.)

---

## Branches & Varianten

Diese Funktionen sind bereits implementiert, **aber noch nicht in `main` gemergt**:

- **macOS-Capture (CoreAudio)**: siehe Branch **`paki_branch`**
  Bietet Audioaufnahme unter macOS analog zur Template-Methodik der Capture-Schicht.
- **Echtzeit-Transkription (Streaming-ASR)**: siehe Branch **`fabian_branch`**
  Liefert Transkript-Updates während der Aufnahme in (nahezu) Echtzeit.

---

## Architektur (Kurzüberblick)

- **GUI/Orchestrierung**: `MainWindow`
- **Audioaufnahme**: `CaptureThread` (Basis) + `PulseCaptureThread` (Linux) / `WinCaptureThread` (Windows) / *(macOS auf Feature-Branch)*
- **Dateischreiben**: `WavWriterThread` (Producer–Consumer, Downmix + Downsampling)
- **ASR**: `AsrProcessManager` (Python-Prozess, Streaming von Segmenten)
- **Tags**: `TagGeneratorManager` (Python-Prozess → Liste von Tags)
- **Utilities**: `PythonEnvironmentManager`, `TranscriptPdfExporter`, `FileManager`, `DatabaseManager`

Dokumentation (LaTeX): siehe `/Dokumentation/FP-T2-Dokumentation.pdf` im Repo.

---

## Build & Run

### Voraussetzungen

- **Qt** (5.15+ oder 6.x) mit C++17
- **CMake** (empfohlen) oder qmake/Qt Creator
- **Python 3.10+** (für ASR/Tags; wird von der App via `PythonEnvironmentManager` genutzt)
- **Windows**: Windows SDK (WASAPI verfügbar)
- **Linux**: PulseAudio + Dev-Headers, `pactl`
- **macOS (Feature-Branch)**: Xcode/Command Line Tools; CoreAudio-Headers (System)


### Bauen (CMake, Release)

```bash
git clone https://github.com/<org>/<repo>.git
cd <repo>
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

Für die Feature-Branches vorab auf den gewünschten Branch wechseln:
```bash
git checkout paki_branch      # CoreAudio-Backend für MacOS
git checkout fabian_branch    # Streaming-ASR (Echtzeit)
```
Anschließend mittels QtCreator kompilieren (und ggf. ausführen).

---

### Starten

```bash
# Aus Build-Ordner
./build/AudioTranskriptor
```
Beim ersten Start wird automatisch einmalig eine virtuelle Python-Umgebung erstellt. Danach sollten zunächst die Einstellungen überprüft / ausgefüllt werden:

1. **Einstellungen** öffnen → Python-Interpreter/Skripte prüfen (oder Neuinstallation der venv auslösen).
2. (Optional) **Datenbank** einrichten (Supabase-URL/Key).
3. Gains (sys/mic) bei Bedarf so einstellen, dass Systemaudio und Mikrofonaudio etwa gleich laut sind.

---

### Python-Umgebung & Skripte

Die App kann eine isolierte Python-Umgebung anlegen und die benötigten Pakete installieren. Dies kann man ggf. auch manuell ohne die Applikation in einer Konsole durchführen.

Manuell (optional):

```bash
python -m venv .venv
source .venv/bin/activate  # Windows: .venv\Scripts\activate
pip install -U pip wheel
pip install -r requirements.txt
python -m spacy download de_core_news_md
```

---

### Nutzung (Kurzguide)

1. **Start** klicken → Aufnahme läuft, Zeit zählt.
2. **Stop** klicken → Writer finalisiert WAVs; ASR-Prozess startet automatisch (oder Streaming-ASR auf Feature-Branch).
3. **Transkript** erscheint segmentweise; optional **Sprecher zuweisen**, **Text bearbeiten**.
4. **Tags generieren** → Ergebnis wird ins Transkript übernommen.
5. **PDF exportieren** und/oder in **DB speichern**.
6. **Audio speichern** (HQ-WAV, optional).
