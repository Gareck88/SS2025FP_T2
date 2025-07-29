#ifndef MACCAPTURE_H
#define MACCAPTURE_H

#include "capturethread.h"

class MacCapture : public CaptureThread
{
    Q_OBJECT
public:
    explicit MacCapture(QObject *parent = nullptr);

    // QThread interface
protected:
    void run();

    // CaptureThread interface
public:
    void stopCapture();
};

#endif // MACCAPTURE_H
