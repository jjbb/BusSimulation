#include "mfi_attr_deal.h"
#include "mfi_attribute.h"
#include "mfi_message.h"
#include "mfi_rsrc_manager.h"
#include "mfi_system_command.h"
#include "mfi_define.h"
#include "mfi_linux.h"
#include "mfi_reg.h"
#include <string.h>

/***************************************************************/
//���º���Ϊ�����Ծ�������úͻ�ȡ����
//���ڲ�ͬ��������Ҫ�в�ͬ�����úͻ�ȡʱ�Ĳ����������Զ����ⲿ�����ԣ��ֱ��д���������úͻ�ȡ������
//�磺һЩ��Ӳ����ص����ԣ������߻���ʱ�����ڡ��̶�ʱ�䴰���ȣ����������ԣ����ͨ��һ����Ϣһ��������
//    ��ǰ�˻��ߴ�ǰ�˻�ȡ�����ǰ�����������Խ����������ԡ������������֮�䣬���������һ�£����������
//    ������֮�䣬������������ʵ�ֿ϶���ͬ���޷�ͳһ�����ң������ù�������ʱ����ͬ������ռ��һ����Ϣ����Щ
//    �ֽڶ���Ҫ����ָ����
/***************************************************************/
//������Ϣ��ʱ�䴰������ʱ�䴰�����ñ��������а��Ӷ��յ�������Ϣ֮��ͬʱ���У����Ա��뱣֤�ȷ���Ϣ�������á�
MfiStatus SetAttr_Msg_Tlen(MfiObject Mfi, MfiAttr attrName, MfiAttrState attrValue, MfiPAttribute attr, MfiInt32 attrindex, MfiUInt32 attr_num, MfiUInt16 refresh)
{
	MfiInt32 tempindex=0;
	MTXT_REG buf[2];
	MfiUInt32 msgdstaddr;
	
	memcpy(&(attr[attrindex].attr_val),&(attrValue),sizeof(MfiAttrState));
	if((refresh!=0)||(attr_table[attrName-MFI_ATTR_BASE].attr_cfg.bit.is_rt==REALTIME))
	{
		//���÷��ͺ���,������ֵ���µ�Ӳ��
		switch(attrName){
			case MFI_ATTR_MSG_BASIC_TLEN:
				tempindex=FindAttr(MFI_ATTR_MSG_FIX_TLEN, attr, attr_num);              //���ҹ�������
				buf[0].m_int=attrValue;
				buf[1].m_int=attr[tempindex].attr_val.attr_ui;  //�˴�Ӧ���Ѳ��ҵ���ֱֵ�Ӹ��ƽ���������Ϣ�Ļ�������
				break;
			case MFI_ATTR_MSG_FIX_TLEN:
				tempindex=FindAttr(MFI_ATTR_MSG_BASIC_TLEN, attr, attr_num);              //���ҹ�������
				buf[0].m_int=attr[tempindex].attr_val.attr_ui;
				buf[1].m_int=attrValue;	
				break;
		}
		
			//װ����������÷�����Ϣ����
		//Mfi_to_Ip(Mfi,&msgdstaddr);
		SysWriteMsg(Mfi, MSG_SET_DEF_PORITY, MSG_SET_CLASS, attr_table[attrName-MFI_ATTR_BASE].set_command, BROADCAST_IP, (MfiPBuf)buf, 8, MFI_TMO_INFINITE,0);
	}
				
	return MFI_SUCCESS;
}

