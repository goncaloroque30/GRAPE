// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Operation.h"

#include "Aircraft/Aircraft.h"

namespace GRAPE {
    void Operation::setCount(double CountIn) {
        if (!(CountIn >= 0.0))
            throw GrapeException("Operation count must be at least 0.");
        Count = CountIn;
    }

    void Operation::setTime(const std::string& UtcTimeStr) {
        const auto timeOpt = utcStringToTime(UtcTimeStr);
        if (timeOpt)
            Time = timeOpt.value();
        else
            throw GrapeException(std::format("Invalid operation time '{}'.", UtcTimeStr));
    }

    Duration Operation::timeOfDay() const { return Time - std::chrono::floor<std::chrono::days>(Time); }

    const Aircraft& Operation::aircraft() const { return m_Aircraft; }

    void Operation::setAircraft(const Aircraft& Acft) noexcept { m_Aircraft = Acft; }
}
