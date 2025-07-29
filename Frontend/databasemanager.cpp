#include "databasemanager.h"
#include "transcription.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QSettings>
#include <QMessageBox>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{}

//--------------------------------------------------------------------------------------------------

bool DatabaseManager::connectToSupabase() {
    m_connected = false;

    // Zugriff auf gespeicherte Einstellungen (z.B. in der Registry oder INI-Datei)
    QSettings settings("SS2025FP_T2", "AudioTranskriptor");

    // Überprüfen, ob alle benötigten Datenbank-Konfigurationswerte gesetzt sind
    QStringList requiredKeys = {"db/host", "db/port", "db/name", "db/user", "db/password"};
    bool settingsValid = true;

    // Prüfe jede benötigte Einstellung, logge Warnung bei fehlenden Einträgen
    for (const QString &key : requiredKeys) {
        if (!settings.contains(key)) {
            qWarning() << QString("Fehlende Einstellung: \"%1\". Bitte korrigieren Sie die Datenbankeinstellungen in den Einstellungen.").arg(key);
            settingsValid = false;
        }
    }
    if (!settingsValid) {
        // Wenn eine Einstellung fehlt, Verbindung nicht versuchen
        return false;
    }
    QSqlDatabase db;
    // Prüfen, ob bereits eine offene Verbindung zur Supabase existiert
    if (QSqlDatabase::contains("supabase") && QSqlDatabase::database("supabase").isOpen()) {
        db = QSqlDatabase::database("supabase");
    } else {
        // Neue Verbindung zur PostgreSQL-Datenbank aufbauen
        db = QSqlDatabase::addDatabase("QPSQL", "supabase");
        db.setHostName(settings.value("db/host").toString());
        db.setPort(settings.value("db/port").toInt());
        db.setDatabaseName(settings.value("db/name").toString());
        db.setUserName(settings.value("db/user").toString());
        db.setPassword(settings.value("db/password").toString());
    }

    // Öffne Verbindung zur Datenbank
    if (!db.open()) {
        qWarning() << "Verbindung zu Supabase fehlgeschlagen: " << db.lastError().text();
        qWarning() << "Bitte überprüfen Sie Ihre Datenbankeinstellungen in den Einstellungen.";
        return false;
    }

    qDebug() << "Verbindung zu Supabase PostgreSQL hergestellt.";
    m_connected = true;
    return true;
}

//--------------------------------------------------------------------------------------------------

QSqlDatabase DatabaseManager::getDatabase() {
    return QSqlDatabase::database("supabase");
}

//--------------------------------------------------------------------------------------------------

QStringList DatabaseManager::loadAllTranscriptionsName() {
    QStringList titles;
    QSqlQuery query(getDatabase());

    // SQL-Abfrage: Nur die Titel aller Besprechungen holen
    if (!query.exec("SELECT titel FROM besprechungen")) {
        qWarning() << "Fehler beim Laden der Titel:" << query.lastError().text();
        return titles;
    }

    // Alle Ergebniszeilen durchlaufen und Titel speichern
    while (query.next()) {
        titles << query.value(0).toString();
    }

    return titles;
}

//--------------------------------------------------------------------------------------------------