MfiStatus SetAttr_Msg_FILTER(MfiObject Mfi, MfiAttr attrName, MfiAttrState attrValue, MfiPAttribute attr, MfiInt32 attrindex, MfiUInt32 attr_num, MfiUInt16 refresh)
{
	MfiInt32 tempindex=0;
	MTXT_REG buf[3];
	MfiUInt32 msgdstaddr;
	
	memcpy(&(attr[attrindex].attr_val),&(attrValue),sizeof(MfiAttrState));
	
	if(RsrcManager->session_list[Mfi].rsrc_type==BUS_RSRC_TYPE)
		goto BUS;

	if((refresh!=0)||(attr_table[attrName-MFI_ATTR_BASE].attr_cfg.bit.is_rt==REALTIME))
	{
		//���÷��ͺ���,������ֵ���µ�Ӳ��
		switch(attrName){
			case MFI_ATTR_MSG_AMR1_FILTER:
				buf[0].m_int=attrValue;
				tempindex=FindAttr(MFI_ATTR_MSG_ACR2_FILTER, attr, attr_num);              //���ҹ�������
				buf[1].m_int=attr[tempindex].attr_val.attr_ui;  //�˴�Ӧ���Ѳ��ҵ���ֱֵ�Ӹ��ƽ���������Ϣ�Ļ�������
				tempindex=FindAttr(MFI_ATTR_MSG_ACR3_FILTER, attr, attr_num); 
				buf[2].m_int=attr[tempindex].attr_val.attr_ui;
				break;
			case MFI_ATTR_MSG_ACR2_FILTER:
				buf[1].m_int=attrValue;
				tempindex=FindAttr(MFI_ATTR_MSG_AMR1_FILTER, attr, attr_num);              //���ҹ�������
				buf[0].m_int=attr[tempindex].attr_val.attr_ui;  //�˴�Ӧ���Ѳ��ҵ���ֱֵ�Ӹ��ƽ���������Ϣ�Ļ�������
				tempindex=FindAttr(MFI_ATTR_MSG_ACR3_FILTER, attr, attr_num); 
				buf[2].m_int=attr[tempindex].attr_val.attr_ui;
				break;
			case MFI_ATTR_MSG_ACR3_FILTER:
				buf[2].m_int=attrValue;
				tempindex=FindAttr(MFI_ATTR_MSG_AMR1_FILTER, attr, attr_num);              //���ҹ�������
				buf[0].m_int=attr[tempindex].attr_val.attr_ui;  //�˴�Ӧ���Ѳ��ҵ���ֱֵ�Ӹ��ƽ���������Ϣ�Ļ�������
				tempindex=FindAttr(MFI_ATTR_MSG_ACR2_FILTER, attr, attr_num); 
				buf[1].m_int=attr[tempindex].attr_val.attr_ui;
				break;
		}
		
		//װ����������÷�����Ϣ����
		Mfi_to_Ip(Mfi,&msgdstaddr);
		SysWriteMsg(Mfi, MSG_SET_DEF_PORITY, MSG_SET_CLASS, attr_table[attrName-MFI_ATTR_BASE].set_command, msgdstaddr, (MfiPBuf)buf, 12, MFI_TMO_INFINITE,0);
	}
	return MFI_SUCCESS;
	
	BUS:
		switch(attrName){
			case MFI_ATTR_MSG_AMR1_FILTER:
				Write_Reg(fd_gpmc,&(MsgBus_regs->Msg_AMR1_L),attrName);
				Write_Reg(fd_gpmc,&(MsgBus_regs->Msg_AMR1_H),attrName>>16); 
				break;
			case MFI_ATTR_MSG_ACR2_FILTER:
				Write_Reg(fd_gpmc,&(MsgBus_regs->Msg_ACR2_L),attrName);
				Write_Reg(fd_gpmc,&(MsgBus_regs->Msg_ACR2_H),attrName>>16); 
				break;
			case MFI_ATTR_MSG_ACR3_FILTER:
				Write_Reg(fd_gpmc,&(MsgBus_regs->Msg_ACR3_L),attrName);
				Write_Reg(fd_gpmc,&(MsgBus_regs->Msg_ACR3_H),attrName>>16); 
				break;
		}
		
	return MFI_SUCCESS;
}


MfiStatus SetAttr_Data_Tlen(MfiObject Mfi, MfiAttr attrName, MfiAttrState attrValue, MfiPAttribute attr, MfiInt32 attrindex, MfiUInt32 attr_num, MfiUInt16 refresh)
{
	MfiInt32 tempindex=0;
	
	memcpy(&(attr[attrindex].attr_val),&(attrValue),sizeof(MfiAttrState));
	if((refresh!=0)||(attr_table[attrName-MFI_ATTR_BASE].attr_cfg.bit.is_rt==REALTIME))
	{
		//���÷��ͺ���,������ֵ���µ�Ӳ��
		switch(attrName){
			case MFI_ATTR_DATA_BASIC_TLEN:
				tempindex=FindAttr(MFI_ATTR_DATA_FIX_TLEN, attr, attr_num);              //���ҹ�������
				memcpy(&(attr[tempindex].attr_val),&(attrValue),sizeof(MfiAttrState));  //�˴�Ӧ���Ѳ��ҵ���ֱֵ�Ӹ��ƽ���������Ϣ�Ļ�������
		
				//װ����������÷�����Ϣ����
				
				break;
			case MFI_ATTR_DATA_FIX_TLEN:
				tempindex=FindAttr(MFI_ATTR_DATA_BASIC_TLEN, attr, attr_num);              //���ҹ�������
				memcpy(&(attr[tempindex].attr_val),&(attrValue),sizeof(MfiAttrState));
				
				break;
		}
	}
	
	return MFI_SUCCESS;
}

