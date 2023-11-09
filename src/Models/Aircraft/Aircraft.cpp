// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Aircraft.h"

#include "Doc29/Doc29Aircraft.h"
#include "Doc29/Doc29Noise.h"
#include "FuelEmissions/LTO.h"
#include "FuelEmissions/SFI.h"

namespace GRAPE {
    Aircraft::Aircraft(std::string_view NameIn) noexcept : Name(NameIn) {}

    Aircraft::Aircraft(std::string_view NameIn, const Doc29Aircraft* Doc29AcftIn, const SFI* SFIIn, const LTOEngine* LTOEngineIn, const Doc29Noise* Doc29NsIn) noexcept : Name(NameIn), Doc29Acft(Doc29AcftIn), SFIFuel(SFIIn), LTOEng(LTOEngineIn), Doc29Ns(Doc29NsIn) {}

    void Aircraft::setEngineCountE(int EngineCountIn) {
        if (!(EngineCountIn > 0 && EngineCount <= 4))
            throw GrapeException("Engine count must be between 1 and 4.");
        EngineCount = EngineCountIn;
    }
}
