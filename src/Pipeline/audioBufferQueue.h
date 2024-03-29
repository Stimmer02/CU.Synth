#ifndef AUDIOBUFFERQUEUE_H
#define AUDIOBUFFERQUEUE_H

#include "pipelineAudioBuffer.h"
#include "IDManager.h"
#include <string>
#include <vector>



namespace pipeline{

    struct audioBufferQueue{
    public:
        audioBufferQueue(const ID_type parentType, const uint sampleSize);
        ~audioBufferQueue();

        short getParentID();

        pipelineAudioBuffer buffer;
        const ID_type parentType;
        std::vector<short> componentIDQueue;

    private:
        short parentID; //seems wrong but it is ok

        friend class AudioPipelineManager;
    };

}

#endif
