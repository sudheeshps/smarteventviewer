#pragma once

#include "ViewerCommon.h"
#include "System/String.h"
#include "System/SmartPointer.h"
#include <vector>

struct llama_model;
struct llama_context;

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    class EventRecord;

    class ILlamaModelProvider
    {
    public:
        virtual ~ILlamaModelProvider() = default;

        virtual bool InitBackend() = 0;
        virtual bool LoadModel(const String& sModelPath) = 0;
        virtual bool CreateContext() = 0;
        virtual String ExecuteInference(const String& sSystemPrompt, const String& sUserQuery, const std::vector<EventRecord>& events) = 0;
        virtual void FreeContextAndModel() = 0;
        virtual bool IsLoaded() const = 0;
    };

    class SMARTEVENTVIEWER_API DefaultLlamaModelProvider : public ILlamaModelProvider
    {
    private:
        bool m_bLoaded{ false };
        struct llama_model* m_pModel{ nullptr };
        struct llama_context* m_pCtx{ nullptr };

    public:
        DefaultLlamaModelProvider();
        ~DefaultLlamaModelProvider() override;

        bool InitBackend() override;
        bool LoadModel(const String& sModelPath) override;
        bool CreateContext() override;
        String ExecuteInference(const String& sSystemPrompt, const String& sUserQuery, const std::vector<EventRecord>& events) override;
        void FreeContextAndModel() override;
        bool IsLoaded() const override { return m_bLoaded; }
    };
}
