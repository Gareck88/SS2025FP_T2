#include "transcription.h"

#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>

Transcription::Transcription (
    QObject *parent)
    : QObject (parent)
    , m_unknownCounter (0)
    , m_batchUpdateCounter (0)
    , m_changesPending (false)
{
}

//--------------------------------------------------------------------------------------------------

QString Transcription::text () const
{
    //  Erstellt einen zusammenhängenden String aus allen Text-Segmenten, getrennt durch Leerzeichen.
    QString erg;
    for (const auto &item : m_content)
    {
        if (!erg.isEmpty ())
        {
            erg += " ";
        }
        erg += item.Text;
    }

    return erg;
}

//--------------------------------------------------------------------------------------------------

QString Transcription::script () const
{
    //  Erstellt eine HTML-formatierte Repräsentation des Transkripts für die Anzeige im QTextEdit.
    QString erg;
    for (const auto &item : m_content)
    {
        //  Jedem Sprecher wird eine deterministische Farbe basierend auf seinem Namen zugewiesen.
        QColor farbe = speakerColor (item.Speaker);
        erg += QString ("<font color='%1'>").arg (farbe.name ());
        //  HINWEIS: Die Zeile `item.Start += ...` modifiziert den Start-String des Objekts.
        //  Dies ist wahrscheinlich nicht beabsichtigt.
        erg += '[' + item.Start + "s - " + item.End + "s] <b>" + item.Speaker + ":</b>";
        erg += "&nbsp;&nbsp;&nbsp;&nbsp;" + item.Text.toHtmlEscaped () + " </font> <br>";
    }

    return erg;
}

//--------------------------------------------------------------------------------------------------

bool Transcription::changeSpeaker (
    const QString &oldSpeaker, const QString &newSpeaker)
{
    bool erg = false;

    //  Iteriert durch alle Segmente und ersetzt den alten Sprechernamen durch den neuen.
    for (auto &item : m_content)
    {
        if (item.Speaker == oldSpeaker)
        {
            erg = true;
            item.Speaker = newSpeaker;
        }
    }

    if (erg)
    {
        //  Die Signale werden nur gesendet, wenn tatsächlich eine Änderung stattgefunden hat.
        //  Das `changed`-Signal wird durch den Batch-Mechanismus eventuell unterdrückt.
        if (m_batchUpdateCounter > 0)
        {
            m_changesPending = true;
        }
        else
        {
            emit changed ();
        }

        //  Das `edited`-Signal wird immer gesendet, um die Undo/Redo-Funktionalität zu triggern.
        emit edited ();
    }

    return erg;
}

//--------------------------------------------------------------------------------------------------

bool Transcription::changeText (
    const QString &start, const QString &end, const QString &newText)
{
    bool erg = false;

    //  Findet das spezifische Segment anhand der exakten Zeitstempel und aktualisiert den Text.
    for (qsizetype i = 0; i < m_content.length (); i++)
    {
        if (m_content[i].Start == start && m_content[i].End == end)
        {
            m_content[i].Text = newText;
            erg = true;
            break; //  Da jedes Segment einzigartig sein sollte, kann die Schleife hier abbrechen.
        }
    }

    if (erg)
    {
        if (m_batchUpdateCounter > 0)
        {
            m_changesPending = true;
        }
        else
        {
            emit changed ();
        }
        emit edited ();
    }

    return erg;
}

//--------------------------------------------------------------------------------------------------

bool Transcription::changeSpeakerForSegment (
    const QString &start, const QString &end, const QString &newSpeaker)
{
    bool hasChanged = false;
    for (MetaText &item : m_content)
    {
        //  Hier ist Vorsicht geboten bei String-Vergleichen von Zeitstempeln
        //  Besser wäre es, die Zeitstempel als numerische Werte zu speichern
        //  oder eine Toleranz beim Vergleich zu verwenden.
        if (item.Start == start && item.End == end)
        {
            item.Speaker = newSpeaker;
            hasChanged = true;
            break;
        }
    }

    if (hasChanged)
    {
        if (m_batchUpdateCounter > 0)
        {
            m_changesPending = true;
        }
        else
        {
            emit changed ();
        }
        emit edited ();
    }

    return hasChanged;
}

//--------------------------------------------------------------------------------------------------

