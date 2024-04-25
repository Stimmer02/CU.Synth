#ifndef COMPONENT_ECHO_H
#define COMPONENT_ECHO_H

#include "AComponent.h"

namespace pipeline{
    class Component_Echo: public AComponent{
    public:
        Component_Echo(const audioFormatInfo* audioInfo);
        ~Component_Echo();

        void apply(pipelineAudioBuffer_CUDA* buffer) override;
        void clear() override;
        void defaultSettings() override;
        void set(uint index, float value);

    private:
        static const std::string privateNames[];

        float** lMemory;
        float** rMemory;

        int currentSample;
        int sampleCount;

        const uint maxDelayTime;

        float& lvol = settings.values[0];
        float& rvol = settings.values[1];
        float& delay = settings.values[2];
        float& fade = settings.values[3];
        float& repeats = settings.values[4];

    };
}

#endif
