/**
 * @file transcriptpdfexporter.h
 * @brief Enthält die Deklaration der TranscriptPdfExporter-Klasse.
 * @author Mike Wild
 */
#ifndef TRANSCRIPTPDFEXPORTER_H
#define TRANSCRIPTPDFEXPORTER_H

#include <QString>

// Forward-Deklarationen
class Transcription;
class QPdfWriter;

/**
 * @brief Erstellt eine formatierte, mehrseitige PDF-Repräsentation eines Transcription-Objekts.
 *
 * Diese Klasse ist als "One-Shot"-Utility konzipiert. Man erstellt ein Objekt,
 * das die zu exportierenden Daten und die Layout-Einstellungen (aus QSettings)
 * einliest. Die Methode exportToPdf() generiert dann die fertige Datei.
 */
class TranscriptPdfExporter
{
public:
    /**
     * @brief Konstruktor, der das zu exportierende Transkript entgegennimmt und die Layout-Einstellungen lädt.
     * @param transcription Das Transcription-Datenmodell, das als PDF exportiert werden soll.
     */
    explicit TranscriptPdfExporter (const Transcription &transcription);

    /**
     * @brief Führt den Export durch und speichert das Ergebnis im angegebenen Dateipfad.
     *
     * Diese Methode orchestriert den gesamten Prozess: Erstellen des HTML-Inhalts,
     * Aufsetzen des QTextDocument und Aufruf der print()-Funktion, welche die
     * Paginierung automatisch handhabt.
     * @param filePath Der vollständige Pfad, unter dem die PDF-Datei gespeichert werden soll.
     * @return true bei Erfolg, andernfalls false.
     */
    bool exportToPdf (const QString &filePath) const;

private:
    /** @brief Konfiguriert das QPdfWriter-Objekt mit Seitengröße, Auflösung und Rändern. */
    void setupPdfWriter (QPdfWriter &writer) const;

    /** @brief Erstellt den gesamten HTML-Code für das PDF-Dokument. */
    void buildHtmlContent (QString &html) const;

    // calculateDuration() ist eine private Hilfsfunktion und muss nicht im Header deklariert werden.

    // --- Member-Variablen ---
    const Transcription
        &m_transcription; ///< Eine konstante Referenz auf das zu exportierende Datenmodell.

    // Geladene Einstellungen für das Layout
    int m_fontSizeHeadline; ///< Schriftgröße für die Hauptüberschrift in pt.
    int m_fontSizeMetadata; ///< Schriftgröße für den Metadaten-Block in pt.
    int m_fontSizeBody;     ///< Schriftgröße für den Haupttext (Dialog) in pt.
    QString m_fontFamily;   ///< Name der zu verwendenden Schriftfamilie (z.B. "sans-serif").
    int m_marginLeft;       ///< Linker Seitenrand in mm.
    int m_marginTop;        ///< Oberer Seitenrand in mm.
    int m_marginRight;      ///< Rechter Seitenrand in mm.
    int m_marginBottom;     ///< Unterer Seitenrand in mm.
};

#endif // TRANSCRIPTPDFEXPORTER_H
