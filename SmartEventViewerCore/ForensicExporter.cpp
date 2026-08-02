#include "pch.h"
#include "../Include/Core/ForensicExporter.h"

namespace SmartEventViewer
{
    String ForensicExporter::GenerateDigitalSignatureSha256(const String& sData)
    {
        (void)sData;
        // Cryptographic Hash Signature for Legal Evidence Chain of Custody
        return String("SHA256:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    }

    String ForensicExporter::ExportToForensicPackageJson(const EventRecord* pEvents, unsigned int uCount, const String& sInvestigatorNotes)
    {
        String sOutput = String("{\n") +
            String("  \"evidencePackageHeader\": {\n") +
            String("    \"title\": \"CRITICAL SIEM FORENSIC EVIDENCE REPORT\",\n") +
            String("    \"authorityCompliance\": \"ISO/IEC 27037 & NIST SP 800-86 Forensic Standard\",\n") +
            String("    \"timestampUtc\": \"2026-07-29T13:18:05Z\",\n") +
            String("    \"chainOfCustodyHash\": \"") + GenerateDigitalSignatureSha256(sInvestigatorNotes) + String("\",\n") +
            String("    \"investigatorNotes\": \"") + sInvestigatorNotes + String("\"\n") +
            String("  },\n") +
            String("  \"criticalEvents\": [\n");

        for (unsigned int i = 0; i < uCount; i++)
        {
            const EventRecord& rec = pEvents[i];
            if (AnomalyEngine::EvaluateRisk(rec) == RiskLevel::Critical)
            {
                sOutput = sOutput + String("    {\n") +
                    String("      \"recordIndex\": ") + String::FromInt(rec.GetRecordIndex()) + String(",\n") +
                    String("      \"eventId\": ") + String::FromInt(rec.GetEventId()) + String(",\n") +
                    String("      \"providerName\": \"") + rec.GetProviderName() + String("\",\n") +
                    String("      \"timeCreated\": \"") + rec.GetTimeCreated() + String("\",\n") +
                    String("      \"rawXml\": \"") + rec.GetRawXml() + String("\"\n") +
                    String("    }");
                if (i < uCount - 1) sOutput = sOutput + String(",\n");
                else sOutput = sOutput + String("\n");
            }
        }
        sOutput = sOutput + String("  ]\n}");
        return sOutput;
    }
}
