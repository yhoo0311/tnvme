#ifndef _ALLCTRLREGS_r10b_H_
#define _ALLCTRLREGS_r10b_H_

#include "test.h"


/**
 * -----------------------------------------------------------------------------
 * ----------------Mandatory rules for children to follow-----------------------
 * -----------------------------------------------------------------------------
 * 1) See notes in the header file of the Test base class
 */
class AllCtrlRegs_r10b : public Test
{
public:
    AllCtrlRegs_r10b(int fd);
    virtual ~AllCtrlRegs_r10b();

    virtual AllCtrlRegs_r10b *Clone() const
        { return new AllCtrlRegs_r10b(*this); }


protected:
    virtual bool RunCoreTest();


private:
    ///////////////////////////////////////////////////////////////////////////
    // Adding a member variable? Think carefully, see Test::Clone() hdr comment
    // Adding a member functions is fine.
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Report bit position of val which is not like expectedVal
     * @param val Pass value to search against for inequality
     * @param expectedVal Pass the value to compare against for correctness
     * @return INT_MAX if they are equal, otherwise the bit position that isn't
     */
    int ReportOffendingBitPos(uint64_t val, uint64_t expectedVal);

    /**
     * Validate the specified ctrl'r register RO bits report correct values if
     * and only if they are not vendor specific.
     * @param reg Pass the register to validate
     * @return returns upon success, otherwise throws exception
     */
    void ValidateCtlRegisterROAttribute(CtlSpc reg);

    /**
     * Validate all the registers have default values being reported for
     * the RO bits which are not vendor specific.
     * @return returns upon success, otherwise throws exception
     */
    void ValidateDefaultValues();

    /**
     * Validate all the registers hare RO after attempting to write to them.
     * @return returns upon success, otherwise throws exception
     */
    void ValidateROBitsAfterWriting();
};


#endif
