#include "AudioPipelineManager.h"
#include "Pipeline/Components/componentSettings.h"
#include "Pipeline/IDManager.h"
#include "Pipeline/audioBufferQueue.h"
#include "enumConversion.h"
#include <vector>


using namespace pipeline;

AudioPipelineManager::AudioPipelineManager(audioFormatInfo audioInfo, ushort keyCount): audioInfo(audioInfo), keyCount(keyCount), component(&this->audioInfo){
    running = false;
    pipelineThread = nullptr;

    statisticsService = new statistics::PipelineStatisticsService(audioInfo.sampleSize*long(1000000)/audioInfo.sampleRate, 64, audioInfo, 0);

    input.init(audioInfo, keyCount);
    output.init(audioInfo);

    outputBuffer = nullptr;
}

AudioPipelineManager::~AudioPipelineManager(){
    stop();
    delete statisticsService;
    for (uint i = 0; i < componentQueues.size(); i++){
        delete componentQueues.at(i);
    }
}

char AudioPipelineManager::start(){
    if (running){
        std::fprintf(stderr, "ERR: AudioPipelineManager::start PIPELINE ALREADY RUNNING\n");
        return -1;
    }
    if (outputBuffer == nullptr){
        std::fprintf(stderr, "ERR: AudioPipelineManager::start OUTPUT BUFFER IS NOT SET\n");
        return -2;
    }
    if (input.startAllInputs()){
        input.stopAllInputs();
        std::fprintf(stderr, "ERR: AudioPipelineManager::start COULD NOT START ALL INPUTS\n");
        return -3;
    }

    executionQueue.build(componentQueues, outputBuffer, component.components);
    char executionQueueError = executionQueue.error();
    switch (executionQueueError){
        case -1:
            std::fprintf(stderr, "ERR: AudioPipelineManager::start PIPELINE DOES NOT HAVE ANY CONNECTED SYNTH\n");
            return -4;
        case -2:
            std::fprintf(stderr, "ERR: AudioPipelineManager::start PIPELINE IS NOT VALID\n");
            const std::vector<short>& notConnectedAdvComp = executionQueue.getInvalidAdvCompIDs();
            for (uint i = 0; i < notConnectedAdvComp.size(); i++){
                std::fprintf(stderr, "ERR: AudioPipelineManager::start COMP(%d) IS NOT CONNECTED PROPERLY\n", notConnectedAdvComp.at(i));
            }
            return -4;
    }

    if (pipelineThread != nullptr){
        delete pipelineThread;
    }

    static const std::vector<audioBufferQueue*>& backwardsExecution = executionQueue.getQueue();

    for (int i = backwardsExecution.size() - 1; i >= 0; i--){
        std::printf("%s(%d)\n", IDTypeToString(backwardsExecution.at(i)->parentType).c_str(), backwardsExecution.at(i)->getParentID());
    }

    pipelineThread = new std::thread(&AudioPipelineManager::pipelineThreadFunction, this);

    return 0;
}

void AudioPipelineManager::stop(){
    if (running == false){
        return;
    }
    running = false;
    output.stopRecording();
    if (pipelineThread->joinable()){
        pipelineThread->join();
    }
    input.stopAllInputs();
}

bool AudioPipelineManager::isRuning(){
    return running;
}


const statistics::pipelineStatistics* AudioPipelineManager::getStatistics(){
    return statisticsService->getStatistics();
}

const audioFormatInfo* AudioPipelineManager::getAudioInfo(){
    return &audioInfo;
}

void AudioPipelineManager::pipelineThreadFunction(){
    running = true;
    ulong sampleTimeLength = audioInfo.sampleSize*long(1000000)/audioInfo.sampleRate;
    static const std::vector<audioBufferQueue*>& backwardsExecution = executionQueue.getQueue();

    // midiInput->buffer->swapActiveBuffer();
    input.swapActiveBuffers();

    // ulong nextLoop = midiInput->buffer->getActivationTimestamp() + sampleTimeLength;
    ulong nextLoop = input.getActivationTimestamp() + sampleTimeLength;

    statisticsService->firstInvocation();

    while (running){
        std::this_thread::sleep_until(std::chrono::time_point<std::chrono::system_clock>(std::chrono::nanoseconds((nextLoop)*1000)));
        statisticsService->loopStart();
        nextLoop += sampleTimeLength;

        input.cycleBuffers();

        input.generateSamples(executionQueue.getConnectedSynthIDs());
        for (int i = backwardsExecution.size() - 1; i >= 0; i--){
            component.applyEffects(backwardsExecution[i]);
        }

        statisticsService->loopWorkEnd();

        output.play(&outputBuffer->buffer);
    }
}

