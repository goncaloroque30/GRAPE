// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Doc29Profile.h"

#include "Doc29Aircraft.h"

#include "Base/Conversions.h"

namespace GRAPE {
    // Arrival Points
    Doc29ProfileArrivalPoints::Doc29ProfileArrivalPoints(Doc29Aircraft& Doc29PerformanceIn, std::string_view NameIn) : Doc29ProfileArrival(Doc29PerformanceIn, NameIn) {}

    void Doc29ProfileArrivalPoints::addPoint() noexcept {
        if (empty())
        {
            addPoint(0.0, fromFeet(50.0), fromKnots(100.0), fromPoundsOfForce(10000.0));
            return;
        }

        auto& [cumDist, firstPt] = *m_Points.begin(); // Cumulative ground distance is negative up to the threshold
        double newCumDist = cumDist - 1.0;
        m_Points.insert({ newCumDist, firstPt });
    }

    void Doc29ProfileArrivalPoints::addPoint(double CumulativeGroundDistance, double AltitudeAfe, double TrueAirspeed, double CorrNetThrustPerEng) noexcept {
        GRAPE_ASSERT(TrueAirspeed >= 0.0);
        GRAPE_ASSERT(CorrNetThrustPerEng > 0.0);

        m_Points.try_emplace(CumulativeGroundDistance, AltitudeAfe, TrueAirspeed, CorrNetThrustPerEng);
    }

    void Doc29ProfileArrivalPoints::updateCumulativeGroundDistance(std::size_t Index, double NewCumulativeGroundDistance) noexcept {
        GRAPE_ASSERT(Index < size());

        auto node = m_Points.extract(std::next(m_Points.begin(), Index));
        node.key() = NewCumulativeGroundDistance;
        m_Points.insert(std::move(node));
    }

    void Doc29ProfileArrivalPoints::insertPoint(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index <= size());

        if (empty())
        {
            addPoint();
            return;
        }

        if (Index == size()) // Inserting after the last point
        {
            auto& [currCumGroundDistance, currPt] = *m_Points.rbegin();
            addPoint(currCumGroundDistance + 100.0, currPt.AltitudeAfe, currPt.TrueAirspeed, currPt.CorrNetThrustPerEng);
            return;
        }

        if (Index == 0) // Inserting before the first point
        {
            auto& [currCumGroundDistance, currPt] = *m_Points.begin();
            addPoint(currCumGroundDistance - 100.0, currPt.AltitudeAfe, currPt.TrueAirspeed, currPt.CorrNetThrustPerEng);
            return;
        }

        // At least two points in the profile and index is neither 0 nor size
        // There is a point at index and a point at index - 1
        auto& [prevCumGroundDistance, prevPt] = *std::next(m_Points.begin(), Index - 1);
        auto& [nextCumGroundDistance, nextPt] = *std::next(m_Points.begin(), Index);

        const double newCumGroundDist = std::midpoint(nextCumGroundDistance, prevCumGroundDistance);
        const double newAltitudeAfe = std::midpoint(prevPt.AltitudeAfe, nextPt.AltitudeAfe);
        const double newTrueAirspeed = std::midpoint(prevPt.TrueAirspeed, nextPt.TrueAirspeed);
        const double newThrust = std::midpoint(prevPt.CorrNetThrustPerEng, nextPt.CorrNetThrustPerEng);

