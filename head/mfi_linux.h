#ifndef _MFI_LINUX_HEADER_
#define _MFI_LINUX_HEADER_

#include <signal.h>

extern int fd_gpmc,fd_uart;
extern sigset_t mask;

void signal_f(int signum);
int GPMC_OPEN(void);
int UART_OPEN(void);
void Sigio_init(void);
void Pthread_init(void);
void* thr_sighandle(void* arg);
void SystemInit(void);
void* thr_communication(void* arg);

#endif
