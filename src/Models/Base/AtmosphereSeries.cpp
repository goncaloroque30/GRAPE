// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "AtmosphereSeries.h"

namespace GRAPE {
    const Atmosphere& AtmosphereSeries::atmosphere(TimePoint Time) const noexcept {
        GRAPE_ASSERT(!empty());

        const auto it = m_Atmospheres.lower_bound(Time);

        if (it == m_Atmospheres.begin())
            return m_Atmospheres.begin()->second;

        if (it == m_Atmospheres.end())
            return m_Atmospheres.rbegin()->second;

        const auto prev = std::prev(it);

        if (Time - prev->first <= it->first - Time)
            return prev->second;

        return it->second;
    }

    void AtmosphereSeries::addAtmosphere() noexcept {
        auto maxTime = empty() ? std::chrono::round<Duration>(std::chrono::tai_clock::now()) : m_Atmospheres.rbegin()->first;
        addAtmosphere(maxTime + std::chrono::minutes(30));
    }

    void AtmosphereSeries::addAtmosphere(TimePoint Time, const Atmosphere& Atm) noexcept {
        auto [atm, added] = m_Atmospheres.try_emplace(Time, Atm);
    }

    void AtmosphereSeries::addAtmosphereE(TimePoint Time, const Atmosphere& Atm) {
        auto [atm, added] = m_Atmospheres.try_emplace(Time, Atm);
        if (!added)
            throw GrapeException(std::format("Atmosphere at {} already exists for this atmosphere series.", timeToUtcString(Time)));
    }

    bool AtmosphereSeries::updateTime(const TimePoint& OldTime, const TimePoint NewTime) noexcept {
        GRAPE_ASSERT(m_Atmospheres.contains(OldTime));
        if (m_Atmospheres.contains(NewTime))
            return false;

        auto node = m_Atmospheres.extract(OldTime);
        node.key() = NewTime;
        m_Atmospheres.insert(std::move(node));
        return true;
    }

    void AtmosphereSeries::deleteAtmosphere(const TimePoint Time) noexcept {
        GRAPE_ASSERT(m_Atmospheres.contains(Time));

        m_Atmospheres.erase(Time);
    }

    void AtmosphereSeries::clear() noexcept {
        m_Atmospheres.clear();
    }

    TEST_CASE("Atmosphere Series Class") {
        AtmosphereSeries atmSeries;

        // Add three atmospheres
        atmSeries.addAtmosphere(utcStringToTime("2000-01-01 00:00:00").value(), Atmosphere(0.0, 0.0));
        atmSeries.addAtmosphere(utcStringToTime("2000-01-01 00:00:02").value(), Atmosphere(5.0, 0.0));
        atmSeries.addAtmosphere(utcStringToTime("2000-01-01 00:00:10").value(), Atmosphere(10.0, 0.0));

        // Existing atmosphere check
        CHECK_EQ(5.0, atmSeries.atmosphere(utcStringToTime("2000-01-01 00:00:02").value()).temperatureDelta());

        // At time point exactly in the middle
        CHECK_EQ(0.0, atmSeries.atmosphere(utcStringToTime("2000-01-01 00:00:01").value()).temperatureDelta());

        // Closer to previous
        CHECK_EQ(5.0, atmSeries.atmosphere(utcStringToTime("2000-01-01 00:00:04").value()).temperatureDelta());

        // Closer to next
        CHECK_EQ(10.0, atmSeries.atmosphere(utcStringToTime("2000-01-01 00:00:08").value()).temperatureDelta());

        // Time point before all atmospheres
        CHECK_EQ(0.0, atmSeries.atmosphere(utcStringToTime("1999-01-01 00:00:00").value()).temperatureDelta());

        // Time point after all atmospheres
        CHECK_EQ(10.0, atmSeries.atmosphere(utcStringToTime("3000-01-01 00:00:00").value()).temperatureDelta());
    }
}
