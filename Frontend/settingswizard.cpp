#include "settingswizard.h"

#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QVBoxLayout>
#include "filemanager.h" // Nötig, um Standard-Pfade abzufragen
#include <cmath> //  Für std::log10 und std::pow

//--------------------------------------------------------------------------------------------------

SettingsWizard::SettingsWizard (
    QWidget *parent)
    : QDialog (parent)
    , pythonEdit (new QLineEdit (this))
    , scriptEdit (new QLineEdit (this))
    , wavEdit (new QLineEdit (this))
    , asrWavEdit (new QLineEdit (this))
    , meetingEdit (new QLineEdit (this))
    , bufferSlider (new QSlider (Qt::Horizontal, this))
    , durationLabel (new QLabel (this))
    , sysGainSpin (new QDoubleSpinBox (this))
    , sysGainSlider (new QSlider (Qt::Horizontal, this))
    , micGainSpin (new QDoubleSpinBox (this))
    , micGainSlider (new QSlider (Qt::Horizontal, this))
    , pdfHeadlineSpin (new QSpinBox (this))
    , pdfBodySpin (new QSpinBox (this))
    , pdfMetaSpin (new QSpinBox (this))
    , marginTopSpin (new QSpinBox (this))
    , marginRightSpin (new QSpinBox (this))
    , marginBottomSpin (new QSpinBox (this))
    , marginLeftSpin (new QSpinBox (this))
    , fontFamilyCombo (new QFontComboBox (this))
{
    QPushButton *browsePython = new QPushButton (tr ("..."));
    QPushButton *browseScript = new QPushButton (tr ("..."));
    QPushButton *browseWav = new QPushButton (tr ("..."));
    QPushButton *browseAsrWav = new QPushButton (tr ("..."));
    QPushButton *browseMeetings = new QPushButton (tr ("..."));
    QPushButton *okButton = new QPushButton (tr ("Speichern"));

    //  Konfiguriert alle Einstellungs-Widgets mit ihren Wertebereichen und Einheiten.
    pdfHeadlineSpin->setRange (20, 80);
    pdfHeadlineSpin->setSuffix (" pt");
    pdfBodySpin->setRange (8, 40);
    pdfBodySpin->setSuffix (" pt");
    pdfMetaSpin->setRange (8, 40);
    pdfMetaSpin->setSuffix (" pt");

    marginTopSpin->setRange (5, 50);
    marginTopSpin->setSuffix (" mm");
    marginRightSpin->setRange (5, 50);
    marginRightSpin->setSuffix (" mm");
    marginBottomSpin->setRange (5, 50);
    marginBottomSpin->setSuffix (" mm");
    marginLeftSpin->setRange (5, 50);
    marginLeftSpin->setSuffix (" mm");

    //  Lade alle Werte aus den QSettings und setze sie als aktuelle Werte der Widgets.
    //  Falls ein Schlüssel nicht existiert, wird der angegebene Standardwert verwendet.
    QSettings settings ("SS2025FP_T2", "AudioTranskriptor");
    pdfHeadlineSpin->setValue (settings.value ("pdf/fontSizeHeadline", 42).toInt ());
    pdfBodySpin->setValue (settings.value ("pdf/fontSizeBody", 12).toInt ());
    pdfMetaSpin->setValue (settings.value ("pdf/fontSizeMeta", 10).toInt ());
    fontFamilyCombo->setCurrentFont (
        QFont (settings.value ("pdf/fontFamily", "sans-serif").toString ()));
    marginTopSpin->setValue (settings.value ("pdf/marginTop", 25).toInt ());
    marginRightSpin->setValue (settings.value ("pdf/marginRight", 25).toInt ());
    marginBottomSpin->setValue (settings.value ("pdf/marginBottom", 25).toInt ());
    marginLeftSpin->setValue (settings.value ("pdf/marginLeft", 25).toInt ());
    pythonEdit->setText (settings.value ("pythonPath").toString ());
    scriptEdit->setText (settings.value ("scriptPath").toString ());

    //  Lade Standard-Pfade
    QString initWavPath, initAsrWavPath, initMeetingsPath;
    FileManager fm (this);
    initWavPath = fm.getTempWavPath (false);
    initAsrWavPath = fm.getTempWavPath (true);
    initMeetingsPath = fm.getMeetingsDirectory ();

    wavEdit->setText (initWavPath);
    asrWavEdit->setText (initAsrWavPath);
    meetingEdit->setText (initMeetingsPath);

    // --- Verbinde alle "Browse"-Buttons mit den entsprechenden Datei- oder Ordnerdialogen ---
    connect (browsePython,
             &QPushButton::clicked,
             this,
             [=]
             {
                 QString path = QFileDialog::getOpenFileName (this, tr ("Python auswählen"));
                 if (!path.isEmpty ())
                 {
                     pythonEdit->setText (path);
                 }
             });

    connect (browseScript,
             &QPushButton::clicked,
             this,
             [=]
             {
                 QString path = QFileDialog::getOpenFileName (this, tr ("Skript auswählen"));
                 if (!path.isEmpty ())
                 {
                     scriptEdit->setText (path);
                 }
             });

    connect (browseWav,
             &QPushButton::clicked,
             this,
             [=]
             {
                 QString path = QFileDialog::getSaveFileName (this,
                                                              tr ("HQ-Wav-Datei speichern"),
                                                              initWavPath,
                                                              tr ("Audio (*.wav)"));
                 if (!path.isEmpty ())
                 {
                     wavEdit->setText (path);
                 }
             });

    connect (browseAsrWav,
             &QPushButton::clicked,
             this,
             [=]
             {
                 QString path = QFileDialog::getSaveFileName (this,
                                                              tr ("ASR-Wav-Datei speichern"),
                                                              initAsrWavPath,
                                                              tr ("Audio (*.wav)"));
                 if (!path.isEmpty ())
                 {
                     asrWavEdit->setText (path);
                 }
             });

    connect (browseMeetings,
             &QPushButton::clicked,
             this,
             [=]
             {
                 QString path
                     = QFileDialog::getExistingDirectory (this,
                                                          tr ("Ordner für die Meetings auswählen"),
                                                          initMeetingsPath,
                                                          QFileDialog::ShowDirsOnly
                                                              | QFileDialog::DontResolveSymlinks);
                 if (!path.isEmpty ())
                 {
                     meetingEdit->setText (path);
                 }
             });

    connect (okButton, &QPushButton::clicked, this, &SettingsWizard::saveSettings);
    connect (bufferSlider, &QSlider::valueChanged, this, &SettingsWizard::updateDurationLabel);

    //  Konfiguration der Audio-Einstellungs-Widgets.
    bufferSlider->setMinimum (128);
    bufferSlider->setMaximum (3840);
    bufferSlider->setSingleStep (64);

    sysGainSpin->setRange (0.01, 10.0);
    sysGainSpin->setSingleStep (0.1);
    sysGainSpin->setDecimals (2);
    sysGainSlider->setRange (0, 1000);

    micGainSpin->setRange (0.01, 10.0);
    micGainSpin->setSingleStep (0.1);
    micGainSpin->setDecimals (2);
    micGainSlider->setRange (0, 1000);

    //  Setzen der Gain-Werte. Da die Slider logarithmisch sind, ist eine Umrechnung nötig.
    float sysGain = settings.value ("sysGain", 0.5f).toFloat ();
    float micGain = settings.value ("micGain", 6.0f).toFloat ();
    sysGainSpin->setValue (sysGain);
    syncSysGainSlider (sysGain); //  Initialisiert den Slider-Wert passend zur SpinBox
    micGainSpin->setValue (micGain);
    syncMicGainSlider (micGain); //  Initialisiert den Slider-Wert passend zur SpinBox

    //  Verbindungen zur Synchronisation der Gain-Slider mit den Gain-SpinBoxes.
    connect (sysGainSpin,
             QOverload<double>::of (&QDoubleSpinBox::valueChanged),
             this,
             &SettingsWizard::syncSysGainSlider);
    connect (sysGainSlider, &QSlider::valueChanged, this, &SettingsWizard::syncSysGainSpin);
    connect (micGainSpin,
             QOverload<double>::of (&QDoubleSpinBox::valueChanged),
             this,
             &SettingsWizard::syncMicGainSlider);
    connect (micGainSlider, &QSlider::valueChanged, this, &SettingsWizard::syncMicGainSpin);

    //  Laden und Anzeigen des initialen Buffer-Wertes.
    int defaultKB = settings.value ("audio/bufferThreshold", 384).toInt ();
    bufferSlider->setValue (validateBufferSize (defaultKB));
    updateDurationLabel (bufferSlider->value ());

    //  Aufbau des UI-Layouts.
    QFormLayout *form = new QFormLayout;
    QGroupBox *pathGroup = new QGroupBox (tr ("Pfad-Einstellungen"));
    QFormLayout *pathLayout = new QFormLayout ();
    pathLayout->addRow (tr ("Python-Pfad:"), pythonEdit);
    pathLayout->addRow ("", browsePython);
    pathLayout->addRow (tr ("ASR-Skriptpfad:"), scriptEdit);
    pathLayout->addRow ("", browseScript);
    pathLayout->addRow (tr ("HQ-Wav-Dateipfad:"), wavEdit);
    pathLayout->addRow ("", browseWav);
    pathLayout->addRow (tr ("ASR-Wav-Dateipfad:"), asrWavEdit);
    pathLayout->addRow ("", browseAsrWav);
    pathLayout->addRow (tr ("Meetings-Ordner:"), meetingEdit);
    pathLayout->addRow ("", browseMeetings);
    pathGroup->setLayout (pathLayout);
    form->addRow (pathGroup);

    QGroupBox *audioGroup = new QGroupBox (tr ("Audio-Einstellungen"));
    QFormLayout *audioLayout = new QFormLayout ();
    audioLayout->addRow (tr ("Puffergröße (KB):"), bufferSlider);
    audioLayout->addRow ("", durationLabel);
    audioLayout->addRow (tr ("System-Audio-Gain:"), sysGainSpin);
    audioLayout->addRow ("", sysGainSlider);
    audioLayout->addRow (tr ("Mikrofon-Gain:"), micGainSpin);
    audioLayout->addRow ("", micGainSlider);
    audioGroup->setLayout (audioLayout);
    form->addRow (audioGroup);

    QGroupBox *pdfGroup = new QGroupBox (tr ("PDF-Export Einstellungen"));
    QFormLayout *pdfLayout = new QFormLayout ();
    pdfLayout->addRow (tr ("Schriftart:"), fontFamilyCombo);
    pdfLayout->addRow (tr ("Schriftgröße Überschrift:"), pdfHeadlineSpin);
    pdfLayout->addRow (tr ("Schriftgröße Haupttext:"), pdfBodySpin);
    pdfLayout->addRow (tr ("Schriftgröße Metadaten:"), pdfMetaSpin);

    QHBoxLayout *marginLayout = new QHBoxLayout ();
    marginLayout->addWidget (new QLabel (tr ("Oben:")));
    marginLayout->addWidget (marginTopSpin);
    marginLayout->addWidget (new QLabel (tr ("Rechts:")));
    marginLayout->addWidget (marginRightSpin);
    marginLayout->addWidget (new QLabel (tr ("Unten:")));
    marginLayout->addWidget (marginBottomSpin);
    marginLayout->addWidget (new QLabel (tr ("Links:")));
    marginLayout->addWidget (marginLeftSpin);
    pdfLayout->addRow (tr ("Seitenränder:"), marginLayout);
    pdfGroup->setLayout (pdfLayout);
    form->addRow (pdfGroup);

    form->addRow (okButton);
    setLayout (form);
    setWindowTitle (tr ("Einstellungen"));
}

