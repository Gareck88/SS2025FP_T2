#@author Fabian Scherer

import torch
import numpy as np
import time
import collections
import os
import sys
import json

# --- Importe für Diarisierung ---
from pyannote.audio import Pipeline

# --- Festlegen der Cache-Pfade (Standard und persistent auf Linux) ---
new_base_cache_dir = os.path.expanduser("~/.cache/my_asr_models")
os.makedirs(new_base_cache_dir, exist_ok=True)

os.environ['HF_HOME'] = os.path.join(new_base_cache_dir, "huggingface")
os.makedirs(os.environ['HF_HOME'], exist_ok=True)
print(f"Hugging Face Cache-Pfad gesetzt auf: {os.environ.get('HF_HOME')}")
print(f"Modelle werden in {os.path.join(os.environ.get('HF_HOME'), 'hub')} heruntergeladen.")

pyctcdecode_cache_dir = os.path.join(new_base_cache_dir, "pyctcdecode_cache")
os.makedirs(pyctcdecode_cache_dir, exist_ok=True)
os.environ['PYCTCDECODE_HOME'] = pyctcdecode_cache_dir
print(f"pyctcdecode Cache-Pfad gesetzt auf: {os.environ.get('PYCTCDECODE_HOME')}")


# --- Importe von Hugging Face Transformers ---
from transformers import AutoProcessor, AutoModelForCTC

# --- Konfiguration für Audioaufnahme und ASR ---

SAMPLE_RATE = 16000 # Dies MUSS 16kHz sein für Wav2Vec2 und pyannote
CHUNK_SIZE_BYTES = 32000 # Audio-Chunk, der von der Pipe gelesen wird soll 1s lang sein (heißt doppelte samplerate, da 16bit integer)
DIARIZATION_CHUNK_SIZE = 3 * SAMPLE_RATE # 3 Sekunden Audio-Chunk für Diarisierung
DIARIZATION_OVERLAP_SAMPLES = int(SAMPLE_RATE * 1.5) # 1.5 Sekunden Überlappung für Diarisierung


# --- Gerät festlegen (CPU oder GPU) ---
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(f"Verwende Gerät: {device}")

# --- Wav2Vec2-Modell über Hugging Face laden ---
MODEL_ID = "aware-ai/wav2vec2-large-xlsr-53-german-with-lm" 

def send_to_qt(message):
    try:
        json_message = json.dumps(message) + "\n" # JSON-Nachricht und Zeilenumbruch
        sys.stdout.write(json_message)           # An Standardausgabe schreiben
        sys.stdout.flush()                       # WICHTIG: Puffer sofort leeren
    except Exception as e:
        print(f"Fehler beim Senden an Qt über stdout: {e}", file=sys.stderr)


print(f"\nLade Wav2Vec2 ASR-Modell '{MODEL_ID}' und Processor (Tokenizer/Decoder)...")
def load_models():
    global processor, vad_pipeline, speaker_embedding_pipeline, model
    try:
        processor = AutoProcessor.from_pretrained(MODEL_ID)
        model = AutoModelForCTC.from_pretrained(MODEL_ID).to(device)
        model.eval()
        print("Wav2Vec2-Modell und Processor erfolgreich geladen.")
        print(f"Modell-Logits-Größe (letzte Dimension): {model.config.vocab_size}")


    except Exception as e:
        print(f"Fehlermeldung: {e}")
        exit()

    # --- Pyannote.audio Diarisierungs-Pipeline laden ---
    print(f"\nLade Pyannote.audio Diarisierungs-Pipeline...")
    try:
        # Pyannote benötigt einen HF-Token. Stelle sicher, dass du 'huggingface-cli login' ausgeführt hast.
        speaker_embedding_pipeline = Pipeline.from_pretrained("pyannote/speaker-diarization-3.1",
                                     use_auth_token=os.environ["HF_TOKEN"])
        print("Pyannote.audio Diarisierungs-Pipeline erfolgreich geladen.")
    except Exception as e:
        print(f"Fehler beim Laden der Pyannote.audio Pipeline: {e}")
        print("Stelle sicher, dass du 'huggingface-cli login' ausgeführt und einen Token mit Leseberechtigungen gesetzt hast.")
        exit()

