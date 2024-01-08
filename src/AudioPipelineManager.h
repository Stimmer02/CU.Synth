#ifndef AUDIOPIPELINEMANAGER_H
#define AUDIOPIPELINEMANAGER_H

#include "Pipeline/IDManager.h"
#include "Pipeline/Input.h"
#include "Pipeline/Output.h"
#include "Pipeline/Statistics/PipelineStatisticsService.h"
#include "Pipeline/Statistics/pipelineStatistics.h"

#include "Pipeline/pipelineAudioBuffer.h"
#include "UserInput/MIDI/MidiFileReader.h"


namespace pipeline{
    class AudioPipelineManager{
    public:
        AudioPipelineManager(audioFormatInfo audioInfo, ushort keyCount);
        ~AudioPipelineManager();

        char start();
        void stop();
        bool isRuning();

        const statistics::pipelineStatistics* getStatistics();
        const audioFormatInfo* getAudioInfo();

        char recordUntilStreamEmpty(MIDI::MidiFileReader& midi, short synthID, std::string filename = "");//TODO: rethink this function
        bool IDValid(pipeline::ID_type type, short ID);


        //OUTPUT CONTROL
        char startRecording();
        char startRecording(std::string outPath);
        char stopRecording();
        bool isRecording();


        //INPUT CONTROL
        void pauseInput();
        void reausumeInput();

        short addInput(AKeyboardRecorder*& input);
        char removeInput(short ID);
        short getInputCount();

        short addSynthesizer();
        char removeSynthesizer(short ID);
        short getSynthesizerCount();

        char connectInputToSynth(short inputID, short synthID);

        const synthesizer::settings* getSynthSettings(const ushort& ID);
        synthesizer::generator_type getSynthType(const ushort& ID);
        void setSynthSettings(const ushort& ID, const synthesizer::settings_name& settingsName, const float& value);
        void setSynthSettings(const ushort& ID, const synthesizer::generator_type& type);

        char saveSynthConfig(std::string path, ushort ID);
        char loadSynthConfig(std::string path, ushort ID);

    private:
        void pipelineThreadFunction();

        const audioFormatInfo audioInfo;
        const ushort keyCount;

        Input input;
        Output output;

        pipelineAudioBuffer* temporaryBuffer; //TODO:remove this

        statistics::PipelineStatisticsService* statisticsService;

        bool running;
        std::thread* pipelineThread;
    };
}

#endif
