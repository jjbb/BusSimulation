#include "mfi_module_info.h"
#include "stdio.h"

Module_t Module={NULL,0,0};

//-----------------------------------------------------------------
//动态地址分配之后，检查主机板到各模块板之间的数据链路是否通畅（启动时循环执行）
//-----------------------------------------------------------------
void Module_Useful_check(void)
{
/*	static MfiUInt32 num=0;
	static MfiUInt32 flag=0,send_flag=0;
	MSG message;
	
	while(num<Module.number)
	{
		message.m_id.all = (0<<11) | (Module.Module_Info_p[num].mod_ip<<6) | 0<<1;
		message.m_cf.all = 0;
		
		BUS_manage.MSG_TX_FIFO.Write_Fifo_Msg(&(BUS_manage.MSG_TX_FIFO),&message); 
		BUS_manage.Read_Fifo2Bus_Msg(&(BUS_manage.MSG_TX_FIFO) , &BUS);
		message.m_id.all=0;
		
		usleep(1000);//1000us=1ms
		
		if(TRUE == BUS_manage.MSG_RX_FIFO.Read_Fifo_Msg(&(BUS_manage.MSG_RX_FIFO),&message)){
			if(((message.m_id.all & 0x3E) >> 1)==Module.Module_Info_p[num].mod_ip)
				Module.Available_flag|=1<<num;
			else
				Module.Available_flag&=~(1<<num);
		}
		else
			Module.Available_flag&=~(1<<num);
		
		num++;
	}
	
	return;*/
	
}

/*void Module_Useful_check(void)
{
	static MfiUInt32 num=0;
	static MfiUInt32 flag=0,send_flag=0;
	
	if(flag==0)
	{
		BUS_manage.MT_class=0;
		BUS_manage.MT_priority=0;
		BUS_manage.MT_length=0;
		BUS_manage.MT_Src_IP_Adr=0;
		BUS_manage.MT_type=1;
		BUS_manage.MT_arry=NULL;
		flag=1;
	}
	
	if(num<Module.number)
	{
		BUS_manage.MT_Dst_IP_Adr=Module.Module_Info_p[num].mod_ip;
		BUS_manage.Write_User2Fifo_Msg(&BUS_manage,&(BUS_manage.MSG_TX_FIFO),&(BUS));
	}
	
	usleep(1000);//1000us=1ms
	
	if(BUS_manage.MR_flag==1)
	{
		if(BUS_manage.MR_Src_IP_Adr==Module.Module_Info_p[num].mod_ip)
			Module.Available_flag|=1<<num;
		else
			Module.Available_flag&=~(1<<num);
	}
	else
		Module.Available_flag&=~(1<<num);
		
	num++;
	BUS_manage.MR_flag=0;
	BUS_manage.MR_finish=1;

}*/


/*
MfiUInt32 Module_IP_Find(MfiUInt32 mod_mac)
{
	MfiUInt32 i=0;
	
	for(i=0;i<Module.number;i++)
	{
		if(Module.Available_flag&(1<<i))
		{
			if(mod_mac==Module.Module_Info_p[i].mod_mac)
				return Module.Module_Info_p[i].mod_ip;
		}
	}

	return 0;//没有找到对应模块，或者对应模块链路不通
}*/