char AudioPipelineManager::recordUntilStreamEmpty(MIDI::MidiFileReader& midi, short synthID, std::string filename){
    if (running){
        return 1;
    }

    if (input.synthIDValid(synthID) == false){
        return 3;
    }

    keyboardTransferBuffer* keyboardState = new keyboardTransferBuffer(audioInfo.sampleSize, keyCount);
    pipelineAudioBuffer* pipelineBuffer = new pipelineAudioBuffer(audioInfo.sampleSize);

    midi.rewindFile();
    if (filename.empty()){
        output.startRecording();
    } else {
        output.startRecording(filename);
    }

    while (midi.isFileReady() && !midi.eofChunk(0)){
        midi.fillBuffer(keyboardState, 0);
        input.generateSampleWith(synthID, pipelineBuffer, keyboardState);
        output.onlyRecord(pipelineBuffer);
    }

    midi.fillBuffer(keyboardState, 0);
    for (uint i = 0; i <= 2*audioInfo.sampleRate/audioInfo.sampleSize; i++){
        input.generateSampleWith(synthID, pipelineBuffer, keyboardState);
        output.onlyRecord(pipelineBuffer);
    }
    output.stopRecording();

    delete keyboardState;
    delete pipelineBuffer;

    return 0;
}

bool AudioPipelineManager::IDValid(pipeline::ID_type type, short ID){
    switch (type) {
        case INPUT:
            return input.inputIDValid(ID);
        case SYNTH:
            return input.synthIDValid(ID);
        case COMP:
            return component.components.IDValid(ID);
        default:
            return false;
    }
}

void AudioPipelineManager::reorganizeIDs(){
    input.reorganizeIDs();
}

void AudioPipelineManager::emptyQueueBuffer(ID_type IDType, short ID){
    audioBufferQueue& toEmpty = *componentQueues.at(findAudioQueueBufferIndex(IDType, ID));
    for (uint i = 0; i < toEmpty.componentIDQueue.size(); i++){
        component.components.getElement(toEmpty.componentIDQueue.at(i))->includedIn = nullptr;
    }
    toEmpty.componentIDQueue.clear();
}


//OUTPUT
char AudioPipelineManager::startRecording(){
    return output.startRecording();
}

char AudioPipelineManager::startRecording(std::string filename){
    return output.startRecording(filename);
}

char AudioPipelineManager::stopRecording(){
    return output.stopRecording();
}

bool AudioPipelineManager::isRecording(){
    return output.isRecording();
}


//INPUT
char AudioPipelineManager::saveSynthConfig(std::string path, ushort ID){
    return input.saveSynthConfig(path, ID);
}

char AudioPipelineManager::loadSynthConfig(std::string path, ushort ID){
    return input.loadSynthConfig(path, ID);
}

short AudioPipelineManager::addSynthesizer(){
    audioBufferQueue* newQueue = new audioBufferQueue(pipeline::SYNTH, audioInfo.sampleSize);
    short newSynthID = input.addSynthesizer(&newQueue->buffer);
    newQueue->parentID = newSynthID;
    componentQueues.push_back(newQueue);
    return newSynthID;
}

int AudioPipelineManager::findAudioQueueBufferIndex(ID_type IDType, short ID){
    for (uint i = 0; i < componentQueues.size(); i++){//REALLY INEFFICENT, but bearable
        if (componentQueues.at(i)->parentID == ID && componentQueues.at(i)->parentType == IDType){
            return i;
        }
    }
    return -1;
}

audioBufferQueue* AudioPipelineManager::findAudioQueueBuffer(ID_type IDType, short ID){
    for (uint i = 0; i < componentQueues.size(); i++){//REALLY INEFFICENT, but bearable
        if (componentQueues.at(i)->parentID == ID && componentQueues.at(i)->parentType == IDType){
            return componentQueues.at(i);
        }
    }
    return nullptr;
}

