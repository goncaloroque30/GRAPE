// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Doc29ProfileCalculator.h"

#include "Doc29Performance.h"

namespace GRAPE {
    Doc29ProfileArrivalCalculator::Doc29ProfileArrivalCalculator(const CoordinateSystem& CsIn, const Atmosphere& AtmIn, const Aircraft& AcftIn, const Runway& RwyIn, const RouteOutput& RteOutputIn, double WeightIn) : Cs(CsIn), Atm(AtmIn), Acft(AcftIn), Rwy(RwyIn), RteOutput(RteOutputIn), Weight(WeightIn) {}

    std::optional<ProfileOutput> Doc29ProfileArrivalCalculator::calculate(const Doc29ProfileArrival& Prof) {
        Prof.accept(*this);

        if (ProfOutput.empty())
            return {};

        return std::move(ProfOutput);
    }

    std::optional<ProfileOutput> Doc29ProfileDepartureCalculator::calculate(const Doc29ProfileDeparture& Prof) {
        Prof.accept(*this);

        if (ProfOutput.empty())
            return {};

        return std::move(ProfOutput);
    }

    void Doc29ProfileArrivalCalculator::visitDoc29ProfileArrivalPoints(const Doc29ProfileArrivalPoints& Profile) {
        for (const auto& [cumulativeGroundDistance, pt] : Profile)
        {
            auto& [altitudeAfe, trueAirspeed, thrust] = pt;
            const FlightPhase flPhase = cumulativeGroundDistance > 0.0 && altitudeAfe <= Rwy.elevationAt(cumulativeGroundDistance) + Constants::Precision ? FlightPhase::LandingRoll : FlightPhase::Approach;
            const double groundSpeed = trueAirspeed - Atm.headwind(RteOutput.heading(cumulativeGroundDistance));
            const double bankAngl = bankAngle(groundSpeed, RteOutput.turnRadius(cumulativeGroundDistance));
            ProfOutput.addPoint(cumulativeGroundDistance, altitudeAfe + Rwy.Elevation, trueAirspeed, groundSpeed, thrust, bankAngl, flPhase);
        }
    }

    void Doc29ProfileDepartureCalculator::visitDoc29ProfileDeparturePoints(const Doc29ProfileDeparturePoints& Profile) {
        // Detect thrust cutback (maximum of thrust difference between 500ft and 5000ft)
        std::size_t thrustCutback = 0;
        double thrustDiffMax = 0;
        for (auto it = std::next(Profile.begin()); it != Profile.end(); ++it)
        {
            const auto& [cumGroundDistPrev, ptPrev] = *std::prev(it);
            const auto& [cumGroundDist, pt] = *it;

            // Break on first segment starting above 5000.0ft
            if (ptPrev.AltitudeAfe > fromFeet(5000.0))
                break;

            // Continue if segment end below 500.0ft
            if (pt.AltitudeAfe < fromFeet(500.0))
                continue;

            const double thrustDiff = ptPrev.CorrNetThrustPerEng - pt.CorrNetThrustPerEng;
            if (thrustDiff >= thrustDiffMax)
            {
                thrustDiffMax = thrustDiff;
                thrustCutback = std::distance(Profile.begin(), std::prev(it));
            }
        }

        // Add Points
        for (auto it = Profile.begin(); it != Profile.end(); ++it)
        {
            const auto& [cumGroundDist, pt] = *it;
            const std::size_t i = std::distance(Profile.begin(), it);

            auto& [altitudeAfe, trueAirspeed, thrust] = pt;
            FlightPhase flPhase = FlightPhase::Climb;
            if (cumGroundDist < Rwy.Length && altitudeAfe <= Rwy.elevationAt(cumGroundDist) + Constants::Precision)
                flPhase = FlightPhase::TakeoffRoll;
            else if (i <= thrustCutback)
                flPhase = FlightPhase::InitialClimb;

            const double groundSpeed = flPhase == FlightPhase::TakeoffRoll ? trueAirspeed : trueAirspeed - Atm.headwind(RteOutput.heading(cumGroundDist));
            const double bankAngl = bankAngle(groundSpeed, RteOutput.turnRadius(cumGroundDist));
            const double corrThrust = flPhase != FlightPhase::Climb ? thrust * ThrustPercentageTakeoff : thrust * ThrustPercentageClimb;
            ProfOutput.addPoint(cumGroundDist, altitudeAfe + Rwy.Elevation, trueAirspeed, groundSpeed, corrThrust, bankAngl, flPhase);
        }
    }