MfiStatus GetAttr_Msg_FILTER0(MfiObject Mfi, MfiPAttribute attr, MfiUInt32 attr_num, MfiPBuf buf, MfiUInt32 retCnt)
{
	MfiInt32 tempindex=0;
	MfiUInt32 tmp,val;
	
	if(RsrcManager->session_list[Mfi].rsrc_type==BUS_RSRC_TYPE)
		goto BUS;
	
	tempindex=FindAttr(MFI_ATTR_MSG_AMR0_FILTER, attr, attr_num);
	memcpy(&(attr[tempindex].attr_val),buf,sizeof(MfiAttrState));
	tempindex=FindAttr(MFI_ATTR_MSG_ACR0_FILTER, attr, attr_num);
	memcpy(&(attr[tempindex].attr_val),buf+4,sizeof(MfiAttrState));
	tempindex=FindAttr(MFI_ATTR_MSG_ACR1_FILTER, attr, attr_num);
	memcpy(&(attr[tempindex].attr_val),buf+8,sizeof(MfiAttrState));
	
	return MFI_SUCCESS;
	
	BUS:
		val=Read_Reg(fd_gpmc , &(MsgBus_regs->Msg_AMR0_L));
		tmp=Read_Reg(fd_gpmc , &(MsgBus_regs->Msg_AMR0_H));
		val=(tmp<<16) | val;
		tempindex=FindAttr(MFI_ATTR_MSG_AMR0_FILTER, attr, attr_num);
		memcpy(&(attr[tempindex].attr_val),&val,sizeof(MfiAttrState));
	
		val=Read_Reg(fd_gpmc , &(MsgBus_regs->Msg_ACR0_L));
		tmp=Read_Reg(fd_gpmc , &(MsgBus_regs->Msg_ACR0_H));
		val=(tmp<<16) | val;
		tempindex=FindAttr(MFI_ATTR_MSG_ACR0_FILTER, attr, attr_num);
		memcpy(&(attr[tempindex].attr_val),&val,sizeof(MfiAttrState));
		
		val=Read_Reg(fd_gpmc , &(MsgBus_regs->Msg_ACR1_L));
		tmp=Read_Reg(fd_gpmc , &(MsgBus_regs->Msg_ACR1_H));
		val=(tmp<<16) | val;
		tempindex=FindAttr(MFI_ATTR_MSG_ACR1_FILTER, attr, attr_num);
		memcpy(&(attr[tempindex].attr_val),&val,sizeof(MfiAttrState));
		
	return MFI_SUCCESS;
}

MfiStatus GetAttr_Msg_FILTER1(MfiObject Mfi, MfiPAttribute attr, MfiUInt32 attr_num, MfiPBuf buf, MfiUInt32 retCnt)
{
	MfiInt32 tempindex=0;
	MfiUInt32 tmp,val;

	if(RsrcManager->session_list[Mfi].rsrc_type==BUS_RSRC_TYPE)
		goto BUS;
		
	tempindex=FindAttr(MFI_ATTR_MSG_AMR1_FILTER, attr, attr_num);
	memcpy(&(attr[tempindex].attr_val),buf,sizeof(MfiAttrState));
	tempindex=FindAttr(MFI_ATTR_MSG_ACR2_FILTER, attr, attr_num);
	memcpy(&(attr[tempindex].attr_val),buf+4,sizeof(MfiAttrState));
	tempindex=FindAttr(MFI_ATTR_MSG_ACR3_FILTER, attr, attr_num);
	memcpy(&(attr[tempindex].attr_val),buf+8,sizeof(MfiAttrState));
	
	return MFI_SUCCESS;
	
	BUS:
		val=Read_Reg(fd_gpmc , &(MsgBus_regs->Msg_AMR1_L));
		tmp=Read_Reg(fd_gpmc , &(MsgBus_regs->Msg_AMR1_H));
		val=(tmp<<16) | val;
		tempindex=FindAttr(MFI_ATTR_MSG_AMR1_FILTER, attr, attr_num);
		memcpy(&(attr[tempindex].attr_val),&val,sizeof(MfiAttrState));
	
		val=Read_Reg(fd_gpmc , &(MsgBus_regs->Msg_ACR2_L));
		tmp=Read_Reg(fd_gpmc , &(MsgBus_regs->Msg_ACR2_H));
		val=(tmp<<16) | val;
		tempindex=FindAttr(MFI_ATTR_MSG_ACR2_FILTER, attr, attr_num);
		memcpy(&(attr[tempindex].attr_val),&val,sizeof(MfiAttrState));
		
		val=Read_Reg(fd_gpmc , &(MsgBus_regs->Msg_ACR3_L));
		tmp=Read_Reg(fd_gpmc , &(MsgBus_regs->Msg_ACR3_H));
		val=(tmp<<16) | val;
		tempindex=FindAttr(MFI_ATTR_MSG_ACR3_FILTER, attr, attr_num);
		memcpy(&(attr[tempindex].attr_val),&val,sizeof(MfiAttrState));
		
	return MFI_SUCCESS;
}

