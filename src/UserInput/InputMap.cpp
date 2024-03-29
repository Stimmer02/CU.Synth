#include "InputMap.h"

InputMap::InputMap(ushort keyCount, ushort* map){
    this->keyCount = keyCount;
    arr = map;
}

InputMap::InputMap(std::string path){
    std::ifstream file(path);
    file >> keyCount;
    arr = new ushort[keyCount];
    for (uint i = 0; i < keyCount; i++){
        file >> arr[i];
    }
    file.close();
}

InputMap::~InputMap(){
    if (arr != nullptr){
        delete [] arr;
    }
}

ushort InputMap::map(const ushort& key)const{
    if (key > keyCount){
        return -1;
    }
    return arr[key];
}

void InputMap::replace(ushort& key)const{
    if (key > keyCount){
        key = -1;
    }
    key = arr[key];
}

ushort InputMap::getMaxValue()const{
    uint max = -1;
    for (uint i = 0; i <keyCount; i++){
        if (arr[i] > max){
            max = arr[i];
        }
    }
    return max;
}

ushort InputMap::getKeyCount()const{
    return keyCount;
}
