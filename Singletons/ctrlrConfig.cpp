#include <math.h>
#include "ctrlrConfig.h"
#include "globals.h"


bool CtrlrConfig::mInstanceFlag = false;
CtrlrConfig* CtrlrConfig::mSingleton = NULL;
CtrlrConfig* CtrlrConfig::GetInstance(int fd, SpecRev specRev)
{
    if(mInstanceFlag == false) {
        mSingleton = new CtrlrConfig(fd, specRev);
        mInstanceFlag = true;
        return mSingleton;
    } else {
        return mSingleton;
    }
}
void CtrlrConfig::KillInstance()
{
    if(mInstanceFlag) {
        mInstanceFlag = false;
        delete mSingleton;
        mSingleton = NULL;
    }
}


CtrlrConfig::CtrlrConfig(int fd, SpecRev specRev) :
    SubjectCtrlrState(ST_DISABLE_COMPLETELY)
{
    mFd = fd;
    if (mFd < 0) {
        LOG_DBG("Object created with a bad FD=%d", fd);
        return;
    }

    mSpecRev = specRev;

    uint64_t tmp;
    gRegisters->Read(CTLSPC_CAP, tmp);
    mRegCAP = (uint32_t)tmp;
}


CtrlrConfig::~CtrlrConfig()
{
    mInstanceFlag = false;
}


bool
CtrlrConfig::GetIrqScheme(enum nvme_irq_type &irq)
{
    //------------------------------------------------------------------------
    // todo Add logic to gather the current IRQ being supported for this device,
    //      rather than just assigning some constant we must ask the device.
    //------------------------------------------------------------------------
    LOG_DBG("todo not implemented yet");
    irq = INT_NONE;
    return true;
}


bool
CtrlrConfig::SetIrqScheme(enum nvme_irq_type newIrq)
{
    if (IsStateEnabled()) {
        LOG_DBG("The NVMe must be disabled in order to change the IRQ scheme");
        return false;
    }

    //------------------------------------------------------------------------
    // todo Add logic to set the current IRQ scheme for this device,
    //      currently IRQ's are not supported by dnvme.
    //------------------------------------------------------------------------
    LOG_DBG("todo SetIrqScheme() not implemented yet");
    newIrq = INT_NONE;  // satisfy compiler complaint
    return true;
}


bool
CtrlrConfig::IsStateEnabled()
{
    uint64_t tmp = 0;
    if (gRegisters->Read(CTLSPC_CSTS, tmp))
        return (tmp & CSTS_RDY);
    return false;
}


bool
CtrlrConfig::SetState(enum nvme_state state)
{
    string toState;

    switch (state) {
    case ST_ENABLE:             toState = "Enabling";               break;
    case ST_DISABLE:            toState = "Disabling";              break;
    case ST_DISABLE_COMPLETELY: toState = "Disabling completely";   break;
    default:
        LOG_DBG("Illegal state detected = %d", state);
        throw exception();
    }

    LOG_NRM("%s the NVME device", toState.c_str());
    if (ioctl(mFd, NVME_IOCTL_DEVICE_STATE, &state) < 0) {
        LOG_ERR("Could not set state, currently %s",
            IsStateEnabled() ? "enabled" : "disabled");
        LOG_NRM("dnvme waits a TO period for CC.RDY to indicate ready" );
        return false;
    }

    // The state of the ctrlr is important to many objects
    Notify(state);

    return true;
}


bool
CtrlrConfig::ReadRegCC(uint32_t &regVal)
{
    uint64_t tmp;
    bool retVal = gRegisters->Read(CTLSPC_CC, tmp);
    regVal  = (uint32_t)tmp;
    return retVal;
}


bool
CtrlrConfig::WriteRegCC(uint32_t regVal)
{
    uint64_t tmp =  regVal;
    bool retVal = gRegisters->Write(CTLSPC_CC, tmp);
    return retVal;
}


bool
CtrlrConfig::GetRegValue(uint8_t &value, uint64_t regMask, uint8_t bitShift)
{
    uint32_t regVal;
    bool retVal = ReadRegCC(regVal);
    value = (uint8_t)((regVal & regMask) >> bitShift);
    return retVal;
}


bool
CtrlrConfig::SetRegValue(uint8_t value, uint8_t valueMask, uint64_t regMask,
    uint8_t bitShift)
{
    if (value & ~valueMask) {
        LOG_DBG("Parameter value is to large: 0x%02X", value);
        return false;
    }

    uint32_t regVal;
    if (ReadRegCC(regVal) == false)
        return false;
    regVal &= ~regMask;
    regVal |= ((uint32_t)value << bitShift);
    return WriteRegCC(regVal);
}



bool
CtrlrConfig::SetIOCQES(uint8_t value)
{
    LOG_NRM("Writing CC.IOCQES = 0x%02X; effectively (2^%d) = %d", value, value,
        (int)pow(2, value));
    return SetRegValue(value, 0x0f, CC_IOCQES, 20);
}


bool
CtrlrConfig::SetIOSQES(uint8_t value)
{
    LOG_NRM("Writing CC.IOSQES = 0x%02X; effectively (2^%d) = %d", value, value,
        (int)pow(2, value));
    return SetRegValue(value, 0x0f, CC_IOSQES, 16);
}


bool
CtrlrConfig::SetSHN(uint8_t value)
{
    LOG_NRM("Writing CC.SHN = 0x%02X", value);
    return SetRegValue(value, 0x03, CC_SHN, 14);
}


bool
CtrlrConfig::SetAMS(uint8_t value)
{
    LOG_NRM("Writing CC.AMS = 0x%02X", value);
    return SetRegValue(value, 0x07, CC_AMS, 11);
}


bool
CtrlrConfig::SetMPS(uint8_t value)
{
    LOG_NRM("Writing CC.MPS = 0x%02X", value);
    return SetRegValue(value, 0x0f, CC_MPS, 7);
}
bool
CtrlrConfig::SetMPS()
{
    switch (sysconf(_SC_PAGESIZE)) {
    case 4096:
        LOG_NRM("Writing CC.MPS for a 4096B page size");
        return SetRegValue(0, 0x0f, CC_MPS, 7);
    default:
        LOG_DBG("Kernel reporting unsupported page size: 0x%08lX",
            sysconf(_SC_PAGESIZE));
        return false;
    }
}


bool
CtrlrConfig::SetCSS(uint8_t value)
{
    LOG_NRM("Writing CC.CSS = 0x%02X", value);
    return SetRegValue(value, 0x07, CC_CSS, 4);
}
