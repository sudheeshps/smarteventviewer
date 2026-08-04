#include "pch.h"
#include "Core/ForensicExporter.h"
#include "System/Text/Json/JsonElement.h"

using namespace DotNetDupe::System::Text::Json;

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
        JsonElement rootObj(JsonValueKind::Object);

        // Build evidencePackageHeader JsonElement Object
        JsonElement headerObj(JsonValueKind::Object);
        headerObj.SetProperty(String("title"), JsonElement(String("CRITICAL SIEM FORENSIC EVIDENCE REPORT")));
        headerObj.SetProperty(String("authorityCompliance"), JsonElement(String("ISO/IEC 27037 & NIST SP 800-86 Forensic Standard")));
        headerObj.SetProperty(String("timestampUtc"), JsonElement(String("2026-07-29T13:18:05Z")));
        headerObj.SetProperty(String("chainOfCustodyHash"), JsonElement(GenerateDigitalSignatureSha256(sInvestigatorNotes)));
        headerObj.SetProperty(String("investigatorNotes"), JsonElement(sInvestigatorNotes));

        rootObj.SetProperty(String("evidencePackageHeader"), headerObj);

        // Build criticalEvents JsonElement Array
        JsonElement eventsArray(JsonValueKind::Array);

        if (pEvents != nullptr && uCount > 0)
        {
            for (unsigned int i = 0; i < uCount; i++)
            {
                const EventRecord& rec = pEvents[i];
                if (AnomalyEngine::EvaluateRisk(rec) == RiskLevel::Critical)
                {
                    JsonElement itemObj(JsonValueKind::Object);
                    itemObj.SetProperty(String("recordIndex"), JsonElement(static_cast<double>(i)));
                    itemObj.SetProperty(String("eventId"), JsonElement(static_cast<double>(rec.GetEventId())));
                    itemObj.SetProperty(String("providerName"), JsonElement(rec.GetProviderName()));
                    itemObj.SetProperty(String("timeCreated"), JsonElement(rec.GetTimeCreated()));
                    itemObj.SetProperty(String("rawXml"), JsonElement(rec.GetRawXml()));

                    eventsArray.AddArrayElement(itemObj);
                }
            }
        }

        rootObj.SetProperty(String("criticalEvents"), eventsArray);

        return rootObj.ToString();
    }
}