QMap<QString, Transcription*> DatabaseManager::loadAllTranscriptions() {
    QMap<QString, Transcription*> map;
    QSqlQuery query(getDatabase());

    // Komplexe SQL-Abfrage mit JOINs auf Besprechungen, Aussagen und Sprecher
    const QString sql = R"(
        SELECT b.id AS besprechung_id, b.titel AS title, b.created_at AS start_time,
               a.zeit_start AS start, a.zeit_ende AS end,
               s.name AS speaker_name,
               a.verarbeiteter_text, a.roher_text, a.tags
        FROM besprechungen b
        JOIN aussagen a ON b.id = a.besprechungen_id
        JOIN sprecher s ON a.sprecher_id = s.id
        ORDER BY b.id, a.zeit_start
    )";

    // SQL ausführen und bei Fehler abbrechen
    if (!query.exec(sql)) {
        qWarning() << "Fehler beim Laden der Transkriptionen:" << query.lastError().text();
        return map;
    }
    // Alle Zeilen durchgehen und Transcriptions map befüllen
    while (query.next()) {
        QString title = query.value("title").toString();

        // Wenn Transcription noch nicht existiert, neues Objekt erstellen
        if (!map.contains(title)) {
            auto *t = new Transcription(nullptr);
            t->setName(title);
            t->setDateTime(query.value("start_time").toDateTime());
            map.insert(title, t);
        }
        Transcription *t = map[title];
        // Zeitstempel der einzelnen Segment starten und enden
        QString start = query.value("start").toDateTime().toString(Qt::ISODate);
        QString end = query.value("end").toDateTime().toString(Qt::ISODate);
        // Falls verarbeiteter Text leer, rohen Text nehmen
        QString speaker = query.value("speaker_name").toString();
        QString processed = query.value("verarbeiteter_text").toString();
        QString raw = query.value("roher_text").toString();
        QString finalText = processed.isEmpty() ? raw : processed;
        // Tags aus PostgreSQL Array-Notation parsen
        QString tagString = query.value("tags").toString();
        QStringList tags = parsePgTextArray(tagString);

        // MetaText Objekt mit allen Infos erzeugen
        MetaText mt(start, end, speaker, finalText);
        mt.Tags = tags;
        // Segment zur Transcription hinzufügen
        t->add(mt);

    }

    return map;
}
//--------------------------------------------------------------------------------------------------

QStringList DatabaseManager::parsePgTextArray(const QString &pgArrayString) {
    QString trimmed = pgArrayString;
    // Entfernt geschweifte Klammern am Anfang und Ende
    if (trimmed.startsWith("{") && trimmed.endsWith("}")) {
        trimmed = trimmed.mid(1, trimmed.length() - 2);
    } else {
        // Falls nicht korrekt formatiert, leere Liste zurückgeben
        return QStringList();
    }
    // Komma-getrennte Elemente trennen
    QStringList elements = trimmed.split(',', Qt::SkipEmptyParts);

    // Anführungszeichen entfernen und einfache Anführungszeichen escapen
    for (int i = 0; i < elements.size(); ++i) {
        QString elem = elements[i].trimmed();

        // Einfache Anführungszeichen entfernen, falls vorhanden
        if (elem.startsWith("'") && elem.endsWith("'") && elem.length() >= 2) {
            elem = elem.mid(1, elem.length() - 2);
            elem.replace("''", "'");  // single quotes entscapen
        }
        elements[i] = elem;
    }
    return elements;
}


//--------------------------------------------------------------------------------------------------

