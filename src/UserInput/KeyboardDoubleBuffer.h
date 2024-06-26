#ifndef KEYBOARDDOUBLEBUFFER_H
#define KEYBOARDDOUBLEBUFFER_H

#include <chrono>
#include "IKeyboardDoubleBuffer.h"

typedef unsigned int uint;
typedef unsigned short int ushort;
typedef unsigned char uchar;

class KeyboardDoubleBuffer: public IKeyboardDoubleBuffer{

public:
    KeyboardDoubleBuffer(const uint& sampleSize, const ushort& keyCount);
    ~KeyboardDoubleBuffer() override;
    uchar** getInactiveBuffer() override;
    uchar** getActiveBuffer() override;
    void swapActiveBuffer() override;
    void clearInactiveBuffer() override;
    long getActivationTimestamp() override;

    ushort getKeyCount() override;
    uint getSampleSize() override;

private:
    const ushort keyCount;
    const uint sampleSize;

    bool activeBuffer;
    long activationTimestamp[2];
    uchar** buffer[2];//first dimension is keyCount second sampleSize
};

#endif
