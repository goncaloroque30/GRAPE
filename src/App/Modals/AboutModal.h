// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Modal.h"

namespace GRAPE {
    class AboutModal : public Modal {
    public:
        AboutModal() : Modal("About") {}

        void imGuiDraw() override;
    private:
        std::string m_LegalString;
    private:
        void legalModal();
        void loadLegalString();
        void clearLegalString();
    };
}
