#include "mfi_reg.h"
#include "mfi_message.h"
#include "mfi_data.h"

Common_regs_p Common_regs = (Common_regs_p)(EMIF_BASE+COMMON_REG_BASE);
Rst_bus_regs_p Rst_bus_regs = (Rst_bus_regs_p)(EMIF_BASE+RST_BUS_REG_BASE);
TimeSync_regs_p TimeSync_regs = (TimeSync_regs_p)(EMIF_BASE+TIMESYNC_REG_BASE);
MsgBus_regs_p MsgBus_regs = (MsgBus_regs_p)(EMIF_BASE+MSGBUS_REG_BASE);	
DataBus_regs_p DataBus_regs = (DataBus_regs_p)(EMIF_BASE+DATABUS_REG_BASE);

void Write_Reg (int filp , MfiPUInt16 addr , MfiUInt16 value)
{
	unsigned char buf[6];
	MfiUInt32 reg=(MfiUInt32)addr;
	
	buf[0]=(char)value;
	buf[1]=(char)(value>>8);
	buf[2]=(char)reg;
	buf[3]=(char)(reg>>8);
	buf[4]=(char)(reg>>16);
	buf[5]=(char)(reg>>24);
	
	write(filp,&buf,6);
}

MfiUInt16 Read_Reg (int filp , MfiPUInt16 addr)
{
	unsigned char buf[6];
	unsigned short int value;
	MfiUInt32 reg=(MfiUInt32)addr;
	
	buf[2]=(char)reg;
	buf[3]=(char)(reg>>8);
	buf[4]=(char)(reg>>16);
	buf[5]=(char)(reg>>24);
	
	value=read(filp,&buf,6);
	
	value=buf[0]|buf[1]<<8;
	return value;
}

//get VID,PID
void Vid_Pid_Get(int filp,MfiUInt32* vid,MfiUInt32* pid)
{
	*vid = Read_Reg(filp,&(Common_regs->VID));
	*pid = Read_Reg(filp,&(Common_regs->PID));
	
	return;
}

//set LED
void LED_Set(int filp,MfiUInt16 status)
{
	Write_Reg(filp,&(Common_regs->LED_status),status);
}

//Time msg set(固定时间窗)
/*
void Time_Msg_Set(BUS_message_t* time_message, MfiUInt16 Subwin_start_time,MfiUInt16 Subwin_length,MfiUInt16 Subwin_N)
{
	MSG message;
	int j=0;
	
	message.m_id.bit.m_src_addr=time_message->Msg_Src_Mac_Addr;
	message.m_id.bit.m_dst_addr=time_message->Msg_Dst_Mac_Addr;
	message.m_id.bit.m_type=time_message->Msg_type;
	message.m_id.bit.priority3=time_message->Msg_priority>>3;
	message.m_id.bit.priority2=time_message->Msg_priority>>2;
	message.m_id.bit.priority1=time_message->Msg_priority>>1;
	message.m_id.bit.priority0=time_message->Msg_priority;
	message.m_id.bit.m_class2=time_message->Msg_class>>2;
	message.m_id.bit.m_class1=time_message->Msg_class>>1;	
	message.m_id.bit.m_class0=time_message->Msg_class;
	message.m_id.bit.m_single=0;
	message.m_cf.bit.dlc=time_message->Msg_length;
	
	if(time_message->Msg_length!=0)
	{
		message.m_cf.bit.rtr=1;	
		message.m_txt1.m_char[0]=time_message->Msg_length;
		  
		for(j=1;j<Message_max_length;j++)
		{
			if(j<time_message->Msg_length+1)
			{
				message.m_txt1.m_char[j]=*(time_message->Msg_arry);
				(time_message->Msg_arry)++;
			}
			else
				message.m_txt1.m_char[j]=0;						
		}
		
		Write_Reg(filp,&(MsgBus_regs->Time_msg_body0),message.m_txt1.m_short[0]);
		Write_Reg(filp,&(MsgBus_regs->Time_msg_body1),message.m_txt1.m_short[1]);
		Write_Reg(filp,&(MsgBus_regs->Time_msg_body2),message.m_txt2.m_short[0]);
		Write_Reg(filp,&(MsgBus_regs->Time_msg_body3),message.m_txt2.m_short[1]);
		Write_Reg(filp,&(MsgBus_regs->Time_msg_body4),message.m_txt3.m_short[0]);
		Write_Reg(filp,&(MsgBus_regs->Time_msg_body5),message.m_txt3.m_short[1]);
	}
	else 
		message.m_cf.bit.rtr=0;
		
	Write_Reg(filp,&(MsgBus_regs->Time_msg_head_L),message.m_id.half[0]); //5151
	Write_Reg(filp,&(MsgBus_regs->Time_msg_head_H),message.m_id.half[1]);//0001
	Write_Reg(filp,&(MsgBus_regs->Time_msg_cfg_L),message.m_cf.half[0]);//0001
	Write_Reg(filp,&(MsgBus_regs->Time_msg_cfg_H),0x0000);//0001
	
	Write_Reg(filp,&(MsgBus_regs->Msg_Subwin_start_time),Subwin_start_time);
	Write_Reg(filp,&(MsgBus_regs->Msg_Subwin_length),Subwin_length);
	Write_Reg(filp,&(MsgBus_regs->Msg_Subwin_load_N),Subwin_N);
	Write_Reg(filp,&(MsgBus_regs->Msg_Subwin_load_en),1);
}
*/

