#pragma once

#include "ViewerCommon.h"
#include "System/EventArgs.h"
#include "System/String.h"
#include "System/SmartPointer.h"
#include "Dto/AnalysisDtos.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;
    template <typename T>
    using SmartPointer = DotNetDupe::System::SmartPointer<T>;

    class SMARTEVENTVIEWER_API AnalysisStateChangedEventArgs : public DotNetDupe::System::EventArgs {
    private:
        String m_sTaskId;
        String m_sPreviousState;
        String m_sNewState;
        String m_sStatus;
        String m_sProgressMessage;
        AnalyzeResponseDto m_responseDto;
        bool m_bIsTerminal;

    public:
        AnalysisStateChangedEventArgs(
            const String& sTaskId,
            const String& sPreviousState,
            const String& sNewState,
            const String& sStatus,
            const String& sProgressMessage,
            const AnalyzeResponseDto& responseDto = AnalyzeResponseDto(),
            bool bIsTerminal = false)
            : m_sTaskId(sTaskId),
              m_sPreviousState(sPreviousState),
              m_sNewState(sNewState),
              m_sStatus(sStatus),
              m_sProgressMessage(sProgressMessage),
              m_responseDto(responseDto),
              m_bIsTerminal(bIsTerminal) {}

        String GetTaskId() const { return m_sTaskId; }
        String GetPreviousState() const { return m_sPreviousState; }
        String GetNewState() const { return m_sNewState; }
        String GetStatus() const { return m_sStatus; }
        String GetProgressMessage() const { return m_sProgressMessage; }
        const AnalyzeResponseDto& GetResponse() const { return m_responseDto; }
        bool IsTerminal() const { return m_bIsTerminal; }
    };

    class SMARTEVENTVIEWER_API AnalysisProgressChangedEventArgs : public DotNetDupe::System::EventArgs {
    private:
        String m_sTaskId;
        double m_dProgressPercentage;
        String m_sProgressMessage;
        SmartPointer<DotNetDupe::System::EventArgs> m_spDetails{ nullptr };

    public:
        AnalysisProgressChangedEventArgs(
            const String& sTaskId,
            double dProgressPercentage,
            const String& sProgressMessage,
            const SmartPointer<DotNetDupe::System::EventArgs>& spDetails = nullptr)
            : m_sTaskId(sTaskId),
              m_dProgressPercentage(dProgressPercentage),
              m_sProgressMessage(sProgressMessage),
              m_spDetails(spDetails) {}

        String GetTaskId() const { return m_sTaskId; }
        double GetProgressPercentage() const { return m_dProgressPercentage; }
        String GetProgressMessage() const { return m_sProgressMessage; }

        bool HasDetails() const { return !m_spDetails.IsNull(); }
        SmartPointer<DotNetDupe::System::EventArgs> GetDetails() const { return m_spDetails; }

        template <typename T>
        SmartPointer<T> GetDetailsAs() const {
            return m_spDetails.DynamicCast<T>();
        }
    };
}