//--------------------------------------------------------------------------------------------------

void SettingsWizard::updateDurationLabel (
    int value)
{
    //  Berechnet die ungefähre Audio-Dauer basierend auf der Puffergröße in Kilobyte.
    //  Formel: (Größe in Bytes) / (Samplerate * Kanalanzahl * Bytes pro Sample)
    double seconds = (value * 1024.0) / (48000.0 * 2.0 * 4.0);
    durationLabel->setText (
        tr ("Entspricht ca. %1 Sekunden Audio").arg (QString::number (seconds, 'f', 2)));
}

//--------------------------------------------------------------------------------------------------

void SettingsWizard::syncSysGainSlider (
    double value)
{
    //  Blockiert Signale, um eine Endlos-Schleife aus "valueChanged"-Signalen zu verhindern.
    sysGainSlider->blockSignals (true);
    //  Rechnet den linearen SpinBox-Wert in einen logarithmischen Slider-Wert um.
    int sliderValue = int (((std::log10 (value) + 2.0) / 3.0) * 1000.0);
    sysGainSlider->setValue (sliderValue);
    sysGainSlider->blockSignals (false);
}

//--------------------------------------------------------------------------------------------------

void SettingsWizard::syncMicGainSlider (
    double value)
{
    micGainSlider->blockSignals (true);
    int sliderValue = int (((std::log10 (value) + 2.0) / 3.0) * 1000.0);
    micGainSlider->setValue (sliderValue);
    micGainSlider->blockSignals (false);
}

