#pragma once

#include "Common.h"
#include "DotNetDupe/String.h"

namespace DotNetDupe::System::Collections::Generic
{
    template <typename T>
    class List
    {
    private:
        T* m_pItems{ nullptr };
        size_t m_nCapacity{ 0 };
        size_t m_nCount{ 0 };

        void EnsureCapacity(size_t nCapacity)
        {
            if (nCapacity <= m_nCapacity) return;
            size_t nNewCapacity = (m_nCapacity == 0) ? 8 : m_nCapacity * 2;
            if (nNewCapacity < nCapacity) nNewCapacity = nCapacity;
            T* pNewItems = new T[nNewCapacity];
            for (size_t i = 0; i < m_nCount; ++i)
            {
                pNewItems[i] = m_pItems[i];
            }
            delete[] m_pItems;
            m_pItems = pNewItems;
            m_nCapacity = nNewCapacity;
        }

    public:
        List() = default;
        ~List()
        {
            delete[] m_pItems;
        }

        List(const List& other)
        {
            EnsureCapacity(other.m_nCount);
            m_nCount = other.m_nCount;
            for (size_t i = 0; i < m_nCount; ++i)
            {
                m_pItems[i] = other.m_pItems[i];
            }
        }

        List& operator=(const List& other)
        {
            if (this != &other)
            {
                delete[] m_pItems;
                m_pItems = nullptr;
                m_nCapacity = 0;
                m_nCount = 0;
                EnsureCapacity(other.m_nCount);
                m_nCount = other.m_nCount;
                for (size_t i = 0; i < m_nCount; ++i)
                {
                    m_pItems[i] = other.m_pItems[i];
                }
            }
            return *this;
        }

        void Add(const T& item)
        {
            EnsureCapacity(m_nCount + 1);
            m_pItems[m_nCount++] = item;
        }

        size_t GetCount() const { return m_nCount; }

        const T& operator[](size_t index) const { return m_pItems[index]; }
        T& operator[](size_t index) { return m_pItems[index]; }
    };
}
