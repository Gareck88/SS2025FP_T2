#include "wavwriterthread.h"

#include <QDataStream>
#include <QSettings>

//--------------------------------------------------------------------------------------------------

WavWriterThread::WavWriterThread (
    QObject *parent)
    : QThread (parent)
    , m_hqBytesWritten (0)
    , m_asrBytesWritten (0)
    , m_flushThresholdBytes (384 * 1024)
    , m_downsampleOffset (0)
    , m_sampleRateHQ (48000)
    , m_channelsHQ (2)
    , m_bitsPerSampleHQ (32)
    , m_sampleRateASR (16000)
{
    m_active.store (false);
    m_shutdown.store (false);

    QSettings settings;
    m_flushThresholdBytes = settings.value ("audio/bufferThreshold", 384 * 1024).toInt ();
}

//--------------------------------------------------------------------------------------------------

WavWriterThread::~WavWriterThread ()
{
    shutdown ();
}

//--------------------------------------------------------------------------------------------------

void WavWriterThread::startWriting (
    const QString &hqPath, const QString &asrPath)
{
    QMutexLocker locker (&m_mutex);

    m_bufferFloat.clear ();
    m_hqBytesWritten = 0;
    m_asrBytesWritten = 0;
    m_downsampleOffset = 0;

    m_hqFile.setFileName (hqPath);
    m_asrFile.setFileName (asrPath);

    if (!m_hqFile.open (QIODevice::WriteOnly) || !m_asrFile.open (QIODevice::WriteOnly))
    {
        qWarning () << "WavWriterThread: Konnte Ausgabedateien nicht öffnen.";
        return;
    }

    //  Schreibt einen 44-Byte Platzhalter an den Anfang der Dateien. Dies reserviert den Platz
    //  für den WAV-Header, der erst am Ende geschrieben werden kann, wenn die
    //  Gesamtgröße der Audiodaten bekannt ist.
    m_hqFile.write (QByteArray (44, '\0'));
    m_asrFile.write (QByteArray (44, '\0'));

    m_active.store (true);
    m_mainLoopCond.wakeAll ();
}

//--------------------------------------------------------------------------------------------------

void WavWriterThread::shutdown ()
{
    QMutexLocker locker (&m_mutex);
    m_shutdown.store (true);
    m_active.store (false);
    m_mainLoopCond.wakeAll ();
    m_dataAvailableCond.wakeAll ();
    locker.unlock ();

    //  Wartet, bis die run()-Methode vollständig beendet ist. Dies ist entscheidend,
    //  um Race Conditions zu vermeiden. Ohne wait() könnte das WavWriterThread-Objekt
    //  im Hauptthread zerstört werden, während die run()-Methode noch auf Ressourcen zugreift,
    //  was zu einem Absturz führen würde.
    wait ();
}

//--------------------------------------------------------------------------------------------------

void WavWriterThread::stopWriting ()
{
    QMutexLocker locker (&m_mutex);
    m_active.store (false);
    m_dataAvailableCond.wakeAll ();
}

//--------------------------------------------------------------------------------------------------

void WavWriterThread::writeChunk (
    QList<float> chunk)
{
    //  Dieser Slot wird vom Capture-Thread aufgerufen (Producer) und ist thread-sicher.
    QMutexLocker locker (&m_mutex);
    m_bufferFloat.append (chunk);
    m_dataAvailableCond.wakeOne (); //  Weckt den Consumer (die run() Methode) auf.
}

//--------------------------------------------------------------------------------------------------

void WavWriterThread::run ()
{
    //  Die Hauptschleife implementiert ein Producer-Consumer-Muster.
    //  writeChunk() ist der Producer, diese run()-Schleife ist der Consumer.
    while (!m_shutdown.load ())
    {
        //  --- Phase 1: Warten auf den Startbefehl ---
        {
            QMutexLocker locker (&m_mutex);
            if (!m_active.load ())
            {
                m_mainLoopCond.wait (&m_mutex);
            }
        }

        if (m_shutdown.load ())
        {
            break;
        }

        QList<float> bufferToWrite;

        //  --- Phase 2: Aktive Schreib-Schleife ---
        while (m_active.load ())
        {
            QList<float> receivedChunk;
            {
                QMutexLocker locker (&m_mutex);
                if (m_bufferFloat.isEmpty () && m_active.load ())
                {
                    m_dataAvailableCond.wait (&m_mutex);
                }
                receivedChunk.swap (m_bufferFloat);
            }

            if (!receivedChunk.isEmpty ())
            {
                bufferToWrite.append (receivedChunk);
            }

            if (bufferToWrite.size () * sizeof (float) >= m_flushThresholdBytes)
            {
                writeCurrentBufferToDisk (bufferToWrite);
            }
        }

        //  --- Phase 3: Finalisierung am Ende einer Schreib-Session ---
        {
            QMutexLocker locker (&m_mutex);
            bufferToWrite.append (m_bufferFloat);
            m_bufferFloat.clear ();

            if (!bufferToWrite.isEmpty ())
            {
                writeCurrentBufferToDisk (bufferToWrite);
            }

            writeHeaders (m_hqBytesWritten, m_asrBytesWritten);
        }

        emit finishedWriting ();
    }
}