    void Doc29ProfileArrivalCalculator::visitDoc29ProfileArrivalProcedural(const Doc29ProfileArrivalProcedural& Profile) {
        addLandingStep(Profile);
        addGroundSteps(Profile);
        addAirSteps(Profile);
        ProfOutput.recalculateBankAngle(RteOutput);
    }

    void Doc29ProfileArrivalCalculator::addLandingStep(const Doc29ProfileArrivalProcedural& Profile) {
        const auto& descendLandStep = Profile.descendLandStep();
        const double thrAltMsl = descendLandStep.ThresholdCrossingAltitudeAfe + Rwy.Elevation;
        const double thrCas = descendLandStep.Doc29AerodynamicCoefficients->D * std::sqrt(Weight * Constants::g0);
        const double thrTas = trueAirspeed(thrCas, thrAltMsl, Atm);
        const double thrGs = groundSpeed(thrTas, descendLandStep.DescentAngle, Atm.headwind(RteOutput.heading(0.0)));

        // Threshold Point (thr)
        double thrThrust = Weight * Constants::g0 / (Acft.EngineCount * Atm.pressureRatio(thrAltMsl)) * (descendLandStep.Doc29AerodynamicCoefficients->R + std::sin(toRadians(descendLandStep.DescentAngle)) / 1.03); // Land has its own thrust formula
        thrThrust = thrThrust + 1.03 * (Weight * Constants::g0 / Atm.pressureRatio(thrAltMsl)) * (std::sin(toRadians(descendLandStep.DescentAngle)) * (Atm.headwind(RteOutput.heading(0.0)) - fromKnots(8.0))) / (Acft.EngineCount * thrCas);
        ProfOutput.addPoint(0.0, thrAltMsl, thrTas, thrGs, thrThrust, Constants::NaN, FlightPhase::Approach);

        // Touchdown (td)
        const double tdDist = groundDistance(thrAltMsl, Rwy.Elevation, descendLandStep.DescentAngle);
        const double tdAltMsl = Rwy.Elevation + Rwy.Gradient * tdDist;
        const double tdTas = trueAirspeed(thrCas, tdAltMsl, Atm);
        const double tdGs = groundSpeed(tdTas, descendLandStep.DescentAngle, Atm.headwind(RteOutput.heading(tdDist)));
        const double tdThrust = Weight * Constants::g0 / (Acft.EngineCount * Atm.pressureRatio(tdAltMsl)) * (descendLandStep.Doc29AerodynamicCoefficients->R + std::sin(toRadians(descendLandStep.DescentAngle)) / 1.03); // Land has its own thrust formula
        const double tdThr = tdThrust + 1.03 * (Weight * Constants::g0 / Atm.pressureRatio(tdAltMsl)) * (std::sin(toRadians(descendLandStep.DescentAngle)) * (Atm.headwind(RteOutput.heading(tdDist)) - fromKnots(8.0))) / (Acft.EngineCount * thrCas);
        ProfOutput.addPoint(tdDist, tdAltMsl, tdTas, tdGs, tdThrust, Constants::NaN, FlightPhase::LandingRoll);
    }

    void Doc29ProfileArrivalCalculator::addGroundSteps(const Doc29ProfileArrivalProcedural& Profile) {
        GRAPE_ASSERT(!ProfOutput.empty());

        double currCumGroundDist = Profile.descendLandStep().TouchdownRoll;
        double currAltMsl = ProfOutput.rbegin()->second.AltitudeMsl;

        for (auto grIt = Profile.groundStepsBegin(); grIt != Profile.groundStepsEnd(); ++grIt)
        {
            const auto& grStep = std::get<Doc29ProfileArrivalProcedural::GroundDecelerate>(*grIt);
            const double cas = grStep.StartCalibratedAirspeed;
            const double tas = trueAirspeed(cas, currAltMsl, Atm);
            const double gs = tas;
            const double thrust = grStep.StartThrustPercentage * Acft.MaximumSeaLevelStaticThrust;
            ProfOutput.addPoint(currCumGroundDist, currAltMsl, tas, gs, thrust, Constants::NaN, FlightPhase::LandingRoll);

            // Last GroundDistance is currently ignored
            currAltMsl = currAltMsl + grStep.GroundDistance * Rwy.Gradient;
            currCumGroundDist += grStep.GroundDistance;
        }
    }

