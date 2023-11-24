#include "BufferConverter_Mono24.h"

void BufferConverter_Mono24::toPCM(pipelineAudioBuffer* pipelineBuffer, audioBuffer* pcmBuffer){
    static const uint maxValue = 0x00EFFFFF;
    int value;
    for (uint i = 0, j = 0; i < pipelineBuffer->size; i++){
        value = maxValue * pipelineBuffer->buffer[i];
        pcmBuffer->buff[j++] = value;
        pcmBuffer->buff[j++] = value >> 8;
        pcmBuffer->buff[j++] = value >> 16;
    }
}
