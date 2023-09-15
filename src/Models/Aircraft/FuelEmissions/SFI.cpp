// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "SFI.h"

#include "Base/Atmosphere.h"
#include "Base/Math.h"

namespace GRAPE {
    double SFI::departureFuelFlow(double AltitudeMsl, double TrueAirspeed, double CorrNetThrustPerEng, const Atmosphere& Atm) const noexcept {
        return CorrNetThrustPerEng * Atm.pressureRatio(AltitudeMsl) * std::sqrt(Atm.temperatureRatio(AltitudeMsl)) * (K1 + K2 * machNumber(TrueAirspeed, AltitudeMsl, Atm) + K3 * AltitudeMsl + K4 * CorrNetThrustPerEng);
    }

    double SFI::arrivalFuelFlow(double AltitudeMsl, double TrueAirspeed, double CorrNetThrustPerEng, double MaximumSeaLevelStaticThrust, const Atmosphere& Atm) const noexcept {
        return CorrNetThrustPerEng * Atm.pressureRatio(AltitudeMsl) * std::sqrt(Atm.temperatureRatio(AltitudeMsl)) * (A + B1 * machNumber(TrueAirspeed, AltitudeMsl, Atm) + B2 * std::exp(-B3 * CorrNetThrustPerEng / MaximumSeaLevelStaticThrust));
    }
}