        // Case: previous and next cumulative ground distances are close to one another
        if (std::abs(nextCumGroundDistance - prevCumGroundDistance) < 10.0) // 10 meters defined as 'close'
            insertPoint(--Index);                                           // Insert before
        // Case: previous and next cumulative ground distances are not close to one another
        else
            addPoint(newCumGroundDist, newAltitudeAfe, newTrueAirspeed, newThrust);
    }

    void Doc29ProfileArrivalPoints::deletePoint(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index < size());

        m_Points.erase(std::next(m_Points.begin(), Index));
    }

    void Doc29ProfileArrivalPoints::deletePoint() noexcept {
        if (empty())
            return;

        deletePoint(size() - 1);
    }

    void Doc29ProfileArrivalPoints::clear() noexcept {
        m_Points.clear();
    }

    void Doc29ProfileArrivalPoints::addPointE(double CumulativeGroundDistance, double AltitudeAfe, double TrueAirspeed, double CorrNetThrustPerEng) {
        if (!(TrueAirspeed >= 0.0))
            throw GrapeException("True airspeed must be at least 0 m/s.");
        if (!(CorrNetThrustPerEng > 0.0))
            throw GrapeException("Thrust must be higher than 0 N.");

        addPoint(CumulativeGroundDistance, AltitudeAfe, TrueAirspeed, CorrNetThrustPerEng);
    }

    void Doc29ProfileArrivalPoints::accept(Doc29ProfileVisitor& Vis) { Vis.visitDoc29ProfileArrivalPoints(*this); }

    void Doc29ProfileArrivalPoints::accept(Doc29ProfileVisitor& Vis) const { Vis.visitDoc29ProfileArrivalPoints(*this); }

    void Doc29ProfileArrivalPoints::accept(Doc29ProfileArrivalVisitor& Vis) { Vis.visitDoc29ProfileArrivalPoints(*this); }
    void Doc29ProfileArrivalPoints::accept(Doc29ProfileArrivalVisitor& Vis) const { Vis.visitDoc29ProfileArrivalPoints(*this); }

    // Departure Points
    Doc29ProfileDeparturePoints::Doc29ProfileDeparturePoints(Doc29Aircraft& Doc29PerformanceIn, std::string_view NameIn) : Doc29ProfileDeparture(Doc29PerformanceIn, NameIn) {}

    void Doc29ProfileDeparturePoints::addPoint() noexcept {
        if (empty())
        {
            addPoint(0.0, 0.0, 0.0, fromPoundsOfForce(10000.0));
            return;
        }

        auto& [cumDist, lastPt] = *m_Points.rbegin();
        m_Points.insert({ cumDist + 1.0, lastPt });
    }

    void Doc29ProfileDeparturePoints::addPoint(double CumulativeGroundDistance, double AltitudeAfe, double TrueAirspeed, double CorrNetThrustPerEng) noexcept {
        GRAPE_ASSERT(CumulativeGroundDistance >= 0.0);
        GRAPE_ASSERT(TrueAirspeed >= 0.0);
        GRAPE_ASSERT(CorrNetThrustPerEng > 0.0);

        m_Points.try_emplace(CumulativeGroundDistance, AltitudeAfe, TrueAirspeed, CorrNetThrustPerEng);
    }

    void Doc29ProfileDeparturePoints::updateCumulativeGroundDistance(std::size_t Index, double NewCumulativeGroundDistance) noexcept {
        GRAPE_ASSERT(Index < size());
        GRAPE_ASSERT(NewCumulativeGroundDistance >= 0.0);

        auto node = m_Points.extract(std::next(m_Points.begin(), Index));
        node.key() = NewCumulativeGroundDistance;
        m_Points.insert(std::move(node));
    }

    void Doc29ProfileDeparturePoints::insertPoint(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index <= size());

        if (empty())
        {
            addPoint();
            return;
        }

        if (Index == size()) // Inserting after the last point
        {
            auto& [currCumGroundDist, currPt] = *m_Points.rbegin(); // Last point
            addPoint(currCumGroundDist + 100.0, currPt.AltitudeAfe, currPt.TrueAirspeed, currPt.CorrNetThrustPerEng);
            return;
        }

        if (Index == 0) // Inserting before the first point
        {
            auto& [currCumGroundDist, currPt] = *m_Points.begin(); //First point
            addPoint(std::max(currCumGroundDist - 100.0, 0.0), currPt.AltitudeAfe, currPt.TrueAirspeed, currPt.CorrNetThrustPerEng);
            return;
        }

        // At least two points in the profile and index is neither 0 nor size
        // There is a point at index and a point at index - 1
        auto& [prevCumGroundDistance, prevPt] = *std::next(m_Points.begin(), Index - 1);
        auto& [nextCumGroundDistance, nextPt] = *std::next(m_Points.begin(), Index);

        const double nemCumGroundDist = std::midpoint(nextCumGroundDistance, prevCumGroundDistance);
        const double newAltitudeAfe = std::midpoint(prevPt.AltitudeAfe, nextPt.AltitudeAfe);
        const double newTrueAirspeed = std::midpoint(prevPt.TrueAirspeed, nextPt.TrueAirspeed);
        const double newThrust = std::midpoint(prevPt.CorrNetThrustPerEng, nextPt.CorrNetThrustPerEng);

        // Case: previous and next cumulative ground distances are close to one another
        if (std::abs(nextCumGroundDistance - prevCumGroundDistance) < 10.0) // 10 meters defined as 'close'
            insertPoint(--Index);                                           // Insert after
        else
            addPoint(nemCumGroundDist, newAltitudeAfe, newTrueAirspeed, newThrust);
    }

    void Doc29ProfileDeparturePoints::deletePoint(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index < size());
        m_Points.erase(std::next(m_Points.begin(), Index));
    }

    void Doc29ProfileDeparturePoints::deletePoint() noexcept {
        if (!empty())
            m_Points.erase(std::next(m_Points.begin(), size() - 1));
    }

    void Doc29ProfileDeparturePoints::clear() noexcept { m_Points.clear(); }

    void Doc29ProfileDeparturePoints::addPointE(double CumulativeGroundDistance, double AltitudeAfe, double TrueAirspeed, double CorrNetThrustPerEng) {
        if (!(CumulativeGroundDistance >= 0.0))
            throw GrapeException("Cumulative ground distance must be higher than 0 m.");
        if (!(TrueAirspeed >= 0.0))
            throw GrapeException("True airspeed must be at least 0 m/s.");
        if (!(CorrNetThrustPerEng > 0.0))
            throw GrapeException("Thrust must be higher than 0 N.");

        addPoint(CumulativeGroundDistance, AltitudeAfe, TrueAirspeed, CorrNetThrustPerEng);
    }

    void Doc29ProfileDeparturePoints::accept(Doc29ProfileVisitor& Vis) { Vis.visitDoc29ProfileDeparturePoints(*this); }

    void Doc29ProfileDeparturePoints::accept(Doc29ProfileVisitor& Vis) const { Vis.visitDoc29ProfileDeparturePoints(*this); }

    void Doc29ProfileDeparturePoints::accept(Doc29ProfileDepartureVisitor& Vis) { Vis.visitDoc29ProfileDeparturePoints(*this); }

    void Doc29ProfileDeparturePoints::accept(Doc29ProfileDepartureVisitor& Vis) const { Vis.visitDoc29ProfileDeparturePoints(*this); }

    Doc29ProfileArrivalProcedural::Doc29ProfileArrivalProcedural(Doc29Aircraft& Doc29PerformanceIn, std::string_view NameIn) : Doc29ProfileArrival(Doc29PerformanceIn, NameIn) {
        const Doc29AerodynamicCoefficients* aeroCoeffs = nullptr;
        for (const auto& coeffs : parentDoc29Performance().AerodynamicCoefficients | std::views::values)
        {
            if (coeffs.CoefficientType == Doc29AerodynamicCoefficients::Type::Land)
            {
                aeroCoeffs = &coeffs;
                break;
            }
        }
        GRAPE_ASSERT(aeroCoeffs); // Land coefficients guaranteed to exist at least once by Doc29Aircraft
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(*aeroCoeffs, *this);

        m_Steps.emplace_back(DescendLand(aeroCoeffs, -3.0, fromFeet(50.0), fromFeet(300.0)));
    }

    Doc29ProfileArrivalProcedural::~Doc29ProfileArrivalProcedural() {
        for (const auto& arrStep : m_Steps)
            unblockCoefficients(arrStep);
    }

    void Doc29ProfileArrivalProcedural::addDescendDecelerate() noexcept {
        const auto st = nextState(0);
        addDescendDecelerateImpl(st.Doc29AerodynamicCoefficients, st.AltitudeAfe, -3.0, st.CalibratedAirspeed);
    }

    void Doc29ProfileArrivalProcedural::addDescendIdle() noexcept {
        const auto st = nextState(0);
        addDescendIdleImpl(st.AltitudeAfe, -3.0, st.CalibratedAirspeed); // Default descent angle -3.0 Deg
    }

    void Doc29ProfileArrivalProcedural::addLevel() noexcept {
        const auto st = nextState(0);
        addLevelImpl(st.Doc29AerodynamicCoefficients, 100.0);
    }

    void Doc29ProfileArrivalProcedural::addLevelDecelerate() noexcept {
        const auto st = nextState(0);
        addLevelDecelerateImpl(st.Doc29AerodynamicCoefficients, 100.0, st.CalibratedAirspeed);
    }

    void Doc29ProfileArrivalProcedural::addLevelIdle() noexcept {
        const auto st = nextState(0);
        addLevelIdleImpl(100.0, st.CalibratedAirspeed);
    }

    void Doc29ProfileArrivalProcedural::addGroundDecelerate() noexcept {
        addGroundDecelerateImpl(100.0, 0.0, 0.4); // Defaults to 40% reverse thrust percentage
    }

    void Doc29ProfileArrivalProcedural::addDescendDecelerate(const std::string& AerodynamicCoefficientsName, double StartAltitudeAfe, double DescentAngle, double StartCalibratedAirspeed) noexcept {
        GRAPE_ASSERT(parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName));
        GRAPE_ASSERT(DescentAngle < 0.0);
        GRAPE_ASSERT(StartCalibratedAirspeed >= 0.0);

        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);
        addDescendDecelerateImpl(&coeffs, StartAltitudeAfe, DescentAngle, StartCalibratedAirspeed);
    }

    void Doc29ProfileArrivalProcedural::addDescendIdle(double StartAltitudeAfe, double DescentAngle, double StartCalibratedAirspeed) noexcept {
        GRAPE_ASSERT(DescentAngle < 0.0);
        GRAPE_ASSERT(StartCalibratedAirspeed >= 0.0);

        addDescendIdleImpl(StartAltitudeAfe, DescentAngle, StartCalibratedAirspeed);
    }

    void Doc29ProfileArrivalProcedural::addLevel(const std::string& AerodynamicCoefficientsName, double GroundDistance) noexcept {
        GRAPE_ASSERT(parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName));
        GRAPE_ASSERT(GroundDistance > 0.0);

        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);
        addLevelImpl(&coeffs, GroundDistance);
    }

    void Doc29ProfileArrivalProcedural::addLevelDecelerate(const std::string& AerodynamicCoefficientsName, double GroundDistance, double StartCalibratedAirspeed) noexcept {
        GRAPE_ASSERT(parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName));
        GRAPE_ASSERT(GroundDistance > 0.0);
        GRAPE_ASSERT(StartCalibratedAirspeed >= 0.0);

        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);
        addLevelDecelerateImpl(&coeffs, GroundDistance, StartCalibratedAirspeed);
    }

    void Doc29ProfileArrivalProcedural::addLevelIdle(double GroundDistance, double StartCalibratedAirspeed) noexcept {
        GRAPE_ASSERT(GroundDistance > 0.0);
        GRAPE_ASSERT(StartCalibratedAirspeed >= 0.0);

        addLevelIdleImpl(GroundDistance, StartCalibratedAirspeed);
    }

    void Doc29ProfileArrivalProcedural::setDescendLandParameters(const std::string& AerodynamicCoefficientsName, double DescentAngle, double ThresholdCrossingAltitudeAfe, double TouchdownRoll) noexcept {
        GRAPE_ASSERT(parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName));
        GRAPE_ASSERT(parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName).CoefficientType == Doc29AerodynamicCoefficients::Type::Land);
        GRAPE_ASSERT(DescentAngle < 0.0);
        GRAPE_ASSERT(TouchdownRoll > 0.0);

        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);
        auto& [descLandDoc29AerodynamicCoefficients, descLandDescentAngle, descLandThresholdCrossingAltitudeAfe, touchdownRoll] = std::get<DescendLand>(m_Steps.at(m_LandIndex));
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.unblock(*descLandDoc29AerodynamicCoefficients, *this);
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(coeffs, *this);

        descLandDoc29AerodynamicCoefficients = &coeffs;
        descLandDescentAngle = DescentAngle;
        descLandThresholdCrossingAltitudeAfe = ThresholdCrossingAltitudeAfe;
        touchdownRoll = TouchdownRoll;
    }

    void Doc29ProfileArrivalProcedural::setDescendLandParameters(const std::string& AerodynamicCoefficientsName, double TouchdownRoll) noexcept {
        const auto& [descLandDoc29AerodynamicCoefficients, descLandDescentAngle, descLandThresholdCrossingAltitudeAfe, touchdownRoll] = std::get<DescendLand>(m_Steps.at(m_LandIndex));
        setDescendLandParameters(AerodynamicCoefficientsName, descLandDescentAngle, descLandThresholdCrossingAltitudeAfe, TouchdownRoll);
    }

    void Doc29ProfileArrivalProcedural::setDescendLandParameters(double DescentAngle, double ThresholdCrossingAltitudeAfe) noexcept {
        const auto& [descLandDoc29AerodynamicCoefficients, descLandDescentAngle, descLandThresholdCrossingAltitudeAfe, touchdownRoll] = std::get<DescendLand>(m_Steps.at(m_LandIndex));
        setDescendLandParameters(descLandDoc29AerodynamicCoefficients->Name, DescentAngle, ThresholdCrossingAltitudeAfe, touchdownRoll);
    }

    void Doc29ProfileArrivalProcedural::addGroundDecelerate(double GroundDistance, double StartCalibratedAirspeed, double ThrustPercentage) noexcept {
        GRAPE_ASSERT(GroundDistance >= 0.0);
        GRAPE_ASSERT(StartCalibratedAirspeed >= 0.0);
        GRAPE_ASSERT(ThrustPercentage >= 0.0 && ThrustPercentage <= 1.0);

        addGroundDecelerateImpl(GroundDistance, StartCalibratedAirspeed, ThrustPercentage);
    }

    void Doc29ProfileArrivalProcedural::deleteStep(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index < size() && Index != m_LandIndex);

        const auto stepIt = std::next(m_Steps.begin(), Index);
        if (Index < m_LandIndex)
        {
            unblockCoefficients(*stepIt);
            m_Steps.erase(stepIt);
            m_LandIndex--;
        }
        else { m_Steps.erase(stepIt); }
    }

    void Doc29ProfileArrivalProcedural::deleteAirStep() noexcept {
        if (m_LandIndex == 0) // No air steps
            return;

        const auto stepIt = m_Steps.begin() + m_LandIndex - 1;
        unblockCoefficients(*stepIt);
        m_Steps.erase(stepIt);
        m_LandIndex--;
    }

    void Doc29ProfileArrivalProcedural::deleteGroundDecelerate() noexcept {
        if (m_LandIndex == size() - 1) // No ground steps
            return;

        m_Steps.pop_back();
    }

    void Doc29ProfileArrivalProcedural::clearAirSteps() noexcept {
        if (airStepsEmpty())
            return;

        for (auto stepIt = m_Steps.begin(); stepIt != m_Steps.begin() + m_LandIndex; ++stepIt) // Unblocks all steps up to landing step
            unblockCoefficients(*stepIt);
        m_Steps.erase(m_Steps.begin(), m_Steps.begin() + m_LandIndex); // Deletes all steps between start and landing

        m_UseIdleThrustCount = 0;
        m_LandIndex = 0;
    }

    void Doc29ProfileArrivalProcedural::clearGroundSteps() noexcept {
        m_Steps.erase(m_Steps.begin() + m_LandIndex + 1, m_Steps.end()); // Deletes all ground decelerate steps (after landing)
    }

    void Doc29ProfileArrivalProcedural::clear() noexcept {
        clearAirSteps();
        clearGroundSteps();
    }

    void Doc29ProfileArrivalProcedural::addDescendDecelerateE(const std::string& AerodynamicCoefficientsName, double StartAltitudeAfe, double DescentAngle, double StartCalibratedAirspeed) {
        if (!parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName))
            throw GrapeException(std::format("Aerodynamic coefficients '{}' do not exists in aircraft '{}'.", AerodynamicCoefficientsName, parentDoc29Performance().Name));
        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);

        if (std::isnan(StartAltitudeAfe))
            throw GrapeException("Start altitude AFE must be provided.");

        if (!(DescentAngle < 0.0))
            throw GrapeException("Descent angle must be lower than 0.");

        if (!(StartCalibratedAirspeed >= 0.0))
            throw GrapeException("Start calibrated airspeed must be higher or equal to 0 m/s.");

        addDescendDecelerateImpl(&coeffs, StartAltitudeAfe, DescentAngle, StartCalibratedAirspeed);
    }

    void Doc29ProfileArrivalProcedural::addDescendIdleE(double StartAltitudeAfe, double DescentAngle, double StartCalibratedAirspeed) {
        if (std::isnan(StartAltitudeAfe))
            throw GrapeException("Start altitude AFE must be provided.");

        if (!(DescentAngle < 0.0))
            throw GrapeException("Descent angle must be lower than 0.");

        if (!(StartCalibratedAirspeed >= 0.0))
            throw GrapeException("Start calibrated airspeed must be higher or equal to 0 m/s.");

        addDescendIdleImpl(StartAltitudeAfe, DescentAngle, StartCalibratedAirspeed);
    }

    void Doc29ProfileArrivalProcedural::addLevelE(const std::string& AerodynamicCoefficientsName, double GroundDistance) {
        if (!parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName))
            throw GrapeException(std::format("Aerodynamic coefficients '{}' do not exists in aircraft '{}'.", AerodynamicCoefficientsName, parentDoc29Performance().Name));
        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);

        if (!(GroundDistance > 0.0))
            throw GrapeException("Ground distance must be higher than 0 m.");

        addLevelImpl(&coeffs, GroundDistance);
    }

    void Doc29ProfileArrivalProcedural::addLevelDecelerateE(const std::string& AerodynamicCoefficientsName, double GroundDistance, double StartCalibratedAirspeed) {
        if (!parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName))
            throw GrapeException(std::format("Aerodynamic coefficients '{}' do not exists in aircraft '{}'.", AerodynamicCoefficientsName, parentDoc29Performance().Name));
        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);

        if (!(GroundDistance > 0.0))
            throw GrapeException("Ground distance must be higher than 0 m.");

        if (!(StartCalibratedAirspeed >= 0.0))
            throw GrapeException("Start calibrated airspeed must be higher or equal to 0 m/s.");

        addLevelDecelerateImpl(&coeffs, GroundDistance, StartCalibratedAirspeed);
    }

    void Doc29ProfileArrivalProcedural::addLevelIdleE(double GroundDistance, double StartCalibratedAirspeed) {
        if (!(GroundDistance > 0.0))
            throw GrapeException("Ground distance must be higher than 0 m.");

        if (!(StartCalibratedAirspeed >= 0.0))
            throw GrapeException("Start calibrated airspeed must be higher or equal to 0 m/s.");

        addLevelIdleImpl(GroundDistance, StartCalibratedAirspeed);
    }

    void Doc29ProfileArrivalProcedural::setDescendLandParametersE(const std::string& AerodynamicCoefficientsName, double DescentAngle, double ThresholdCrossingAltitudeAfe, double TouchdownRoll) {
        if (!parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName))
            throw GrapeException(std::format("Aerodynamic coefficients '{}' do not exists in aircraft '{}'.", AerodynamicCoefficientsName, parentDoc29Performance().Name));
        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);
        if (coeffs.CoefficientType != Doc29AerodynamicCoefficients::Type::Land)
            throw GrapeException(std::format("Aerodynamic coefficients '{}' are not land coefficients.", AerodynamicCoefficientsName));

        if (!(DescentAngle < 0.0))
            throw GrapeException("Descent angle must be lower than 0.");

        if (std::isnan(ThresholdCrossingAltitudeAfe))
            throw GrapeException("Threshold crossing altitude AFE must be provided.");

        if (!(TouchdownRoll > 0.0))
            throw GrapeException("Touchdown roll must be higher than 0 m.");

        auto& [descLandDoc29AerodynamicCoefficients, descLandDescentAngle, descLandThresholdCrossingAltitudeAfe, touchdownRoll] = std::get<DescendLand>(m_Steps.at(m_LandIndex));
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.unblock(*descLandDoc29AerodynamicCoefficients, *this);
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(coeffs, *this);

        descLandDoc29AerodynamicCoefficients = &coeffs;
        descLandDescentAngle = DescentAngle;
        descLandThresholdCrossingAltitudeAfe = ThresholdCrossingAltitudeAfe;
        touchdownRoll = TouchdownRoll;
    }

    void Doc29ProfileArrivalProcedural::setDescendLandParametersE(const std::string& AerodynamicCoefficientsName, double TouchdownRoll) {
        const auto& [descLandDoc29AerodynamicCoefficients, descLandDescentAngle, descLandThresholdCrossingAltitudeAfe, touchdownRoll] = std::get<DescendLand>(m_Steps.at(m_LandIndex));
        setDescendLandParametersE(AerodynamicCoefficientsName, descLandDescentAngle, descLandThresholdCrossingAltitudeAfe, TouchdownRoll);
    }

    void Doc29ProfileArrivalProcedural::setDescendLandParametersE(double DescentAngle, double ThresholdCrossingAltitudeAfe) {
        const auto& [descLandDoc29AerodynamicCoefficients, descLandDescentAngle, descLandThresholdCrossingAltitudeAfe, touchdownRoll] = std::get<DescendLand>(m_Steps.at(m_LandIndex));
        setDescendLandParametersE(descLandDoc29AerodynamicCoefficients->Name, DescentAngle, ThresholdCrossingAltitudeAfe, touchdownRoll);
    }

    void Doc29ProfileArrivalProcedural::addGroundDecelerateE(double GroundDistance, double StartCalibratedAirspeed, double ThrustPercentage) {
        if (!(GroundDistance >= 0.0))
            throw GrapeException("Ground distance must be at least 0.");

        if (!(StartCalibratedAirspeed >= 0.0))
            throw GrapeException("End calibrated airspeed must be higher or equal to 0 m/s.");

        if (!(ThrustPercentage >= 0.0 && ThrustPercentage <= 1.0))
            throw GrapeException("Thrust percentage must be higher or equal to 0 and lower or equal to 1.");

        addGroundDecelerateImpl(GroundDistance, StartCalibratedAirspeed, ThrustPercentage);
    }

    void Doc29ProfileArrivalProcedural::accept(Doc29ProfileVisitor& Vis) { Vis.visitDoc29ProfileArrivalProcedural(*this); }

    void Doc29ProfileArrivalProcedural::accept(Doc29ProfileVisitor& Vis) const { Vis.visitDoc29ProfileArrivalProcedural(*this); }

    void Doc29ProfileArrivalProcedural::accept(Doc29ProfileArrivalVisitor& Vis) { Vis.visitDoc29ProfileArrivalProcedural(*this); }
    void Doc29ProfileArrivalProcedural::accept(Doc29ProfileArrivalVisitor& Vis) const { Vis.visitDoc29ProfileArrivalProcedural(*this); }

    void Doc29ProfileArrivalProcedural::addDescendDecelerateImpl(const Doc29AerodynamicCoefficients* Coefficients, double StartAltitudeAfe, double DescentAngle, double StartCalibratedAirspeed) {
        m_Steps.emplace(std::next(m_Steps.begin(), m_LandIndex), DescendDecelerate(Coefficients, StartAltitudeAfe, DescentAngle, StartCalibratedAirspeed));
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(*Coefficients, *this);
        ++m_LandIndex;
    }

    void Doc29ProfileArrivalProcedural::addDescendIdleImpl(double StartAltitudeAfe, double DescentAngle, double StartCalibratedAirspeed) {
        m_Steps.emplace(std::next(m_Steps.begin(), m_LandIndex), DescendIdle(StartAltitudeAfe, DescentAngle, StartCalibratedAirspeed));
        ++m_UseIdleThrustCount;
        ++m_LandIndex;
    }

    void Doc29ProfileArrivalProcedural::addLevelImpl(const Doc29AerodynamicCoefficients* Coefficients, double GroundDistance) {
        m_Steps.emplace(std::next(m_Steps.begin(), m_LandIndex), Level(Coefficients, GroundDistance));
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(*Coefficients, *this);
        ++m_LandIndex;
    }

    void Doc29ProfileArrivalProcedural::addLevelDecelerateImpl(const Doc29AerodynamicCoefficients* Coefficients, double GroundDistance, double StartCalibratedAirspeed) {
        m_Steps.emplace(std::next(m_Steps.begin(), m_LandIndex), LevelDecelerate(Coefficients, GroundDistance, StartCalibratedAirspeed));
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(*Coefficients, *this);
        ++m_LandIndex;
    }

    void Doc29ProfileArrivalProcedural::addLevelIdleImpl(double GroundDistance, double StartCalibratedAirspeed) {
        m_Steps.emplace(std::next(m_Steps.begin(), m_LandIndex), LevelIdle(GroundDistance, StartCalibratedAirspeed));
        ++m_UseIdleThrustCount;
        ++m_LandIndex;
    }

    void Doc29ProfileArrivalProcedural::addGroundDecelerateImpl(double GroundDistance, double StartCalibratedAirspeed, double ThrustPercentage) { m_Steps.emplace_back(GroundDecelerate(GroundDistance, StartCalibratedAirspeed, ThrustPercentage)); }

    void Doc29ProfileArrivalProcedural::unblockCoefficients(const Step& ArrivalStep) {
        std::visit(Overload{
            [&](const DescendIdle&) {},
            [&](const LevelIdle&) {},
            [&](const GroundDecelerate&) {},
            [&](const auto& ArrStep) {
                parentDoc29Performance().b_BlockedAerodynamicCoefficients.unblock(*ArrStep.Doc29AerodynamicCoefficients, *this); },
            }, ArrivalStep);
    }

    // State before land is NEVER invalid, landing step guarantees aerodynamic coefficients and altitude, calibrated airspeed initialized to 100 knots.
    // State after land is invalid
    Doc29ProfileArrivalProcedural::State Doc29ProfileArrivalProcedural::nextState(std::size_t Index) const {
        GRAPE_ASSERT(Index < size());

        State st;

        if (Index > m_LandIndex)
        {
            return st;
        }
        else if (Index == m_LandIndex)
        {
            st.CalibratedAirspeed = fromKnots(100.0);
            std::visit(VisitorState(st), m_Steps.at(m_LandIndex));
            return st;
        }
        else
        {
            st.CalibratedAirspeed = fromKnots(100.0);
            for (auto it = std::next(airStepsBegin(), Index); it != std::next(airStepsEnd()); ++it) // Iterate air steps and landing step
            {
                std::visit(VisitorState(st), *it); // Edits St
                if (st.valid())
                    break;
            }
        }

        return st;
    }

    void Doc29ProfileArrivalProcedural::VisitorState::operator()(const DescendDecelerate& DescendDecelerateStep) const {
        if (!St.Doc29AerodynamicCoefficients)
            St.Doc29AerodynamicCoefficients = DescendDecelerateStep.Doc29AerodynamicCoefficients;

        if (std::isnan(St.AltitudeAfe))
            St.AltitudeAfe = DescendDecelerateStep.StartAltitudeAfe;

        if (std::isnan(St.CalibratedAirspeed))
            St.CalibratedAirspeed = DescendDecelerateStep.StartCalibratedAirspeed;
    }

    void Doc29ProfileArrivalProcedural::VisitorState::operator()(const DescendIdle& DescendIdleStep) const {
        if (std::isnan(St.AltitudeAfe))
            St.AltitudeAfe = DescendIdleStep.StartAltitudeAfe;

        if (std::isnan(St.CalibratedAirspeed))
            St.CalibratedAirspeed = DescendIdleStep.StartCalibratedAirspeed;
    }

    void Doc29ProfileArrivalProcedural::VisitorState::operator()(const Level& LevelStep) const {
        if (!St.Doc29AerodynamicCoefficients)
            St.Doc29AerodynamicCoefficients = LevelStep.Doc29AerodynamicCoefficients;
    }

    void Doc29ProfileArrivalProcedural::VisitorState::operator()(const LevelDecelerate& LevelDecelerateStep) const {
        if (!St.Doc29AerodynamicCoefficients)
            St.Doc29AerodynamicCoefficients = LevelDecelerateStep.Doc29AerodynamicCoefficients;

        if (std::isnan(St.CalibratedAirspeed))
            St.CalibratedAirspeed = LevelDecelerateStep.StartCalibratedAirspeed;
    }

    void Doc29ProfileArrivalProcedural::VisitorState::operator()(const LevelIdle& LevelIdleStep) const {
        if (std::isnan(St.CalibratedAirspeed))
            St.CalibratedAirspeed = LevelIdleStep.StartCalibratedAirspeed;
    }

    void Doc29ProfileArrivalProcedural::VisitorState::operator()(const DescendLand& DescendLandStep) const {
        if (!St.Doc29AerodynamicCoefficients)
            St.Doc29AerodynamicCoefficients = DescendLandStep.Doc29AerodynamicCoefficients;

        if (std::isnan(St.AltitudeAfe))
            St.AltitudeAfe = DescendLandStep.ThresholdCrossingAltitudeAfe;
    }

    void Doc29ProfileArrivalProcedural::VisitorState::operator()(const GroundDecelerate& GroundDecelerateStep) const { return; }

    Doc29ProfileDepartureProcedural::Doc29ProfileDepartureProcedural(Doc29Aircraft& Doc29PerformanceIn, const std::string& NameIn) : Doc29ProfileDeparture(Doc29PerformanceIn, NameIn) {
        const Doc29AerodynamicCoefficients* aeroCoeffs = nullptr;
        for (const auto& coeffs : parentDoc29Performance().AerodynamicCoefficients | std::views::values)
        {
            if (coeffs.CoefficientType == Doc29AerodynamicCoefficients::Type::Takeoff)
                aeroCoeffs = &coeffs;
        }

        GRAPE_ASSERT(aeroCoeffs); // Takeoff coefficients guaranteed to exist at least once by Doc29Aircraft
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(*aeroCoeffs, *this);
        m_Steps.emplace_back(Takeoff(aeroCoeffs, 0.0));
    }

    Doc29ProfileDepartureProcedural::~Doc29ProfileDepartureProcedural() {
        for (const auto& depStep : m_Steps)
            unblockCoefficients(depStep);
    }

    void Doc29ProfileDepartureProcedural::setThrustCutback(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index < m_Steps.size());
        m_ThrustCutbackIndex = Index;
    }

    void Doc29ProfileDepartureProcedural::addClimb() noexcept {
        const auto [PrevDoc29AerodynamicCoefficients, PrevAltitudeAfe, PrevCalibratedAirspeed] = previousState(size()); // Last
        m_Steps.emplace_back(Climb(PrevDoc29AerodynamicCoefficients, PrevAltitudeAfe));
    }

    void Doc29ProfileDepartureProcedural::addClimbAccelerate() noexcept {
        const auto [PrevDoc29AerodynamicCoefficients, PrevAltitudeAfe, PrevCalibratedAirspeed] = previousState(size()); // Last
        m_Steps.emplace_back(ClimbAccelerate(PrevDoc29AerodynamicCoefficients, PrevCalibratedAirspeed, fromFeetPerMinute(500.0)));
    }

    void Doc29ProfileDepartureProcedural::addClimbAcceleratePercentage() noexcept {
        const auto [PrevDoc29AerodynamicCoefficients, PrevAltitudeAfe, PrevCalibratedAirspeed] = previousState(size()); // Last
        m_Steps.emplace_back(ClimbAcceleratePercentage(PrevDoc29AerodynamicCoefficients, PrevCalibratedAirspeed, 0.6));
    }

    void Doc29ProfileDepartureProcedural::setTakeoffParameters(const std::string& AerodynamicCoefficientsName, double InitialCalibratedAirspeed) noexcept {
        GRAPE_ASSERT(parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName));
        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);
        GRAPE_ASSERT(coeffs.CoefficientType == Doc29AerodynamicCoefficients::Type::Takeoff);
        GRAPE_ASSERT(InitialCalibratedAirspeed >= 0.0);

        auto& takeoffStep = std::get<Takeoff>(m_Steps.at(0)); // First step guaranteed to be Takeoff
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.unblock(*takeoffStep.Doc29AerodynamicCoefficients, *this);
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(coeffs, *this);
        takeoffStep.Doc29AerodynamicCoefficients = &coeffs;
        takeoffStep.InitialCalibratedAirspeed = InitialCalibratedAirspeed;
    }

    void Doc29ProfileDepartureProcedural::addClimb(const std::string& AerodynamicCoefficientsName, double EndAltitudeAfe) noexcept {
        GRAPE_ASSERT(parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName));
        GRAPE_ASSERT(!std::isnan(EndAltitudeAfe));

        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(coeffs, *this);
        m_Steps.emplace_back(Climb(&coeffs, EndAltitudeAfe));
    }

    void Doc29ProfileDepartureProcedural::addClimbAccelerate(const std::string& AerodynamicCoefficientsName, double EndCalibratedAirspeed, double ClimbRate) noexcept {
        GRAPE_ASSERT(parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName));
        GRAPE_ASSERT(EndCalibratedAirspeed >= 0.0);
        GRAPE_ASSERT(ClimbRate >= 0.0);

        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(coeffs, *this);
        m_Steps.emplace_back(ClimbAccelerate(&coeffs, EndCalibratedAirspeed, ClimbRate));
    }

    void Doc29ProfileDepartureProcedural::addClimbAcceleratePercentage(const std::string& AerodynamicCoefficientsName, double EndCalibratedAirspeed, double AccelerationPercentage) noexcept {
        GRAPE_ASSERT(parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName));
        GRAPE_ASSERT(EndCalibratedAirspeed >= 0.0);
        GRAPE_ASSERT(AccelerationPercentage > 0.0 && AccelerationPercentage <= 1.0);

        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(coeffs, *this);
        m_Steps.emplace_back(ClimbAcceleratePercentage(&coeffs, EndCalibratedAirspeed, AccelerationPercentage));
    }

    void Doc29ProfileDepartureProcedural::deleteStep(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index != 0 && Index < m_Steps.size());

        if (m_ThrustCutbackIndex == Index)
        {
            Log::dataLogic()->info("Deleted thrust cutback step of profile '{}' in aircraft '{}'. Thrust cutback set to previous step.", Name, parentDoc29Performance().Name);
            --m_ThrustCutbackIndex;
        }

        unblockCoefficients(m_Steps.at(Index));
        m_Steps.erase(std::next(m_Steps.begin(), Index));
    }

    void Doc29ProfileDepartureProcedural::deleteStep() noexcept {
        if (!empty())
            deleteStep(m_Steps.size() - 1);
    }

    void Doc29ProfileDepartureProcedural::clear() noexcept {
        for (auto it = m_Steps.begin() + 1; it != m_Steps.end(); ++it)
            unblockCoefficients(*it);
        m_Steps.erase(m_Steps.begin() + 1, m_Steps.end()); // Deletes all steps except takeoff
        m_ThrustCutbackIndex = 0;
    }

    void Doc29ProfileDepartureProcedural::setTakeoffParametersE(const std::string& AerodynamicCoefficientsName, double InitialCalibratedAirspeed) {
        if (!parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName))
            throw GrapeException(std::format("Aerodynamic coefficients '{}' do not exists in aircraft '{}'.", AerodynamicCoefficientsName, parentDoc29Performance().Name));

        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);
        if (coeffs.CoefficientType != Doc29AerodynamicCoefficients::Type::Takeoff)
            throw GrapeException(std::format("Aerodynamic coefficients '{}' are not takeoff coefficients.", AerodynamicCoefficientsName));

        if (!(InitialCalibratedAirspeed >= 0.0))
            throw GrapeException("Initial calibrated airspeed must be at least 0 m/s.");

        auto& takeoffStep = std::get<Takeoff>(m_Steps.at(0)); // First step guaranteed to be Takeoff
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.unblock(*takeoffStep.Doc29AerodynamicCoefficients, *this);
        parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(coeffs, *this);
        takeoffStep.Doc29AerodynamicCoefficients = &coeffs;
        takeoffStep.InitialCalibratedAirspeed = InitialCalibratedAirspeed;
    }

    void Doc29ProfileDepartureProcedural::addClimbE(const std::string& AerodynamicCoefficientsName, double EndAltitudeAfe) {
        if (!parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName))
            throw GrapeException(std::format("Aerodynamic coefficients '{}' do not exists in aircraft '{}'.", AerodynamicCoefficientsName, parentDoc29Performance().Name));

        if (std::isnan(EndAltitudeAfe))
            throw GrapeException("End altitude AFE must be provided.");

        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);

        parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(coeffs, *this);
        m_Steps.emplace_back(Climb(&coeffs, EndAltitudeAfe));
    }

    void Doc29ProfileDepartureProcedural::addClimbAccelerateE(const std::string& AerodynamicCoefficientsName, double EndCalibratedAirspeed, double ClimbRate) {
        if (!parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName))
            throw GrapeException(std::format("Aerodynamic coefficients '{}' do not exists in aircraft '{}'.", AerodynamicCoefficientsName, parentDoc29Performance().Name));
        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);

        if (!(EndCalibratedAirspeed >= 0.0))
            throw GrapeException("End calibrated airspeed must be at least 0 m/s.");

        if (!(ClimbRate >= 0.0))
            throw GrapeException("Climb rate must be at least 0 m/s.");

        parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(coeffs, *this);
        m_Steps.emplace_back(ClimbAccelerate(&coeffs, EndCalibratedAirspeed, ClimbRate));
    }

    void Doc29ProfileDepartureProcedural::addClimbAcceleratePercentageE(const std::string& AerodynamicCoefficientsName, double EndCalibratedAirspeed, double AccelerationPercentage) {
        if (!parentDoc29Performance().AerodynamicCoefficients.contains(AerodynamicCoefficientsName))
            throw GrapeException(std::format("Aerodynamic coefficients '{}' do not exists in aircraft '{}'.", AerodynamicCoefficientsName, parentDoc29Performance().Name));
        const auto& coeffs = parentDoc29Performance().AerodynamicCoefficients(AerodynamicCoefficientsName);

        if (!(EndCalibratedAirspeed >= 0.0))
            throw GrapeException("End calibrated airspeed must be at least 0 m/s.");

        if (!(AccelerationPercentage > 0.0 && AccelerationPercentage <= 1.0))
            throw GrapeException("Acceleration percentage must be higher than 0 and lower or equal to 1.");

        parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(coeffs, *this);
        m_Steps.emplace_back(ClimbAcceleratePercentage(&coeffs, EndCalibratedAirspeed, AccelerationPercentage));
    }

    // Previous state is set to 0.0 for first point | Index allowed to be size() for checking the state of the last point
    // State is NEVER invalid, takeoff step guarantees aerodynamic coefficients, altitude and calibrated airspeed
    Doc29ProfileDepartureProcedural::State Doc29ProfileDepartureProcedural::previousState(std::size_t Index) const {
        GRAPE_ASSERT(Index <= size());

        State st;

        if (Index == 0)
        {
            st.Doc29AerodynamicCoefficients = std::get<Takeoff>(m_Steps.at(0)).Doc29AerodynamicCoefficients;
            st.AltitudeAfe = 0.0;
            st.CalibratedAirspeed = 0.0;
        }
        else
        {
            for (auto it = m_Steps.rbegin() + (size() - Index); it != m_Steps.rend(); ++it) // Reverse iterate from index - 1 to begin
            {
                std::visit(VisitorState(st), *it); // Edits St
                if (st.valid())
                    break;
            }
        }

        return st;
    }

    void Doc29ProfileDepartureProcedural::VisitorState::operator()(const Takeoff& TakeoffStep) const {
        if (!St.Doc29AerodynamicCoefficients)
            St.Doc29AerodynamicCoefficients = TakeoffStep.Doc29AerodynamicCoefficients;

        if (std::isnan(St.AltitudeAfe))
            St.AltitudeAfe = 0.0; // Altitude AFE can't be less than 0!

        if (std::isnan(St.CalibratedAirspeed))
            St.CalibratedAirspeed = TakeoffStep.InitialCalibratedAirspeed;
    }

    void Doc29ProfileDepartureProcedural::VisitorState::operator()(const Climb& ClimbStep) const {
        if (!St.Doc29AerodynamicCoefficients)
            St.Doc29AerodynamicCoefficients = ClimbStep.Doc29AerodynamicCoefficients;

        if (std::isnan(St.AltitudeAfe))
            St.AltitudeAfe = ClimbStep.EndAltitudeAfe;
    }

    void Doc29ProfileDepartureProcedural::VisitorState::operator()(const ClimbAccelerate& ClimbAccelerateStep) const {
        if (!St.Doc29AerodynamicCoefficients)
            St.Doc29AerodynamicCoefficients = ClimbAccelerateStep.Doc29AerodynamicCoefficients;

        if (std::isnan(St.CalibratedAirspeed))
            St.CalibratedAirspeed = ClimbAccelerateStep.EndCalibratedAirspeed;
    }

    void Doc29ProfileDepartureProcedural::VisitorState::operator()(const ClimbAcceleratePercentage& ClimbAcceleratePercentageStep) const {
        if (!St.Doc29AerodynamicCoefficients)
            St.Doc29AerodynamicCoefficients = ClimbAcceleratePercentageStep.Doc29AerodynamicCoefficients;

        if (std::isnan(St.CalibratedAirspeed))
            St.CalibratedAirspeed = ClimbAcceleratePercentageStep.EndCalibratedAirspeed;
    }

    void Doc29ProfileDepartureProcedural::accept(Doc29ProfileVisitor& Vis) { Vis.visitDoc29ProfileDepartureProcedural(*this); }

    void Doc29ProfileDepartureProcedural::accept(Doc29ProfileVisitor& Vis) const { Vis.visitDoc29ProfileDepartureProcedural(*this); }

    void Doc29ProfileDepartureProcedural::accept(Doc29ProfileDepartureVisitor& Vis) { Vis.visitDoc29ProfileDepartureProcedural(*this); }

    void Doc29ProfileDepartureProcedural::accept(Doc29ProfileDepartureVisitor& Vis) const { Vis.visitDoc29ProfileDepartureProcedural(*this); }

    void Doc29ProfileDepartureProcedural::unblockCoefficients(const Step& DepartureStep) { std::visit(Overload{ [&](const auto& DepStep) { parentDoc29Performance().b_BlockedAerodynamicCoefficients.unblock(*DepStep.Doc29AerodynamicCoefficients, *this); }, }, DepartureStep); }
}
