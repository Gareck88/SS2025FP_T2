#include "maccapture.h"

#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <QThread>
#include <QList>

// Globale Variablen (extern aus CaptureThread.h)
std::vector<float> g_hqBuffer;
std::vector<int16_t> g_asrBuffer;
std::mutex g_hqMutex;
std::mutex g_asrMutex;

static AudioUnit inputUnit = nullptr;

static OSStatus inputCallback(
    void *inRefCon,
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp *inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList *ioData)
{
    MacCapture *capture = static_cast<MacCapture*>(inRefCon);

    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0].mNumberChannels = 1; // Mono
    bufferList.mBuffers[0].mDataByteSize = inNumberFrames * sizeof(float);
    float buffer[inNumberFrames];
    bufferList.mBuffers[0].mData = buffer;

    OSStatus status = AudioUnitRender(inputUnit, ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, &bufferList);
    if (status != noErr) {
        return status;
    }

    {
        std::lock_guard<std::mutex> lock(g_hqMutex);
        for (UInt32 i = 0; i < inNumberFrames; ++i) {
            g_hqBuffer.push_back(buffer[i]);
        }
    }

    // Optional: Signal senden (wenn nötig, über Qt-Thread-Signalmechanismus)
    // emit capture->pcmChunkReady(QList<float>::fromVector(QVector<float>::fromStdVector(g_hqBuffer)));

    return noErr;
}

MacCapture::MacCapture(QObject *parent)
    : CaptureThread(parent)
{
}

void MacCapture::run()
{
    // Setup Audio Unit
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    AudioComponent inputComponent = AudioComponentFindNext(nullptr, &desc);
    if (!inputComponent) {
        qWarning("AudioComponentFindNext failed");
        return;
    }

    if (AudioComponentInstanceNew(inputComponent, &inputUnit) != noErr) {
        qWarning("AudioComponentInstanceNew failed");
        return;
    }

    // Input aktivieren (Bus 1 ist Input)
    UInt32 enableIO = 1;
    if (AudioUnitSetProperty(inputUnit,
                             kAudioOutputUnitProperty_EnableIO,
                             kAudioUnitScope_Input,
                             1,
                             &enableIO,
                             sizeof(enableIO)) != noErr) {
        qWarning("Enable IO failed");
        return;
    }

    // Stream Format setzen (Float 32bit, Mono, 44100 Hz)
    AudioStreamBasicDescription streamFormat = {0};
    streamFormat.mSampleRate = 44100;
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    streamFormat.mBitsPerChannel = 32;
    streamFormat.mChannelsPerFrame = 1;
    streamFormat.mBytesPerFrame = sizeof(float);
    streamFormat.mFramesPerPacket = 1;
    streamFormat.mBytesPerPacket = sizeof(float);

    if (AudioUnitSetProperty(inputUnit,
                             kAudioUnitProperty_StreamFormat,
                             kAudioUnitScope_Output,
                             1,
                             &streamFormat,
                             sizeof(streamFormat)) != noErr) {
        qWarning("Set Stream Format failed");
        return;
    }

    // Callback setzen
    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = inputCallback;
    callbackStruct.inputProcRefCon = this;
    if (AudioUnitSetProperty(inputUnit,
                             kAudioOutputUnitProperty_SetInputCallback,
                             kAudioUnitScope_Global,
                             0,
                             &callbackStruct,
                             sizeof(callbackStruct)) != noErr) {
        qWarning("Set Input Callback failed");
        return;
    }

    if (AudioUnitInitialize(inputUnit) != noErr) {
        qWarning("AudioUnitInitialize failed");
        return;
    }

    if (AudioOutputUnitStart(inputUnit) != noErr) {
        qWarning("AudioOutputUnitStart failed");
        return;
    }

    // Thread-Lauf-Schleife
    while (!QThread::currentThread()->isInterruptionRequested()) {
        QThread::msleep(10);

        // Hier kannst du z.B. Buffer auslesen und eventuell ein Signal senden
        // oder Buffer aufräumen
    }

    AudioOutputUnitStop(inputUnit);
    AudioUnitUninitialize(inputUnit);
    AudioComponentInstanceDispose(inputUnit);
    inputUnit = nullptr;

    emit stopped();
}

void MacCapture::stopCapture()
{
    requestInterruption();
}