//-----------------------------------------------------------------
//设置子窗起始时刻及子窗长度 subwin从0开始
//-----------------------------------------------------------------
void SubwinSet(int filp,MfiUInt16 subwin,MfiUInt16 starttime,MfiUInt16 length)
{
	Write_Reg(filp,&(MsgBus_regs->Msg_Subwin_start_time),starttime);
	Write_Reg(filp,&(MsgBus_regs->Msg_Subwin_length),length);
	Write_Reg(filp,&(MsgBus_regs->Msg_Subwin_load_N),subwin);
	Write_Reg(filp,&(MsgBus_regs->Msg_Subwin_load_en),1);
}

//-----------------------------------------------------------------
//子窗使能 subwin的位宽（类型）取决于子窗个数
//-----------------------------------------------------------------
void MsgSubwinEn(int filp,MfiUInt16 subwin)
{	
	Write_Reg(filp,&(MsgBus_regs->Msg_subwin_en),subwin);
}

void DataSubwinEn(int filp,MfiUInt16 subwin)
{
//		Write_Reg(filp,&(DataBus_regs->Data_subwin_en),subwin);
}

//-----------------------------------------------------------------
//时间同步模块初始化 trig_select：触发线选择 trig_en:触发使能 T_basic：设置基本周期长度 Twin_Length：固定时间窗长度
//-----------------------------------------------------------------
void TimeSyncInit (int filp,MfiUInt16 trig_select,MfiUInt16 trig_en,MfiUInt16 T_basic,MfiUInt16 Twin_Length)
{
	Write_Reg(filp,&(TimeSync_regs->Rst_n),0);
	Write_Reg(filp,&(TimeSync_regs->Sync_trig_set),trig_select);
	Write_Reg(filp,&(TimeSync_regs->SyncP_senden),trig_en);
	Write_Reg(filp,&(TimeSync_regs->T_basic_set),T_basic);
	Write_Reg(filp,&(TimeSync_regs->Twin_length),Twin_Length);
	Write_Reg(filp,&(TimeSync_regs->Rst_n),1); //Local time开始计时
}

void TimeSyncRst(int filp)
{
	Write_Reg(filp,&(TimeSync_regs->Rst_n),0);
}

void MsgBusInit (int filp,MfiUInt32 Msg_head)
{}

void DataBusInit (int filp,MfiUInt32 Data_head)
{}

