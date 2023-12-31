#ifndef AUDIOPIPELINESUBSTITUTE_H
#define AUDIOPIPELINESUBSTITUTE_H


#include "AudioOutput/audioFormatInfo.h"
#include "Synthesizer/AGenerator.h"
#include "Synthesizer/settings.h"
#include "UserInput/AKeyboardRecorder.h"
#include "UserInput/KeyboardRecorder_DevInput.h"
#include "UserInput/MIDI/MidiFileReader.h"
#include "UserInput/keyboardTransferBuffer.h"
#include "AudioOutput/IOutStream.h"
#include "AudioOutput/audioBuffer.h"
#include "AudioOutput/OutStream_PulseAudio.h"
#include "Pipeline/pipelineAudioBuffer.h"
#include "Synthesizer.h"
#include "Pipeline/BufferConverter/IBufferConverter.h"
#include "Pipeline/BufferConverter/BufferConverter_Mono8.h"
#include "Pipeline/BufferConverter/BufferConverter_Mono16.h"
#include "Pipeline/BufferConverter/BufferConverter_Mono24.h"
#include "Pipeline/BufferConverter/BufferConverter_Mono32.h"
#include "Pipeline/BufferConverter/BufferConverter_Stereo8.h"
#include "Pipeline/BufferConverter/BufferConverter_Stereo16.h"
#include "Pipeline/BufferConverter/BufferConverter_Stereo24.h"
#include "Pipeline/BufferConverter/BufferConverter_Stereo32.h"
#include "Pipeline/Statistics/PipelineStatisticsService.h"
#include "Pipeline/Statistics/pipelineStatistics.h"
#include "AudioOutput/AudioRecorder.h"


class AudioPipelineSubstitute{
public:
    AudioPipelineSubstitute(audioFormatInfo audioInfo, ushort keyCount, AKeyboardRecorder* midiInput);
    ~AudioPipelineSubstitute();

    void start();
    void stop();

    void startRecording();
    void startRecording(std::string outPath);
    void stopRecording();
    bool isRecording();

    const statistics::pipelineStatistics* getStatistics();
    const audioFormatInfo* getAudioInfo();
    const synthesizer::settings* getSynthSettings(const ushort& id);
    synthesizer::generator_type getSynthType(const ushort& id);
    void setSynthSettings(const ushort& id, const synthesizer::settings_name& settingsName, const float& value);
    void setSynthSettings(const ushort& id, const synthesizer::generator_type& type);

    void recordUntilStreamEmpty(MIDI::MidiFileReader& midi, std::string filename = "");

    char saveSynthConfig(std::string path, ushort id);
    char loadSynthConfig(std::string path, ushort id);

private:
    void pipelineThreadFunction();

    audioFormatInfo audioInfo;
    ushort keyCount;

    AKeyboardRecorder* midiInput;
    IOutStream* audioOutput;
    synthesizer::Synthesizer* synth;
    AudioRecorder audioRecorder;

    statistics::PipelineStatisticsService* statisticsService;

    keyboardTransferBuffer* keyboardState;
    pipelineAudioBuffer* pipelineBuffer;
    audioBuffer* buffer;
    IBufferConverter* bufferConverter;

    bool running;
    bool recording;

    std::thread* pipelineThread;
};

#endif
