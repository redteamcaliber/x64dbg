#ifndef _THREADING_H
#define _THREADING_H

#include "_global.h"

enum WAIT_ID
{
    WAITID_RUN,
    WAITID_STOP,
    WAITID_LAST
};

//functions
void waitclear();
void wait(WAIT_ID id);
void lock(WAIT_ID id);
void unlock(WAIT_ID id);
bool waitislocked(WAIT_ID id);
void waitinitialize();
void waitdeinitialize();

//
// THREAD SYNCHRONIZATION
//
// Win Vista and newer: (Faster) SRW locks used
// Win 2003 and older:  (Slower) Critical sections used
//
#define EXCLUSIVE_ACQUIRE(Index)    SectionLocker<Index, false> __ThreadLock
#define EXCLUSIVE_REACQUIRE()       __ThreadLock.Lock()
#define EXCLUSIVE_RELEASE()         __ThreadLock.Unlock()

#define SHARED_ACQUIRE(Index)       SectionLocker<Index, true> __SThreadLock
#define SHARED_REACQUIRE()          __SThreadLock.Lock()
#define SHARED_RELEASE()            __SThreadLock.Unlock()

enum SectionLock
{
    LockMemoryPages,
    LockVariables,
    LockModules,
    LockComments,
    LockLabels,
    LockBookmarks,
    LockFunctions,
    LockLoops,
    LockBreakpoints,
    LockPatches,
    LockThreads,
    LockSym,
    LockCmdLine,
    LockDatabase,
    LockPluginList,
    LockPluginCallbackList,
    LockPluginCommandList,
    LockPluginMenuList,
    LockSehCache,
    LockMnemonicHelp,
    LockTraceRecord,
    LockDebugStartStop,
    LockArguments,

    // Number of elements in this enumeration. Must always be the last
    // index.
    LockLast
};

class SectionLockerGlobal
{
    template<SectionLock LockIndex, bool Shared>
    friend class SectionLocker;

public:
    static void Initialize();
    static void Deinitialize();

private:
    static inline void AcquireLock(SectionLock LockIndex, bool Shared)
    {
        if(m_SRWLocks)
        {
            if(Shared)
                m_AcquireSRWLockShared(&m_srwLocks[LockIndex]);
            else
                m_AcquireSRWLockExclusive(&m_srwLocks[LockIndex]);
        }
        else
            EnterCriticalSection(&m_crLocks[LockIndex]);
    }

    static inline void ReleaseLock(SectionLock LockIndex, bool Shared)
    {
        if(m_SRWLocks)
        {
            if(Shared)
                m_ReleaseSRWLockShared(&m_srwLocks[LockIndex]);
            else
                m_ReleaseSRWLockExclusive(&m_srwLocks[LockIndex]);
        }
        else
            LeaveCriticalSection(&m_crLocks[LockIndex]);
    }

    typedef void (WINAPI* SRWLOCKFUNCTION)(PSRWLOCK SWRLock);

    static bool m_Initialized;
    static bool m_SRWLocks;
    static SRWLOCK m_srwLocks[SectionLock::LockLast];
    static CRITICAL_SECTION m_crLocks[SectionLock::LockLast];
    static SRWLOCKFUNCTION m_InitializeSRWLock;
    static SRWLOCKFUNCTION m_AcquireSRWLockShared;
    static SRWLOCKFUNCTION m_AcquireSRWLockExclusive;
    static SRWLOCKFUNCTION m_ReleaseSRWLockShared;
    static SRWLOCKFUNCTION m_ReleaseSRWLockExclusive;
};

template<SectionLock LockIndex, bool Shared>
class SectionLocker
{
public:
    SectionLocker()
    {
        m_LockCount = 0;
        Lock();
    }

    ~SectionLocker()
    {
        if(m_LockCount > 0)
            Unlock();

#ifdef _DEBUG
        // Assert that the lock count is zero on destructor
        if(m_LockCount > 0)
            __debugbreak();
#endif
    }

    inline void Lock()
    {
        Internal::AcquireLock(LockIndex, Shared);

        m_LockCount++;
    }

    inline void Unlock()
    {
        m_LockCount--;

        Internal::ReleaseLock(LockIndex, Shared);
    }

protected:
    BYTE m_LockCount;

private:
    using Internal = SectionLockerGlobal;
};

#endif // _THREADING_H