char AudioPipelineManager::removeSynthesizer(short ID){
    int index = findAudioQueueBufferIndex(pipeline::SYNTH, ID);
    if (index < 0){
        return -1;
    }

    if (outputBuffer != nullptr && outputBuffer->parentID == ID && outputBuffer->parentType == pipeline::SYNTH){
        std::printf("WARNING: REMOVING OUTPUT BUFFER\n");
        if (running){
            stop();
            std::printf("PIPELINE STOPPED\n");
        }
        outputBuffer = nullptr;
    }
    for (uint j = 0; j < componentQueues.at(index)->componentIDQueue.size(); j++){
        component.components.getElement(componentQueues.at(index)->componentIDQueue.at(j))->includedIn = nullptr;
    }
    delete componentQueues.at(index);
    componentQueues.erase(componentQueues.begin() + index);

    return input.removeSynthesizer(ID);
}

short AudioPipelineManager::getSynthesizerCount(){
    return input.getSynthesizerCount();
}

char AudioPipelineManager::connectInputToSynth(short inputID, short synthID){
    return input.connectInputToSynth(inputID, synthID);
}

char AudioPipelineManager::disconnectSynth(short synthID){
    return input.disconnectSynth(synthID);
}

const synthesizer::settings* AudioPipelineManager::getSynthSettings(const ushort& ID){
    return input.getSynthetiserSettings(ID);
}

float AudioPipelineManager::getSynthSetting(const ushort& ID, synthesizer::settings_name settingName){
    return input.getSynthetiserSetting(ID, settingName);
}

synthesizer::generator_type AudioPipelineManager::getSynthType(const ushort& ID){
    return input.getSynthetiserType(ID);
}

void AudioPipelineManager::setSynthSetting(const ushort& ID, const synthesizer::settings_name& settingsName, const float& value){
    input.setSynthetiserSetting(ID, settingsName, value);
}

void AudioPipelineManager::setSynthSetting(const ushort& ID, const synthesizer::generator_type& type){
    input.setSynthetiserSetting(ID, type);
}

char AudioPipelineManager::printSynthInfo(short synthID){
    if (input.synthIDValid(synthID) == false){
        return -1;
    }

    std::printf("SYNTH(%d):\n", synthID);

    audioBufferQueue& queue = *findAudioQueueBuffer(pipeline::SYNTH, synthID);
    if (queue.componentIDQueue.size() > 1){
        for (uint i = 1; i < queue.componentIDQueue.size(); i++){
            short subCompID = queue.componentIDQueue.at(i);
            std::printf("   %d: %s(%d)\n", i, componentTypeToString(component.components.getElement(subCompID)->type).c_str(), subCompID);
        }
    } else {
        std::printf("   (empty queue)\n");
    }
    return 0;
}



short AudioPipelineManager::addInput(AKeyboardRecorder*& newInput){
    return input.addInput(newInput);
}

char AudioPipelineManager::removeInput(short ID){
    return input.removeInput(ID);
}

short AudioPipelineManager::getInputCount(){
    return input.getInputCount();
}


char AudioPipelineManager::pauseInput(){
    return input.stopAllInputs();
}

char AudioPipelineManager::reausumeInput(){
    return input.startAllInputs();
}

void AudioPipelineManager::clearInputBuffers(){
    input.clearBuffers();
}


//EFFECT CONTROLL

char AudioPipelineManager::setOutputBuffer(short ID, ID_type IDType){
    if (IDValid(IDType, ID) == false){
        return -1;
    }

    if (IDType != SYNTH && IDType != COMP){
       return -2;
    }

    for (uint i = 0; i < componentQueues.size(); i++){
        audioBufferQueue& queueIterator = *componentQueues.at(i);
        if (queueIterator.parentID == ID && queueIterator.parentType == IDType){
            outputBuffer = &queueIterator;
            return 0;
        }
    }

    return -3;
}

short AudioPipelineManager::addComponent(component_type type){
    return component.addComponent(type);
}

short AudioPipelineManager::addComponent(advanced_component_type type){
    audioBufferQueue* newQueue = new audioBufferQueue(pipeline::COMP, audioInfo.sampleSize);
    short newCompID = component.addComponent(type, newQueue);
    newQueue->parentID = newCompID;
    componentQueues.push_back(newQueue);
    newQueue->componentIDQueue.push_back(newCompID);
    component.advancedIDs.insert(newCompID);
    return newCompID;
}