# --- Hauptlogik des Skripts ---
if __name__ == "__main__":
    if len(sys.argv) != 1:
        print("Verwendung: python asr_backend.py", file=sys.stderr)
        sys.exit(1)
    # *** Hier die Modelle laden ***
    load_models()
    # *** Statusmeldung an Qt senden: Backend ist bereit ***
    send_to_qt({"type": "ready", "message": "ready", "details": "Models loaded and backend ready for transcription."})
    
    full_diarization_buffer = np.array([], dtype=np.int16)
    diarization_buffer_start_time = 0.0 # Um relative Zeitstempel zu verfolgen

    transcription_history = collections.deque(maxlen=2)
    current_asr_audio_accumulator = np.array([], dtype=np.int16)
    last_printed_text = ""
    speaker_identified = "Unbekannt" # Initialisieren des Sprechers
    while True:#stream.is_active():

        chunk = sys.stdin.buffer.read(CHUNK_SIZE_BYTES)

        if not chunk:
            # Dies bedeutet, dass das Ende des Streams (EOF) erreicht wurde.
            # Normalerweise ist dies ein Signal von der C++-Anwendung (QProcess),
            # dass sie keine weiteren Daten senden wird (z.B. wenn der QProcess::write-Stream geschlossen wird).
            # Das Python-Skript sollte sich dann sauber beenden.
            print("EOF auf stdin erreicht. Beende ASR-Backend.", file=sys.stderr)
            break # Beendet die Schleife und führt zum finally-Block
        

        audio_chunk_np = np.frombuffer(chunk, dtype='int16')

        current_asr_audio_accumulator = np.concatenate((current_asr_audio_accumulator, audio_chunk_np))
        full_diarization_buffer = np.concatenate((full_diarization_buffer, audio_chunk_np))
        
        # --- Diarisierungs-Logik ---
        if full_diarization_buffer.shape[0] >= (DIARIZATION_CHUNK_SIZE):
                    
            diarization_chunk_np = full_diarization_buffer[:DIARIZATION_CHUNK_SIZE]
            diarization_chunk_tensor = torch.from_numpy(diarization_chunk_np.astype(np.int16) / 32768.0).float().unsqueeze(0).to(device)
            
            # Führe die gesamte Speaker Diarization Pipeline auf dem Chunk aus.
            diarization_output = speaker_embedding_pipeline({"waveform": diarization_chunk_tensor, "sample_rate": SAMPLE_RATE})
            
            speaker_identified = "Unbekannt" # Setze Standardwert für diesen Chunk
            if diarization_output:
                for segment, _, label in diarization_output.itertracks(yield_label=True):
                    speaker_identified = label 
                    break # Nimm den ersten erkannten Sprecher dieses Chunks
            
            full_diarization_buffer = full_diarization_buffer[DIARIZATION_CHUNK_SIZE - DIARIZATION_OVERLAP_SAMPLES:]
            diarization_buffer_start_time += (DIARIZATION_CHUNK_SIZE - DIARIZATION_OVERLAP_SAMPLES) / SAMPLE_RATE
        # --- ASR-Logik ---
        if current_asr_audio_accumulator.shape[0] >= SAMPLE_RATE:
            print("ASR part", file=sys.stderr)
                    
            process_chunk_np = current_asr_audio_accumulator[:SAMPLE_RATE]
            current_asr_audio_accumulator = current_asr_audio_accumulator[SAMPLE_RATE:]
            audio_input = torch.from_numpy(process_chunk_np.astype(np.int16) / 32768.0).float().to(device)
            with torch.inference_mode():
                input_values = processor(audio_input, sampling_rate=SAMPLE_RATE, return_tensors="pt").input_values
                input_values = input_values.to(device)
                logits = model(input_values).logits
                raw_decode_output = processor.batch_decode(logits.cpu().numpy())
                
                transcribed_text_chunk = "" # Standardwert
                # Zugriff auf das 'text'-Attribut des Wav2Vec2DecoderWithLMOutput-Objekts
                # Das 'text'-Attribut ist eine Liste (z.B. ['Dies ist ein Test.'])
                if hasattr(raw_decode_output, 'text') and isinstance(raw_decode_output.text, list) and len(raw_decode_output.text) > 0:
                    transcribed_text_chunk = raw_decode_output.text[0] # Nimm den ersten (und einzigen) String
                transcribed_text_chunk = transcribed_text_chunk.strip()
                transcribed_text_chunk = " ".join(transcribed_text_chunk.split())
                if transcribed_text_chunk: # Wird nur True, wenn der String nicht leer ist
                    
                    print(f"transcribed: {transcribed_text_chunk}", file=sys.stderr)
                    current_output = ""
                    if transcription_history:
                        last_full_text = transcription_history[-1]
                        overlap_found = False
                        max_overlap_chars = min(len(last_full_text), len(transcribed_text_chunk))
                        for i in range(max_overlap_chars, 0, -1):
                            if last_full_text.lower().endswith(transcribed_text_chunk[:i].lower()):
                                current_output = last_full_text + transcribed_text_chunk[i:]
                                overlap_found = True
                                break
                        if not overlap_found:
                            current_output = last_full_text + " " + transcribed_text_chunk
                    else:
                        current_output = transcribed_text_chunk.strip()
                    #final_output = f"[{speaker_identified}] {current_output.strip()}" 
                    if current_output.lower() != last_printed_text.lower():
                        message = {
                            "type": "transcription",
                            "speaker": speaker_identified,
                            "text": transcribed_text_chunk.strip()
                        }
                        send_to_qt(message)
                        last_printed_text = current_output
                        transcription_history.append(current_output)
            
        time.sleep(0.01)
    print("ASR-Backend beendet.", file=sys.stderr)
    sys.exit(0)