//--------------------------------------------------------------------------------------------------

void SettingsWizard::syncSysGainSpin (
    int sliderValue)
{
    sysGainSpin->blockSignals (true);
    //  Rechnet den logarithmischen Slider-Wert zurück in einen linearen Wert für die SpinBox.
    double gain = std::pow (10.0, (sliderValue / 1000.0) * 3.0 - 2.0);
    sysGainSpin->setValue (gain);
    sysGainSpin->blockSignals (false);
}

//--------------------------------------------------------------------------------------------------

void SettingsWizard::syncMicGainSpin (
    int sliderValue)
{
    micGainSpin->blockSignals (true);
    double gain = std::pow (10.0, (sliderValue / 1000.0) * 3.0 - 2.0);
    micGainSpin->setValue (gain);
    micGainSpin->blockSignals (false);
}

//--------------------------------------------------------------------------------------------------

int SettingsWizard::validateBufferSize (
    int kb)
{
    //  Stellt sicher, dass die Puffergröße ein Vielfaches von 64 ist, um
    //  Audio-Artefakte durch ungerade Blockgrößen zu vermeiden.
    if (kb < 128)
    {
        kb = 128;
    }
    else if (kb > 3840)
    {
        kb = 3840;
    }

    int remainder = kb % 64;
    if (remainder != 0)
    {
        kb += (64 - remainder);
    }
    return kb;
}