char AudioPipelineManager::removeComponent(short ID){
    if (component.advancedIDs.find(ID) != component.advancedIDs.end()){
        return removeAdvancedComponent(ID);
    } else {
        return removeSimpleComponent(ID);
    }
}

char AudioPipelineManager::removeSimpleComponent(short ID){
    disconnectSimpleCommponent(ID);
    return component.components.remove(ID);
}

char AudioPipelineManager::removeAdvancedComponent(short ID){
    disconnectAdvancedCommponentFromAll(ID);
    component.advancedIDs.erase(ID);
    emptyQueueBuffer(pipeline::COMP, ID);
    if (outputBuffer != nullptr && outputBuffer->parentID == ID && outputBuffer->parentType == pipeline::COMP){
        std::printf("WARNING: REMOVING OUTPUT BUFFER\n");
        if (running){
            stop();
            std::printf("PIPELINE STOPPED\n");
        }
        outputBuffer = nullptr;
    }
    for (short potentialConnectionID : component.advancedIDs){//disconnecting those attached to it
        AAdvancedComponent* potentialConnection = reinterpret_cast<AAdvancedComponent*>(component.components.getElement(potentialConnectionID));
        for (uint i = 0; i < potentialConnection->maxConnections; i++){
            ID_type paretType;
            short paretID;
            potentialConnection->getConnection(i, paretType, paretID);

            if (paretID == ID && paretType == pipeline::COMP){
                potentialConnection->disconnect(i);
            }
        }
    }


    return component.components.remove(ID);
}

short AudioPipelineManager::getComponentCout(){
    return component.components.getElementCount();
}

char AudioPipelineManager::connectComponent(short componentID, ID_type parentType, short parentID){
    if (component.components.IDValid(componentID) == false){
        return -1;
    }
    if (IDValid(parentType, parentID) == false){
        return -2;
    }

    audioBufferQueue* queue = findAudioQueueBuffer(parentType, parentID);
    if (queue == nullptr){
        return -3;
    }

    AComponent& tempComponent = *component.components.getElement(componentID);
    disconnectSimpleCommponent(componentID);
    queue->componentIDQueue.push_back(componentID);
    tempComponent.includedIn = queue;

    return 0;
}

char AudioPipelineManager::setAdvancedComponentInput(short componentID, short inputIndex, ID_type IDType, short connectToID){
    AAdvancedComponent* toConnect = component.getAdvancedComponent(componentID);
    if (toConnect == nullptr){
        return -1;
    }
    audioBufferQueue* toBeConnectedTo;
    switch (IDType) {

        case INVALID:
            return -2;
        case INPUT:
            return -2;
        case SYNTH:
            if (input.synthIDValid(connectToID) == false){
                return -3;
            }
            toBeConnectedTo = findAudioQueueBuffer(pipeline::SYNTH, connectToID);
            break;
        case COMP:
            if (component.components.IDValid(connectToID) == false){
                return -3;
            }
            AAdvancedComponent* potentialAdvComponent = component.getAdvancedComponent(connectToID);
            if (potentialAdvComponent == nullptr){
                return -4;
            }
            toBeConnectedTo = potentialAdvComponent->includedIn;
            break;
    }

    if (IDValid(IDType, connectToID) == false){
        return -3;
    }

    toConnect->connect(inputIndex, toBeConnectedTo);
    return 0;
}

char AudioPipelineManager::disconnectCommponent(short componentID){
    if (component.components.IDValid(componentID) == false){
        return -1;
    }

    if (component.advancedIDs.find(componentID) != component.advancedIDs.end()){
        disconnectAdvancedCommponentFromAll(componentID);
    } else {
        disconnectSimpleCommponent(componentID);
    }

    return 0;
}

void AudioPipelineManager::disconnectSimpleCommponent(short componentID){
    AComponent& tempComponent = *component.components.getElement(componentID);
    if (tempComponent.includedIn != nullptr){
        audioBufferQueue& oldQueue = *tempComponent.includedIn;
        for (uint i = 0; i < oldQueue.componentIDQueue.size(); i++){//TODO: (i < oldQueue.componentIDQueue.size()) changed to if statement below will detect system bugs
            if (oldQueue.componentIDQueue.at(i) == componentID){
                oldQueue.componentIDQueue.erase(oldQueue.componentIDQueue.begin() + i);
                break;
            }
        }
        tempComponent.includedIn = nullptr;
    }
}