    void Doc29ProfileArrivalCalculator::addAirSteps(const Doc29ProfileArrivalProcedural& Profile) {
        const auto& [thrDist, thrPt] = *ProfOutput.begin();
        GRAPE_ASSERT(thrDist == 0.0);

        double currCumGroundDist = 0.0;
        double currAltMsl = thrPt.AltitudeMsl;
        double currCas = Profile.descendLandStep().Doc29AerodynamicCoefficients->D * std::sqrt(Weight * Constants::g0);
        double currTas = thrPt.TrueAirspeed;
        double currGs = thrPt.Groundspeed;

        for (auto it = Profile.airStepsRBegin(); it != Profile.airStepsREnd(); ++it)
        {
            const bool success = std::visit(Overload{
                [&](const Doc29ProfileArrivalProcedural::DescendDecelerate& StepDescDecel) {
                    if (StepDescDecel.StartAltitudeAfe <= currAltMsl - Rwy.Elevation)
                    {
                        Log::models()->warn(fmt::format("Calculating arrival profile '{}' for Doc29 Performance '{}'. The descend decelerate step starting at altitude AFE {:.0f} m will be ignored. Altitude has already been reached by previous step.", Profile.Name, Profile.parentDoc29Performance().Name, StepDescDecel.StartAltitudeAfe));
                        return true;
                    }
                    const double groundDist = groundDistance(currAltMsl, StepDescDecel.StartAltitudeAfe + Rwy.Elevation, StepDescDecel.DescentAngle); // negative
                    currCumGroundDist = currCumGroundDist + groundDist;

                    const double altMsl = StepDescDecel.StartAltitudeAfe + Rwy.Elevation;
                    const double tas = trueAirspeed(StepDescDecel.StartCalibratedAirspeed, altMsl, Atm);
                    const double gs = groundSpeed(tas, StepDescDecel.DescentAngle, Atm.headwind(RteOutput.heading(currCumGroundDist)));

                    const double midAltMsl = std::midpoint(currAltMsl, altMsl);
                    const double accel = acceleration(gs, currGs, StepDescDecel.DescentAngle, groundDist);
                    const double thrust = forceBalanceThrust(currAltMsl, StepDescDecel.Doc29AerodynamicCoefficients->R, StepDescDecel.DescentAngle, accel);

                    ProfOutput.addPoint(currCumGroundDist, altMsl, tas, gs, thrust, Constants::NaN, FlightPhase::Approach);

                    currAltMsl = altMsl;
                    currCas = StepDescDecel.StartCalibratedAirspeed;
                    currTas = tas;
                    currGs = gs;

                    return true;
                    },
                [&](const Doc29ProfileArrivalProcedural::DescendIdle& StepDescIdle) {

                    const double altMsl = StepDescIdle.StartAltitudeAfe + Rwy.Elevation;

                    const double groundDist = groundDistance(currAltMsl, altMsl, StepDescIdle.DescentAngle); // negative
                    currCumGroundDist = currCumGroundDist + groundDist;

                    const double tas = trueAirspeed(StepDescIdle.StartCalibratedAirspeed, altMsl, Atm);
                    const double gs = groundSpeed(tas, StepDescIdle.DescentAngle, Atm.headwind(RteOutput.heading(currCumGroundDist)));

                    const double midAltMsl = std::midpoint(currAltMsl, altMsl);
                    const double thrust = Profile.parentDoc29Performance().thrust()(Doc29Thrust::Rating::Idle, StepDescIdle.StartCalibratedAirspeed, midAltMsl, Acft.EngineBreakpointTemperature, Atm);

                    ProfOutput.addPoint(currCumGroundDist, altMsl, tas, gs, thrust, Constants::NaN, FlightPhase::Approach);

                    currAltMsl = altMsl;
                    currCas = StepDescIdle.StartCalibratedAirspeed;
                    currTas = tas;
                    currGs = gs;

                    return true;
                    },
                [&](const Doc29ProfileArrivalProcedural::Level& StepLevel) {
                    currCumGroundDist = currCumGroundDist + StepLevel.GroundDistance;
                    const double thrust = Weight * Constants::g0 * StepLevel.Doc29AerodynamicCoefficients->R / (Acft.EngineCount * Atm.pressureRatio(currAltMsl));
                    ProfOutput.addPoint(currCumGroundDist, currAltMsl, currTas, currGs, thrust, Constants::NaN, FlightPhase::Approach);
                        return true;
                    },
                [&](const Doc29ProfileArrivalProcedural::LevelDecelerate& StepLevelDecel) {
                    currCumGroundDist = currCumGroundDist + StepLevelDecel.GroundDistance;

                    const double tas = trueAirspeed(StepLevelDecel.StartCalibratedAirspeed, currAltMsl, Atm);
                    const double gs = groundSpeed(tas, 0.0, Atm.headwind(RteOutput.heading(currCumGroundDist)));

                    const double accel = acceleration(gs, currGs, 0.0, StepLevelDecel.GroundDistance);
                    const double thrust = forceBalanceThrust(currAltMsl, StepLevelDecel.Doc29AerodynamicCoefficients->R, 0.0, accel);
                    ProfOutput.addPoint(currCumGroundDist, currAltMsl, tas, gs, thrust, Constants::NaN, FlightPhase::Approach);

                    currCas = StepLevelDecel.StartCalibratedAirspeed;
                    currTas = tas;
                    currGs = gs;

                    return true;
                    },
                [&](const Doc29ProfileArrivalProcedural::LevelIdle& StepLevelIdle) {
                    currCumGroundDist = currCumGroundDist + StepLevelIdle.GroundDistance;

                    const double tas = trueAirspeed(StepLevelIdle.StartCalibratedAirspeed, currAltMsl, Atm);
                    const double gs = groundSpeed(tas, 0.0, Atm.headwind(RteOutput.heading(currCumGroundDist)));

                    const double midCas = std::midpoint(currCas, StepLevelIdle.StartCalibratedAirspeed);
                    const double thrust = Profile.parentDoc29Performance().thrust()(Doc29Thrust::Rating::Idle, midCas, currAltMsl, Acft.EngineBreakpointTemperature, Atm);

                    ProfOutput.addPoint(currCumGroundDist, currAltMsl, tas, gs, thrust, Constants::NaN, FlightPhase::Approach);

                    currCas = StepLevelIdle.StartCalibratedAirspeed;
                    currTas = tas;
                    currGs = gs;

                    return true;
                    },
                [&](const auto&) {
                        GRAPE_ASSERT(false);
                        return false;
                    },
                }, *it);

        }
    }

