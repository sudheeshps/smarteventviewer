#pragma once

#include "ViewerCommon.h"
#include "System/String.h"
#include "System/SmartPointer.h"
#include "System/Collections/Generic/List.h"

struct llama_model;
struct llama_context;

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;
    template<typename T>
    using List = DotNetDupe::System::Collections::Generic::List<T>;
    class EventRecord;

    class ILlamaModelProvider {
    public:
        virtual ~ILlamaModelProvider() = default;

        virtual void InitBackend() = 0;
        virtual void LoadModel(const String& sModelPath) = 0;
        virtual void CreateContext() = 0;
        virtual String ExecuteInference(const String& sSystemPrompt, const String& sUserQuery, const List<EventRecord>& events) = 0;
        virtual void FreeContextAndModel() = 0;
        virtual bool IsLoaded() const = 0;
        virtual bool IsModelFilePresent(const String& sModelPath) const = 0;
    };

    class SMARTEVENTVIEWER_API DefaultLlamaModelProvider : public ILlamaModelProvider {
    private:
        bool m_bLoaded{ false };
        struct llama_model* m_pModel{ nullptr };
        struct llama_context* m_pCtx{ nullptr };

    public:
        DefaultLlamaModelProvider();
        ~DefaultLlamaModelProvider() override;

        void InitBackend() override;
        void LoadModel(const String& sModelPath) override;
        void CreateContext() override;
        String ExecuteInference(const String& sSystemPrompt, const String& sUserQuery, const List<EventRecord>& events) override;
        void FreeContextAndModel() override;
        bool IsLoaded() const override { return m_bLoaded; }
        bool IsModelFilePresent(const String& sModelPath) const override;
    };
}