void DatabaseManager::loadMeetingTranscriptions(const QString &meetingTitle,
                                                const QString &textColumn,
                                                Transcription *m_script)
{
    QSqlDatabase db = getDatabase();
    // Meeting ID und Erstellungsdatum anhand des Titels abfragen
    QSqlQuery query(db);
    query.prepare("SELECT id, created_at FROM besprechungen WHERE titel = :titel");
    query.bindValue(":titel", meetingTitle);

    if (!query.exec() || !query.next()) {
        qWarning() << "Meeting nicht gefunden:" << meetingTitle;
        return;
    }

    int meetingId = query.value("id").toInt();
    m_script->clear();
    m_script->setName(meetingTitle);
    m_script->setDateTime(query.value("created_at").toDateTime());

    // Aussagen zu dem Meeting laden, Spalte für Text wird dynamisch übergeben
    QSqlQuery stmtQuery(db);
    stmtQuery.prepare(QString(R"(
        SELECT id, zeit_start, zeit_ende, %1, sprecher_id
        FROM aussagen
        WHERE besprechungen_id = :id
        ORDER BY zeit_start
    )").arg(textColumn));
    stmtQuery.bindValue(":id", meetingId);

    if (!stmtQuery.exec()) {
        qWarning() << "Aussagen konnten nicht geladen werden:" << stmtQuery.lastError().text();
        return;
    }

    bool hasText = false;
    // Alle Segmente durchgehen und zur Transcription hinzufügen
    while (stmtQuery.next()) {
        QString text = stmtQuery.value(textColumn).toString().trimmed();
        if (!text.isEmpty()) hasText = true;

        QDateTime start = stmtQuery.value("zeit_start").toDateTime();
        QDateTime end = stmtQuery.value("zeit_ende").toDateTime();
        int speakerId = stmtQuery.value("sprecher_id").toInt();

        QString speakerName = getSpeakerName(speakerId, meetingId, db);
        MetaText segment(QString::number(start.toSecsSinceEpoch()),
                         QString::number(end.toSecsSinceEpoch()),
                         speakerName, text);
        m_script->add(segment);
    }
    // Falls kein bearbeiteter Text vorhanden und der textColumn "verarbeiteter_text" war,
    // wird nochmal mit "roher_text" nachgeladen und Hinweis angezeigt
    if (!hasText && textColumn == "verarbeiteter_text") {
        QMessageBox::warning(nullptr, "Hinweis", "Kein bearbeiteter Text gefunden.");
        m_script->setViewMode(TranscriptionViewMode::Original);
        loadMeetingTranscriptions(meetingTitle, "roher_text", m_script);
    }
}

//--------------------------------------------------------------------------------------------------

QString DatabaseManager::getSpeakerName(int speakerId, int meetingId, const QSqlDatabase &db) {
    QSqlQuery speakerQuery(db);
    speakerQuery.prepare("SELECT name FROM sprecher WHERE id = :id AND besprechungen_id = :besprechungen_id");
    speakerQuery.bindValue(":id", speakerId);
    speakerQuery.bindValue(":besprechungen_id", meetingId);

    // Falls vorhanden, Namen zurückgeben, sonst "Unbekannt"
    return (speakerQuery.exec() && speakerQuery.next())
               ? speakerQuery.value("name").toString()
               : "Unbekannt";
}

//--------------------------------------------------------------------------------------------------

int DatabaseManager::getMeetingIdByTitle(const QString &title)
{
    QSqlQuery query(getDatabase());
    query.prepare("SELECT id FROM besprechungen WHERE titel = :titel");
    query.bindValue(":titel", title);

    if (query.exec() && query.next())
        return query.value(0).toInt();

    qWarning() << "Meeting-ID nicht gefunden für Titel:" << title;
    return -1;
}
//--------------------------------------------------------------------------------------------------
bool DatabaseManager::saveNewTranscription(Transcription *script, const QString &newTitle)
{
    QSqlDatabase db = getDatabase();

    // Prüfen ob der Titel bereits existiert, um Duplikate zu vermeiden
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT id FROM besprechungen WHERE titel = :titel");
    checkQuery.bindValue(":titel", newTitle);
    checkQuery.exec();
    if (checkQuery.next()) return false;

    // Neue Besprechung mit Titel und Erstellungsdatum anlegen
    QSqlQuery insertMeeting(db);
    insertMeeting.prepare("INSERT INTO besprechungen (titel, created_at) VALUES (:titel, :created_at)");
    insertMeeting.bindValue(":titel", newTitle);
    insertMeeting.bindValue(":created_at", script->dateTime());

    if (!insertMeeting.exec()) return false;

    int newMeetingId = insertMeeting.lastInsertId().toInt();
    if (newMeetingId == 0) return false;

    // Alle Segmente der Transkription in die DB einfügen
    for (const MetaText &segment : script->getMetaTexts()) {
        int speakerId = getSpeakerId(segment.Speaker, newMeetingId, db);
        QVariant speakerIdValue;
        if (speakerId > 0) {
            speakerIdValue = QVariant(speakerId);
        } else {
            speakerIdValue = QVariant(); // NULL
        }

        // Tags in PostgreSQL text[] Format konvertieren
        QStringList escapedTags;
        for (const QString &tag : segment.Tags) {
            QString escaped = tag;
            escaped.replace("'", "''");
            escapedTags << ("'" + escaped + "'");
        }
        QString pgArray = "{" + escapedTags.join(",") + "}";

        // Aussage (Segment) einfügen
        QSqlQuery insertStmt(db);
        insertStmt.prepare(R"(
            INSERT INTO aussagen (besprechungen_id, zeit_start, zeit_ende, roher_text, sprecher_id, tags)
            VALUES (:mid, TO_TIMESTAMP(:start), TO_TIMESTAMP(:end), :text, :sid, :tags)
        )");

        qint64 startSeconds = segment.Start.toLongLong();
        qint64 endSeconds = segment.End.toLongLong();

        insertStmt.bindValue(":mid", newMeetingId);
        insertStmt.bindValue(":start", startSeconds);
        insertStmt.bindValue(":end", endSeconds);
        insertStmt.bindValue(":text", segment.Text);
        insertStmt.bindValue(":sid", speakerId);
        insertStmt.bindValue(":tags", pgArray);

        if (!insertStmt.exec()) {
            qWarning() << "Fehler beim Einfügen der Aussage:" << insertStmt.lastError().text();
            return false;
        }
    }
    // Titel der Transkription setzen
    script->setName(newTitle);
    return true;
}

//--------------------------------------------------------------------------------------------------
bool DatabaseManager::updateTranscription(Transcription *m_script)
{
    QSqlDatabase db = getDatabase();

    QString name = m_script->name();
    QDateTime startTime = m_script->dateTime();

    // Schritt 1: Meeting-ID anhand des Transkriptionsnamens abfragen
    QSqlQuery lookupQuery(db);
    lookupQuery.prepare("SELECT id FROM besprechungen WHERE titel = :titel");
    lookupQuery.bindValue(":titel", name);
    if (!lookupQuery.exec() || !lookupQuery.next()) {
        // Wenn kein Eintrag gefunden oder Fehler beim Ausführen der Abfrage -> Abbruch
        return false;
    }
    int meetingId = lookupQuery.value(0).toInt();

    // Schritt 2: Das Erstellungsdatum des Meetings in der Datenbank aktualisieren
    QSqlQuery updateMeeting(db);
    updateMeeting.prepare("UPDATE besprechungen SET created_at = :created_at WHERE id = :id");
    updateMeeting.bindValue(":created_at", startTime);
    updateMeeting.bindValue(":id", meetingId);
    if (!updateMeeting.exec()) {
        qWarning() << "Fehler beim Aktualisieren des Meetings:" << updateMeeting.lastError().text();
        return false;
    }

    // Schritt 3: Wenn die Transkription bearbeitet wurde, lösche den verarbeiteten Text (zurücksetzen)
    if (m_script->isEdited()) {
        QSqlQuery clearProcessedTextQuery(db);
        clearProcessedTextQuery.prepare(R"(
            UPDATE aussagen
            SET verarbeiteter_text = NULL
            WHERE besprechungen_id = :id
        )");
        clearProcessedTextQuery.bindValue(":id", meetingId);
        if (!clearProcessedTextQuery.exec()) {
            qWarning() << "Fehler beim Zurücksetzen von verarbeiteter_text:" << clearProcessedTextQuery.lastError().text();
            return false;
        }
    }

    // Schritt 4: Vorhandene Sprecher aus der Datenbank laden (Cache für spätere Zuordnung)
    QMap<QString, int> speakerCache;
    QSqlQuery speakerLoad(db);
    if (speakerLoad.exec("SELECT id, name FROM sprecher")) {
        while (speakerLoad.next()) {
            int id = speakerLoad.value("id").toInt();
            QString name = speakerLoad.value("name").toString();
            speakerCache[name] = id;
        }
    }
    // Schritt 5: UPSERT-Abfrage vorbereiten für Einfügen/Aktualisieren von Aussagen
    QSqlQuery upsertQuery(db);
    upsertQuery.prepare(R"(
    INSERT INTO aussagen (
        besprechungen_id,
        zeit_start,
        zeit_ende,
        verarbeiteter_text,
        sprecher_id,
        tags
    )
    VALUES (
        :besprechungen_id,
        :zeit_start,
        :zeit_ende,
        :text,
        :sprecher_id,
        :tags
    )
    ON CONFLICT (besprechungen_id, zeit_start, zeit_ende)
    DO UPDATE SET
        verarbeiteter_text = EXCLUDED.verarbeiteter_text,
        sprecher_id = EXCLUDED.sprecher_id,
        tags = EXCLUDED.tags
        )");

    // Alle Segmente der Transkription durchgehen und einzeln in die Datenbank schreiben
    for (const MetaText &segment : m_script->getMetaTexts()) {
        QString speakerName = segment.Speaker.trimmed();
        int speakerId = -1;

        // Schritt 6: Sprecher-ID bestimmen
        if (speakerCache.contains(speakerName)) {
            // Sprecher existiert bereits im Cache
            speakerId = speakerCache[speakerName];

            // Optional: Sprechername aktualisieren, falls sich dieser geändert hat
            QSqlQuery updateSpeaker(db);
            updateSpeaker.prepare(R"(
            UPDATE sprecher
            SET name = :name
            WHERE id = :id
            )");
            updateSpeaker.bindValue(":name", speakerName);
            updateSpeaker.bindValue(":id", speakerId);
            if (!updateSpeaker.exec()) {
                qWarning() << "Fehler beim Aktualisieren des Sprechers:" << updateSpeaker.lastError().text();
            }
        } else if (!speakerName.isEmpty()) {
            // Sprecher existiert noch nicht → neuen Eintrag erstellen
            QSqlQuery insertSpeaker(db);
            insertSpeaker.prepare(R"(
                INSERT INTO sprecher (name, besprechungen_id)
                VALUES (:name, :besprechungen_id)
                RETURNING id
            )");
            insertSpeaker.bindValue(":name", speakerName);
            insertSpeaker.bindValue(":besprechungen_id", meetingId);
            if (insertSpeaker.exec() && insertSpeaker.next()) {
                speakerId = insertSpeaker.value(0).toInt();
                speakerCache[speakerName] = speakerId;
            } else {
                qWarning() << "Sprecher konnte nicht hinzugefügt werden:" << insertSpeaker.lastError().text();
            }
        }

        // Schritt 7: Zeitinformationen des Segments in QDateTime umwandeln
        QDateTime start = QDateTime::fromSecsSinceEpoch(segment.Start.toLongLong());
        QDateTime end = QDateTime::fromSecsSinceEpoch(segment.End.toLongLong());

        // Schritt 8: Tags (QStringList) in PostgreSQL-kompatibles text[] Literal konvertieren
        QStringList escapedTags;
        for (const QString &tag : segment.Tags) {
            QString escaped = tag;
            escaped.replace("'", "''");
            escapedTags << ("'" + escaped + "'");
        }
        QString pgArray = "{" + escapedTags.join(",") + "}";

        // Schritt 9: Werte in die UPSERT-Abfrage binden
        upsertQuery.bindValue(":besprechungen_id", meetingId);
        upsertQuery.bindValue(":zeit_start", start);
        upsertQuery.bindValue(":zeit_ende", end);
        upsertQuery.bindValue(":text", segment.Text);
        upsertQuery.bindValue(":sprecher_id", speakerId != -1 ? speakerId : QVariant());
        upsertQuery.bindValue(":tags", pgArray);

        // Schritt 10: Abfrage ausführen
        if (!upsertQuery.exec()) {
            qWarning() << "UPSERT fehlgeschlagen:" << upsertQuery.lastError().text();
            return false;
        }
    }
    // Wenn alle Aussagen erfolgreich aktualisiert/eingefügt wurden → Erfolg zurückgeben
    return true;
}

//--------------------------------------------------------------------------------------------------
int DatabaseManager::getSpeakerId(const QString &speakerName, int meetingId, QSqlDatabase &db) {
    // Prüfen ob Sprecher existiert
    QSqlQuery speakerQuery(db);
    speakerQuery.prepare("SELECT id FROM sprecher WHERE name = :name");
    speakerQuery.bindValue(":name", speakerName);
    if (!speakerQuery.exec()) {
        qWarning() << "Speaker lookup failed:" << speakerQuery.lastError();
        return 0;
    }

    if (speakerQuery.next()) {
        return speakerQuery.value(0).toInt();
    }

    // Sprecher nicht gefunden → anlegen
    QSqlQuery speakerInsertQuery(db);
    speakerInsertQuery.prepare("INSERT INTO sprecher (name, besprechungen_id) VALUES (:name, :bid) RETURNING id");
    speakerInsertQuery.bindValue(":name", speakerName);
    speakerInsertQuery.bindValue(":bid", meetingId);

    if (!speakerInsertQuery.exec()) {
        qWarning() << "Speaker insert failed:" << speakerInsertQuery.lastError();
        return 0;
    }

    if (speakerInsertQuery.next()) {
        return speakerInsertQuery.value(0).toInt();
    }

    return 0; // Fallback
}
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
