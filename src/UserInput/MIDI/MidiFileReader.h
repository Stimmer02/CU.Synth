#ifndef MIDIFILEREADR_H
#define MIDIFILEREADR_H

#include "../keyboardTransferBuffer.h"
#include "MidiMessageInterpreter.h"
#include "midiEvent.h"
#include "midiHeaderChunk.h"
#include "midiSettings.h"
#include <string>
#include <cstring>
#include <memory>

namespace MIDI{
    class MidiFileReader{
    public:
        MidiFileReader(std::string path, uint sampleSize, uint sampleRate);
        ~MidiFileReader();

        char rewindFile();
        char rewindChunk(ushort chunkNumber);
        bool eofChunk(ushort chunkNumber);
        bool isFileReady();
        void fillBuffer(keyboardTransferBuffer* buffer, ushort chunkNumber);
        uint16_t getChunkCount();
        char close();

    private:

        char parseFile();
        void readReverse(void* out, uint byteCount);
        void readReverse(uint16_t& out);
        void readReverse(uint32_t& out);
        void readReverse(uint64_t& out);

        uint eventTimePlacement(ushort chunkNumber);

        MidiMessageInterpreter interpreter;
        uchar* tempNoteBuffer[127];

        midiCheaderChunk info;
        midiSettings settings;

        midiChunk* chunks;
        midiEvent* lastEvent;
        double* chunkTime;
        ulong* lastEventTime;
        bool* endOfChunk;

        bool fileReady;
        std::ifstream* file;

        const uint sampleSize;
        const uint sampleRate;
    };
}
#endif
