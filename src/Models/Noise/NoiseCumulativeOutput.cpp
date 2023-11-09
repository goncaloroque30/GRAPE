// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "NoiseCumulativeOutput.h"

namespace GRAPE {
    NoiseCumulativeOutput::NoiseCumulativeOutput(std::size_t Size, std::size_t NumberAboveCount) : Count(Size, 0.0), CountWeighted(Size, 0.0), MaximumAbsolute(Size, 0.0), MaximumAverage(Size, 0.0), Exposure(Size, 0.0) {
        for (std::size_t i = 0; i < NumberAboveCount; ++i)
            NumberAboveThresholds.emplace_back(Size, 0.0);
    }

    void NoiseCumulativeOutput::accumulateSingleEventOutput(const NoiseSingleEventOutput& NsOut, double OpCount, double OpWeight, double Threshold, const std::vector<double>& NaThresholds) {
        GRAPE_ASSERT(NsOut.size() == Count.size());
        GRAPE_ASSERT(NaThresholds.size() == NumberAboveThresholds.size());

        const double weightedCount = OpCount * OpWeight;
        if (weightedCount <= Constants::Precision)
            return;

        std::ranges::transform(Count, NsOut.lamax(), Count.begin(), [&](double CurrentNumber, double NewLaMax) { return NewLaMax >= Threshold ? CurrentNumber + OpCount : CurrentNumber; });
        std::ranges::transform(CountWeighted, NsOut.lamax(), CountWeighted.begin(), [&](double CurrentWeight, double NewLaMax) { return NewLaMax >= Threshold ? CurrentWeight + weightedCount : CurrentWeight; });

        std::ranges::transform(MaximumAbsolute, NsOut.lamax(), MaximumAbsolute.begin(), [&](double Current, double New) { return New >= Threshold ? std::max(Current, New) : Current; });
        std::ranges::transform(MaximumAverage, NsOut.lamax(), MaximumAverage.begin(), [&](double Current, double New) { return New >= Threshold ? Current + weightedCount * std::pow(10.0, New / 10.0) : Current; });

        std::ranges::transform(Exposure, NsOut, Exposure.begin(), [&](double Current, const std::pair<double, double>& NsVals) {
            const auto& [lamax, sel] = NsVals;
            return lamax >= Threshold ? Current + weightedCount * std::pow(10.0, sel / 10.0) : Current;
            });

        for (std::size_t i = 0; i < NaThresholds.size(); ++i)
        {
            const double threshold = NaThresholds.at(i);
            auto& outNat = NumberAboveThresholds.at(i);
            std::ranges::transform(outNat, NsOut.lamax(), outNat.begin(), [&](double CurrentCount, double LaMax) {
                return LaMax >= threshold && LaMax > Threshold ? CurrentCount + OpCount : CurrentCount;
                });
        }
    }

    void NoiseCumulativeOutput::finishAccumulation(double AveragingTimeConstant) {
        std::ranges::transform(MaximumAverage, Count, MaximumAverage.begin(), [&](double Value, double Count) { return Value < Constants::Precision ? 0.0 : 10.0 * (std::log10(Value) - std::log10(Count)); });

        std::ranges::transform(Exposure, Exposure.begin(), [&](double Value) { return Value < Constants::Precision ? 0.0 : 10.0 * std::log10(Value) - AveragingTimeConstant; });
    }
}
