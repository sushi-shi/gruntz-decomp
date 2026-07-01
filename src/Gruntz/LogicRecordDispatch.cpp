// LogicRecordDispatch.cpp - two trace-discovered __cdecl logic-record state
// dispatchers re-homed from src/Stub/Discovered.cpp (matcher-1, traced under
// CLogicRecord). Each takes a game owner, reaches the embedded logic record at
// owner+0x7c, and dispatches on its state tag (m_1c) to the polymorphic
// sub-record's vtable - except state 0, which lazily constructs the sub-record.
// The two differ only in the sub-record type built by state 0 (size 0x6c ctor
// 0x2c70 vs 0x54 ctor 0x3701). Offsets + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>

// global operator new (engine NAFXCW _RezAlloc @0x1b9b46); external/no-body.
void* operator new(u32 n);

// The polymorphic logic sub-record (m_18). Only the dispatched slots are named;
// the leading slots fix the vtable offsets. External (foreign vtable).
class LogicSubRec {
public:
    virtual void Slot00(); // 0x00
    virtual void Slot04(); // 0x04
    virtual void Slot08(); // 0x08
    virtual void Slot0c(); // 0x0c
    virtual void Slot10(); // 0x10
    virtual void Slot14(); // 0x14
    virtual void Init();   // 0x18  slot 6  (state-0 init)
    virtual void Slot1c(); // 0x1c
    virtual void Slot20(); // 0x20
    virtual void Slot24(); // 0x24
    virtual void Op1e();   // 0x28  slot 10 (state 0x1e)
    virtual void Op1d();   // 0x2c  slot 11 (state 0x1d)
    virtual void Op52();   // 0x30  slot 12 (state 0x52)
    virtual void Op51();   // 0x34  slot 13 (state 0x51)
    virtual void Op50();   // 0x38  slot 14 (state 0x50)
    virtual void Op53();   // 0x3c  slot 15 (state 0x53)
};

struct LogicDispatchRecord {
    char m_pad00[0x18];
    LogicSubRec* m_18; // +0x18 owned sub-record
    u32 m_1c;          // +0x1c state tag (unsigned switch key -> ja/jbe)
};

struct LogicDispatchOwner {
    char m_pad00[0x7c];
    LogicDispatchRecord* m_7c; // +0x7c embedded record
};

// State-0 sub-record types (built lazily). External ctors (reloc-masked).
class LogicSubRecA : public LogicSubRec {
public:
    LogicSubRecA(LogicDispatchOwner* owner); // 0x2c70 (via thunk)
    char m_pad[0x6c - 4];
};
SIZE(LogicSubRecA, 0x6c);

class LogicSubRecB : public LogicSubRec {
public:
    LogicSubRecB(LogicDispatchOwner* owner); // 0x3701 (via thunk)
    char m_pad[0x54 - 4];
};
SIZE(LogicSubRecB, 0x54);

class LogicSubRecC : public LogicSubRec {
public:
    LogicSubRecC(LogicDispatchOwner* owner); // 0x2a04 (via thunk)
    char m_pad[0x54 - 4];
};
SIZE(LogicSubRecC, 0x54);

class LogicSubRecD : public LogicSubRec {
public:
    LogicSubRecD(LogicDispatchOwner* owner); // 0x1348 (via thunk)
    char m_pad[0x68 - 4];
};
SIZE(LogicSubRecD, 0x68);

// The default-case fall-through helper (0x16e4f0, __cdecl, 1 arg). External.
extern "C" void LogicSubDefault_16e4f0(LogicSubRec* sub);

// LogicDispatchA @0x0fb660 - state-0 builds a 0x6c sub-record (ctor 0x2c70).
RVA(0x000fb660, 0xf1)
i32 LogicDispatchA(LogicDispatchOwner* owner) {
    LogicDispatchRecord* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0:
            rec->m_1c = 0x3e8;
            {
                LogicSubRecA* obj = new LogicSubRecA(owner);
                obj->Init();
                rec->m_18 = obj;
            }
            break;
        case 0x1d:
            rec->m_18->Op1d();
            break;
        case 0x1e:
            rec->m_18->Op1e();
            break;
        case 0x50:
            rec->m_18->Op50();
            break;
        case 0x51:
            rec->m_18->Op51();
            break;
        case 0x52:
            rec->m_18->Op52();
            break;
        case 0x53:
            rec->m_18->Op53();
            break;
        case 0x3e8:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_18);
            break;
    }
    return 1;
}

// LogicDispatchC @0x046850 - state-0 builds a 0x54 sub-record (ctor 0x2a04).
RVA(0x00046850, 0xf1)
i32 LogicDispatchC(LogicDispatchOwner* owner) {
    LogicDispatchRecord* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0:
            rec->m_1c = 0x3e8;
            {
                LogicSubRecC* obj = new LogicSubRecC(owner);
                obj->Init();
                rec->m_18 = obj;
            }
            break;
        case 0x1d:
            rec->m_18->Op1d();
            break;
        case 0x1e:
            rec->m_18->Op1e();
            break;
        case 0x50:
            rec->m_18->Op50();
            break;
        case 0x51:
            rec->m_18->Op51();
            break;
        case 0x52:
            rec->m_18->Op52();
            break;
        case 0x53:
            rec->m_18->Op53();
            break;
        case 0x3e8:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_18);
            break;
    }
    return 1;
}

// LogicDispatchD @0x0deb20 - state-0 builds a 0x68 sub-record (ctor 0x1348).
RVA(0x000deb20, 0xf1)
i32 LogicDispatchD(LogicDispatchOwner* owner) {
    LogicDispatchRecord* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0:
            rec->m_1c = 0x3e8;
            {
                LogicSubRecD* obj = new LogicSubRecD(owner);
                obj->Init();
                rec->m_18 = obj;
            }
            break;
        case 0x1d:
            rec->m_18->Op1d();
            break;
        case 0x1e:
            rec->m_18->Op1e();
            break;
        case 0x50:
            rec->m_18->Op50();
            break;
        case 0x51:
            rec->m_18->Op51();
            break;
        case 0x52:
            rec->m_18->Op52();
            break;
        case 0x53:
            rec->m_18->Op53();
            break;
        case 0x3e8:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_18);
            break;
    }
    return 1;
}

// LogicDispatchB @0x10d3d0 - state-0 builds a 0x54 sub-record (ctor 0x3701).
RVA(0x0010d3d0, 0xf1)
i32 LogicDispatchB(LogicDispatchOwner* owner) {
    LogicDispatchRecord* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0:
            rec->m_1c = 0x3e8;
            {
                LogicSubRecB* obj = new LogicSubRecB(owner);
                obj->Init();
                rec->m_18 = obj;
            }
            break;
        case 0x1d:
            rec->m_18->Op1d();
            break;
        case 0x1e:
            rec->m_18->Op1e();
            break;
        case 0x50:
            rec->m_18->Op50();
            break;
        case 0x51:
            rec->m_18->Op51();
            break;
        case 0x52:
            rec->m_18->Op52();
            break;
        case 0x53:
            rec->m_18->Op53();
            break;
        case 0x3e8:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_18);
            break;
    }
    return 1;
}

// H-N misc-Gruntz class-metadata sweep (SIZE).
SIZE_UNKNOWN(LogicDispatchOwner);
SIZE_UNKNOWN(LogicDispatchRecord);
SIZE_UNKNOWN(LogicSubRec);
