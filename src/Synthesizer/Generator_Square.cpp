#include "Generator_Square.h"

inline float synthesizer::Generator_Square::soundFunction(noteBuffer& noteBuffer){
    return (int((noteBuffer.phaze)/noteBuffer.multiplier) & 0x1)*2 - 1;
}
