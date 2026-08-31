#pragma once

#include "Ai/ILlamaModelProvider.h"

namespace SmartEventViewer {

    class MockLlamaModelProvider : public ILlamaModelProvider {
    private:
        bool m_bLoaded{ false };
        String m_sSimulatedResponse{};

    public:
        MockLlamaModelProvider()
            : m_sSimulatedResponse("### SIEM Threat Assessment (Simulated)\n- Incident: Anomalous activity detected.\n- Blast Radius: Host subsystem.\n- Remediation: Audit permissions.") {}

        explicit MockLlamaModelProvider(const String& sSimulatedResponse)
            : m_sSimulatedResponse(sSimulatedResponse) {}

        ~MockLlamaModelProvider() override = default;

        void InitBackend() override {}
        void LoadModel(const String& sModelPath) override { (void)sModelPath; m_bLoaded = true; }
        void CreateContext() override { m_bLoaded = true; }

        String ExecuteInference(const String& sSystemPrompt, const String& sUserQuery, const List<EventRecord>& events) override {
            (void)sSystemPrompt; (void)sUserQuery; (void)events;
            return m_sSimulatedResponse;
        }

        void FreeContextAndModel() override { m_bLoaded = false; }
        bool IsLoaded() const override { return m_bLoaded; }
        bool IsModelFilePresent(const String& sModelPath) const override { (void)sModelPath; return true; }
    };
}
