#include "mfiapi.h"
#include "mfi_rsrc_manager.h"
#include "mfi_operations.h"
#include <pthread.h>
#include <errno.h>

MfiStatus MfiOpen(MfiSession sesn, MfiRsrc name, MfiPSession Mfi)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
	if((sesn>=RM_MIN_SESSION_ID&&sesn<=RM_MAX_SESSION_ID)&&(sesn==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.Open==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.Open(name,Mfi);
	}
	 
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID
}

MfiStatus MfiClose(MfiObject Mfi)
{
	MfiPEventInfo event_info;
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
	/*Rm*/
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.Close==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.Close(Mfi);
	}
	
	/*rsrc session*/
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.Close==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			SesStatusFree(&(RsrcManager->session_list[Mfi]));              //此处必须先释放读锁，否则会死锁
			status=RsrcManager->session_list[Mfi].rsrc_opt.Close(Mfi);
			return status;
		}
	}
	
	/*event id*/
	if(Mfi>=EVENT_MIN_ID){
		event_info=Event_Find(&(RsrcManager->event_closing),Mfi);
		
		if(event_info==NULL)
			return MFI_ERROR_INV_OBJECT;
			
		return EventClose(event_info);
	}
	
	/*findlist id*/
	if(Mfi>=FINDLIST_MIN_ID && Mfi<=FINDLIST_MAX_ID){
		FindListClose(Mfi);
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的对象ID
}

MfiStatus MfiFindRsrc(MfiSession sesn, MfiString expr, MfiPFindList Mfi, MfiPUInt32 retCnt, MfiChar desc[])
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((sesn>=RM_MIN_SESSION_ID&&sesn<=RM_MAX_SESSION_ID)&&(sesn==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.FindRsrc==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.FindRsrc(expr,Mfi,retCnt,desc);
	}
	
	if(sesn>=0&&sesn<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[sesn]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[sesn].rsrc_opt.FindRsrc==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[sesn]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[sesn].rsrc_opt.FindRsrc(expr,Mfi,retCnt,desc);
			SesStatusFree(&(RsrcManager->session_list[sesn]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID	
}
          
MfiStatus MfiFindNext(MfiFindList Mfi, MfiChar desc[])
{
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if(Mfi>=FINDLIST_MIN_ID && Mfi<=FINDLIST_MAX_ID){
		return RsrcManager->rsrc_opt.FindNext(Mfi, desc);
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID		
}
                                    
MfiStatus MfiSetAttribute(MfiObject Mfi, MfiAttr attrName, MfiAttrState attrValue, MfiUInt16 refresh)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	/*Rm*/
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.SetAttribute==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.SetAttribute(Mfi,attrName,attrValue,refresh);
	}
	
	/*rsrc session*/
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.SetAttribute==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.SetAttribute(Mfi,attrName,attrValue,refresh);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	/*event and findlist id*/
	if((Mfi>=EVENT_MIN_ID)||(Mfi>=FINDLIST_MIN_ID && Mfi<=FINDLIST_MAX_ID)){
		return MFI_ERROR_NSUP_OPER;
	}	
	
	return MFI_ERROR_INV_OBJECT;   //无效的对象ID
}

MfiStatus MfiGetAttribute(MfiObject Mfi, MfiAttr attrName, void * attrValue, MfiUInt16 refresh)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	/*Rm*/
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.GetAttribute==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.GetAttribute(Mfi,attrName,attrValue,refresh);
	}
	
	/*rsrc session*/
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.GetAttribute==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.GetAttribute(Mfi,attrName,attrValue,refresh);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	/*event and findlist id*/
	if(Mfi>=EVENT_MIN_ID){
		return RsrcGetAttribute(Mfi, attrName, attrValue, refresh);
	}	
	
	/*findlist id*/
	if(Mfi>=FINDLIST_MIN_ID && Mfi<=FINDLIST_MAX_ID){
		return MFI_ERROR_NSUP_OPER;
	}	
	
	return MFI_ERROR_INV_OBJECT;   //无效的对象ID	
}

MfiStatus MfiEnableEvent(MfiSession Mfi, MfiEventType eventType, MfiUInt16 mechanism)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.EnableEvent==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.EnableEvent(Mfi, eventType, mechanism);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.EnableEvent==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.EnableEvent(Mfi, eventType, mechanism);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID		
}

MfiStatus MfiDisableEvent(MfiSession Mfi, MfiEventType eventType)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.DisableEvent==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.DisableEvent(Mfi, eventType);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.DisableEvent==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.DisableEvent(Mfi, eventType);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID			
}

MfiStatus MfiWaitOnEvent(MfiSession Mfi, MfiEventType inEventType, MfiUInt32 timeout, MfiPEventType outEventType, MfiPEvent outContext)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.WaitOnEvent==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.WaitOnEvent(Mfi, inEventType, timeout, outEventType, outContext);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.WaitOnEvent==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.WaitOnEvent(Mfi, inEventType, timeout, outEventType, outContext);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID		
}

