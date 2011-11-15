#include "queue.h"
#include "../Utils/buffers.h"


Queue::Queue() : Trackable(Trackable::OBJTYPE_FENCE)
{
    // This constructor will throw
}


Queue::Queue(int fd, Trackable::ObjType objBeingCreated) :
    Trackable(objBeingCreated)
{
    mFd = fd;
    if (mFd < 0) {
        LOG_DBG("Object created with a bad FD=%d", fd);
        return;
    }

    mQId = 0;
    mEntrySize = 0;
    mNumEntries = 0;
    mDiscontigBuf = MemBuffer::NullMemBufferPtr;
    mContigBuf = NULL;
}


Queue::~Queue()
{
    // Children are responsible for delete Q memory
}


bool
Queue::GetIsContig()
{
    if ((mContigBuf == NULL) &&
        (mDiscontigBuf == MemBuffer::NullMemBufferPtr)) {

        LOG_DBG("Detected an uninitialized Q");
        throw exception();
    } else if ((mContigBuf != NULL) &&
        (mDiscontigBuf != MemBuffer::NullMemBufferPtr)) {

        LOG_DBG("Detected an illegally configured Q");
        throw exception();
    }
    return (mContigBuf != NULL);
}


uint8_t const *
Queue::GetQBuffer()
{
    if (GetIsContig())
        return mContigBuf;
    else
        return mDiscontigBuf->GetBuffer();
}


void
Queue::Init(uint16_t qId, uint16_t entrySize, uint16_t numEntries)
{
    if (mDiscontigBuf != MemBuffer::NullMemBufferPtr) {
        LOG_DBG("Obj already init'd for discontiguous parameters");
        throw exception();
    } else if (mContigBuf != NULL) {
        LOG_DBG("Obj does not support re-allocations, create a new obj");
        throw exception();
    }

    mQId = qId;
    mEntrySize = entrySize;
    mNumEntries = numEntries;
}


void
Queue::Log(uint32_t bufOffset, unsigned long length)
{
    Buffers::Log(GetQBuffer(), bufOffset, length, GetQSize(), "Queue");
}


void
Queue::Dump(LogFilename filename, string fileHdr)
{
    Buffers::Dump(filename, GetQBuffer(), 0, ULONG_MAX, GetQSize(), fileHdr);
}