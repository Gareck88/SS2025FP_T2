#include "pulsecapturethread.h"

#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <pulse/error.h>
#include <pulse/simple.h>

//--------------------------------------------------------------------------------------------------

PulseCaptureThread::PulseCaptureThread (
    QObject* parent)
    : CaptureThread (parent)
    , m_paSys (nullptr)
    , m_paMic (nullptr)
    , m_modLoop (-1)
    , m_modNull (-1)
    , m_sysGain (1.0f)
    , m_micGain (1.0f)
{
}

//--------------------------------------------------------------------------------------------------

bool PulseCaptureThread::initializeCapture ()
{
    //  ---- 1. Gain-Werte aus Settings lesen ----
    QSettings settings ("SS2025FP_T2", "AudioTranskriptor");
    m_sysGain = settings.value ("sysGain", 0.5f).toFloat ();
    m_micGain = settings.value ("micGain", 6.0f).toFloat ();

    //  ---- 2. Default-Sink & -Source ermitteln ----
    //  Führt 'pactl info' aus, um die Namen der Standard-Audio-Geräte zu ermitteln.
    QProcess infoProc;
    infoProc.start ("pactl", {"info"});
    infoProc.waitForFinished (500); //  Kurzer Timeout von 500ms.
    QString info = QString::fromUtf8 (infoProc.readAllStandardOutput ());

    //  Da die Ausgabe von 'pactl' je nach Systemsprache variiert (z.B. Englisch/Deutsch),
    //  wird mit regulären Ausdrücken nach beiden Varianten gesucht.
    QRegularExpression reEnSink (R"(Default Sink:\s*(\S+))"),
        reDeSink (R"(Standard-Ziel:\s*(\S+))"), reEnSrc (R"(Default Source:\s*(\S+))"),
        reDeSrc (R"(Standard-Quelle:\s*(\S+))");
    QRegularExpressionMatch m;
    QString origSink, origSource;

    if ((m = reEnSink.match (info)).hasMatch ())
    {
        origSink = m.captured (1);
    }
    else if ((m = reDeSink.match (info)).hasMatch ())
    {
        origSink = m.captured (1);
    }

    if (origSink.isEmpty ())
    {
        qWarning () << "PulseCapture: Default-Sink nicht gefunden";
        return false;
    }

    if ((m = reEnSrc.match (info)).hasMatch ())
    {
        origSource = m.captured (1);
    }
    else if ((m = reDeSrc.match (info)).hasMatch ())
    {
        origSource = m.captured (1);
    }

    if (origSource.isEmpty ())
    {
        qWarning () << "PulseCapture: Default-Source nicht gefunden";
        return false;
    }

    //  ---- 3. Null-Sink + Loopback für's Mikrofon erstellen ----
    //  Dieser Lambda-Ausdruck kapselt den Aufruf von 'pactl load-module'.
    auto loadModule = [&] (const QString& params) -> int
    {
        QProcess p;
        p.start ("pactl", QStringList{"load-module"} << params.split (' '));
        p.waitForFinished ();
        bool ok;
        int id = QString::fromUtf8 (p.readAllStandardOutput ()).trimmed ().toInt (&ok);
        if (!ok)
        {
            qWarning () << "load-module failed:" << params;
        }
        return ok ? id : -1;
    };

    //  Erstellt eine virtuelle Senke (quasi eine virtuelle Soundkarte) nur für das Mikrofon.
    m_modNull = loadModule (
        "module-null-sink sink_name=mic_sink sink_properties=device.description=MicSink");
    //  Leitet die Standard-Audioquelle (Mikrofon) auf unsere virtuelle Senke um.
    m_modLoop = loadModule (QString ("module-loopback source=%1 sink=mic_sink").arg (origSource));

    if (m_modNull < 0 || m_modLoop < 0)
    {
        qWarning () << "PulseCapture: Erstellen der PulseAudio-Module fehlgeschlagen.";
        cleanupCapture (); //  Räumt eventuell schon erstellte Module wieder ab.
        return false;
    }

    //  ---- 4. pa_simple Streams öffnen ----
    //  Ein ".monitor"-Source in PulseAudio erlaubt das "Mithören" eines Geräts.
    QString sysMon = origSink + ".monitor";
    QString micMon = "mic_sink.monitor";
    //  Audio-Spezifikation: 32-bit Float, Little Endian, 48000 Hz, 2 Kanäle (Stereo).
    pa_sample_spec ss{PA_SAMPLE_FLOAT32LE, 48000, 2};
    int error = 0;

    m_paSys = pa_simple_new (nullptr,
                             "AudioTranskriptor",
                             PA_STREAM_RECORD,
                             sysMon.toUtf8 ().constData (),
                             "syscap",
                             &ss,
                             nullptr,
                             nullptr,
                             &error);
    if (!m_paSys)
    {
        qWarning () << "pa_simple_new(sys) failed:" << pa_strerror (error);
        cleanupCapture ();
        return false;
    }

    m_paMic = pa_simple_new (nullptr,
                             "AudioTranskriptor",
                             PA_STREAM_RECORD,
                             micMon.toUtf8 ().constData (),
                             "miccap",
                             &ss,
                             nullptr,
                             nullptr,
                             &error);
    if (!m_paMic)
    {
        qWarning () << "pa_simple_new(mic) failed:" << pa_strerror (error);
        cleanupCapture ();
        return false;
    }

    //  ---- 5. Puffer vorbereiten ----
    const int FRAMES = 1024;    //  Anzahl der Audio-Frames pro Lese-Iteration.
    bufSys.resize (FRAMES * 2); //  Größe = Frames * Anzahl der Kanäle.
    bufMic.resize (FRAMES * 2);
    bufMix.resize (FRAMES * 2);

    return true; //  Alles hat geklappt!
}