//--------------------------------------------------------------------------------------------------

void SettingsWizard::saveSettings ()
{
    QSettings settings ("SS2025FP_T2", "AudioTranskriptor");
    settings.setValue ("pythonPath", pythonEdit->text ());
    settings.setValue ("scriptPath", scriptEdit->text ());
    settings.setValue ("wavPath", wavEdit->text ());
    settings.setValue ("asrWavPath", asrWavEdit->text ());
    settings.setValue ("meetingPath", meetingEdit->text ());
    settings.setValue ("audio/bufferThreshold", validateBufferSize (bufferSlider->value ()));
    settings.setValue ("sysGain", sysGainSpin->value ());
    settings.setValue ("micGain", micGainSpin->value ());
    settings.setValue ("pdf/fontSizeHeadline", pdfHeadlineSpin->value ());
    settings.setValue ("pdf/fontSizeBody", pdfBodySpin->value ());
    settings.setValue ("pdf/fontSizeMeta", pdfMetaSpin->value ());
    settings.setValue ("pdf/fontFamily", fontFamilyCombo->currentFont ().family ());
    settings.setValue ("pdf/marginTop", marginTopSpin->value ());
    settings.setValue ("pdf/marginRight", marginRightSpin->value ());
    settings.setValue ("pdf/marginBottom", marginBottomSpin->value ());
    settings.setValue ("pdf/marginLeft", marginLeftSpin->value ());

    accept (); //  Schließt den Dialog mit dem "Accepted"-Status.
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
