#include "KeyboardRecorder_DevInput.h"


KeyboardRecorder_DevInput::KeyboardRecorder_DevInput(const ushort& keyCount, InputMap*& keyboardMap) : keyCount(keyCount), keyboardMap(keyboardMap){
    keyboardMap = nullptr;
    buffer = nullptr;
    inputStream = nullptr;
    scannerThread = nullptr;
    running = false;
    path = "";
    sampleRate = 0;
    sampleSize = 0;
}

KeyboardRecorder_DevInput::~KeyboardRecorder_DevInput(){
    if (running){
        stop();
    } else {
        if (scannerThread != nullptr){
            delete scannerThread;
        }
    }
    if (inputStream != nullptr){
        inputStream->close();
        delete inputStream;
    }

    if (buffer != nullptr){
        delete buffer;
    }
    delete keyboardMap;
}

char KeyboardRecorder_DevInput::init(const std::string path, const uint& sampleSize, const uint& sampleRate){
    if (running){
        std::fprintf(stderr, "ERR: KeyboardRecorder_DevInput::init CANNOT INITAILIZE WHILE READIONG THREAD IS RUNNING\n");
        return -1;
    }

    if (inputStream != nullptr){
        delete inputStream;
    }
    inputStream = new std::fstream(path, std::fstream::in|std::ios::binary);
    this->sampleSize = sampleSize;
    this->sampleRate = sampleRate;

    if (buffer != nullptr){
        delete buffer;
    }
    buffer = new KeyboardDoubleBuffer(sampleSize, keyCount);

    if (!inputStream->is_open()){
        delete inputStream;
        inputStream = nullptr;
        std::fprintf(stderr, "ERR: KeyboardRecorder_DevInput::init FILE \"%s\" DOES NOT EXIST OR USER DOES NOT HAVE PERMISSIONS TO READ IT\n", path.c_str());
        return -2;
    }
    if (inputStream->bad()){
        delete inputStream;
        inputStream = nullptr;
        std::fprintf(stderr, "ERR: KeyboardRecorder_DevInput::init BAD BIT IS SET AFTER OPPENING FILE \"%s\"\n", path.c_str());
        return -3;
    }
    this->path = path;
    return 0;
}

char KeyboardRecorder_DevInput::reInit(const uint& sampleSize, const uint& sampleRate){
    if (running){
        std::fprintf(stderr, "ERR: KeyboardRecorder_DevInput::reInit CANNOT INITAILIZE WHILE READIONG THREAD IS RUNNING\n");
        return -1;
    }

    if (inputStream == nullptr){
        std::fprintf(stderr, "ERR: KeyboardRecorder_DevInput::reInit CANNOT RE-INITAILIZE WITHOUT INITIALIZATION\n");
        return -2;
    }

    this->sampleSize = sampleSize;
    this->sampleRate = sampleRate;

    delete buffer;
    buffer = new KeyboardDoubleBuffer(sampleSize, keyCount);
    return 0;
}

char KeyboardRecorder_DevInput::start(){
    if (running){
        std::fprintf(stderr, "ERR: KeyboardRecorder_DevInput::start READING THREAD WAS ALREADY RUNNING\n");
        return -1;
    }
    if (inputStream == nullptr){
        std::fprintf(stderr, "ERR: KeyboardRecorder_DevInput::start CAN NOT START WITHOUT PROPER INITIALIZATION\n");
        return -2;
    }
    if (scannerThread != nullptr){
        delete scannerThread;
    }
    if (inputStream->is_open() == false){
        // inputStream->open(path);
        if (inputStream->bad()){
            delete inputStream;
            inputStream = nullptr;
            std::fprintf(stderr, "ERR: KeyboardRecorder_DevInput::start BAD BIT IS SET AFTER OPPENING FILE \"%s\"\n", path.c_str());
            return -3;
        }
    }
    scannerThread = new std::thread(&KeyboardRecorder_DevInput::scannerThreadFunction, this);
    return 0;
}

char KeyboardRecorder_DevInput::stop(){
    if (running == false){
        std::fprintf(stderr, "ERR: KeyboardRecorder_DevInput::stop READING THREAD WAS NOT RUNNIG\n");
        return -1;
    }
    running = false;
    if (scannerThread->joinable()){
        scannerThread->join();
    }
    // inputStream->close();
    delete scannerThread;
    scannerThread = nullptr;
    return 0;
}

bool KeyboardRecorder_DevInput::isRunning(){
    return running;
}


void KeyboardRecorder_DevInput::scannerThreadFunction(){
    running = true;
    static const uchar eventValueMap[3] = {255, 127, 127};
    input_event event;
    ulong samplePosition;
    ulong sampleLength = 1000000/sampleRate;
    buffer->clearInactiveBuffer();
    buffer->swapActiveBuffer();
    buffer->clearInactiveBuffer();
    while (running){
        inputStream->read(reinterpret_cast<char*>(&event), sizeof(input_event));
        if (event.type == EV_KEY){
            samplePosition = floor((double(event.time.tv_sec*1000000+event.time.tv_usec) - buffer->getActivationTimestamp()) / sampleLength);
            ushort mappedKey = keyboardMap->map(event.code);
            if (samplePosition < sampleSize){
                if (mappedKey < keyCount){
                    buffer->getActiveBuffer()[mappedKey][samplePosition] = eventValueMap[event.value];
                } else {
                    // std::printf("UNMAPPED KEY:%d\n", event.code);
                }
            } else {
                if (mappedKey < keyCount){
                    buffer->getActiveBuffer()[mappedKey][sampleSize-1] = eventValueMap[event.value];
                    // std::printf("WARNING: KeyboardRecorder_DevInput::scannerThreadFunction BUFFER WAS NOT SWAPPED FAST ENOUGH\n");
                }
            }
        }
    }
}
