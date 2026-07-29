#pragma once

#include "Common.h"

namespace DotNetDupe::System
{
    class String
    {
    private:
        const char* m_szData{ "" };

    public:
        String() = default;
        String(const char* szData) : m_szData(szData ? szData : "") {}

        const char* CStr() const { return m_szData; }
        bool IsEmpty() const { return m_szData[0] == '\0'; }
    };
}