//--------------------------------------------------------------------------------------------------

void WavWriterThread::writeCurrentBufferToDisk (
    QList<float> &buffer)
{
    //  HQ-Datei: Die 32-bit float-Daten werden unverändert geschrieben.
    const char *data = reinterpret_cast<const char *> (buffer.data ());
    qint64 byteCount = buffer.size () * sizeof (float);
    m_hqFile.write (data, byteCount);
    m_hqBytesWritten += byteCount;

    //  ASR-Datei: Konvertierung und Downsampling für die Spracherkennung.
    int frames = buffer.size () / 2;
    QByteArray mono;

    //  Das Downsampling von 48kHz auf 16kHz (Faktor 3) erfordert, dass wir nur jedes dritte Sample nehmen.
    int available = frames + m_downsampleOffset;
    int outFrames = available / 3;
    mono.resize (outFrames * sizeof (int16_t));
    int16_t *out = reinterpret_cast<int16_t *> (mono.data ());

    int outIndex = 0;
    for (int i = m_downsampleOffset; i + 2 < frames; i += 3)
    {
        float l = buffer[2 * i];
        float r = buffer[2 * i + 1];

        //  Downmix von Stereo zu Mono durch Mittelwertbildung.
        float m = 0.5f * (l + r);

        //  Konvertierung von float [-1.0, 1.0] zu 16-bit signed integer [-32767, 32767].
        out[outIndex++] = qBound (int16_t (-32768), int16_t (m * 32767.f), int16_t (32767));
    }

    //  Der Offset merkt sich, wie viele Samples am Ende des Chunks übrig bleiben,
    //  um beim nächsten Chunk an der richtigen Position weiterzumachen.
    m_downsampleOffset = (frames + m_downsampleOffset) % 3;

    emit audioBytesReady(mono);


    m_asrFile.write (mono);
    m_asrBytesWritten += mono.size ();
    buffer.clear ();
}

//--------------------------------------------------------------------------------------------------

void WavWriterThread::writeHeaders (
    qint64 hqBytes, qint64 asrBytes)
{
    //  --- Header für die High-Quality-Datei (Stereo, 48kHz, 32-bit Float) ---
    m_hqFile.seek (0);
    QDataStream hqs (&m_hqFile);
    hqs.setByteOrder (QDataStream::LittleEndian);

    //  "RIFF" (Resource Interchange File Format) ist der Container für die WAV-Daten.
    hqs.writeRawData ("RIFF", 4);
    hqs << quint32 (36 + hqBytes); //  Gesamtdateigröße minus der ersten 8 Bytes.

    //  "WAVE" deklariert den spezifischen RIFF-Typ.
    hqs.writeRawData ("WAVE", 4);

    //  "fmt " (mit Leerzeichen) leitet den Format-Block ein.
    hqs.writeRawData ("fmt ", 4);

    hqs << quint32 (16); //  Größe des Format-Blocks (16 für PCM/Float).
    hqs << quint16 (3);  //  Audio-Format-Code: 3 = 32-bit IEEE Float.
    hqs << quint16 (m_channelsHQ);
    hqs << quint32 (m_sampleRateHQ);
    hqs << quint32 (m_sampleRateHQ * m_channelsHQ
                    * (m_bitsPerSampleHQ / 8)); //  ByteRate = wie viele Bytes pro Sekunde.
    hqs << quint16 (m_channelsHQ * (m_bitsPerSampleHQ / 8)); //  BlockAlign = Bytes pro Sample-Frame.
    hqs << quint16 (m_bitsPerSampleHQ);

    //  "data" leitet den Block mit den eigentlichen Audio-Samples ein.
    hqs.writeRawData ("data", 4);
    hqs << quint32 (hqBytes); //  Größe der reinen Audiodaten.

    //  --- Header für die ASR-Datei (Mono, 16kHz, 16-bit PCM) ---
    m_asrFile.seek (0);
    QDataStream asrs (&m_asrFile);
    asrs.setByteOrder (QDataStream::LittleEndian);

    asrs.writeRawData ("RIFF", 4);
    asrs << quint32 (36 + asrBytes);
    asrs.writeRawData ("WAVE", 4);
    asrs.writeRawData ("fmt ", 4);
    asrs << quint32 (16);
    asrs << quint16 (1); //  Audio-Format-Code: 1 = 16-bit Integer PCM.
    asrs << quint16 (1); //  Anzahl Kanäle: 1 = Mono.
    asrs << quint32 (m_sampleRateASR);
    asrs << quint32 (m_sampleRateASR * 1 * (16 / 8));
    asrs << quint16 (1 * (16 / 8));
    asrs << quint16 (16);
    asrs.writeRawData ("data", 4);
    asrs << quint32 (asrBytes);

    m_hqFile.close ();
    m_asrFile.close ();
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
