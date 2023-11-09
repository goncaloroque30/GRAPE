// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "ReceptorSets.h"

namespace GRAPE {
    void ReceptorGrid::setReferenceLongitude(double RefLongitudeIn) {
        if (!(RefLongitudeIn >= -180.0 && RefLongitudeIn <= 180.0))
            throw GrapeException("Reference longitude must be between -180.0 and 180.0.");
        RefLongitude = RefLongitudeIn;
    }

    void ReceptorGrid::setReferenceLatitude(double RefLatitudeIn) {
        if (!(RefLatitudeIn >= -90.0 && RefLatitudeIn <= 90.0))
            throw GrapeException("Reference latitude must be between -90.0 and 90.0.");
        RefLatitude = RefLatitudeIn;
    }

    void ReceptorGrid::setHorizontalSpacing(double HorizontalSpacingIn) {
        if (!(HorizontalSpacingIn > 0.0))
            throw GrapeException("Horizontal spacing must be higher than 0.0.");
        HorizontalSpacing = HorizontalSpacingIn;
    }

    void ReceptorGrid::setVerticalSpacing(double VerticalSpacingIn) {
        if (!(VerticalSpacingIn > 0.0))
            throw GrapeException("Vertical spacing must be higher than 0.0.");
        VerticalSpacing = VerticalSpacingIn;
    }

    void ReceptorGrid::setHorizontalCount(std::size_t HorizontalCountIn) {
        if (!(HorizontalCountIn >= 1))
            throw GrapeException("Horizontal count must be at least 1.");
        HorizontalCount = HorizontalCountIn;
    }

    void ReceptorGrid::setVerticalCount(std::size_t VerticalCountIn) {
        if (!(VerticalCountIn >= 1))
            throw GrapeException("Vertical count must be at least 1.");
        VerticalCount = VerticalCountIn;
    }

    void ReceptorGrid::setGridRotation(double GridRotationIn) {
        if (!(GridRotationIn >= -180.0 && GridRotationIn <= 180.0))
            throw GrapeException("Grid rotation must be between -180.0 and 180.0.");
        GridRotation = GridRotationIn;
    }

    ReceptorOutput ReceptorGrid::receptorList(const CoordinateSystem& Cs) const {
        ReceptorOutput receptOut(size());

        double lonOrigin = RefLongitude;
        double latOrigin = RefLatitude;

        // Move Origin to bottom left corner if point location is not bottom left
        double gridHdgV = 180.0 + GridRotation; // Down
        double gridHdgH = 270.0 + GridRotation; // Left

        switch (RefLocation)
        {
        case PointLocation::Center:
            {
                auto [lonOriginH, latOriginH] = Cs.point(RefLongitude, RefLatitude, VerticalSpacing * static_cast<double>(VerticalCount) / 2.0, gridHdgV);
                auto [lonOriginO, latOriginO] = Cs.point(lonOriginH, latOriginH, HorizontalSpacing * static_cast<double>(HorizontalCount) / 2.0, gridHdgH);
                lonOrigin = lonOriginO;
                latOrigin = latOriginO;
                break;
            }
        case PointLocation::BottomLeft: break;
        case PointLocation::BottomRight:
            {
                auto [lonOriginO, latOriginO] = Cs.point(RefLongitude, RefLatitude, HorizontalSpacing * static_cast<double>(HorizontalCount), gridHdgH);
                lonOrigin = lonOriginO;
                latOrigin = latOriginO;
                break;
            }
        case PointLocation::TopLeft:
            {
                auto [lonOriginO, latOriginO] = Cs.point(RefLongitude, RefLatitude, VerticalSpacing * static_cast<double>(VerticalCount), gridHdgV);
                lonOrigin = lonOriginO;
                latOrigin = latOriginO;
                break;
            }
        case PointLocation::TopRight:
            {
                auto [lonOriginH, latOriginH] = Cs.point(RefLongitude, RefLatitude, VerticalSpacing * static_cast<double>(VerticalCount), gridHdgV);
                auto [lonOriginO, latOriginO] = Cs.point(lonOriginH, latOriginH, HorizontalSpacing * static_cast<double>(HorizontalCount), gridHdgH);
                lonOrigin = lonOriginO;
                latOrigin = latOriginO;
                break;
            }
        default: GRAPE_ASSERT(false); break;
        }

        // Reset grid headings to be up and right
        gridHdgV = 0.0 + GridRotation; // Up
        gridHdgH = 90.0 + GridRotation; // Right

        // Iterate through points from bottom to top (up direction) and left to right (right direction)
        for (std::size_t i = 0; i < HorizontalCount; i++)
        {
            // Current bottom point
            auto [lonH, latH] = Cs.point(lonOrigin, latOrigin, static_cast<double>(i) * HorizontalSpacing, gridHdgH);

            for (std::size_t j = 0; j < VerticalCount; j++)
            {
                auto [lon, lat] = Cs.point(lonH, latH, static_cast<double>(j) * VerticalSpacing, gridHdgV);
                receptOut.addReceptor(std::format("{},{}", i + 1, j + 1), lon, lat, RefAltitudeMsl);
            }
        }

        return receptOut;
    }

