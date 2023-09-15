// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    // Interface class for a drawable panel
    class Panel {
    public:
        // Constructors & Destructor
        explicit Panel(std::string_view Name = "Default") : m_Name(Name), m_Open(true) {}
        virtual ~Panel() = default;

        // Access Data
        [[nodiscard]] const std::string& name() const { return m_Name; }

        void open() { m_Open = true; }
        void close() { m_Open = false; }
        void toggle() { m_Open = !m_Open; }

        // Status checks
        [[nodiscard]] bool isOpen() const { return m_Open; }

        // Interface		
        virtual void reset() {}
        virtual void onPerformanceRunStart() {}
        virtual void onNoiseRunStart() {}
        virtual void imGuiDraw() = 0;
    protected:
        std::string m_Name;
        bool m_Open;
    };
}
