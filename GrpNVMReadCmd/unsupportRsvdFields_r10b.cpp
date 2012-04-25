/*
 * Copyright (c) 2011, Intel Corporation.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "unsupportRsvdFields_r10b.h"
#include "globals.h"
#include "grpDefs.h"
#include "../Queues/iocq.h"
#include "../Queues/iosq.h"
#include "../Utils/io.h"

namespace GrpNVMReadCmd {


UnsupportRsvdFields_r10b::UnsupportRsvdFields_r10b(int fd, string mGrpName,
    string mTestName, ErrorRegs errRegs) :
    Test(fd, mGrpName, mTestName, SPECREV_10b, errRegs)
{
    // 63 chars allowed:     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    mTestDesc.SetCompliance("revision 1.0b, section 6");
    mTestDesc.SetShort(     "Set unsupported/rsvd fields in cmd");
    // No string size limit for the long description
    mTestDesc.SetLong(
        "Search for 1 of the following namspcs to run test. Find 1st bare "
        "namspc, or find 1st meta namspc, or find 1st E2E namspc. Unsupported "
        "DW's and rsvd fields are treated identical, the recipient shall not "
        "check their value. Issue a read cmd requesting 1 block and approp "
        "supporting meta/E2E if necessary from the selected namspc at LBA 0, "
        "expect success. Issue same cmd setting all unsupported/rsvd fields, "
        "expect success. Set: DW0_b15:10, DW2, DW3, DW12_b25:16, DW13_b31:8");
}


UnsupportRsvdFields_r10b::~UnsupportRsvdFields_r10b()
{
    ///////////////////////////////////////////////////////////////////////////
    // Allocations taken from the heap and not under the control of the
    // RsrcMngr need to be freed/deleted here.
    ///////////////////////////////////////////////////////////////////////////
}


UnsupportRsvdFields_r10b::
UnsupportRsvdFields_r10b(const UnsupportRsvdFields_r10b &other) : Test(other)
{
    ///////////////////////////////////////////////////////////////////////////
    // All pointers in this object must be NULL, never allow shallow or deep
    // copies, see Test::Clone() header comment.
    ///////////////////////////////////////////////////////////////////////////
}


UnsupportRsvdFields_r10b &
UnsupportRsvdFields_r10b::operator=(const UnsupportRsvdFields_r10b &other)
{
    ///////////////////////////////////////////////////////////////////////////
    // All pointers in this object must be NULL, never allow shallow or deep
    // copies, see Test::Clone() header comment.
    ///////////////////////////////////////////////////////////////////////////
    Test::operator=(other);
    return *this;
}


void
UnsupportRsvdFields_r10b::RunCoreTest()
{
    /** \verbatim
     * Assumptions:
     * 1) Test CreateResources_r10b has run prior.
     * \endverbatim
     */

    // Lookup objs which were created in a prior test within group
    SharedIOSQPtr iosq = CAST_TO_IOSQ(gRsrcMngr->GetObj(IOSQ_GROUP_ID));
    SharedIOCQPtr iocq = CAST_TO_IOCQ(gRsrcMngr->GetObj(IOCQ_GROUP_ID));

    SharedReadPtr readCmd = CreateCmd();
    IO::SendAndReapCmd(mGrpName, mTestName, DEFAULT_CMD_WAIT_ms, iosq, iocq,
        readCmd, "none.set", true);

    LOG_NRM("Set all cmd's rsvd bits");
    uint32_t work = readCmd->GetDword(0);
    work |= 0x0000fc00;      // Set DW0_b15:10 bits
    readCmd->SetDword(work, 0);

    readCmd->SetDword(0xffff, 2);
    readCmd->SetDword(0xffff, 3);

    work = readCmd->GetDword(12);
    work |= 0x03ff0000;      // Set DW12_b25:16 bits
    readCmd->SetDword(work, 12);

    work = readCmd->GetDword(13);
    work |= 0xffffff00;     // Set DW13_b31:8 bits
    readCmd->SetDword(work, 13);

    IO::SendAndReapCmd(mGrpName, mTestName, DEFAULT_CMD_WAIT_ms, iosq, iocq,
        readCmd, "all.set", true);
}


SharedReadPtr
UnsupportRsvdFields_r10b::CreateCmd()
{
    Informative::Namspc namspcData = gInformative->Get1stBareMetaE2E();
    if (namspcData.type != Informative::NS_BARE) {
        LBAFormat lbaFormat = namspcData.idCmdNamspc->GetLBAFormat();
        if (gRsrcMngr->SetMetaAllocSize(lbaFormat.MS) == false)
            throw FrmwkEx(HERE);
    }
    LOG_NRM("Processing read cmd using namspc id %d", namspcData.id);

    ConstSharedIdentifyPtr namSpcPtr = namspcData.idCmdNamspc;
    uint64_t lbaDataSize = namSpcPtr->GetLBADataSize();;
    SharedMemBufferPtr dataPat = SharedMemBufferPtr(new MemBuffer());
    dataPat->Init(lbaDataSize);

    SharedReadPtr readCmd = SharedReadPtr(new Read());
    send_64b_bitmask prpBitmask = (send_64b_bitmask)(MASK_PRP1_PAGE
        | MASK_PRP2_PAGE | MASK_PRP2_LIST);

    if (namspcData.type == Informative::NS_META) {
        readCmd->AllocMetaBuffer();
    } else if (namspcData.type == Informative::NS_E2E) {
        readCmd->AllocMetaBuffer();
        LOG_ERR("Deferring E2E namspc work to the future");
        throw FrmwkEx(HERE, "Need to add CRC's to correlate to buf pattern");
    }

    readCmd->SetPrpBuffer(prpBitmask, dataPat);
    readCmd->SetNSID(namspcData.id);
    readCmd->SetNLB(0);
    return readCmd;
}


}   // namespace

