#ifndef _MFI_GPMC_CMD_HEADER_
#define _MFI_GPMC_CMD_HEADER_

#define GPMC_MAGIC 'x'    //使用x作为幻数
#define GPMC_IRQ_GET_CMD _IOR(GPMC_MAGIC,0,int)
#define GPMC_MAX_NUM 1

#endif
