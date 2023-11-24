#include "Synthesizer.h"

using namespace synthesizer;

Synthesizer::Synthesizer(const audioFormatInfo& audioInfo, const ushort& keyCount){
    settings.keyCount = keyCount;
    settings.sampleSize = audioInfo.sampleSize;
    settings.sampleRate = audioInfo.sampleRate;
    settings.pitch = 0;
    settings.volume = 0.1;
    settings.attack.set(0.2, audioInfo.sampleRate);
    settings.sustain.set(0, audioInfo.sampleRate);
    settings.fade.set(0, audioInfo.sampleRate);
    settings.release.set(1.35, audioInfo.sampleRate);

    settings.maxValue = 0;
    uint a = 1;
    for (uint i = 1; i < audioInfo.bitDepth; i++){
        settings.maxValue += a;
        a <<= 1;
    }
    settings.maxValue -= 1;

    soundGenerator = new Generator_Sine();

    notes = new noteBuffer[keyCount];
    for (uint i = 0; i < keyCount; i++){
        notes[i].init(audioInfo.sampleSize);
    }

    calculateFrequencies();
}

Synthesizer::~Synthesizer(){
    delete soundGenerator;
    delete[] notes;
}

struct settings* Synthesizer::getSettings(){
    return &this->settings;
}

void Synthesizer::setGenerator(generator_type type){
    delete soundGenerator;
    switch (type) {
        case SINE:
            soundGenerator = new Generator_Sine();
            break;
        case SQUARE:
            soundGenerator = new Generator_Square();
            break;
        case TRIANGLE:
            soundGenerator = new Generator_Triangle();
            break;
    }
}

char Synthesizer::setPitch(const char& value, const bool& add){
    if (add){
        settings.pitch += value;
    } else {
        settings.pitch = value;
    }

    calculateFrequencies();
    return settings.pitch;
}

void Synthesizer::calculateFrequencies(){
    for (int i = 0; i < settings.keyCount; i++){
        notes[i].frequency = 440.0 * pow(2.0, (i+settings.pitch)/12.0);
        notes[i].multiplier = PI * 2 / settings.sampleRate * notes[i].frequency;

    }
}

void Synthesizer::generateSample(pipelineAudioBuffer* audioBuffer,  const keyboardTransferBuffer* keyboardState){
    for (uint i = 0; i < settings.keyCount; i++){
        soundGenerator->generate(notes[i], keyboardState->buffer[i], settings);
    }

    for (uint i = 0; i < settings.sampleSize; i++){
        audioBuffer->buffer[i] = 0;
        for (uint j = 0; j < settings.keyCount; j++){
            audioBuffer->buffer[i] += notes[j].buffer[i];//TODO: check multiplication
        }
    }
}
