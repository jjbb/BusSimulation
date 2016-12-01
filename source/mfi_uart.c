#include "mfi_uart.h"
#include "mfi_module_info.h"
#include "mfi_test_define.h"
#include <sys/select.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

char uart_rx_buff[300];
char uart_tx_buff[300];
int speed_arr[] = {B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300};
int name_arr[] = {115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 300};

//设置串口波特率
void Uart_Speed_Set(int fd, int speed)
{
    int i;
    struct termios Opt;    //定义termios结构

    if(tcgetattr(fd, &Opt) != 0)
    {
        printf("tcgetattr fd error！\n");
        return;
    }
    for(i = 0; i < sizeof(speed_arr) / sizeof(int); i++)
    {
        if(speed == name_arr[i])
        {
            tcflush(fd, TCIOFLUSH);
            cfsetispeed(&Opt, speed_arr[i]);    //设置输入波特率
            cfsetospeed(&Opt, speed_arr[i]);   //设置输出波特率
            if(tcsetattr(fd, TCSANOW, &Opt) != 0)
            {
                printf("tcsetattr fd error！\n");
                return;
            }
            tcflush(fd, TCIOFLUSH);
        }
    }
}

//设置串口数据位数，停止位数，校验方式
MfiBoolean Uart_Parity_Set(int fd, int databits, int stopbits, char parity)
{
    struct termios Opt;
    if(tcgetattr(fd, &Opt) != 0)
    {
        printf("tcgetattr fd erro！\n");
        return MFI_FALSE;
    }
   Opt.c_cflag |= (CLOCAL | CREAD);        //一般必设置的标志，允许输入

    switch(databits)        //设置数据位数
    {
    case 7:
        Opt.c_cflag &= ~CSIZE;
        Opt.c_cflag |= CS7;
        break;
    case 8:
        Opt.c_cflag &= ~CSIZE;
        Opt.c_cflag |= CS8;
        break;
    default:
        printf("Unsupported data size.\n");
        return MFI_FALSE;
    }

    switch(parity)            //设置校验位
    {
    case 'n':
    case 'N':
        Opt.c_cflag &= ~PARENB;        //清除校验位
        Opt.c_iflag &= ~INPCK;        //enable parity checking
        break;
    case 'o':
    case 'O':
        Opt.c_cflag |= PARENB;        //enable parity
        Opt.c_cflag |= PARODD;        //奇校验
        Opt.c_iflag |= INPCK;            //disable parity checking
        break;
    case 'e':
    case 'E':
        Opt.c_cflag |= PARENB;        //enable parity
        Opt.c_cflag &= ~PARODD;        //偶校验
        Opt.c_iflag |= INPCK;            //disable pairty checking
        break;
    case 's':
    case 'S':
        Opt.c_cflag &= ~PARENB;        //清除校验位
        Opt.c_cflag &= ~CSTOPB;        //??????????????
        Opt.c_iflag |= INPCK;            //disable pairty checking
        break;
    default:
        printf("Unsupported parity.\n");
        return MFI_FALSE;    
    }

    switch(stopbits)        //设置停止位
    {
    case 1:
        Opt.c_cflag &= ~CSTOPB;
        break;
    case 2:
        Opt.c_cflag |= CSTOPB;
        break;
    default:
        printf("Unsupported stopbits.\n");
        return MFI_FALSE;
    }

    Opt.c_cflag |= (CLOCAL | CREAD);  //使能输入

    Opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); //ICANON设置原始模式，ECHO设置无回显

    Opt.c_oflag &= ~OPOST;   //设置不处理，直接输出
    Opt.c_oflag &= ~(ONLCR | OCRNL);    //不对换行回车符号进行转换
//
    Opt.c_iflag &= ~(ICRNL | INLCR);    //不对输入的换行回车符号进行转换
    Opt.c_iflag &= ~(IXON | IXOFF | IXANY);    //禁止流控制

    tcflush(fd, TCIFLUSH);
    Opt.c_cc[VTIME] = 0;        //设置超时为15sec
    Opt.c_cc[VMIN] = 0;        //Update the Opt and do it now
    if(tcsetattr(fd, TCSANOW, &Opt) != 0)
    {
        printf("tcsetattr fd erro！\n");
        return MFI_FALSE;
    }

    return MFI_TRUE;
}