    double Doc29ProfileArrivalCalculator::forceBalanceThrust(double AltitudeMsl, double R, double Angle, double Acceleration) const { return Weight * Constants::g0 / (Acft.EngineCount * Atm.pressureRatio(AltitudeMsl)) * (R * std::cos(toRadians(Angle)) + std::sin(toRadians(Angle)) + Acceleration / Constants::g0); }

    double Doc29ProfileArrivalCalculator::acceleration(double V1, double V2, double Angle, double GroundDistance) const { return (std::pow(V2 / std::cos(toRadians(Angle)), 2.0) - std::pow(V1 / std::cos(toRadians(Angle)), 2.0)) / (2.0 * GroundDistance / std::cos(toRadians(Angle))); }

    Doc29ProfileDepartureCalculator::Doc29ProfileDepartureCalculator(const CoordinateSystem& CsIn, const Atmosphere& AtmIn, const Aircraft& AcftIn, const Runway& RwyIn, const RouteOutput& RteOutputIn, double WeightIn, double ThrustPercentageTakeoffIn, double ThrustPercentageClimbIn) : Cs(CsIn), Atm(AtmIn), Acft(AcftIn), Rwy(RwyIn), RteOutput(RteOutputIn), Weight(WeightIn), ThrustPercentageTakeoff(ThrustPercentageTakeoffIn), ThrustPercentageClimb(ThrustPercentageClimbIn) {}

