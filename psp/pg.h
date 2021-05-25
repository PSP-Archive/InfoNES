// primitive graphics for Hello World PSP

#ifndef PG_H
#define PG_H

#include "syscall.h"

#define RGB(r,g,b) ((((b>>3) & 0x1F)<<10)|(((g>>3) & 0x1F)<<5)|(((r>>3) & 0x1F)<<0)|0x8000)

extern u32 new_pad, now_pad, old_pad;
extern ctrl_data_t paddata;

#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272
#define		PIXELSIZE	1				//in short
#define		LINESIZE	512				//in short
#define		FRAMESIZE	0x44000			//in byte

//480*272 = 60*38
#define CMAX_X 60
#define CMAX_Y 34
#define CMAX2_X 30
#define CMAX2_Y 17
#define CMAX4_X 15
#define CMAX4_Y 8

void pgInit();
void pgWaitV();
void pgWaitVn(unsigned long count);
void pgScreenFrame(long mode,long frame);
void pgScreenFlip();
void pgScreenFlipV();
void pgPrint_drawbg(unsigned long x,unsigned long y,unsigned long color,unsigned long bgcolor,const char *str);
void pgPrint(unsigned long x,unsigned long y,unsigned long color,const char *str);
void pgPrint2(unsigned long x,unsigned long y,unsigned long color,const char *str);
void pgPrint4(unsigned long x,unsigned long y,unsigned long color,const char *str);
void pgFillvram(unsigned long color);
void pgBitBlt(unsigned long x,unsigned long y,unsigned long w,unsigned long h,unsigned long mag,const unsigned short *d);
void pgPutPoint(int x, int y, unsigned short color);
void pgPutChar(unsigned long x,unsigned long y,unsigned long color,unsigned long bgcolor,unsigned char ch,char drawfg,char drawbg,char mag);
void pgDrawFrame(unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long color);
void pgFillBox(unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long color);
void mh_print(int x,int y,const char *str,int col);
char *pgGetVramAddr(unsigned long x,unsigned long y);

void pgaSetChannelCallback(int channel, void *callback);

void readpad(void);

void pgcPuthex2(const unsigned long s);
void pgcPuthex8(const unsigned long s);


extern u32 now_tick;

//optimize

//long配列をコピー。配列境界は4バイトアラインされている必要あり
static inline void __memcpy4(void *d, void *s, unsigned long c)
{
	//for (; c>0; --c) *(((unsigned long *)d)++)=*(((unsigned long *)s)++);
	unsigned long *dd=d, *ss=s;
	for (; c>0; --c) *dd++ = *ss++;
}


//long配列にセット。配列境界は4バイトアラインされている必要あり
static inline void __memset4(void *d, unsigned long v, unsigned long c)
{
	//for (; c>0; --c) *(((unsigned long *)d)++)=v;
	unsigned long *dd=d;
	for (; c>0; --c) *dd++=v;
}

#endif