    void ReceptorGrid::accept(ReceptorSetVisitor& Vis) {
        Vis.visitGrid(*this);
    }

    void ReceptorGrid::accept(ReceptorSetVisitor& Vis) const {
        Vis.visitGrid(*this);
    }

    bool ReceptorPoints::addPoint(const std::string& Name) {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(m_Receptors, "New Point") : Name;

        bool added;
        if (m_Receptors.empty())
        {
            auto [pt, addedEmpty] = m_Receptors.add(newName, newName, 0.0, 0.0, 0.0);
            added = addedEmpty;
        }
        else
        {
            auto& [lastName, lastRecept] = *std::prev(m_Receptors.end(), 1);
            auto newRecept = lastRecept;
            newRecept.Name = newName;
            auto [pt, addedFromBack] = m_Receptors.add(newName, newRecept);
            added = addedFromBack;
        }

        return added;
    }

    bool ReceptorPoints::addPoint(const std::string& Name, double Longitude, double Latitude, double AltitudeMsl) {
        GRAPE_ASSERT(Longitude >= -180.0 && Longitude <= 180.0);
        GRAPE_ASSERT(Latitude >= -90.0 && Latitude <= 90.0);

        auto [pt, added] = m_Receptors.add(Name, Name, Longitude, Latitude, AltitudeMsl);
        return added;
    }

    bool ReceptorPoints::addPoint(const Receptor& Recept) {
        auto [pt, added] = m_Receptors.add(Recept.Name, Recept);
        return added;
    }

    void ReceptorPoints::addPointE(const std::string& Name, double Longitude, double Latitude, double AltitudeMsl) {
        if (Name.empty())
            throw GrapeException("Empty name not allowed.");

        if (!(Longitude >= -180.0 && Longitude <= 180.0))
            throw GrapeException("Longitude must be between -180.0 and 180.0.");

        if (!(Latitude >= -90.0 && Latitude <= 90.0))
            throw GrapeException("Latitude must be between -90.0 and 90.0.");

        auto [pt, added] = m_Receptors.add(Name, Name, Longitude, Latitude, AltitudeMsl);
        if (!added)
            throw GrapeException(std::format("The receptor point '{}' already exists in this receptor set.", Name));
    }

    bool ReceptorPoints::deletePoint(const std::string& Name) {
        return m_Receptors.erase(Name);
    }

    void ReceptorPoints::clear() {
        m_Receptors.clear();
    }

    bool ReceptorPoints::updateName(const std::string& ReceptId) {
        GRAPE_ASSERT(m_Receptors.contains(ReceptId));
        auto& recept = m_Receptors.at(ReceptId);

        if (recept.Name.empty())
        {
            Log::dataLogic()->error("Updating receptor point '{}'. Empty name not allowed.", ReceptId, recept.Name);
            recept.Name = ReceptId;
            return false;
        }

        const bool added = m_Receptors.update(ReceptId, recept.Name);
        if (!added)
            Log::dataLogic()->error("Updating receptor point '{}'. The point '{}' already exists in this receptor set.", ReceptId, recept.Name);
        return added;
    }

    ReceptorOutput ReceptorPoints::receptorList(const CoordinateSystem&) const {
        ReceptorOutput receptOut(size());

        for (const auto& recept : m_Receptors | std::views::values)
            receptOut.addReceptor(recept);

        return receptOut;
    }

    void ReceptorPoints::accept(ReceptorSetVisitor& Vis) {
        Vis.visitPoints(*this);
    }

    void ReceptorPoints::accept(ReceptorSetVisitor& Vis) const {
        Vis.visitPoints(*this);
    }
}