//----------------------------------------------------------------------------
// 读串口，使用select在一定时间内监控串口是否有数据可读
//----------------------------------------------------------------------------
int tread(int fd, char *buf, unsigned int nbytes, unsigned int sectimout, unsigned int usectimout)
{
   int nfds;
   fd_set readfds;
   struct timeval tv;
   ssize_t status=0;
   
//select函数可以指示内核等待多个事件中的任一个发生，仅在一个或多个事件发生，或者等待一个足够的时间后才唤醒进程！
   tv.tv_sec = sectimout;
   tv.tv_usec = usectimout;
   FD_ZERO(&readfds);
   FD_SET(fd, &readfds);
   nfds = select(fd+1, &readfds, NULL, NULL, &tv);
//   printf("tread:1 --- nfds=%d, uart_fd=%d!!!\n",nfds, fd);
   if (nfds <= 0) 
   {
     if(nfds == 0){
       errno = ETIME;
       #ifdef test_uart
       perror("tread select:");
       #endif
     }
     else{
     	#ifdef test_uart
     	perror("tread select:");
     	#endif
    }
     	
     return(-1);
   }
   
   status=read(fd, buf, nbytes);
   #ifdef test_uart
   if(status < 0) {
   		perror("tread read uart:");
   		char c;
  		scanf("%c",&c);
  		while(c!='s');
 		}
   #endif
   
   return(status);
}

//----------------------------------------------------------------------------
// 从串口中读去nbytes个字节的数据
//----------------------------------------------------------------------------
int treadn(int fd, char *buf, unsigned int nbytes, unsigned int sectimout, unsigned int usectimout)
{
   unsigned int nleft;
   int nread;

   nleft = nbytes;
   
//   printf("treadn:1 --- nleft=%d!!!\n",nleft);
   while(nleft > 0)
   {
     if((nread = tread(fd, buf, nleft, sectimout,usectimout)) < 0) 
     {
       if(nleft == nbytes)
         return(-1); /* error, return -1 */
       else
         break;      /* error, return amount read so far */
     } 
     else if(nread == 0) 
     {
       break;          /* EOF */
     }
     
     nleft -= nread;
     buf += nread;
   }
   
//   printf("treadn:3 !!!\n");
   *buf='\0';
   
	 return(nbytes - nleft);      /* return >= 0 */
}

//----------------------------------------------------------------------------
// 向串口写入nbytes个字节的数据
//----------------------------------------------------------------------------
int writen(int fd, char *buf, unsigned int nbytes)
{
	unsigned int nleft;
	int nwrite;
	int WriteTimers;
	
	nleft = nbytes;
	WriteTimers = 0;

	while((nleft>0)&&(WriteTimers<10))
	{
		WriteTimers++;
		
		if((nwrite=write(fd,buf,nleft))<0)
			break;

		nleft-=nwrite;
		buf+=nwrite;
	}
	
	return (nbytes-nleft);
}

