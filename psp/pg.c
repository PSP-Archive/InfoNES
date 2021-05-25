// primitive graphics for Hello World sce

#include <stdio.h>
#include "syscall.h"
#include "pg.h"

#include "font.h"
#include "fontNaga10.h"

//variables
//char *pg_vramtop=(char *)0x04000000;
#define pg_vramtop ((char *)0x04000000)
long pg_screenmode;
long pg_showframe;
long pg_drawframe;
unsigned long pgc_csr_x[2], pgc_csr_y[2];
unsigned long pgc_fgcolor[2], pgc_bgcolor[2];
char pgc_fgdraw[2], pgc_bgdraw[2];
char pgc_mag[2];

void pgWaitVn(unsigned long count)
{
	for (; count>0; --count) {
		sceDisplayWaitVblankStart();
	}
}


void pgWaitV()
{
	sceDisplayWaitVblankStart();
}


char *pgGetVramAddr(unsigned long x,unsigned long y)
{
	return pg_vramtop+(pg_drawframe?FRAMESIZE:0)+x*PIXELSIZE*2+y*LINESIZE*2+0x40000000;
//	return pg_vramtop+(pg_drawframe?FRAMESIZE:0)+x*PIXELSIZE*2+y*LINESIZE*2;//+0x40000000;	//変わらないらしい
}

void pgPrint_drawbg(unsigned long x, unsigned long y, unsigned long color, unsigned long bgcolor, const char *str)
{
	while (*str!=0 && x<CMAX_X && y<CMAX_Y) {
		pgPutChar(x*8,y*8,color,bgcolor,*str,1,1,1);
		str++;
		x++;
		if (x>=CMAX_X) {
			x=0;
			y++;
		}
	}
}

void pgPrint(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
	while (*str!=0 && x<CMAX_X && y<CMAX_Y) {
		pgPutChar(x*8,y*8,color,0,*str,1,0,1);
		str++;
		x++;
		if (x>=CMAX_X) {
			x=0;
			y++;
		}
	}
}

void pgPrint2(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
	while (*str!=0 && x<CMAX2_X && y<CMAX2_Y) {
		pgPutChar(x*16,y*16,color,0,*str,1,0,2);
		str++;
		x++;
		if (x>=CMAX2_X) {
			x=0;
			y++;
		}
	}
}


void pgPrint4(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
	while (*str!=0 && x<CMAX4_X && y<CMAX4_Y) {
		pgPutChar(x*32,y*32,color,0,*str,1,0,4);
		str++;
		x++;
		if (x>=CMAX4_X) {
			x=0;
			y++;
		}
	}
}

void pgDrawFrame(unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long color)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned long i;

	vptr0=(unsigned char *)pgGetVramAddr(0,0);
	for(i=x1; i<=x2; i++){
		((unsigned short *)vptr0)[i*PIXELSIZE + y1*LINESIZE] = color;
		((unsigned short *)vptr0)[i*PIXELSIZE + y2*LINESIZE] = color;
	}
	for(i=y1; i<=y2; i++){
		((unsigned short *)vptr0)[x1*PIXELSIZE + i*LINESIZE] = color;
		((unsigned short *)vptr0)[x2*PIXELSIZE + i*LINESIZE] = color;
	}
}

void pgFillBox(unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long color)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned long i, j;

	vptr0=(unsigned char *)pgGetVramAddr(0,0);
	for(i=y1; i<=y2; i++){
		for(j=x1; j<=x2; j++){
			((unsigned short *)vptr0)[j*PIXELSIZE + i*LINESIZE] = color;
		}
	}
}

void pgFillvram(unsigned long color)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned long i;

	vptr0=(unsigned char *)pgGetVramAddr(0,0);
	for (i=0; i<FRAMESIZE/2; i++) {
		*(unsigned short *)vptr0=color;
		vptr0+=PIXELSIZE*2;
	}
}

void pgPutPoint(int x, int y, unsigned short color)
{
	*(unsigned short*)pgGetVramAddr(x,y) = color;
}