MfiStatus MfiDiscardEvents(MfiSession Mfi, MfiEventType eventType, MfiUInt16 mechanism)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.DiscardEvents==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.DiscardEvents(Mfi, eventType, mechanism);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.DiscardEvents==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.DiscardEvents(Mfi, eventType, mechanism);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID				
}

MfiStatus MfiInstallHandler(MfiSession Mfi, MfiEventType eventType, MfiHndlr handler, MfiAddr userHandle)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.InstallHandler==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.InstallHandler(Mfi, eventType, handler, userHandle);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.InstallHandler==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.InstallHandler(Mfi, eventType, handler, userHandle);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID			
}

MfiStatus MfiUninstallHandler(MfiSession Mfi, MfiEventType eventType, MfiHndlr handler, MfiAddr userHandle)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.UninstallHandler==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.UninstallHandler(Mfi, eventType, handler, userHandle);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.UninstallHandler==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.UninstallHandler(Mfi, eventType, handler, userHandle);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID			
}

MfiStatus MfiReadMsg(MfiSession Mfi, MfiPUInt32 msgtype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.ReadMsg==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.ReadMsg(Mfi, msgtype, bufp, retCnt, timeout);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.ReadMsg==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.ReadMsg(Mfi, msgtype, bufp, retCnt, timeout);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID	
}

MfiStatus MfiWriteMsg(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgtype, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.WriteMsg==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.WriteMsg(Mfi, priorty, msgtype, buf, retCnt, timeout, sendtype);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.WriteMsg==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.WriteMsg(Mfi, priorty, msgtype, buf, retCnt, timeout, sendtype);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID	
}


MfiStatus MfiReadMsgAsync(MfiSession Mfi, MfiPUInt32 msgtype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.ReadMsgAsync==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.ReadMsgAsync(Mfi, msgtype, bufp, retCnt, jobId);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.ReadMsgAsync==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.ReadMsgAsync(Mfi, msgtype, bufp, retCnt, jobId);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID				
}

MfiStatus MfiWriteMsgAsync (MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgtype, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.WriteMsgAsync==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.WriteMsgAsync(Mfi, priorty, msgtype, buf, retCnt, jobId);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.WriteMsgAsync==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.WriteMsgAsync(Mfi, priorty, msgtype, buf, retCnt, jobId);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID				
}

MfiStatus MfiReadData(MfiSession Mfi, MfiPUInt32 datatype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.ReadData==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.ReadData(Mfi, datatype, bufp, retCnt, timeout);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.ReadData==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.ReadData(Mfi, datatype, bufp, retCnt, timeout);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID	
}

MfiStatus MfiWriteData(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 datatype, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.WriteData==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.WriteData(Mfi, priorty, datatype, buf, retCnt, timeout, sendtype);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.WriteData==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.WriteData(Mfi, priorty, datatype, buf, retCnt, timeout, sendtype);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID	
}


MfiStatus MfiReadDataAsync(MfiSession Mfi, MfiPUInt32 datatype, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.ReadDataAsync==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.ReadDataAsync(Mfi, datatype, bufp, retCnt, jobId);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.ReadDataAsync==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.ReadDataAsync(Mfi, datatype, bufp, retCnt, jobId);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID				
}

MfiStatus MfiWriteDataAsync (MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 datatype, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.WriteDataAsync==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.WriteDataAsync(Mfi, priorty, datatype, buf, retCnt, jobId);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.WriteDataAsync==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.WriteDataAsync(Mfi, priorty, datatype, buf, retCnt, jobId);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID				
}

MfiStatus MfiConfigTrigger(MfiSession Mfi, MfiInt16 trigline, MfiUInt16 mode, MfiUInt32 condition ,MfiPUInt16 oldmode, MfiPUInt32 oldcondition)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.ConfigTrigger==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.ConfigTrigger(Mfi, trigline, mode, condition, oldmode, oldcondition);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.ConfigTrigger==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.ConfigTrigger(Mfi, trigline, mode, condition, oldmode, oldcondition);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID
}

MfiStatus MfiDeleteTrigger(MfiSession Mfi, MfiInt16 trigline)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.DeleteTrigger==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.DeleteTrigger(Mfi, trigline);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.DeleteTrigger==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.DeleteTrigger(Mfi, trigline);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID	
}

MfiStatus MfiAssertTrigger(MfiSession Mfi, MfiInt16 trigline)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.AssertTrigger==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.AssertTrigger(Mfi, trigline);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.AssertTrigger==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.AssertTrigger(Mfi, trigline);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID		
}

MfiStatus MfiSysReadMsg(MfiSession Mfi, MfiPUInt32 msgclass, MfiPUInt32 msgtype, MfiPUInt32 msgsrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.SysReadMsg==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.SysReadMsg(Mfi, msgclass, msgtype, msgsrcaddr, bufp, retCnt, timeout);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.SysReadMsg==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.SysReadMsg(Mfi, msgclass, msgtype, msgsrcaddr, bufp, retCnt, timeout);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID	
}