QJsonDocument Transcription::toJson () const
{
    //  Serialisiert das gesamte Transcription-Objekt in ein JSON-Format.
    QJsonArray contentArray;
    for (const MetaText &item : m_content)
    {
        QJsonObject entry;
        entry["speaker"] = item.Speaker;
        entry["text"] = item.Text;
        entry["start"] = item.Start;
        entry["end"] = item.End;

        if (!item.Tags.isEmpty ())
        {
            entry["tags"] = QJsonArray::fromStringList (item.Tags);
        }

        contentArray.append (entry);
    }

    //  Das Root-Objekt enthält die Metadaten und das Array mit den Segmenten.
    QJsonObject root;
    root["meeting_name"] = m_meetingName;
    root["start_time"] = m_startTime.toString (
        Qt::ISODate); //  ISO 8601 Format für gute Kompatibilität.
    root["transcription"] = contentArray;
    if (!m_tags.isEmpty ())
    {
        root["tags"] = QJsonArray::fromStringList (m_tags);
    }

    return QJsonDocument (root);
}

//--------------------------------------------------------------------------------------------------

bool Transcription::fromJson (
    const QByteArray &data)
{
    //  Deserialisiert ein JSON-Dokument und befüllt das Transcription-Objekt mit den Daten.
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson (data, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning () << "JSON Parse Error:" << parseError.errorString ();
        return false;
    }

    if (!doc.isObject ())
    {
        qWarning () << "Ungültiges JSON: Root ist kein Objekt.";
        return false;
    }

    QJsonObject rootObj = doc.object ();

    //  Laden der Meeting-Metadaten aus dem Root-Objekt.
    m_meetingName = rootObj.value ("meeting_name").toString ();
    m_startTime = QDateTime::fromString (rootObj.value ("start_time").toString (), Qt::ISODate);

    //  Laden der globalen Tags.
    m_tags.clear ();
    if (rootObj.contains ("tags") && rootObj["tags"].isArray ())
    {
        QJsonArray tagArray = rootObj["tags"].toArray ();
        for (const QJsonValue &v : tagArray)
        {
            if (v.isString ())
            {
                m_tags << v.toString ();
            }
        }
    }

    if (!rootObj.contains ("transcription") || !rootObj["transcription"].isArray ())
    {
        qWarning () << "'transcription'-Array fehlt oder ist ungültig.";
        return false;
    }

    QJsonArray array = rootObj["transcription"].toArray ();

    //  Startet den Batch-Modus, um zu verhindern, dass für jedes 'add' ein Signal gesendet wird.
    beginBatchUpdate ();
    clear (); //  Löscht alle alten Daten, bevor die neuen geladen werden.

    for (const QJsonValue &val : array)
    {
        if (!val.isObject ())
        {
            continue;
        }

        QJsonObject obj = val.toObject ();
        QString speaker = obj.value ("speaker").toString ();
        QString text = obj.value ("text").toString ();
        QString start = obj.value ("start").toString ();
        QString end = obj.value ("end").toString ();

        //  Validierung der gelesenen Daten, um ungültige Einträge zu überspringen.
        bool ok1 = false, ok2 = false;
        double startSec = start.toDouble (&ok1);
        double endSec = end.toDouble (&ok2);

        if (speaker.trimmed ().isEmpty ())
        {
            qWarning () << "Eintrag übersprungen: Sprecher ist leer.";
            continue;
        }

        if (!ok1 || !ok2 || startSec >= endSec)
        {
            qWarning () << "Eintrag übersprungen: ungültiger Zeitbereich [" << start << " – " << end
                        << "]";
            continue;
        }

        MetaText mt (start, end, speaker, text);

        //  Laden der optionalen, segment-spezifischen Tags.
        if (obj.contains ("tags") && obj["tags"].isArray ())
        {
            QJsonArray tagArray = obj["tags"].toArray ();
            for (const QJsonValue &v : tagArray)
            {
                if (v.isString ())
                {
                    mt.Tags << v.toString ();
                }
            }
        }
        add (mt); //  Fügt das Segment hinzu, ohne ein Signal auszulösen (wegen Batch-Update).
    }

    //  Beendet den Batch-Modus und sendet ein einziges 'changed'-Signal, um die UI zu aktualisieren.
    endBatchUpdate ();
    return true;
}

//--------------------------------------------------------------------------------------------------

void Transcription::setTags (
    const QStringList &tags)
{
    //  Setzt die globalen Tags für das Meeting.
    m_tags = tags;
}

