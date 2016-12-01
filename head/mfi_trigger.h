#ifndef _MFI_TRIGGER_HEAD_
#define _MFI_TRIGGER_HEAD_

#include "mfiapi.h"
MfiStatus ConfigTrigger(MfiSession Mfi, MfiInt16 trigline, MfiUInt16 mode, MfiUInt32 condition ,MfiPUInt16 oldmode, MfiPUInt32 oldcondition);
MfiStatus DeleteTrigger(MfiSession Mfi, MfiInt16 trigline);
MfiStatus AssertTrigger (MfiSession Mfi, MfiInt16 trigline);

#endif