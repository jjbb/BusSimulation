#ifndef _MFI_ATTR_DEAL_HEADER_
#define _MFI_ATTR_DEAL_HEADER_

#include "mfi_attribute.h"

MfiStatus SetAttr_Data_Tlen(MfiObject Mfi, MfiAttr attrName, MfiAttrState attrValue, MfiPAttribute attr, MfiInt32 attrindex, MfiUInt32 attr_num, MfiUInt16 refresh);
MfiStatus SetAttr_Msg_Tlen(MfiObject Mfi, MfiAttr attrName, MfiAttrState attrValue, MfiPAttribute attr, MfiInt32 attrindex, MfiUInt32 attr_num, MfiUInt16 refresh);
MfiStatus SetAttr_Msg_FILTER(MfiObject Mfi, MfiAttr attrName, MfiAttrState attrValue, MfiPAttribute attr, MfiInt32 attrindex, MfiUInt32 attr_num, MfiUInt16 refresh);
MfiStatus GetAttr_Msg_FILTER0(MfiObject Mfi, MfiPAttribute attr, MfiUInt32 attr_num, MfiPBuf buf, MfiUInt32 retCnt);
MfiStatus GetAttr_Msg_FILTER1(MfiObject Mfi, MfiPAttribute attr, MfiUInt32 attr_num, MfiPBuf buf, MfiUInt32 retCnt);
MfiStatus SetAttr_Msg_Twin_cfg(MfiObject Mfi, MfiAttr attrName, MfiAttrState attrValue, MfiPAttribute attr, MfiInt32 attrindex, MfiUInt32 attr_num, MfiUInt16 refresh);
MfiStatus SetAttr_Data_Twin_cfg(MfiObject Mfi, MfiAttr attrName, MfiAttrState attrValue, MfiPAttribute attr, MfiInt32 attrindex, MfiUInt32 attr_num, MfiUInt16 refresh);


#endif