//--------------------------------------------------------------------------------------------------

void Transcription::addTag (
    const QString &tag)
{
    //  Fügt einen einzelnen globalen Tag hinzu, falls er noch nicht existiert.
    if (!m_tags.contains (tag))
    {
        m_tags.append (tag);
    }
}

//--------------------------------------------------------------------------------------------------

void Transcription::removeTag (
    const QString &tag)
{
    //  Entfernt alle Vorkommen eines globalen Tags.
    m_tags.removeAll (tag);
}

//--------------------------------------------------------------------------------------------------

bool Transcription::hasTag (
    const QString &tag) const
{
    //  Prüft, ob das Meeting einen bestimmten globalen Tag hat.
    return m_tags.contains (tag);
}

//--------------------------------------------------------------------------------------------------

QList<MetaText> Transcription::segmentsWithTag (
    const QString &tag) const
{
    //  Gibt eine Liste aller Segmente zurück, die den angegebenen lokalen Tag enthalten.
    QList<MetaText> result;
    for (const MetaText &m : m_content)
    {
        if (m.hasTag (tag))
        {
            result.append (m);
        }
    }
    return result;
}

//--------------------------------------------------------------------------------------------------

QString Transcription::getDurationAsString () const
{
    //  Berechnet die Gesamtdauer des Transkripts basierend auf dem Endzeitstempel des letzten Segments.
    if (m_content.isEmpty ())
    {
        return "00:00:00";
    }

    double endTimeSec = m_content.last ().End.toDouble ();
    int durationTotalSeconds = static_cast<int> (endTimeSec);

    int hours = durationTotalSeconds / 3600;
    int minutes = (durationTotalSeconds % 3600) / 60;
    int seconds = durationTotalSeconds % 60;

    return QString ("%1:%2:%3")
        .arg (hours, 2, 10, QLatin1Char ('0'))
        .arg (minutes, 2, 10, QLatin1Char ('0'))
        .arg (seconds, 2, 10, QLatin1Char ('0'));
}

//--------------------------------------------------------------------------------------------------

void Transcription::add (
    const MetaText &part)
{
    //  Fügt ein neues Segment hinzu und behandelt die Signal-Emission im Batch-Modus.
    m_content.append (part);
    if (m_batchUpdateCounter > 0)
    {
        m_changesPending = true;
    }
    else
    {
        emit changed ();
    }
}

//--------------------------------------------------------------------------------------------------

void Transcription::clear ()
{
    //  Setzt den Inhalt des Datenmodells zurück.
    m_content.clear ();
    m_unknownCounter = 0;
    if (m_batchUpdateCounter > 0)
    {
        m_changesPending = true;
    }
    else
    {
        emit changed ();
    }
}

//--------------------------------------------------------------------------------------------------

void Transcription::beginBatchUpdate ()
{
    //  Erhöht den Zähler. Solange dieser > 0 ist, werden `changed`-Signale unterdrückt.
    m_batchUpdateCounter++;
}

//--------------------------------------------------------------------------------------------------

void Transcription::endBatchUpdate ()
{
    m_batchUpdateCounter--;
    //  Prüft, ob dies der äußerste (letzte) Aufruf von endBatchUpdate() ist.
    if (m_batchUpdateCounter == 0)
    {
        //  Wenn während des Batch-Updates Änderungen aufgetreten sind...
        if (m_changesPending)
        {
            m_changesPending = false; //  ...wird das Flag zurückgesetzt...
            emit changed (); //  ...und ein einziges `changed`-Signal für alle Änderungen gesendet.
        }
    }
}

//--------------------------------------------------------------------------------------------------

void Transcription::setName (
    const QString &name)
{
    m_meetingName = name;
}

//--------------------------------------------------------------------------------------------------

void Transcription::setDateTime (
    QDateTime dateTime)
{
    m_startTime = dateTime;
}

//--------------------------------------------------------------------------------------------------

QColor Transcription::speakerColor (
    const QString &speaker) const
{
    //  Verwendet den Hash-Wert des Sprecher-Namens, um eine deterministische,
    //  also für denselben Namen immer gleiche, aber einzigartig erscheinende Farbe zu erzeugen.
    uint h = qHash (speaker);
    int r = (h & 0xFF0000) >> 16;
    int g = (h & 0x00FF00) >> 8;
    int b = h & 0x0000FF;
    return QColor (r, g, b);
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