MfiStatus SetAttr_Msg_Twin_cfg(MfiObject Mfi, MfiAttr attrName, MfiAttrState attrValue, MfiPAttribute attr, MfiInt32 attrindex, MfiUInt32 attr_num, MfiUInt16 refresh)
{
	MfiInt32 tempindex=0;
	MfiUInt32 msgdstaddr;

	if(attrValue & (~attr[attrindex].attr_val.attr_ui & RsrcManager->bus_rsrc->subwin_manager_msg) != 0)
		return MFI_ERROR_NSUP_ATTR_STATE;
	
	memcpy(&(attr[attrindex].attr_val),&(attrValue),sizeof(MfiAttrState));
	
	if(RsrcManager->session_list[Mfi].rsrc_type==BUS_RSRC_TYPE)
		goto BUS;
	
	if((refresh!=0)||(attr_table[attrName-MFI_ATTR_BASE].attr_cfg.bit.is_rt==REALTIME))
	{
		//���÷��ͺ���,������ֵ���µ�Ӳ��
		Mfi_to_Ip(Mfi,&msgdstaddr);
		SysWriteMsg(Mfi, MSG_SET_DEF_PORITY, MSG_SET_CLASS, attr_table[attrName-MFI_ATTR_BASE].set_command, msgdstaddr, (MfiPBuf)(&attrValue), 4, MFI_TMO_INFINITE,0);
		RsrcManager->bus_rsrc->subwin_manager_msg |= attrValue;
	}
				
	return MFI_SUCCESS;

	BUS:
		//������ֵд��FPGA�Ĵ���
		//Write_Reg(fd_gpmc,&(MsgBus_regs->Msg_AMR1_L),attrName);
		RsrcManager->bus_rsrc->subwin_manager_msg |= attrValue;
	return MFI_SUCCESS;
}

MfiStatus SetAttr_Data_Twin_cfg(MfiObject Mfi, MfiAttr attrName, MfiAttrState attrValue, MfiPAttribute attr, MfiInt32 attrindex, MfiUInt32 attr_num, MfiUInt16 refresh)
{
	MfiInt32 tempindex=0;
	MfiUInt32 msgdstaddr;

	if(attrValue & (~attr[attrindex].attr_val.attr_ui & RsrcManager->bus_rsrc->subwin_manager_data) != 0)
		return MFI_ERROR_NSUP_ATTR_STATE;
	
	memcpy(&(attr[attrindex].attr_val),&(attrValue),sizeof(MfiAttrState));
	
	if(RsrcManager->session_list[Mfi].rsrc_type==BUS_RSRC_TYPE)
		goto BUS;
	
	if((refresh!=0)||(attr_table[attrName-MFI_ATTR_BASE].attr_cfg.bit.is_rt==REALTIME))
	{
		//���÷��ͺ���,������ֵ���µ�Ӳ��
		Mfi_to_Ip(Mfi,&msgdstaddr);
		SysWriteMsg(Mfi, MSG_SET_DEF_PORITY, MSG_SET_CLASS, attr_table[attrName-MFI_ATTR_BASE].set_command, msgdstaddr, (MfiPBuf)(&attrValue), 4, MFI_TMO_INFINITE,0);
		RsrcManager->bus_rsrc->subwin_manager_data |= attrValue;
	}
				
	return MFI_SUCCESS;

	BUS:
		//������ֵд��FPGA�Ĵ���
		//Write_Reg(fd_gpmc,&(MsgBus_regs->Msg_AMR1_L),attrName);
		RsrcManager->bus_rsrc->subwin_manager_data |= attrValue;
	return MFI_SUCCESS;
}