void pgBitBlt(unsigned long x,unsigned long y,unsigned long w,unsigned long h,unsigned long mag,const unsigned short *d)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned char *vptr;		//pointer to vram
	unsigned long xx,yy,mx,my;
	const unsigned short *dd;

	vptr0=(unsigned char *)pgGetVramAddr(x,y);
	for (yy=0; yy<h; yy++) {
		for (my=0; my<mag; my++) {
			vptr=vptr0;
			dd=d;
			for (xx=0; xx<w; xx++) {
				for (mx=0; mx<mag; mx++) {
					*(unsigned short *)vptr=*dd;
					vptr+=PIXELSIZE*2;
				}
				dd++;
			}
			vptr0+=LINESIZE*2;
		}
		d+=w;
	}

}

void pgPutChar(unsigned long x,unsigned long y,unsigned long color,unsigned long bgcolor,unsigned char ch,char drawfg,char drawbg,char mag)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned char *vptr;		//pointer to vram
	const unsigned char *cfont;		//pointer to font
	unsigned long cx,cy;
	unsigned long b;
	char mx,my;

	cfont=font+ch*8;
	vptr0=(unsigned char *)pgGetVramAddr(x,y);
	for (cy=0; cy<8; cy++) {
		for (my=0; my<mag; my++) {
			vptr=vptr0;
			b=0x80;
			for (cx=0; cx<8; cx++) {
				for (mx=0; mx<mag; mx++) {
					if ((*cfont&b)!=0) {
						if (drawfg) *(unsigned short *)vptr=color;
					} else {
						if (drawbg) *(unsigned short *)vptr=bgcolor;
					}
					vptr+=PIXELSIZE*2;
				}
				b=b>>1;
			}
			vptr0+=LINESIZE*2;
		}
		cfont++;
	}
}


void pgScreenFrame(long mode,long frame)
{
	pg_screenmode=mode;
	frame=(frame?1:0);
	pg_showframe=frame;
	if (mode==0) {
		//screen off
		pg_drawframe=frame;
		sceDisplaySetFrameBuf(0,0,0,1);
	} else if (mode==1) {
		//show/draw same
		pg_drawframe=frame;
		sceDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
	} else if (mode==2) {
		//show/draw different
		pg_drawframe=(frame?0:1);
		sceDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
	}
}


void pgScreenFlip()
{
	pg_showframe=(pg_showframe?0:1);
	pg_drawframe=(pg_drawframe?0:1);
	sceDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,0);
}


void pgScreenFlipV()
{
	pgWaitV();
	pgScreenFlip();
}

// by kwn
void Draw_Char_Hankaku(int x,int y,const unsigned char c,int col) {
	unsigned short *vr;
	unsigned char  *fnt;
	unsigned char  pt;
	unsigned char ch;
	int x1,y1;

	ch = c;

	// mapping
	if (ch<0x20)
		ch = 0;
	else if (ch<0x80)
		ch -= 0x20;
	else if (ch<0xa0)
		ch = 0;
	else
		ch -= 0x40;

	fnt = (unsigned char *)&hankaku_font10[ch*10];

	// draw
	vr = (unsigned short *)pgGetVramAddr(x,y);
	for(y1=0;y1<10;y1++) {
		pt = *fnt++;
		for(x1=0;x1<5;x1++) {
			if (pt & 1)
				*vr = col;
			//else
			//	*vr = bgcol;
			vr++;
			pt = pt >> 1;
		}
		vr += LINESIZE-5;
	}
}