void AudioPipelineManager::disconnectAdvancedCommponentFromAll(short componentID){
    AAdvancedComponent* toBeDisconnected = component.getAdvancedComponent(componentID);
    for (uint i = 0; i < toBeDisconnected->maxConnections; i++){
        toBeDisconnected->disconnect(i);
    }
}

void AudioPipelineManager::disconnectAdvancedCommponent(short componentID, ID_type parentType, short parentID){
    AAdvancedComponent* toBeDisconnected = component.getAdvancedComponent(componentID);
    toBeDisconnected->disconnect(parentType, parentID);
}

void AudioPipelineManager::disconnectAdvancedCommponent(short componentID, uint index){
    AAdvancedComponent* toBeDisconnected = component.getAdvancedComponent(componentID);
    toBeDisconnected->disconnect(index);
}

char AudioPipelineManager::tryDisconnectAdvancedCommponent(short componentID, uint index){
    AAdvancedComponent* toBeDisconnected = component.getAdvancedComponent(componentID);
    if (toBeDisconnected == nullptr){
        return -1;
    }
    if (toBeDisconnected->maxConnections <= index){
        return -2;
    }

    toBeDisconnected->disconnect(index);
    return 0;
}


char AudioPipelineManager::getComponentConnection(short componentID, ID_type& parentType, short& parentID){
    if (component.components.IDValid(componentID) == false){
        return -1;
    }

    AComponent& tempComponent = *component.components.getElement(componentID);

    if (tempComponent.includedIn == nullptr){
        parentType = pipeline::INVALID;
        parentID = -2;
    } else {
        parentType = tempComponent.includedIn->parentType;
        parentID = tempComponent.includedIn->parentID;
    }

    return 0;
}

char AudioPipelineManager::setComponentSetting(short componentID, uint settingIndex, float value){
    if (component.components.IDValid(componentID) == false){
        return -1;
    }
    AComponent& tempComponent = *component.components.getElement(componentID);

    if (tempComponent.getSettings()->count <= settingIndex){
        return -2;
    }

    tempComponent.set(settingIndex, value);

    return 0;
}

const componentSettings* AudioPipelineManager::getComopnentSettings(short componentID){
    if (component.components.IDValid(componentID) == false){
        return nullptr;
    }

    return component.components.getElement(componentID)->getSettings();
}


char AudioPipelineManager::printAdvancedComponentInfo(short ID){
    AAdvancedComponent* toPrint = component.getAdvancedComponent(ID);
    if (toPrint == nullptr){
        return -1;
    }

    std::printf("COMP(%d):\n", ID);

    for (uint i = 0; i < toPrint->maxConnections; i++){
        if (toPrint->getConnection(i) == nullptr){
            std::printf("   in%d: not connected\n", i);
        } else {
            std::printf("   in%d: %s(%d)\n", i, IDTypeToString(toPrint->getConnection(i)->parentType).c_str(), toPrint->getConnection(i)->parentID);
        }
    }
    std::printf("\n");
    const componentSettings& settings = *toPrint->getSettings();
    if (settings.count > 0){
        for (uint i = 0; i < settings.count; i++){
            std::printf("   %s: %f\n", settings.names[i].c_str(), settings.values[i]);
        }
    } else {
        std::printf("   (no settings)\n");
    }

    std::printf("\n");
    audioBufferQueue& queue = *toPrint->includedIn;
    if (queue.componentIDQueue.size() > 1){
        for (uint i = 1; i < queue.componentIDQueue.size(); i++){
            short subCompID = queue.componentIDQueue.at(i);
            std::printf("   %d: %s(%d)\n", i, componentTypeToString(component.components.getElement(subCompID)->type).c_str(), subCompID);
        }
    } else {
        std::printf("   (empty queue)\n");
    }

    std::printf("\n");

    return 0;
}

bool AudioPipelineManager::isAdvancedComponent(short ID){
    AAdvancedComponent* toCheck = component.getAdvancedComponent(ID);
    if (toCheck == nullptr){
        return false;
    }
    return true;
}