//----------------------------------------------------------------------------
//接收：解析串口协议
//----------------------------------------------------------------------------
void Uart_Rx_Options(int fd, char *buf, unsigned int sectimout, unsigned int usectimout)
{
	static int flag=0;
	int nread=0,i=0;
	static int length=0;
	static char command=0;
	
//	nread=treadn(fd,buf,5,sectimout,usectimout);
//	printf( "nread:%d\n", nread);
//	
//	for(i=0;i<nread;i++)
//		printf( "readdata:0x%02x\n", buf[i]);
	
	if(flag==0)
	{
		nread=treadn(fd,buf,3,sectimout,usectimout);//nread<0:没有收到数据 nead=0:eof nread>0:数据传送中途通信中断 nread=3:传输正常
		
		#ifdef test_uart
			printf( "nread:%d\n", nread);
		
		for(i=0;i<nread;i++)
			printf( "readdata:0x%02x\n", buf[i]);
			
		#endif
		
		if(nread==3)
		{
			if(!strcmp("STA",buf))
			{
				#ifdef test_uart
				printf("STA\n");
				#endif
				
				flag=1;
			}
		}
	}
	else if(flag==1)
	{
		nread=treadn(fd,buf,2,sectimout,usectimout);
		
		if(nread==2)
		{
			command=buf[0];
			length=buf[1];
			if(command==GET_ID_COMMAND)
				length=9*length;
			flag=2;

			#ifdef test_uart
			printf("command:0x%02x len:0x%02x\n",command,length);
			#endif
		}
		else
		{
			#ifdef test_uart
			printf("Error:flag=%d,nread=%d\n",flag,nread);
			#endif
			
			Uart_Tx_Options(fd,uart_tx_buff,REQUEST_RESEND_COMMAND,NULL);
			flag=0;
			//返回错误或错误处理
		}
	}
	else if(flag==2)
	{
		nread=treadn(fd,buf,length,sectimout,usectimout);
		
		if(nread==length)
		{
			if(Uart_Command_Analyze(command,length,buf))
				flag=3;
			else
			{
				#ifdef test_uart
				printf("Error:Illegal Command!\n");
				#endif				
				
				Uart_Tx_Options(fd,uart_tx_buff,REQUEST_RESEND_COMMAND,NULL);
				flag=0;
			}
		}
		else
		{
			#ifdef test_uart
			printf("Error:flag=%d,nread=%d\n",flag,nread);
			#endif
			
			Uart_Tx_Options(fd,uart_tx_buff,REQUEST_RESEND_COMMAND,NULL);
			flag=0;
			//返回错误或错误处理
		}
	}
	else if(flag==3)
	{
		nread=treadn(fd,buf,3,sectimout,usectimout);
		
		if(nread==3)
		{
			if(!strcmp("END",buf))
			{
				#ifdef test_uart
				printf("END\n");
				#endif
				
				flag=0;
			}
			else
			{
				#ifdef test_uart
				printf("Error:flag=%d,buf=%s\n",flag,buf);
				#endif
				
				Uart_Tx_Options(fd,uart_tx_buff,REQUEST_RESEND_COMMAND,NULL);
				flag=0;
			}
		}
		else
		{
			#ifdef test_uart
			printf("Error:flag=%d,nread=%d\n",flag,nread);
			#endif
			
			Uart_Tx_Options(fd,uart_tx_buff,REQUEST_RESEND_COMMAND,NULL);
			flag=0;
			//返回错误或错误处理
		}
	}
	
	return;
}

//----------------------------------------------------------------------------
//串口数据指令解析
//----------------------------------------------------------------------------
int Uart_Command_Analyze(char command,int length,char* buf)
{
	int i=0;
	int num=0;
	char* tmp=NULL;
	
	if(command==TEST_COMMAND) //test
	{
		printf("Len=%d ",length);

		for(i=0;i<length-1;i++)
			printf( "0x%02x ", buf[i]);
				
		printf( "0x%02x\n", buf[i]);
	}
	else if(command==GET_ID_COMMAND)
	{
		if(Module.Module_Info_p!=NULL)
			return 1;
		num=length/9;
		Module.number=num;
		tmp=malloc(length+2); //+2个byte用于存放板数num
		((MfiUInt16*)tmp)[0]=num;
		Module.Module_Info_p=(_Module_Info_p)(tmp+2);
		
		for(i=0;i<num;i++)
		{
			Module.Module_Info_p[i]=((_Module_Info_p)buf)[i];
		}
		
		#ifdef test_uart
		
		//test
		for(i=0;i<num;i++)
		{
			printf( "%d ", Module.Module_Info_p[i].mod_ip);
			printf( "%d ", Module.Module_Info_p[i].mod_id[0]);
			printf( "%d ", Module.Module_Info_p[i].mod_id[1]);
			printf( "%d ", Module.Module_Info_p[i].mod_id[2]);
			printf( "%d ", Module.Module_Info_p[i].mod_id[3]);
			printf( "%d ", Module.Module_Info_p[i].manf_id[0]);
			printf( "%d ", Module.Module_Info_p[i].manf_id[1]);
			printf( "%d ", Module.Module_Info_p[i].mod_resv1);
			printf( "%d\n", Module.Module_Info_p[i].mod_resv2);
		}
		
		#endif
	}
	else
		return 0;
	
	return 1;
}

//----------------------------------------------------------------------------
//发送:按串口协议发送
//----------------------------------------------------------------------------
void Uart_Tx_Options(int fd, char *buf, char cmd, char* data)
{
	int i=0;
	
	strcpy(buf,"STA");
	buf[3]=cmd;
	buf[4]='\0';
	
	if(data!=NULL)
		strcat(buf,data);
		
	strcat(buf,"END");
//	strcpy(buf,"ABCDE");
//	buf[5]='\0';
	
	#ifdef test_uart
	
	printf("uartwrite Len=%d:\n",strlen(buf));

		while(buf[i]!='\0')
		{
			printf( "0x%02x ", buf[i]);
			i++;
		}
		printf("\n");
	
	#endif
				
	i=writen(fd, buf, strlen(buf));
	
	#ifdef test_uart
	printf("writen:%d\n",i);
	#endif
	return;
}