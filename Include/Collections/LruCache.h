#pragma once

#include "Common.h"
#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"
#include "System/Collections/Generic/Dictionary.h"

namespace SmartEventViewer
{
    template <typename TKey, typename TValue>
    class LruCache
    {
    private:
        struct Node
        {
            TKey key;
            TValue value;
            Node* pPrev{ nullptr };
            Node* pNext{ nullptr };

            Node(const TKey& k, const TValue& v) : key(k), value(v) {}
        };

        size_t m_capacity;
        DotNetDupe::System::Collections::Generic::Dictionary<TKey, Node*> m_map;
        Node* m_pHead{ nullptr }; // Most Recently Used (MRU)
        Node* m_pTail{ nullptr }; // Least Recently Used (LRU)
        mutable DotNetDupe::System::Threading::CriticalSection m_cs;

        void RemoveNode(Node* pNode)
        {
            if (pNode == nullptr) return;

            if (pNode->pPrev != nullptr)
            {
                pNode->pPrev->pNext = pNode->pNext;
            }
            else
            {
                m_pHead = pNode->pNext;
            }

            if (pNode->pNext != nullptr)
            {
                pNode->pNext->pPrev = pNode->pPrev;
            }
            else
            {
                m_pTail = pNode->pPrev;
            }

            pNode->pPrev = nullptr;
            pNode->pNext = nullptr;
        }

        void MoveToHead(Node* pNode)
        {
            if (pNode == nullptr || m_pHead == pNode) return;

            RemoveNode(pNode);

            pNode->pNext = m_pHead;
            pNode->pPrev = nullptr;

            if (m_pHead != nullptr)
            {
                m_pHead->pPrev = pNode;
            }

            m_pHead = pNode;

            if (m_pTail == nullptr)
            {
                m_pTail = m_pHead;
            }
        }

        void EvictTail()
        {
            if (m_pTail == nullptr) return;

            Node* pEvict = m_pTail;
            m_map.Remove(pEvict->key);
            RemoveNode(pEvict);

            delete pEvict;
        }

    public:
        explicit LruCache(size_t capacity = 10)
            : m_capacity(capacity < 1 ? 1 : capacity)
        {
        }

        ~LruCache()
        {
            Clear();
        }

        LruCache(const LruCache&) = delete;
        LruCache& operator=(const LruCache&) = delete;

        bool TryGet(const TKey& key, TValue& outValue)
        {
            DotNetDupe::System::Threading::Lock<DotNetDupe::System::Threading::CriticalSection> lock(m_cs);
            Node* pNode = nullptr;
            if (m_map.TryGetValue(key, pNode) && pNode != nullptr)
            {
                outValue = pNode->value;
                MoveToHead(pNode);
                return true;
            }
            return false;
        }

        void Put(const TKey& key, const TValue& value)
        {
            DotNetDupe::System::Threading::Lock<DotNetDupe::System::Threading::CriticalSection> lock(m_cs);
            Node* pNode = nullptr;
            if (m_map.TryGetValue(key, pNode) && pNode != nullptr)
            {
                pNode->value = value;
                MoveToHead(pNode);
                return;
            }

            if (m_map.GetCount() >= m_capacity)
            {
                EvictTail();
            }

            Node* pNewNode = new Node(key, value);
            pNewNode->pNext = m_pHead;
            if (m_pHead != nullptr)
            {
                m_pHead->pPrev = pNewNode;
            }
            m_pHead = pNewNode;
            if (m_pTail == nullptr)
            {
                m_pTail = m_pHead;
            }

            m_map[key] = pNewNode;
        }

        bool Remove(const TKey& key)
        {
            DotNetDupe::System::Threading::Lock<DotNetDupe::System::Threading::CriticalSection> lock(m_cs);
            Node* pNode = nullptr;
            if (m_map.TryGetValue(key, pNode) && pNode != nullptr)
            {
                m_map.Remove(key);
                RemoveNode(pNode);
                delete pNode;
                return true;
            }
            return false;
        }

        void Clear()
        {
            DotNetDupe::System::Threading::Lock<DotNetDupe::System::Threading::CriticalSection> lock(m_cs);
            Node* pCurr = m_pHead;
            while (pCurr != nullptr)
            {
                Node* pNext = pCurr->pNext;
                delete pCurr;
                pCurr = pNext;
            }
            m_pHead = nullptr;
            m_pTail = nullptr;
            m_map.Clear();
        }

        size_t GetCount() const
        {
            DotNetDupe::System::Threading::Lock<DotNetDupe::System::Threading::CriticalSection> lock(m_cs);
            return m_map.GetCount();
        }

        size_t GetCapacity() const
        {
            return m_capacity;
        }
    };
}