//--------------------------------------------------------------------------------------------------

void PulseCaptureThread::captureLoopIteration ()
{
    int error = 0;
    //  Liest synchron von beiden Streams die Audiodaten ein.
    if (pa_simple_read (m_paSys, bufSys.data (), bufSys.size () * sizeof (float), &error) < 0
        || pa_simple_read (m_paMic, bufMic.data (), bufMic.size () * sizeof (float), &error) < 0)
    {
        qWarning () << "PulseCapture: pa_simple_read fehlgeschlagen:" << pa_strerror (error);
        //  Bei einem kritischen Lesefehler wird die aktuelle Aufnahme-Session beendet.
        stopCapture ();
        return;
    }

    //  Die Audiodaten von System und Mikrofon werden hier additiv gemischt.
    for (size_t i = 0; i < bufMix.size (); ++i)
    {
        float v = m_sysGain * bufSys[i] + m_micGain * bufMic[i];
        //  qBound verhindert Übersteuerung (Clipping), indem der Wert auf den Bereich [-1.0, 1.0] begrenzt wird.
        bufMix[i] = qBound (-1.0f, v, 1.0f);
    }

    //  Der fertige Audio-Block wird zur weiteren Verarbeitung an den WavWriterThread gesendet.
    QList<float> chunk;
    chunk.reserve (bufMix.size ());
    std::copy (bufMix.begin (), bufMix.end (), std::back_inserter (chunk));
    emit pcmChunkReady (chunk);
}

//--------------------------------------------------------------------------------------------------

void PulseCaptureThread::cleanupCapture ()
{
    //  --- Draining des PulseAudio-Puffers ---
    //  Dieser optionale Schritt versucht, Audiodaten auszulesen, die sich zum Zeitpunkt
    //  des Stopp-Befehls noch im Server-Puffer befanden, um Datenverlust zu minimieren.
    if (m_paSys && m_paMic)
    {
        pa_sample_spec ss;
        ss.rate = 48000;
        ss.channels = 2;
        int error = 0;

        //  Versuche, bis zu 2 Sekunden nachzulesen.
        int remaining_frames = ss.rate * 2;
        const int read_frames = 1024;

        while (remaining_frames > 0)
        {
            if (pa_simple_read (m_paSys,
                                bufSys.data (),
                                read_frames * ss.channels * sizeof (float),
                                &error)
                    < 0
                || pa_simple_read (m_paMic,
                                   bufMic.data (),
                                   read_frames * ss.channels * sizeof (float),
                                   &error)
                       < 0)
            {
                //  Wenn es hier einen Fehler gibt (z.B. weil der Puffer leer ist), brechen wir ab.
                qWarning () << "pa_simple_read(drain) failed, stopping drain:"
                            << pa_strerror (error);
                break;
            }

            //  Mischen und Senden des letzten Chunks.
            for (size_t i = 0; i < bufMix.size (); ++i)
            {
                bufMix[i] = qBound (-1.0f, m_sysGain * bufSys[i] + m_micGain * bufMic[i], 1.0f);
            }
            QList<float> chunk;
            chunk.reserve (bufMix.size ());
            std::copy (bufMix.begin (), bufMix.end (), std::back_inserter (chunk));
            emit pcmChunkReady (chunk);

            remaining_frames -= read_frames;
        }
    }

    //  Gib die PulseAudio-Streams frei, falls sie erfolgreich erstellt wurden.
    if (m_paSys)
    {
        pa_simple_free (m_paSys);
        m_paSys = nullptr; //  Wichtig: Zeiger zurücksetzen, um doppelte Freigaben zu verhindern.
    }
    if (m_paMic)
    {
        pa_simple_free (m_paMic);
        m_paMic = nullptr;
    }

    //  Entlade die PulseAudio-Module, die wir in initializeCapture() erstellt haben,
    //  um das System in den Ausgangszustand zurückzuversetzen.
    if (m_modLoop >= 0)
    {
        QProcess::execute ("pactl", {"unload-module", QString::number (m_modLoop)});
        m_modLoop = -1; //  ID zurücksetzen, um doppelte Ausführung zu verhindern.
    }
    if (m_modNull >= 0)
    {
        QProcess::execute ("pactl", {"unload-module", QString::number (m_modNull)});
        m_modNull = -1;
    }
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