MfiStatus MfiSysWriteMsg(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgclass, MfiUInt32 msgtype, MfiUInt32 msgdstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.SysWriteMsg==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.SysWriteMsg(Mfi, priorty, msgclass, msgtype, msgdstaddr, buf, retCnt, timeout, sendtype);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.SysWriteMsg==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.SysWriteMsg(Mfi, priorty, msgclass, msgtype, msgdstaddr, buf, retCnt, timeout, sendtype);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID	
}


MfiStatus MfiSysReadMsgAsync(MfiSession Mfi, MfiPUInt32 msgclass, MfiPUInt32 msgtype, MfiPUInt32 msgsrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.SysReadMsgAsync==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.SysReadMsgAsync(Mfi, msgclass, msgtype, msgsrcaddr, bufp, retCnt, jobId);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.SysReadMsgAsync==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.SysReadMsgAsync(Mfi, msgclass, msgtype, msgsrcaddr, bufp, retCnt, jobId);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID				
}

MfiStatus MfiSysWriteMsgAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 msgclass, MfiUInt32 msgtype, MfiUInt32 msgdstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.SysWriteMsgAsync==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.SysWriteMsgAsync(Mfi, priorty, msgclass, msgtype, msgdstaddr, buf, retCnt, jobId);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.SysWriteMsgAsync==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.SysWriteMsgAsync(Mfi, priorty, msgclass, msgtype, msgdstaddr, buf, retCnt, jobId);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID				
}

MfiStatus MfiSysReadData(MfiSession Mfi, MfiPUInt32 dataclass, MfiPUInt32 datatype, MfiPUInt32 datasrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiUInt32 timeout)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.SysReadData==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.SysReadData(Mfi, dataclass, datatype, datasrcaddr, bufp, retCnt, timeout);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.SysReadData==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.SysReadData(Mfi, dataclass, datatype, datasrcaddr, bufp, retCnt, timeout);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID	
}

MfiStatus MfiSysWriteData(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 dataclass, MfiUInt32 datatype, MfiUInt32 datadstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiUInt32 timeout, MfiUInt32 sendtype)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.SysWriteData==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.SysWriteData(Mfi, priorty, dataclass, datatype, datadstaddr, buf, retCnt, timeout, sendtype);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.SysWriteData==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.SysWriteData(Mfi, priorty, dataclass, datatype, datadstaddr, buf, retCnt, timeout, sendtype);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID	
}


MfiStatus MfiSysReadDataAsync(MfiSession Mfi, MfiPUInt32 dataclass, MfiPUInt32 datatype, MfiPUInt32 datasrcaddr, MfiPBuf* bufp, MfiPUInt32 retCnt, MfiPJobId jobId)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.SysReadDataAsync==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.SysReadDataAsync(Mfi, dataclass, datatype, datasrcaddr, bufp, retCnt, jobId);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.SysReadDataAsync==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.SysReadDataAsync(Mfi, dataclass, datatype, datasrcaddr, bufp, retCnt, jobId);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID				
}

MfiStatus MfiSysWriteDataAsync(MfiSession Mfi, MfiUInt32 priorty, MfiUInt32 dataclass, MfiUInt32 datatype, MfiUInt32 datadstaddr, MfiPBuf buf, MfiUInt32 retCnt, MfiPJobId  jobId)
{
	MfiStatus status;
	
	if(RsrcManager==NULL)
		return MFI_ERROR_NCREAT_MANAGER;
		
	if((Mfi>=RM_MIN_SESSION_ID&&Mfi<=RM_MAX_SESSION_ID)&&(Mfi==RsrcManager->rmsession)){
		if(RsrcManager->rsrc_opt.SysWriteDataAsync==MFI_NULL)
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		else
			return RsrcManager->rsrc_opt.SysWriteDataAsync(Mfi, priorty, dataclass, datatype, datadstaddr, buf, retCnt, jobId);
	}
	
	if(Mfi>=0&&Mfi<=RSRC_MAX_SESSION_ID){
		if(SesStatusCheck(&(RsrcManager->session_list[Mfi]))==NOINUSE)
			return MFI_WARN_NULL_OBJECT;   //指定的会话未初始化	
		else if(RsrcManager->session_list[Mfi].rsrc_opt.SysWriteDataAsync==MFI_NULL){
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return MFI_ERROR_NSUP_OPER;   //会话不支持该操作
		}
		else{
			status=RsrcManager->session_list[Mfi].rsrc_opt.SysWriteDataAsync(Mfi, priorty, dataclass, datatype, datadstaddr, buf, retCnt, jobId);
			SesStatusFree(&(RsrcManager->session_list[Mfi]));
			return status;
		}
	}
	
	return MFI_ERROR_INV_OBJECT;   //无效的会话ID				
}