extern u8 elisa_fnt[];
// by kwn
void Draw_Char_Zenkaku(int x,int y,const unsigned char u,unsigned char d,int col) {
	// フォントにない文字
	const unsigned short font404[] = {
		0xA2AF, 11,
		0xA2C2, 8,
		0xA2D1, 11,
		0xA2EB, 7,
		0xA2FA, 4,
		0xA3A1, 15,
		0xA3BA, 7,
		0xA3DB, 6,
		0xA3FB, 4,
		0xA4F4, 11,
		0xA5F7, 8,
		0xA6B9, 8,
		0xA6D9, 38,
		0xA7C2, 15,
		0xA7F2, 13,
		0xA8C1, 720,
		0xCFD4, 43,
		0xF4A5, 1030,
		0,0
	};
	unsigned short *vr;
	unsigned short *fnt;
	unsigned short pt;
	int x1,y1;

	unsigned long n;
	unsigned short code;
	int i, j;

	// SJISコードの生成
	code = u;
	code = (code<<8) + d;

	// SJISからEUCに変換
	if(code >= 0xE000) code-=0x4000;
	code = ((((code>>8)&0xFF)-0x81)<<9) + (code&0x00FF);
	if((code & 0x00FF) >= 0x80) code--;
	if((code & 0x00FF) >= 0x9E) code+=0x62;
	else code-=0x40;
	code += 0x2121 + 0x8080;

	// EUCからフォントの番号を生成
	n = (((code>>8)&0xFF)-0xA1)*(0xFF-0xA1)
		+ (code&0xFF)-0xA1;
	j=0;
	while(font404[j]) {
		if(code >= font404[j]) {
			if(code <= font404[j]+font404[j+1]-1) {
				n = -1;
				break;
			} else {
				n-=font404[j+1];
			}
		}
		j+=2;
	}
	fnt = (unsigned short *)&zenkaku_font10[n*10];
	
	// draw
	vr = (unsigned short *)pgGetVramAddr(x,y);
	for(y1=0;y1<10;y1++) {
		pt = *fnt++;
		for(x1=0;x1<10;x1++) {
			if (pt & 1)
				*vr = col;
			//else
			//	*vr = bgcol;
			vr++;
			pt = pt >> 1;
		}
		vr += LINESIZE-10;
	}
}

// by kwn
void mh_print(int x, int y, const char *msg, int txtcol) {
	int xx = x;
	unsigned char ch = 0,bef = 0, *str=(unsigned char*)msg;

	while(*str != 0) {
		ch = *str++;
		if (bef!=0) {
			Draw_Char_Zenkaku(x,y,bef,ch,txtcol);
			x+=10;
			bef=0;
		} else {
			if (((ch>=0x80) && (ch<0xa0)) || (ch>=0xe0)) {
				bef = ch;
			} else {
				if (ch=='\n'){
					x=xx;
					y+=10;
				}else{
					Draw_Char_Hankaku(x,y,ch,txtcol);
					x+=5;
				}
			}
		}
		if (x>=480) break;
	}
}

/******************************************************************************/


void pgcLocate(unsigned long x, unsigned long y)
{
	if (x>=CMAX_X) x=0;
	if (y>=CMAX_Y) y=0;
	pgc_csr_x[pg_drawframe?1:0]=x;
	pgc_csr_y[pg_drawframe?1:0]=y;
}


void pgcColor(unsigned long fg, unsigned long bg)
{
	pgc_fgcolor[pg_drawframe?1:0]=fg;
	pgc_bgcolor[pg_drawframe?1:0]=bg;
}


void pgcDraw(char drawfg, char drawbg)
{
	pgc_fgdraw[pg_drawframe?1:0]=drawfg;
	pgc_bgdraw[pg_drawframe?1:0]=drawbg;
}


void pgcSetmag(char mag)
{
	pgc_mag[pg_drawframe?1:0]=mag;
}

void pgcCls()
{
	pgFillvram(pgc_bgcolor[pg_drawframe]);
	pgcLocate(0,0);
}

void pgcPutchar_nocontrol(const char ch)
{
	pgPutChar(pgc_csr_x[pg_drawframe]*8, pgc_csr_y[pg_drawframe]*8, pgc_fgcolor[pg_drawframe], pgc_bgcolor[pg_drawframe], ch, pgc_fgdraw[pg_drawframe], pgc_bgdraw[pg_drawframe], pgc_mag[pg_drawframe]);
	pgc_csr_x[pg_drawframe]+=pgc_mag[pg_drawframe];
	if (pgc_csr_x[pg_drawframe]>CMAX_X-pgc_mag[pg_drawframe]) {
		pgc_csr_x[pg_drawframe]=0;
		pgc_csr_y[pg_drawframe]+=pgc_mag[pg_drawframe];
		if (pgc_csr_y[pg_drawframe]>CMAX_Y-pgc_mag[pg_drawframe]) {
			pgc_csr_y[pg_drawframe]=CMAX_Y-pgc_mag[pg_drawframe];
//			pgMoverect(0,pgc_mag[pg_drawframe]*8,SCREEN_WIDTH,SCREEN_HEIGHT-pgc_mag[pg_drawframe]*8,0,0);
		}
	}
}