    void Doc29ProfileDepartureCalculator::visitDoc29ProfileDepartureProcedural(const Doc29ProfileDepartureProcedural& Profile) {
        double currCumGroundDist = Constants::NaN, currAltMsl = Constants::NaN, currCas = Constants::NaN, currTas = Constants::NaN, currGs = Constants::NaN, currThrust = Constants::NaN, currBankAngle = Constants::NaN;

        double currThrustPercentage = ThrustPercentageTakeoff;
        auto currThrustRating = Doc29Thrust::Rating::MaximumTakeoff;
        auto currFlPhase = FlightPhase::InitialClimb;

        for (auto it = Profile.steps().begin(); it != Profile.steps().end(); ++it)
        {
            const std::size_t i = it - Profile.steps().begin();
            bool thrustCutback = i == Profile.thrustCutback();
            if (thrustCutback)
            {
                currThrustPercentage = ThrustPercentageClimb;
                currThrustRating = Doc29Thrust::Rating::MaximumClimb;
                currFlPhase = FlightPhase::Climb;
            }

            const Doc29ProfileDepartureProcedural::Step& stp = *it;
            const bool success = std::visit(Overload{
                [&](const Doc29ProfileDepartureProcedural::Takeoff& TakeoffStep) {
                    // Initial Point
                    currCumGroundDist = 0.0;
                    currAltMsl = Rwy.Elevation;
                    currCas = TakeoffStep.InitialCalibratedAirspeed;
                    currTas = trueAirspeed(currCas, currAltMsl, Atm);
                    currGs = currTas; // On ground Groundspeed = TAS
                    currThrust = currThrustPercentage * Profile.parentDoc29Performance().thrust()(currThrustRating, currCas, currAltMsl, Acft.EngineBreakpointTemperature, Atm);
                    currBankAngle = 0.0;
                    ProfOutput.addPoint(currCumGroundDist, currAltMsl, currTas, currGs, currThrust, currBankAngle, FlightPhase::TakeoffRoll);

                    // Takeoff Point
                    currCas = TakeoffStep.Doc29AerodynamicCoefficients->C * std::sqrt(Weight * Constants::g0);
                    currTas = trueAirspeed(currCas, currAltMsl, Atm);
                    currThrust = currThrustPercentage * Profile.parentDoc29Performance().thrust()(currThrustRating, currCas, currAltMsl, Acft.EngineBreakpointTemperature, Atm);
                    currCumGroundDist = TakeoffStep.Doc29AerodynamicCoefficients->B * Atm.temperatureRatio(currAltMsl) * std::pow(Weight * Constants::g0 / Atm.pressureRatio(currAltMsl), 2.0) / (Acft.EngineCount * currThrust);
                    currCumGroundDist = currCumGroundDist * (std::pow(currCas - Atm.headwind(RteOutput.heading(currCumGroundDist)), 2.0) / std::pow(currCas - fromKnots(8.0), 2.0));
                    const double accel = std::pow(currTas, 2.0) / (2.0 * currCumGroundDist);
                    currCumGroundDist = currCumGroundDist * accel / (accel - Constants::g0 * Rwy.Gradient);
                    currGs = currTas; // On ground Groundspeed = TAS
                    currAltMsl = Rwy.Elevation + Rwy.Gradient * currCumGroundDist;
                    ProfOutput.addPoint(currCumGroundDist, currAltMsl, currTas, currGs, currThrust, currBankAngle, FlightPhase::TakeoffRoll);

                    return true;
                },
                [&](const Doc29ProfileDepartureProcedural::Climb& ClimbStep) {
                    // End Point
                    const double endAltMsl = ClimbStep.EndAltitudeAfe + Rwy.Elevation;
                    if (endAltMsl < currAltMsl)
                        return true; // Altitude already reached by previous step
                    const double endThrust = currThrustPercentage * Profile.parentDoc29Performance().thrust()(currThrustRating, currCas, endAltMsl, Acft.EngineBreakpointTemperature, Atm);

                    // Mid Point
                    const double midAltMsl = std::midpoint(currAltMsl, endAltMsl);
                    const double midThrust = std::midpoint(currThrust, endThrust);
                    const double midWeightForce = Constants::g0 * Weight / Atm.pressureRatio(midAltMsl);

                    // Climb Angle
                    const double k = currCas <= fromKnots(200.0) ? 1.01 : 0.95;
                    const double windCorr = (currCas - fromKnots(8.0)) / (currCas - Atm.headwind(RteOutput.heading(currCumGroundDist))); // Headwind correction taken for headwind at the beginning of the climb
                    const double climbAngle = windCorr * fromRadians(std::asin(k * (Acft.EngineCount * midThrust / midWeightForce - ClimbStep.Doc29AerodynamicCoefficients->R / std::cos(toRadians(currBankAngle)))));

                    double groundDist = groundDistance(currAltMsl, endAltMsl, climbAngle);
                    const double endCumGroundDist = currCumGroundDist + groundDist;

                    double turnRadChangeCumGroundDist = RteOutput.turnRadiusChange(currCumGroundDist, endCumGroundDist);
                    while (!std::isnan(turnRadChangeCumGroundDist))
                    {
                        const double turnRadChangeGroundDist = turnRadChangeCumGroundDist - currCumGroundDist;
                        const double iFactor = (turnRadChangeCumGroundDist - currCumGroundDist) / groundDist;
                        currCumGroundDist = turnRadChangeCumGroundDist;
                        currAltMsl = distanceInterpolation(currAltMsl, endAltMsl, iFactor);
                        currTas = trueAirspeed(currCas, currAltMsl, Atm);
                        currGs = groundSpeed(currTas, climbAngle, Atm.headwind(RteOutput.heading(currCumGroundDist)));
                        currThrust = timeInterpolation(currThrust, endThrust, iFactor);
                        currBankAngle = bankAngle(currGs, RteOutput.turnRadius(currCumGroundDist));

                        if (thrustCutback)
                        {
                            thrustCutback = false;
                            const double cutbackGroundDist = turnRadChangeGroundDist < fromFeet(2000.0) ? turnRadChangeGroundDist / 2.0 : fromFeet(1000.0);
                            const double cutbackCumGroundDist = currCumGroundDist + cutbackGroundDist;
                            const double cutbackAltMsl = currAltMsl + cutbackGroundDist * std::tan(toRadians(climbAngle));
                            const double cutbackTas = trueAirspeed(currCas, cutbackAltMsl, Atm);
                            const double cutbackGs = cutbackTas * std::cos(toRadians(climbAngle)) - Atm.headwind(RteOutput.heading(cutbackCumGroundDist));
                            const double cutbackThrust = currThrustPercentage * Profile.parentDoc29Performance().thrust()(currThrustRating, currCas, cutbackAltMsl, Acft.EngineBreakpointTemperature, Atm);
                            const double cutbackBankAngle = bankAngle(cutbackGs, RteOutput.turnRadius(cutbackCumGroundDist));
                            ProfOutput.addPoint(cutbackCumGroundDist, cutbackAltMsl, cutbackTas, cutbackGs, cutbackThrust, cutbackBankAngle, currFlPhase);
                        }

                        ProfOutput.addPoint(currCumGroundDist, currAltMsl, currTas, currGs, currThrust, currBankAngle, currFlPhase);
                        groundDist = groundDistance(currAltMsl, endAltMsl, climbAngle);
                        turnRadChangeCumGroundDist = RteOutput.turnRadiusChange(currCumGroundDist + 1.0, endCumGroundDist); // + 1.0 ensures that returned CumGroundDist at which turn radius changes is different than the previous one (infinite loop)
                    }

                    groundDist = endCumGroundDist - currCumGroundDist;
                    if (thrustCutback)
                    {
                        const double cutbackGroundDist = groundDist < fromFeet(2000.0) ? groundDist / 2.0 : fromFeet(1000.0);
                        const double cutbackCumGroundDist = currCumGroundDist + cutbackGroundDist;
                        const double cutbackAltMsl = currAltMsl + cutbackGroundDist * std::tan(toRadians(climbAngle));
                        const double cutbackTas = trueAirspeed(currCas, cutbackAltMsl, Atm);
                        const double cutbackGs = cutbackTas * std::cos(toRadians(climbAngle)) - Atm.headwind(RteOutput.heading(cutbackCumGroundDist));
                        const double cutbackThrust = currThrustPercentage * Profile.parentDoc29Performance().thrust()(currThrustRating, currCas, cutbackAltMsl, Acft.EngineBreakpointTemperature, Atm);
                        const double cutbackBankAngle = bankAngle(cutbackGs, RteOutput.turnRadius(cutbackCumGroundDist));
                        ProfOutput.addPoint(cutbackCumGroundDist, cutbackAltMsl, cutbackTas, cutbackGs, cutbackThrust, cutbackBankAngle, currFlPhase);
                    }

                    currCumGroundDist = endCumGroundDist;
                    currAltMsl = endAltMsl;
                    currTas = trueAirspeed(currCas, endAltMsl, Atm);
                    currGs = groundSpeed(currTas, climbAngle, Atm.headwind(RteOutput.heading(currCumGroundDist)));
                    currBankAngle = bankAngle(currGs, RteOutput.turnRadius(currCumGroundDist));
                    currThrust = endThrust;
                    ProfOutput.addPoint(currCumGroundDist, currAltMsl, currTas, currGs, currThrust, currBankAngle, currFlPhase);

                    return true;
                },
                [&]<typename ClimbAccelStep>(const ClimbAccelStep & ClimbAccelerateParamStep) {
                    if (ClimbAccelerateParamStep.EndCalibratedAirspeed < currCas)
                        return true;

                    double groundDist;
                    double climbGrad;
                    double endAltMsl = currAltMsl + fromFeet(250.0);
                    double endThrust;

                    double estimatedEndAltMsl = endAltMsl;

                    const double headwind = Atm.headwind(RteOutput.heading(currCumGroundDist)); // Headwind constant for the end altitude estimation.
                    do
                    {
                        // EndPoint
                        endAltMsl = estimatedEndAltMsl;
                        const double endTas = trueAirspeed(ClimbAccelerateParamStep.EndCalibratedAirspeed, endAltMsl, Atm);
                        endThrust = currThrustPercentage * Profile.parentDoc29Performance().thrust()(currThrustRating, ClimbAccelerateParamStep.EndCalibratedAirspeed, endAltMsl, Acft.EngineBreakpointTemperature, Atm);

                        // Midpoint
                        const double midAltMsl = std::midpoint(currAltMsl, endAltMsl);
                        double midTas = std::midpoint(currTas, endTas);
                        const double midThrust = std::midpoint(currThrust, endThrust);
                        const double midWeightForce = Constants::g0 * Weight / Atm.pressureRatio(midAltMsl);

                        double accelFact = Acft.EngineCount * midThrust / midWeightForce - ClimbAccelerateParamStep.Doc29AerodynamicCoefficients->R * std::cos(toRadians(currBankAngle));

                        if (std::is_same_v<ClimbAccelStep, const Doc29ProfileDepartureProcedural::ClimbAcceleratePercentage&>)
                            climbGrad = accelFact * (1.0 - ClimbAccelerateParamStep.ClimbParameter); // Climb parameter is percentage of thrust
                        else
                            climbGrad = ClimbAccelerateParamStep.ClimbParameter / midTas; // Climb parameter is climb rate

                        if (accelFact - climbGrad <= 0.01)
                        {
                            Log::models()->error("Calculating departure profile '{}' for Doc29 Performance '{}'. The calibrated airspeed {:.2f} m/s can't be reached.", Profile.Name, Profile.parentDoc29Performance().Name, ClimbAccelerateParamStep.EndCalibratedAirspeed);
                            return false;
                        }
                        if (accelFact - climbGrad <= 0.02)
                            climbGrad = accelFact - 0.02;

                        const double windCorr = (endTas - headwind) / (endTas - fromKnots(8.0));
                        groundDist = windCorr * 0.95 * (std::pow(endTas, 2) - std::pow(currTas, 2)) / (2.0 * Constants::g0 * (accelFact - climbGrad));

                        estimatedEndAltMsl = currAltMsl + groundDist * climbGrad / 0.95;
                    }
                    while (std::abs(estimatedEndAltMsl - endAltMsl) > fromFeet(1.0));

                    const double endCumGroundDist = currCumGroundDist + groundDist;
                    const double climbAngle = std::atan(climbGrad);

                    double turnRadChangeCumGroundDist = RteOutput.turnRadiusChange(currCumGroundDist, endCumGroundDist);
                    while (!std::isnan(turnRadChangeCumGroundDist))
                    {
                        const double turnRadChangeGroundDist = turnRadChangeCumGroundDist - currCumGroundDist;
                        double iFactor = (turnRadChangeCumGroundDist - currCumGroundDist) / groundDist;
                        currCumGroundDist = turnRadChangeCumGroundDist;
                        currAltMsl = distanceInterpolation(currAltMsl, endAltMsl, iFactor);
                        currCas = timeInterpolation(currCas, ClimbAccelerateParamStep.EndCalibratedAirspeed, iFactor);
                        currTas = trueAirspeed(currCas, currAltMsl, Atm);
                        currGs = groundSpeed(currTas, climbAngle, Atm.headwind(RteOutput.heading(currCumGroundDist)));
                        currThrust = timeInterpolation(currThrust, endThrust, iFactor);
                        currBankAngle = bankAngle(currGs, RteOutput.turnRadius(currCumGroundDist));
                        if (thrustCutback)
                        {
                            thrustCutback = false;
                            const double cutbackGroundDist = turnRadChangeGroundDist < fromFeet(2000.0) ? turnRadChangeGroundDist / 2.0 : fromFeet(1000.0);
                            const double cutbackCumGroundDist = currCumGroundDist + cutbackGroundDist;
                            const double cutbackAltMsl = currAltMsl + cutbackGroundDist * climbGrad;
                            const double cutbackCas = timeInterpolation(currCas, ClimbAccelerateParamStep.EndCalibratedAirspeed, cutbackGroundDist / groundDist);
                            const double cutbackTas = trueAirspeed(cutbackCas, cutbackAltMsl, Atm);
                            const double cutbackGs = groundSpeed(cutbackTas, climbAngle, Atm.headwind(RteOutput.heading(currCumGroundDist)));
                            const double cutbackThrust = currThrustPercentage * Profile.parentDoc29Performance().thrust()(currThrustRating, currCas, cutbackAltMsl, Acft.EngineBreakpointTemperature, Atm);
                            const double cutbackBankAngle = bankAngle(currGs, RteOutput.turnRadius(cutbackCumGroundDist));
                            ProfOutput.addPoint(cutbackCumGroundDist, cutbackAltMsl, cutbackTas, cutbackGs, cutbackThrust, cutbackBankAngle, currFlPhase);
                        }

                        ProfOutput.addPoint(currCumGroundDist, currAltMsl, currTas, currGs, currThrust, currBankAngle, currFlPhase);
                        groundDist = groundDistance(currAltMsl, endAltMsl, std::atan(climbGrad));
                        turnRadChangeCumGroundDist = RteOutput.turnRadiusChange(currCumGroundDist + 1.0, endCumGroundDist); // + 1.0 ensures that returned CumGroundDist at which turn radius changes is different than the previous one (infinite loop)
                    }

                    if (thrustCutback)
                    {
                        const double cutbackGroundDist = groundDist < fromFeet(2000.0) ? groundDist / 2.0 : fromFeet(1000.0);
                        const double cutbackCumGroundDist = currCumGroundDist + cutbackGroundDist;
                        const double cutbackAltMsl = currAltMsl + cutbackGroundDist * climbGrad;
                        const double cutbackCas = timeInterpolation(currCas, ClimbAccelerateParamStep.EndCalibratedAirspeed, cutbackGroundDist / groundDist);
                        const double cutbackTas = trueAirspeed(cutbackCas, cutbackAltMsl, Atm);
                        const double cutbackGs = groundSpeed(cutbackTas, climbAngle, Atm.headwind(RteOutput.heading(currCumGroundDist)));
                        const double cutbackThrust = currThrustPercentage * Profile.parentDoc29Performance().thrust()(currThrustRating, currCas, cutbackAltMsl, Acft.EngineBreakpointTemperature, Atm);
                        const double cutbackBankAngle = bankAngle(currGs, RteOutput.turnRadius(cutbackCumGroundDist));
                        ProfOutput.addPoint(cutbackCumGroundDist, cutbackAltMsl, cutbackTas, cutbackGs, cutbackThrust, cutbackBankAngle, currFlPhase);
                    }

                    currCumGroundDist = endCumGroundDist;
                    currAltMsl = endAltMsl;
                    currCas = ClimbAccelerateParamStep.EndCalibratedAirspeed;
                    currTas = trueAirspeed(currCas, endAltMsl, Atm);
                    currGs = groundSpeed(currTas, climbAngle, Atm.headwind(RteOutput.heading(currCumGroundDist)));
                    currThrust = endThrust;
                    currBankAngle = bankAngle(currGs, RteOutput.turnRadius(currCumGroundDist));
                    ProfOutput.addPoint(currCumGroundDist, currAltMsl, currTas, currGs, currThrust, currBankAngle, currFlPhase);

                    return true;
                },
                }, stp);

            if (!success)
            {
                ProfOutput.clear();
                return;
            }
        }
    }
}