/*消息接收使能*/
void MsgReceiveEn(int filp,MfiUInt16 value)
{
	MfiUInt16 temp;
	temp=Read_Reg(filp,&(MsgBus_regs->Msg_rx_ctrl));
	temp&=0x13;
	
	if(value==1)
		Write_Reg(filp,&(MsgBus_regs->Msg_rx_ctrl),temp|(1<<0));
	else
		Write_Reg(filp,&(MsgBus_regs->Msg_rx_ctrl),temp&(~(1<<0)));
}

/*消息中断使能*/
void MsgRxIntEn(int filp,MfiUInt16 value)
{
	MfiUInt16 temp;
	temp=Read_Reg(filp,&(MsgBus_regs->Msg_rx_ctrl));
	temp&=0x13;
	
	if(value==1)
		Write_Reg(filp,&(MsgBus_regs->Msg_rx_ctrl),temp|(1<<4));
	else
		Write_Reg(filp,&(MsgBus_regs->Msg_rx_ctrl),temp&(~(1<<4)));
}

/*消息接收FIFO复位*/
void MsgRxFifoRst(int filp,MfiUInt16 value)
{
	MfiUInt16 temp;
	temp=Read_Reg(filp,&(MsgBus_regs->Msg_rx_ctrl));
	temp&=0x13;
	
	if(value==1)
		Write_Reg(filp,&(MsgBus_regs->Msg_rx_ctrl),temp|(1<<1));
	else
		Write_Reg(filp,&(MsgBus_regs->Msg_rx_ctrl),temp&(~(1<<1)));
}

/*使能滤波器*///需修改：读出寄存器值，做位运算写入
void MsgFilterEn (int filp,MfiUInt16 filter)
{
	Write_Reg(filp,&(MsgBus_regs->Msg_Filter_ctrl),filter);
}

void DataFilterEn (int filp,MfiUInt16 filter)
{
		Write_Reg(filp,&(DataBus_regs->Data_Filter_ctrl),filter);
}

/*设置滤波器*/
void MsgFilterSet (int filp,MfiUInt16 filter,MfiUInt16 m_class,MfiUInt16 m_type,MfiUInt16 m_dst_addr,MfiUInt16 m_src_addr)
{
	MID_REG filter_value;
	
	((filter==0x0004)||(filter==0x0005))?(filter_value.all=0xFFFFFFFF):(filter_value.all=0);
	filter_value.bit.m_class3=m_class>>3;
	filter_value.bit.m_class2=m_class>>2;
	filter_value.bit.m_class1=m_class>>1;	
	filter_value.bit.m_class0=m_class;
	filter_value.bit.m_type=m_type;
	filter_value.bit.m_dst_addr=m_dst_addr;
	filter_value.bit.m_src_addr=m_src_addr;
	
	Write_Reg(filp,&(MsgBus_regs->Msg_ACR0_L)+2*filter,filter_value.all);
	Write_Reg(filp,&(MsgBus_regs->Msg_ACR0_L)+2*filter+1,filter_value.all>>16); 
}

void DataFilterSet (int filp,MfiUInt16 filter,MfiUInt16 d_class,MfiUInt16 d_type,MfiUInt16 d_dst_addr,MfiUInt16 d_src_addr)
{
	DID_REG filter_value;
	
	((filter==0x0004)||(filter==0x0005))?(filter_value.all=0xFFFFFFFF):(filter_value.all=0);
	filter_value.bit.d_class3=d_class>>3;
	filter_value.bit.d_class2=d_class>>2;
	filter_value.bit.d_class1=d_class>>1;
	filter_value.bit.d_class0=d_class;
	filter_value.bit.d_type=d_type;
	filter_value.bit.d_dst_addr=d_dst_addr;
	filter_value.bit.d_src_addr=d_src_addr;
	
	Write_Reg(filp,&(DataBus_regs->Data_ACR0_L)+2*filter,filter_value.all);
	Write_Reg(filp,&(DataBus_regs->Data_ACR0_L)+2*filter+1,filter_value.all>>16); 
}
