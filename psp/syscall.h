/* system call prototype for PSP */

#include "debug.h"

#ifndef _SYSCALL_H_INCLUDED
#define _SYSCALL_H_INCLUDED

typedef unsigned char u8;
typedef unsigned short u16;
typedef signed long s32;
typedef unsigned long u32;

#define MAX_PATH 512
#define MAX_NAME 256
#define MAX_ENTRY 2048

/******************************************************************************/
/* SysMemUserForUser */
int sceKernelAllocPartitionMemory(int,const char*,int, unsigned int,void*);
int sceKernelFreePartitionMemory(int);
void *sceKernelGetBlockHeadAddr(int);
long sceKernelMaxFreeMemSize(void);
long sceKernelTotalFreeMemSize(void);

/******************************************************************************/
/* scePower */
#define POWER_CB_POWER		0x80000000
#define POWER_CB_HOLDON		0x40000000
#define POWER_CB_STANDBY	0x00080000
#define POWER_CB_RESCOMP	0x00040000
#define POWER_CB_RESUME		0x00020000
#define POWER_CB_SUSPEND	0x00010000
#define POWER_CB_EXT		0x00001000
#define POWER_CB_BATLOW		0x00000100
#define POWER_CB_BATTERY	0x00000080
#define POWER_CB_BATTPOWER	0x0000007F

long scePowerSetClockFrequency(long,long,long);
void scePowerRegisterCallback(int,int);
int scePowerGetBatteryLifeTime(void);
int scePowerIsPowerOnline(void);
int scePowerIsBatteryExist(void);

/******************************************************************************/
/* sceDmac */
void sceDmacMemcpy(void *dst, const void *src, int size);
void sceDmacTryMemcpy(void *dst, const void *src, int size);

/******************************************************************************/
/* sceDisplay */
void sceDisplayWaitVblankStart();
void sceDisplaySetMode(long,long,long);
void sceDisplaySetFrameBuf(char *topaddr,long linesize,long pixelsize,long);


/******************************************************************************/
/* sceCtrl */
/* Index for the two analog directions */ 
#define CTRL_ANALOG_X   0 
#define CTRL_ANALOG_Y   1 

/* Button bit masks */ 
#define CTRL_SQUARE		0x008000
#define CTRL_TRIANGLE	0x001000
#define CTRL_CIRCLE		0x002000
#define CTRL_CROSS		0x004000
#define CTRL_UP			0x000010
#define CTRL_DOWN		0x000040
#define CTRL_LEFT		0x000080
#define CTRL_RIGHT		0x000020
#define CTRL_START		0x000008
#define CTRL_SELECT		0x000001
#define CTRL_LTRIGGER	0x000100
#define CTRL_RTRIGGER	0x000200
#define CTRL_HOLD		0x020000

/* Returned control data */ 
typedef struct _ctrl_data 
{ 
	u32 frame; 
	u32 buttons; 
	u8  analog[4]; 
	u32 unused; 
} ctrl_data_t; 

/* Pass 1 to enable analogue mode */ 
void sceCtrlSetSamplingMode(s32 on);
void sceCtrlGetSamplingMode(s32 *on);

void sceCtrlSetSamplingCycle(s32 unknown);
void sceCtrlGetSamplingCycle(s32 *unknown);

int sceCtrlPeekBufferPositive(ctrl_data_t* paddata, int);
int sceCtrlPeekBufferNegative(ctrl_data_t* paddata, int);
int sceCtrlReadBufferPositive(ctrl_data_t* paddata, int);
int sceCtrlReadBufferNegative(ctrl_data_t* paddata, int);

/******************************************************************************/
/* IoFileMgrForUser */

#define SCE_O_RDONLY    0x0001 
#define SCE_O_WRONLY    0x0002 
#define SCE_O_RDWR      0x0003 
#define SCE_O_NBLOCK    0x0010 
#define SCE_O_APPEND    0x0100 
#define SCE_O_CREAT     0x0200 
#define SCE_O_TRUNC     0x0400 
#define SCE_O_EXCL      0x0800 
#define SCE_O_NOWAIT    0x8000 

int sceIoOpen(const char* file, int mode, int unknown);
int sceIoClose(int fd);
int sceIoRead(int fd, void *data, int size);
int sceIoWrite(int fd, void *data, int size);
int sceIoLseek(int fd, long long offset, int whence);
int sceIoRemove(const char *file);
int sceIoMkdir(const char *dir, int mode);
int sceIoRmdir(const char *dir);
int sceIoRename(const char *oldname, const char *newname);

enum { 
	TYPE_DIR=0x10, 
	TYPE_FILE=0x20 
};

struct dirent_tm {
	u16 unk[2]; //?????[???H
	u16 year;
	u16 mon;
	u16 mday;
	u16 hour;
	u16 min;
	u16 sec;
};

struct dirent {
	u32 unk0;
	u32 type;
	u32 size;
	struct dirent_tm ctime; //????????
	struct dirent_tm atime; //???I?A?N?Z?X????
	struct dirent_tm mtime; //???I?X?V????
	u32 unk[7]; //?????[???H
	char name[0x108];
};

int sceIoDopen(const char *fn); 
int sceIoDread(int fd, struct dirent *de); 
void sceIoDclose(int fd);
int sceIoGetStat(const char *file, struct dirent *stat);
int sceIoChdir(const char *path);

/******************************************************************************/
/* StdioForUser */
int sceKernelStdin(void);
int sceKernelStdout(void);
int sceKernelStderr(void);

/******************************************************************************/
/* sceAudio */
void sceAudio_0();//
void sceAudio_1();//
long sceAudio_2(long, long, long, void *);//
long sceAudio_3(long, long samplecount, long);//init buffer? returns handle, minus if error
void sceAudio_4(long handle);//free buffer?
void sceAudio_5();//
long sceAudio_6(long, long);//
void sceAudio_7();//
void sceAudio_8();//

/******************************************************************************/
/* LoadExecForUser */
void sceKernelExitGame();
int sceKernelRegisterExitCallback(int);

/******************************************************************************/
/* ThreadManForUser */
typedef int (*pg_threadfunc_t)(int args, void *argp);
int sceKernelCreateThread(const char *name, pg_threadfunc_t func, unsigned long priority, unsigned long stacksize, unsigned long attrib, void *unk);
int sceKernelStartThread(int hthread, int arg0, void *arg1);
void sceKernelExitThread(int ret);
int sceKernelWaitThreadEnd(int hthread, void *unk);
int sceKernelDeleteThread(int hthread);

/******************************************************************************/
/* UtilsForUser */
unsigned long sceKernelLibcClock(void);
unsigned long sceKernelLibcTime(unsigned long *);

#endif // _SYSCALL_H_INCLUDED
