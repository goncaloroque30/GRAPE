// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Noise.h"
#include "ReceptorOutput.h"
#include "Base/CoordinateSystem.h"

namespace GRAPE {
    struct ReceptorSetVisitor;

    struct ReceptorSet {
        enum class Type {
            Grid = 0,
            Points
        };
        static constexpr EnumStrings<Type> Types{ "Grid", "Points" };

        // Constructors & Destructor
        virtual ~ReceptorSet() = default;

        // Status Checks
        [[nodiscard]] virtual std::size_t size() const = 0;
        [[nodiscard]] virtual bool empty() const = 0;
        [[nodiscard]] virtual Type type() const = 0;

        // Calculate Output
        [[nodiscard]] virtual ReceptorOutput receptorList(const CoordinateSystem& Cs) const = 0;

        // Visitor pattern
        virtual void accept(ReceptorSetVisitor& Vis) = 0;
        virtual void accept(ReceptorSetVisitor& Vis) const = 0;
    };

    struct ReceptorGrid : ReceptorSet {
        enum class PointLocation {
            Center = 0,
            BottomLeft,
            BottomRight,
            TopLeft,
            TopRight,
        } RefLocation = PointLocation::Center;
        static constexpr EnumStrings<PointLocation> Locations{ "Center", "Bottom Left", "Bottom Right", "Top Left", "Top Right" };

        // Data
        double RefLongitude = 0.0, RefLatitude = 0.0, RefAltitudeMsl = 0.0;
        double HorizontalSpacing = 100.0, VerticalSpacing = 100.0;
        std::size_t HorizontalCount = 10, VerticalCount = 10;
        double GridRotation = 0.0;

        void setReferenceLongitude(double RefLongitudeIn);
        void setReferenceLatitude(double RefLatitudeIn);
        void setHorizontalSpacing(double HorizontalSpacingIn);
        void setVerticalSpacing(double VerticalSpacingIn);
        void setHorizontalCount(std::size_t HorizontalCountIn);
        void setVerticalCount(std::size_t VerticalCountIn);
        void setGridRotation(double GridRotationIn);


        // Status Checks
        [[nodiscard]] std::size_t size() const override { return HorizontalCount * VerticalCount; }
        [[nodiscard]] bool empty() const override { return size() == 0; }
        [[nodiscard]] Type type() const override { return Type::Grid; }

        // Calculate Output
        [[nodiscard]] ReceptorOutput receptorList(const CoordinateSystem& Cs) const override;

        // Visitor pattern
        void accept(ReceptorSetVisitor& Vis) override;
        void accept(ReceptorSetVisitor& Vis) const override;
    };

    class ReceptorPoints : public ReceptorSet {
    public:
        // Constructors & Destructor
        ReceptorPoints() = default;

        // Access Data
        [[nodiscard]] std::size_t size() const override { return m_Receptors.size(); }
        [[nodiscard]] bool empty() const override { return m_Receptors.empty(); }
        [[nodiscard]] Type type() const override { return Type::Points; }

        [[nodiscard]] const auto& points() const { return m_Receptors; }
        [[nodiscard]] auto begin() const { return m_Receptors.begin(); }
        [[nodiscard]] auto end() const { return m_Receptors.end(); }

        [[nodiscard]] auto begin() { return m_Receptors.begin(); }
        [[nodiscard]] auto end() { return m_Receptors.end(); }

        // Change Data
        bool addPoint(const std::string& Name = "");
        bool addPoint(const std::string& Name, double Longitude, double Latitude, double AltitudeMsl);
        bool addPoint(const Receptor& Recept);
        void addPointE(const std::string& Name, double Longitude, double Latitude, double AltitudeMsl);
        bool deletePoint(const std::string& Name);
        void clear();
        bool updateName(const std::string& ReceptId);

        // Status Checks
        [[nodiscard]] bool contains(const std::string& Name) const { return m_Receptors.contains(Name); }

        // Calculate Output
        [[nodiscard]] ReceptorOutput receptorList(const CoordinateSystem& Cs) const override;

        // Visitor pattern
        void accept(ReceptorSetVisitor& Vis) override;
        void accept(ReceptorSetVisitor& Vis) const override;
    private:
        GrapeMap<std::string, Receptor> m_Receptors;
    };

    struct ReceptorSetVisitor {
        virtual void visitGrid(ReceptorGrid& ReceptSet) {}
        virtual void visitPoints(ReceptorPoints& ReceptSet) {}
        virtual void visitGrid(const ReceptorGrid& ReceptSet) {}
        virtual void visitPoints(const ReceptorPoints& ReceptSet) {}
        virtual ~ReceptorSetVisitor() = default;
    };
}
