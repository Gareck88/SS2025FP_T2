/**
 * @file taggeneratormanager.h
 * @brief Enthält die Deklaration der TagGeneratorManager-Klasse.
 * @author Mike Wild
 */
#ifndef TAGGENERATORMANAGER_H
#define TAGGENERATORMANAGER_H

#include <QObject>
#include <QProcess>
#include <QStringList>

/**
 * @brief Steuert den externen Python-Prozess zur automatischen Tag-Erstellung.
 *
 * Diese Klasse nutzt spaCy über ein Python-Skript, um den Inhalt eines
 * Transkripts zu analysieren und relevante Schlüsselwörter sowie Eigennamen
 * (Personen, Orte, etc.) als Tags zu extrahieren.
 * Die Ausführung geschieht asynchron in einem eigenen Prozess.
 */
class TagGeneratorManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Standard-Konstruktor.
     * @param parent Das QObject-Elternteil für die Speicherverwaltung.
     */
    explicit TagGeneratorManager (QObject *parent = nullptr);

public slots:
    /**
     * @brief Startet die Tag-Analyse für den übergebenen Text.
     *
     * Der Text wird an die Standardeingabe des Python-Prozesses übergeben.
     * Das Ergebnis wird asynchron über das `tagsReady`-Signal zurückgemeldet.
     * @param fullText Der gesamte Transkript-Text, der analysiert werden soll.
     */
    void generateTagsFor (const QString &fullText);

signals:
    /**
     * @brief Wird gesendet, wenn der Tag-Generierungs-Prozess abgeschlossen ist.
     * @param tags Eine Liste der gefundenen Tags. Ist im Fehlerfall leer.
     * @param success true, wenn der Prozess erfolgreich war, andernfalls false.
     * @param errorMsg Enthält eine Fehlermeldung, falls `success` false ist.
     */
    void tagsReady (const QStringList &tags, bool success, const QString &errorMsg = "");

private slots:
    /**
     * @brief Interner Slot, der aufgerufen wird, wenn der Python-Prozess beendet ist.
     *
     * Liest die Ausgabe des Skripts, wandelt sie in eine QStringList um und
     * emittiert das `tagsReady`-Signal.
     */
    void onProcessFinished (int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess *m_process;  ///< Zeiger auf das QProcess-Objekt.
    QString m_pythonPath; ///< Pfad zum Python-Interpreter.
    QString m_scriptPath; ///< Pfad zum 'generate_tags.py'-Skript.
};

#endif // TAGGENERATORMANAGER_H
