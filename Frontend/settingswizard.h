/**
 * @file settingswizard.h
 * @brief Enthält die Deklaration des SettingsWizard-Dialogs zur Konfiguration der Anwendung.
 * @author Mike Wild
 */
#ifndef SETTINGSWIZARD_H
#define SETTINGSWIZARD_H

#include <QDialog>

// Forward-Deklarationen für alle verwendeten Widget-Typen
class QLineEdit;
class QSlider;
class QLabel;
class QDoubleSpinBox;
class QSpinBox;
class QFontComboBox;

/**
 * @brief Ein Dialogfenster zur Bearbeitung der Anwendungseinstellungen.
 *
 * Dieser Dialog bietet eine grafische Oberfläche, um diverse Parameter der Anwendung
 * zu konfigurieren. Die Werte werden aus einem QSettings-Objekt geladen und beim
 * Speichern wieder dorthin zurückgeschrieben, um sie persistent zu machen.
 */
class SettingsWizard : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief Konstruktor, der die UI des Dialogs aufbaut und mit Werten füllt.
     * @param parent Das QObject-Elternteil für die Speicherverwaltung.
     */
    explicit SettingsWizard (QWidget *parent = nullptr);

private slots:
    /** @brief Speichert die aktuellen Werte aller UI-Elemente in die QSettings. */
    void saveSettings ();

    /** @brief Aktualisiert ein Label, das die Puffergröße in Sekunden anzeigt. */
    void updateDurationLabel (int value);

    /** @brief Synchronisiert den System-Gain-Slider mit der SpinBox (logarithmische Skala). */
    void syncSysGainSlider (double value);

    /** @brief Synchronisiert den Mikrofon-Gain-Slider mit der SpinBox (logarithmische Skala). */
    void syncMicGainSlider (double value);

    /** @brief Synchronisiert die System-Gain-SpinBox mit dem Slider. */
    void syncSysGainSpin (int sliderValue);

    /** @brief Synchronisiert die Mikrofon-Gain-SpinBox mit dem Slider. */
    void syncMicGainSpin (int sliderValue);

private:
    /**
     * @brief Stellt sicher, dass die Puffergröße innerhalb eines gültigen Bereichs liegt.
     * @param kb Die zu validierende Größe in Kilobyte.
     * @return Die validierte und ggf. korrigierte Größe.
     */
    int validateBufferSize (int kb);

    // --- UI-Elemente ---
    // Pfad- und Buffer-Einstellungen
    QLineEdit *pythonEdit; ///< Eingabefeld für den Python-Pfad.
    QLineEdit *scriptEdit; ///< Eingabefeld für den ASR-Skript-Pfad.
    QLineEdit *wavEdit;    ///< Eingabefeld für den Wav-Datei-Pfad.
    QLineEdit *asrWavEdit; ///< Eingabefeld für den ASR-Wav-Datei-Pfad.
    QSlider *bufferSlider; ///< Slider zur Einstellung der Audio-Puffergröße.
    QLabel *durationLabel; ///< Label zur Anzeige der Pufferdauer in Sekunden.

    // Audio-Verstärkung
    QDoubleSpinBox *sysGainSpin; ///< SpinBox für den System-Audio-Verstärkungsfaktor.
    QSlider *sysGainSlider;      ///< Slider für den System-Audio-Verstärkungsfaktor.
    QDoubleSpinBox *micGainSpin; ///< SpinBox für den Mikrofon-Verstärkungsfaktor.
    QSlider *micGainSlider;      ///< Slider für den Mikrofon-Verstärkungsfaktor.

    // PDF-Exporteinstellungen
    QSpinBox *pdfHeadlineSpin;      ///< SpinBox für die Schriftgröße der PDF-Überschrift.
    QSpinBox *pdfBodySpin;          ///< SpinBox für die Schriftgröße des PDF-Haupttextes.
    QSpinBox *pdfMetaSpin;          ///< SpinBox für die Schriftgröße der PDF-Metadaten.
    QSpinBox *marginTopSpin;        ///< SpinBox für den oberen Seitenrand des PDFs.
    QSpinBox *marginRightSpin;      ///< SpinBox für den rechten Seitenrand des PDFs.
    QSpinBox *marginBottomSpin;     ///< SpinBox für den unteren Seitenrand des PDFs.
    QSpinBox *marginLeftSpin;       ///< SpinBox für den linken Seitenrand des PDFs.
    QFontComboBox *fontFamilyCombo; ///< Auswahlbox für die PDF-Schriftfamilie.

    // Datenbank-Einstellungen
    QLineEdit *dbHostEdit; ///< Eingabefeld für den Hostname.
    QSpinBox *dbPortSpin;  ///< Eingabefeld für den Port.
    QLineEdit *dbNameEdit; ///< Eingabefeld für den Datenbankname.
    QLineEdit *dbUserEdit; ///< Eingabefeld für den Benutzername.
    QLineEdit *dbPassEdit; ///< Eingabefeld für den Password.
};

#endif // SETTINGSWIZARD_H
