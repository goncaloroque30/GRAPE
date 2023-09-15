// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Base/BaseModels.h"
#include "Route.h"
#include "RouteOutput.h"

namespace GRAPE {
    /**
    * @brief Visitor class to calculate the RouteOutput of a given Route.
    */
    class RouteCalculator : public RouteVisitor {
    public:
        explicit RouteCalculator(const CoordinateSystem& Cs) : m_Cs(Cs) {}

        /**
        * @return Returns #RouteOutput (by move construction).
        */
        RouteOutput calculate(const Route& Rte);

        inline static double s_ArcInterval = 10.0;
        inline static double s_WarnHeadingChange = 90.0;
        inline static double s_WarnRnpRadiusDifference = 10.0;
    private:
        const CoordinateSystem& m_Cs;
        RouteOutput m_Output;
    private:
        void visitArrivalSimple(const RouteArrivalSimple& Rte) override;
        void visitDepartureSimple(const RouteDepartureSimple& Rte) override;
        void visitArrivalVectors(const RouteArrivalVectors& Rte) override;
        void visitDepartureVectors(const RouteDepartureVectors& Rte) override;
        void visitArrivalRnp(const RouteArrivalRnp& Rte) override;
        void visitDepartureRnp(const RouteDepartureRnp& Rte) override;
    };
}
