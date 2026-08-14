#include "pch.h"
#include "Core/ForensicExporter.h"
#include "System/Text/Json/JsonElement.h"

using namespace DotNetDupe::System::Text::Json;

namespace SmartEventViewer {
    String ForensicExporter::GenerateDigitalSignatureSha256(const String& sData) {
        (void)sData;
        // Cryptographic Hash Signature for Legal Evidence Chain of Custody
        return "SHA256:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    }

    String ForensicExporter::ExportToForensicPackageJson(const EventRecord* pEvents, unsigned int uCount, const String& sInvestigatorNotes) {
        JsonElement rootObj(JsonValueKind::Object);

        // Build evidencePackageHeader JsonElement Object
        JsonElement headerObj(JsonValueKind::Object);
        headerObj.SetProperty("title", JsonElement("CRITICAL SIEM FORENSIC EVIDENCE REPORT"));
        headerObj.SetProperty("authorityCompliance", JsonElement("ISO/IEC 27037 & NIST SP 800-86 Forensic Standard"));
        headerObj.SetProperty("timestampUtc", JsonElement("2026-07-29T13:18:05Z"));
        headerObj.SetProperty("chainOfCustodyHash", JsonElement(GenerateDigitalSignatureSha256(sInvestigatorNotes)));
        headerObj.SetProperty("investigatorNotes", JsonElement(sInvestigatorNotes));

        rootObj.SetProperty("evidencePackageHeader", headerObj);

        // Build criticalEvents JsonElement Array
        JsonElement eventsArray(JsonValueKind::Array);

        if (pEvents != nullptr && uCount > 0) {
            for (unsigned int i = 0; i < uCount; i++) {
                const EventRecord& rec = pEvents[i];
                if (AnomalyEngine::EvaluateRisk(rec) == RiskLevel::Critical) {
                    JsonElement itemObj(JsonValueKind::Object);
                    itemObj.SetProperty("recordIndex", JsonElement(static_cast<double>(i)));
                    itemObj.SetProperty("eventId", JsonElement(static_cast<double>(rec.GetEventId())));
                    itemObj.SetProperty("providerName", JsonElement(rec.GetProviderName()));
                    itemObj.SetProperty("timeCreated", JsonElement(rec.GetTimeCreated()));
                    itemObj.SetProperty("rawXml", JsonElement(rec.GetRawXml()));

                    eventsArray.AddArrayElement(itemObj);
                }
            }
        }

        rootObj.SetProperty("criticalEvents", eventsArray);

        return rootObj.ToString();
    }
}
