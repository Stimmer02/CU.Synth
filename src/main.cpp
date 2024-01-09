#include "AudioOutput/audioFormatInfo.h"
#include "AudioPipelineManager.h"
#include "SynthUserInterface.h"
#include "UserInput/InputMap.h"
#include "UserInput/KeyboardInput_DevInput.h"
#include "UserInput/KeyboardRecorder_DevSnd.h"
#include "UserInput/KeyboardRecorder_DevInput.h"
#include "UserInput/MIDI/MidiFileReader.h"
#include "UserInput/TerminalInputDiscard.h"


void updateStatistics(const statistics::pipelineStatistics* pStatistics, const audioFormatInfo& audioInfo);

int main(int argc, char** argv){

    const ushort keyCount = 127;

    audioFormatInfo audioInfo;
    audioInfo.bitDepth = 16;
    audioInfo.channels = 1;
    audioInfo.sampleRate = 48000;
    audioInfo.sampleSize = 512;
    audioInfo.littleEndian = true;
    audioInfo.byteRate = audioInfo.sampleRate * audioInfo.channels * audioInfo.bitDepth/8;
    audioInfo.blockAlign = audioInfo.channels * audioInfo.bitDepth/8;


    if (argc < 3){
        std::printf("USAGE: %s <GUI input event ID> <MIDI input stream>\nor\nUSAGE: %s <MIDI file path> <WAV file out>\n", argv[0], argv[0]);
        return 1;
    }


    AKeyboardRecorder* keyboardInput;
    InputMap* keyboardMap = nullptr;
    if (std::memcmp(argv[2], "input", 5) == 0){
        std::printf("Using keyboard as MIDI device: %s\n", argv[2]);

        keyboardMap = new InputMap("./incrementalKeyboardMap.txt");
        keyboardInput = new KeyboardRecorder_DevInput(keyCount, keyboardMap);
    } else if (std::memcmp(argv[2], "snd", 3) == 0){
        std::printf("Using MIDI device: %s\n", argv[2]);

        keyboardInput = new KeyboardRecorder_DevSnd(keyCount);
    } else {
        std::printf("Reading MIDI file: %s\n", argv[1]);

        MIDI::MidiFileReader midiReader(argv[1] ,audioInfo.sampleSize, audioInfo.sampleRate);
        AudioPipelineManager audioPipeline(audioInfo, keyCount);
        short synthID = audioPipeline.addSynthesizer();
        audioPipeline.loadSynthConfig("./config/synth.config", synthID);

        auto start = std::chrono::high_resolution_clock::now();
        if (audioPipeline.recordUntilStreamEmpty(midiReader, synthID, argv[2])){
            return 4;
        }
        auto end = std::chrono::high_resolution_clock::now();

        std::printf("File successfully saved as: %s\n", argv[2]);
        std::printf("Time elapsed: %f s\n", std::chrono::duration<double>(end-start).count());
    }

    std::string GUIStreamLocation = "/dev/input/event";
    GUIStreamLocation += argv[1];
    std::string MIDIStreamLocation = "/dev/";
    MIDIStreamLocation += argv[2];

    IKeyboardInput* userInput = new KeyboardInput_DevInput();

    if (keyboardInput->init(MIDIStreamLocation, audioInfo.sampleSize, audioInfo.sampleRate)){
        return 2;
    }

    if (userInput->init(GUIStreamLocation)){
        return 3;
    }

    SynthUserInterface userInterface(audioInfo, keyboardInput, userInput, keyCount);

    userInterface.start();

    if (keyboardMap != nullptr){
        delete keyboardMap;
    }

    return 0;
}