void pgcPutchar(const char ch)
{
	if (ch==0x0d) {
		pgc_csr_x[pg_drawframe]=0;
		return;
	}
	if (ch==0x0a) {
		if ((++pgc_csr_y[pg_drawframe])>=CMAX_Y) {
			pgc_csr_y[pg_drawframe]=CMAX_Y-1;
//			pgMoverect(0,8,SCREEN_WIDTH,SCREEN_HEIGHT-8,0,0);
		}
		return;
	}
	pgcPutchar_nocontrol(ch);
}

void pgcPuthex2(const unsigned long s)
{
	char ch;
	ch=((s>>4)&0x0f);
	pgcPutchar((ch<10)?(ch+0x30):(ch+0x40-9));
	ch=(s&0x0f);
	pgcPutchar((ch<10)?(ch+0x30):(ch+0x40-9));
}


void pgcPuthex8(const unsigned long s)
{
	pgcPuthex2(s>>24);
	pgcPuthex2(s>>16);
	pgcPuthex2(s>>8);
	pgcPuthex2(s);
}

/******************************************************************************/

void pgiInit()
{
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(1);
}

u32 new_pad;
u32 old_pad=0;
u32 now_pad;
ctrl_data_t paddata;
void readpad(void)
{
	static int n=0;
	ctrl_data_t paddata;

	sceCtrlReadBufferPositive(&paddata, 1);
	// kmg
	// Analog pad state
#ifndef PSPE
	if (paddata.analog[CTRL_ANALOG_Y] == 0xff) paddata.buttons=CTRL_DOWN;  // DOWN
	if (paddata.analog[CTRL_ANALOG_Y] == 0x00) paddata.buttons=CTRL_UP;    // UP
	if (paddata.analog[CTRL_ANALOG_X] == 0x00) paddata.buttons=CTRL_LEFT;  // LEFT
	if (paddata.analog[CTRL_ANALOG_X] == 0xff) paddata.buttons=CTRL_RIGHT; // RIGHT
#endif

	now_pad = paddata.buttons;
	/*
	new_pad = now_pad & ~old_pad;
	if(old_pad==now_pad){
		n++;
		if(n>=25){
			new_pad=now_pad;
			n = 20;
		}
	}else{
		n=0;
		old_pad = now_pad;
	}
	*/
	if(old_pad==now_pad){
		n++;
		new_pad = 0;
		if(n>=25){
			new_pad = now_pad;
			n = 20;
		}
	}else{
		n=0;
		new_pad = old_pad = now_pad;
	}
}

/******************************************************************************/


#define PGA_CHANNELS 1
#define PGA_SAMPLES 512
#define MAXVOLUME 0x8000

int pga_ready=0;
int pga_handle[PGA_CHANNELS];

short pga_sndbuf[PGA_CHANNELS][2][PGA_SAMPLES][2];


void (*pga_channel_callback[PGA_CHANNELS])(void *buf, unsigned long reqn);

int pga_threadhandle[PGA_CHANNELS];


volatile int pga_terminate=0;


static int pga_channel_thread(int args, void *argp)
{
	volatile int bufidx=0;
	int channel=*(int *)argp;

	while (pga_terminate==0) {
		void *bufptr=&pga_sndbuf[channel][bufidx];
		void (*callback)(void *buf, unsigned long reqn);
		callback=pga_channel_callback[channel];
		if (callback) {
			callback(bufptr,PGA_SAMPLES);
		} else {
			unsigned long *ptr=bufptr;
			int i;
			for (i=0; i<PGA_SAMPLES; ++i) *(ptr++)=0;
		}
		pgaOutBlocking(channel,0x8000,0x8000,bufptr);
		bufidx=(bufidx?0:1);
	}
	sceKernelExitThread(0);
	return 0;
}

