#ifndef IGENERATOR_SINE_H
#define IGENERATOR_SINE_H

#include "AGenerator.h"

namespace synthesizer{
    class Generator_Sine : public AGenerator{
    public:
        float soundFunction(noteBuffer& noteBuffer) override;
    };
}

#endif
