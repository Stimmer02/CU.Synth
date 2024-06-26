#ifndef AUDIOSPECTRUMVISUALIZER_CUDA_H
#define AUDIOSPECTRUMVISUALIZER_CUDA_H

#include "./pipelineAudioBuffer_CUDA.h"
#include "../AudioOutput/audioFormatInfo.h"

#include <cufft.h>
#include <cuda_runtime_api.h>
#include <cuda.h>
#include <cmath>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

class AudioSpectrumVisualizer_CUDA{
public:
    AudioSpectrumVisualizer_CUDA(const audioFormatInfo* audioInfo, uint audioWindowSize, float fps);
    ~AudioSpectrumVisualizer_CUDA();

    void start();
    void stop();
    void readTerminalDimensions();
    void displayBuffer(pipelineAudioBuffer_CUDA* buffer);
    float setFps(float fps);
    float getFps();
    void setAudioWindowSize(uint size);
    uint getAudioWindowSize();

    float getMinFrequency();
    float getMaxFrequency();

    void setVolume(float volume);
    float getVolume();
    float setHighScope(float highScope);
    float getHighScope();
    float setLowScope(float lowScope);
    float getLowScope();

private:
    void draw(const char* c = "#");
    void computeFFT();
    
    bool running;
    uint width;
    uint height;

    const audioFormatInfo* audioInfo;
    float* bandsState;
    uint skipSamples;
    uint sampleCounter;

    uint audioWindowSize;
    uint samplesPerFrame; //how many times displayBuffer() has to be called to execute computeFFT()
    cufftReal* d_workBuffer;
    cufftComplex* d_cufftOutput;
    cufftComplex* cufftOutput;
    cufftHandle cufftPlan;

    float highScope;
    float lowScope;

    float volume;

    uint lastWidth;
    std::string bottomLine;
};
#endif
