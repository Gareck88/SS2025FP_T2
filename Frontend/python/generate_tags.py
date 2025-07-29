# generate_tags.py
import sys
import spacy
from collections import Counter

def generate_tags(text):
    """Analysiert einen deutschen Text und extrahiert zwei Arten von Tags.

    Die Funktion nutzt das spaCy-Modell 'de_core_news_lg' zur Analyse. Es werden
    zwei Strategien zur Tag-Findung kombiniert:
    1.  Named Entity Recognition (NER) zur Extraktion von Personen, Orten und Organisationen.
    2.  Keyword-Extraktion durch Zählung der häufigsten Substantive (in ihrer Grundform).

    Args:
        text (str): Der vollständige Text des Transkripts, der analysiert werden soll.

    Returns:
        list: Eine alphabetisch sortierte Liste der gefundenen, einzigartigen Tags.
    """
    try:
        nlp = spacy.load("de_core_news_lg")
    except OSError:
        #  Gibt eine klare Fehlermeldung aus, wenn das Sprachmodell fehlt.
        print("FEHLER: Deutsches spaCy-Modell 'de_core_news_lg' nicht gefunden.", file=sys.stderr)
        print("Bitte mit 'python -m spacy download de_core_news_lg' installieren.", file=sys.stderr)
        sys.exit(1)

    doc = nlp(text)
    tags = set()

    # --- Methode 1: Eigennamen extrahieren (NER) ---
    allowed_entities = ["PER", "LOC", "ORG", "MISC"]
    for ent in doc.ents:
        if ent.label_ in allowed_entities:
            tags.add(ent.text.strip())

    # --- Methode 2: Schlüsselwörter (häufigste Substantive) extrahieren ---
    keywords = []
    for token in doc:
        if (token.pos_ in ["NOUN", "PROPN"] and not token.is_stop and not token.is_punct):
            #  Verwendung der Grundform (Lemma), um Wortvarianten zu gruppieren.
            keywords.append(token.lemma_.capitalize())

    #  Fügt die 10 häufigsten gefundenen Schlüsselwörter zu den Tags hinzu.
    most_common_keywords = Counter(keywords).most_common(10)
    for keyword, count in most_common_keywords:
        tags.add(keyword)

    return sorted(list(tags))


if __name__ == "__main__":
    #  Das Skript erwartet den zu analysierenden Text über die Standardeingabe (stdin),
    #  da dies für lange Texte robuster ist als Kommandozeilenargumente.
    input_text = sys.stdin.read()

    if input_text:
        generated_tags = generate_tags(input_text)
        #  Gibt jedes gefundene Tag in einer neuen Zeile aus. Dieses Format
        #  kann von der aufrufenden C++-Anwendung einfach geparst werden.
        for tag in generated_tags:
            print(tag)
