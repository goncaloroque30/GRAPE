// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "NoiseSingleEventOutput.h"

namespace GRAPE {
    struct NoiseCumulativeOutput {
        // Constructors & Destructor (Copy, move and delete are default)
        explicit NoiseCumulativeOutput(std::size_t Size, std::size_t NumberAboveCount);

        /**
        * @brief Accumulates single event output into this cumulative output. The MaximumAverage and Exposure values are not in the decibel scale. Call finishAccumulation() to finalize the cumulative output.
        * ASSERT NsOut.size() == Count.size() (And therefore all other vectors).
        * ASSERT NaThresholds.size() == NumberAboveThresholds.size()
        */
        void accumulateSingleEventOutput(const NoiseSingleEventOutput& NsOut, double OpCount, double OpWeight, double Threshold, const std::vector<double>& NaThresholds);

        /**
        * @brief Finishes the accumulation process for MaximumAverage and Exposure metrics. Exposure uses the AveragingTimeConstant.
        */
        void finishAccumulation(double AveragingTimeConstant);

        std::vector<double> Count;
        std::vector<double> CountWeighted;
        std::vector<double> MaximumAbsolute;
        std::vector<double> MaximumAverage;
        std::vector<double> Exposure;

        std::vector<std::vector<double>> NumberAboveThresholds;
    };
}
