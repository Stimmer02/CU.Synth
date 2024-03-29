#include "AdvancedComponent_Sum2.h"

using namespace pipeline;
const std::string AdvancedComponent_Sum2::privateNames[2] = {"vol0", "vol1"};

AdvancedComponent_Sum2::AdvancedComponent_Sum2(const audioFormatInfo* audioInfo, audioBufferQueue* boundBuffer): AAdvancedComponent(audioInfo, 2, privateNames, 2, boundBuffer){
    defaultSettings();
}

AdvancedComponent_Sum2::~AdvancedComponent_Sum2(){}

void AdvancedComponent_Sum2::apply(pipelineAudioBuffer* buffer){
    for (uint i = 0; i < audioInfo->sampleSize; i++){
        buffer->bufferL[i] = connections[0]->buffer.bufferL[i] * vol0 + connections[1]->buffer.bufferL[i] * vol1;
        buffer->bufferR[i] = connections[0]->buffer.bufferR[i] * vol0 + connections[1]->buffer.bufferR[i] * vol1;
    }
}

void AdvancedComponent_Sum2::clear(){}

void AdvancedComponent_Sum2::defaultSettings(){
    settings.values[0] = 0.5;//vol0
    settings.values[1] = 0.5;//vol1
}

bool AdvancedComponent_Sum2::allNeededConnections(){
    return maxConnections == connectionsCount;
}