/*
void pga_channel_thread_callback(int channel, void *buf, unsigned long reqn)
{
	void (*callback)(void *buf, unsigned long reqn);
	callback=pga_channel_callback[channel];
}
*/

void pgaSetChannelCallback(int channel, void *callback)
{
	pga_channel_callback[channel]=callback;
}



/******************************************************************************/



int pgaInit()
{
	int i,ret;
	int failed=0;
	char str[32];

	pga_terminate=0;
	pga_ready=0;

	for (i=0; i<PGA_CHANNELS; i++) {
		pga_handle[i]=-1;
		pga_threadhandle[i]=-1;
		pga_channel_callback[i]=0;
	}
	for (i=0; i<PGA_CHANNELS; i++) {
		if ((pga_handle[i]=sceAudio_3(-1,PGA_SAMPLES,0))<0) failed=1;
	}
	if (failed) {
		for (i=0; i<PGA_CHANNELS; i++) {
			if (pga_handle[i]!=-1) sceAudio_4(pga_handle[i]);
			pga_handle[i]=-1;
		}
		return -1;
	}
	pga_ready=1;

	strcpy(str,"pgasnd0");
	for (i=0; i<PGA_CHANNELS; i++) {
		str[6]='0'+i;
		pga_threadhandle[i]=sceKernelCreateThread(str,(pg_threadfunc_t)&pga_channel_thread,0x12,0x10000,0,NULL);
		if (pga_threadhandle[i]<0) {
			pga_threadhandle[i]=-1;
			failed=1;
			break;
		}
		ret=sceKernelStartThread(pga_threadhandle[i],sizeof(i),&i);
		if (ret!=0) {
			failed=1;
			break;
		}
	}
	if (failed) {
		pga_terminate=1;
		for (i=0; i<PGA_CHANNELS; i++) {
			if (pga_threadhandle[i]!=-1) {
				sceKernelWaitThreadEnd(pga_threadhandle[i],NULL);
				sceKernelDeleteThread(pga_threadhandle[i]);
			}
			pga_threadhandle[i]=-1;
		}
		pga_ready=0;
		return -1;
	}
	return 0;
}


void pgaTermPre()
{
	pga_ready=0;
	pga_terminate=1;
}


void pgaTerm()
{
	int i;
	pga_ready=0;
	pga_terminate=1;

	for (i=0; i<PGA_CHANNELS; i++) {
		if (pga_threadhandle[i]!=-1) {
			sceKernelWaitThreadEnd(pga_threadhandle[i],NULL);
			sceKernelDeleteThread(pga_threadhandle[i]);
		}
		pga_threadhandle[i]=-1;
	}

	for (i=0; i<PGA_CHANNELS; i++) {
		if (pga_handle[i]!=-1) {
			sceAudio_4(pga_handle[i]);
			pga_handle[i]!=-1;
		}
	}
}



int pgaOutBlocking(unsigned long channel,unsigned long vol1,unsigned long vol2,void *buf)
{
	if (!pga_ready) return -1;
	if (channel>=PGA_CHANNELS) return -1;
	if (vol1>MAXVOLUME) vol1=MAXVOLUME;
	if (vol2>MAXVOLUME) vol2=MAXVOLUME;
	return sceAudio_2(pga_handle[channel],vol1,vol2,buf);
}

//バッファは64バイト境界じゃなくても大丈夫みたい
//[0]が左、[1]が右
//サンプル速度は44100
//vol1が左

void pgInit(void)
{
	sceDisplaySetMode(0,SCREEN_WIDTH,SCREEN_HEIGHT);
	
	pgScreenFrame(0,1);
	pgcLocate(0,0);
	pgcColor(0xffff,0x0000);
	pgcDraw(1,1);
	pgcSetmag(1);
	pgScreenFrame(0,0);
	pgcLocate(0,0);
	pgcColor(0xffff,0x0000);
	pgcDraw(1,1);
	pgcSetmag(1);
	
	pgiInit();
	pgaInit();
}
