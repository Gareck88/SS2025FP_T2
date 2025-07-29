/**
 * @file transcription.h
 * @brief Enthält die Deklaration der Datenmodell-Klassen Transcription und MetaText.
 * @author Mike Wild
 */
#ifndef TRANSCRIPTION_H
#define TRANSCRIPTION_H

#include <QColor>
#include <QDateTime>
#include <QJsonDocument> // Nötig für den Rückgabetyp von toJson()
#include <QList>
#include <QObject>
#include <QString>

/**
 * @struct MetaText
 * @brief Eine einfache Datenstruktur, die ein einzelnes Segment eines Transkripts repräsentiert.
 *
 * Enthält den Text selbst sowie Metadaten wie Sprecher, Zeitstempel und zugeordnete Tags.
 */
struct MetaText
{
    MetaText () = default;
    MetaText (
        const QString &start, const QString &end, const QString &speaker, const QString &text)
        : Speaker (speaker)
        , Text (text)
        , Start (start)
        , End (end)
    {
    }

    QString Speaker;  ///< Der Name des Sprechers für dieses Segment.
    QString Text;     ///< Der transkribierte Text des Segments.
    QString Start;    ///< Start-Zeitstempel des Segments (als String in Sekunden).
    QString End;      ///< End-Zeitstempel des Segments (als String in Sekunden).
    QStringList Tags; ///< Eine Liste von Tags, die diesem spezifischen Segment zugeordnet sind.

    void addTag (
        const QString &tag)
    {
        if (!Tags.contains (tag))
            Tags.append (tag);
    }
    void removeTag (
        const QString &tag)
    {
        Tags.removeAll (tag);
    }
    bool hasTag (
        const QString &tag) const
    {
        return Tags.contains (tag);
    }
};

// Definiert den aktuellen Anzeigemodus des Transkripts
enum class TranscriptionViewMode {
    Original,
    Edited
};

/**
 * @author Yolanda Fiska
 * @class TranscriptionViewMode
 * @brief Definiert den aktuellen Anzeigemodus des Transkripts
 */
enum class TranscriptionViewMode
{
    Original,
    Edited
};

/**
 * @class Transcription
 * @brief Das zentrale Datenmodell für ein komplettes Meeting-Transkript.
 *
 * Diese Klasse kapselt alle Informationen für eine einzelne Aufnahmesession,
 * inklusive der Metadaten (Meeting-Name, Datum), einer Liste aller Textsegmente
 * (`MetaText`) und globaler Tags. Sie bietet Methoden zur Bearbeitung und zur
 * Serialisierung des gesamten Objekts nach/von JSON.
 * Änderungen am Zustand werden über Signale (`changed`, `edited`) mitgeteilt.
 */
class Transcription : public QObject
{
    Q_OBJECT
public:
    explicit Transcription (QObject *parent = nullptr);

    /** @brief Gibt den reinen, zusammenhängenden Text aller Segmente zurück. */
    QString text () const;

    /**
     * @brief Erzeugt eine farbige HTML-Repräsentation des Transkripts für die Anzeige.
     * @note Diese Methode koppelt das Datenmodell an die Darstellung. Ein alternativer Ansatz wäre,
     * das HTML direkt in der UI-Schicht (z.B. in MainWindow) zu generieren.
     * @return Ein HTML-formatierter QString.
     */
    QString script () const;

    /** @brief Ändert einen Sprechernamen global im gesamten Transkript. */
    bool changeSpeaker (const QString &oldSpeaker, const QString &newSpeaker);

    /** @brief Ändert den Text eines einzelnen, durch Zeitstempel identifizierten Segments. */
    bool changeText (const QString &start, const QString &end, const QString &newText);

    /** @brief Ändert den Sprecher für ein einzelnes, durch Zeitstempel identifiziertes Segment. */
    bool changeSpeakerForSegment (const QString &start,
                                  const QString &end,
                                  const QString &newSpeaker);

    /** @brief Gibt eine konstante Referenz auf die Liste aller Textsegmente zurück. */
    const QList<MetaText> &getMetaTexts () const { return m_content; }

    /** @brief Serialisiert den gesamten Zustand des Objekts in ein QJsonDocument. */
    QJsonDocument toJson () const;

    /** @brief Füllt das Objekt mit Daten aus einem JSON-Byte-Array. Gibt bei Erfolg true zurück. */
    bool fromJson (const QByteArray &data);

    // --- Getter für Metadaten ---
    QString name () const { return m_meetingName; }
    QDateTime dateTime () const { return m_startTime; }
    QString getDurationAsString () const;

    bool isEdited () const { return m_changed; }
    void setEdited (
        bool value)
    {
        m_changed = value;
    }

    // --- Tag-Management ---
    QStringList tags () const { return m_tags; }
    void setTags (const QStringList &tags);
    void addTag (const QString &tag);
    void removeTag (const QString &tag);
    bool hasTag (const QString &tag) const;
    QList<MetaText> segmentsWithTag (const QString &tag) const;

    /** @brief vergleicht die Inhalt von zwei Transkriptionen. */
    bool isContentEqual(const Transcription* other) const;
public slots:
    /** @brief Fügt ein neues Textsegment zum Transkript hinzu. */
    void add (const MetaText &part);

    /** @brief Löscht alle Inhalte und Metadaten des Transkripts. */
    void clear ();

    /** @brief Startet einen Batch-Update-Modus, um mehrere Änderungen ohne exzessive Signal-Emissionen durchzuführen. */
    void beginBatchUpdate ();

    /** @brief Beendet den Batch-Update-Modus und sendet ggf. ein einzelnes `changed()`-Signal. */
    void endBatchUpdate ();

    /** @brief Setzt den Namen des Meetings. */
    void setName (const QString &name);

    /** @brief Setzt das Startdatum und die Uhrzeit des Meetings. */
    void setDateTime (QDateTime dateTime);

    /** @author Yolanda Fiska
     *  @brief Setzt den aktuellen Anzeigemodus (Original oder Bearbeitet). */
    void setViewMode (TranscriptionViewMode mode);

    /** @author Yolanda Fiska
     *  @brief Gibt den aktuellen Anzeigemodus zurück*/
    TranscriptionViewMode getViewMode () const;

signals:
    /** @brief Wird bei jeder sichtbaren Änderung der Daten gesendet. Dient der UI-Aktualisierung. */
    void changed ();

    /** @brief Wird bei jeder Änderung gesendet, die für die Undo/Redo-Funktionalität relevant ist. */
    void edited ();

private:
    /** @brief Generiert eine deterministische Farbe basierend auf dem Sprechernamen. */
    QColor speakerColor (const QString &speaker) const;

    QList<MetaText> m_content; ///< Die Liste aller transkribierten Textsegmente.
    QStringList m_tags;        ///< Globale Tags, die für das gesamte Meeting gelten.

    // Zähler für den internen Zustand
    int m_unknownCounter{0};     ///< Zähler für die Benennung anonymer Sprecher.
    int m_batchUpdateCounter{0}; ///< Zähler für verschachtelte Batch-Updates.
    bool m_changesPending
        = false; ///< Flag, das merkt, ob während eines Batch-Updates Änderungen aufgetreten sind.
    bool m_changed; ///< Flag, das merkt, ob der transkribierte Text verarbeitet wurde.

    // Meeting-Metadaten
    QString m_meetingName; ///< Der Name des Meetings.
    QDateTime m_startTime; ///< Das Startdatum und die -uhrzeit des Meetings.
    TranscriptionViewMode viewMode = TranscriptionViewMode::Edited; // Standardmäßig Bearbeitet anzeigen
};

#endif // TRANSCRIPTION_H
