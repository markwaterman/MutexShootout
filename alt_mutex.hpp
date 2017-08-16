// Written by Mark Waterman, and placed in the public domain.
// The author hereby disclaims copyright to this source code.

#pragma once

#if _WIN32

#include <Windows.h>

namespace shootout {

template <DWORD SpinCount = 4000>
class cs_mutex
{
public:
    cs_mutex()
    {
        ::InitializeCriticalSectionAndSpinCount(&cs_, SpinCount);
    }

    ~cs_mutex()
    {
        ::DeleteCriticalSection(&cs_);
    }

    cs_mutex(const cs_mutex&) = delete;
    cs_mutex& operator=(const cs_mutex&) = delete;

    void lock()
    {
        EnterCriticalSection(&cs_);
    }

    void unlock()
    {
        LeaveCriticalSection(&cs_);
        
    }

private:
    CRITICAL_SECTION cs_;
};



class srw_mutex
{
public:
    srw_mutex()
    {
        ::InitializeSRWLock(&srw_);
    }

    ~srw_mutex()
    {
    }

    srw_mutex(const srw_mutex&) = delete;
    srw_mutex& operator=(const srw_mutex&) = delete;

    void lock()
    {
        AcquireSRWLockExclusive(&srw_);
    }

    void unlock()
    {
        ReleaseSRWLockExclusive(&srw_);
    }

private:
    SRWLOCK srw_;
};


}

#endif // _WIN32