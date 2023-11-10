// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Units.h"

namespace GRAPE {
    struct Settings {
        // Units
        Unit<Units::Distance> DistanceUnits;
        Unit<Units::Distance> AltitudeUnits;

        Unit<Units::Speed> SpeedUnits;
        Unit<Units::Speed> VerticalSpeedUnits;

        Unit<Units::Weight> WeightUnits;
        Unit<Units::Force> ThrustUnits;

        Unit<Units::Temperature> TemperatureUnits;
        Unit<Units::Pressure> PressureUnits;

        Unit<Units::Power> PowerUnits;

        Unit<Units::WeightPerTime> FuelFlowUnits;
        Unit<Units::WeightPerWeight> EmissionIndexUnits;

        Unit<Units::WeightSmall> EmissionsWeightUnits;

        void initDefineHandler();

        // Units Non Persistent (Constant)
        Unit<Units::DistancePerForce> Doc29AeroBUnits;
        Unit<Units::SpeedPerForceSqrt> Doc29AeroCDUnits;

        Unit<Units::ForcePerSpeed> Doc29ThrustFUnits;
        Unit<Units::ForcePerDistance> Doc29ThrustGaUnits;
        Unit<Units::ForcePerDistance2> Doc29ThrustGbUnits;
        Unit<Units::ForcePerTemperature> Doc29ThrustHUnits;
    };
}
