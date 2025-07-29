#!/usr/bin/env python3
import sys
import whisper
from pyannote.audio import Pipeline

def assign_speakers(transcript_segments, diarization):
    """Ordnet Transkript-Segmenten die erkannten Sprecher zu.

    Die Funktion iteriert durch die von Whisper erkannten Textsegmente und gleicht
    deren Zeitstempel mit den von pyannote erkannten Sprecher-Zeitintervallen
    (diarization) ab.

    Args:
        transcript_segments (list): Eine Liste von Segmenten aus der Whisper-Transkription.
        diarization (pyannote.core.Annotation): Das Diarisierungs-Ergebnis von pyannote.

    Returns:
        list: Eine Liste von Segmenten, bei denen jedes einen "speaker"-Schlüssel enthält.
    """
    final_output = []
    for seg in transcript_segments:
        t_start, t_end = seg["start"], seg["end"]
        speaker = "UNKNOWN"
        #  Finde heraus, welcher Sprecher im Zeitintervall des Textsegments aktiv war.
        for turn, _, label in diarization.itertracks(yield_label=True):
            if turn.start <= t_start and turn.end >= t_end:
                speaker = label
                break
        final_output.append({
            "start": t_start,
            "end": t_end,
            "speaker": speaker,
            "text": seg["text"].strip()
        })
    return final_output

def main():
    """Hauptfunktion des Skripts zur Transkription und Sprecherzuordnung."""
    if len(sys.argv) != 2:
        print(f"Benutzung: {sys.argv[0]} <audio_datei.wav>", file=sys.stderr)
        sys.exit(1)

    audio_path = sys.argv[1]
    #  WICHTIGER HINWEIS: Der Hugging Face Token wird für das pyannote-Modell benötigt.
    #  Er sollte idealerweise als Umgebungsvariable gesetzt oder sicher verwaltet werden.
    HF_TOKEN = os.environ["HF_TOKEN"]

    #  Schritt 1: Transkription mit Whisper ASR
    print("Starte Transkription mit Whisper...", file=sys.stderr)
    model = whisper.load_model("large")
    result = model.transcribe(audio_path, language="de", fp16=False)
    segments = result["segments"]
    print("Transkription beendet.", file=sys.stderr)

    #  Schritt 2: Sprecher-Diarisierung mit pyannote
    print("Starte Sprecher-Diarisierung mit pyannote...", file=sys.stderr)
    try:
        pipeline = Pipeline.from_pretrained(
            "pyannote/speaker-diarization-3.0",
            use_auth_token=HF_TOKEN
        )
    except Exception as e:
        print("FEHLER beim Laden der Diarisierungs-Pipeline:", e, file=sys.stderr)
        print("Bitte die Modell-Bedingungen akzeptieren auf:", file=sys.stderr)
        print("  https://huggingface.co/pyannote/speaker-diarization-3.0", file=sys.stderr)
        sys.exit(1)

    diarization = pipeline(audio_path)
    print("Diarisierung beendet.", file=sys.stderr)

    #  Schritt 3: Ergebnisse zusammenführen
    final_output = assign_speakers(segments, diarization)

    #  Schritt 4: Formattierte Ausgabe für die C++ Anwendung
    for entry in final_output:
        start = entry["start"]
        end = entry["end"]
        speaker = entry["speaker"]
        text = entry["text"]
        print(f"[{start:.2f}s --> {end:.2f}s] {speaker}: {text}")

if __name__ == "__main__":
    main()
