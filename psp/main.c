/*===================================================================*/
/*                                                                   */
/*  InfoNES_System_Win.cpp : The function which depends on a system  */
/*                           (for Windows)                           */
/*                                                                   */
/*  2000/05/12  InfoNES Project                                      */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Include files                                                    */
/*-------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "pg.h"
#include "sound.h"

#include "../InfoNES.h"
#include "../InfoNES_System.h"
#include "../InfoNES_pAPU.h"

WORD NesPalette[ 64 ] =
{
  0x39ce, 0x1071, 0x0015, 0x2013, 0x440e, 0x5402, 0x5000, 0x3c20,
  0x20a0, 0x0100, 0x0140, 0x00e2, 0x0ceb, 0x0000, 0x0000, 0x0000,
  0x5ef7, 0x01dd, 0x10fd, 0x401e, 0x5c17, 0x700b, 0x6ca0, 0x6521,
  0x45c0, 0x0240, 0x02a0, 0x0247, 0x0211, 0x0000, 0x0000, 0x0000,
  0x7fff, 0x1eff, 0x2e5f, 0x223f, 0x79ff, 0x7dd6, 0x7dcc, 0x7e67,
  0x7ae7, 0x4342, 0x2769, 0x2ff3, 0x03bb, 0x0000, 0x0000, 0x0000,
  0x7fff, 0x579f, 0x635f, 0x6b3f, 0x7f1f, 0x7f1b, 0x7ef6, 0x7f75,
  0x7f94, 0x73f4, 0x57d7, 0x5bf9, 0x4ffe, 0x0000, 0x0000, 0x0000
};

/*-------------------------------------------------------------------*/
/*  ROM image file information                                       */
/*-------------------------------------------------------------------*/

char szRomName[ 256 ];
char szSaveName[ 256 ];
int nSRAM_SaveFlag;

int LoadSRAM();
int getFilePath(char *out);
extern char lastpath[256];

// ----------------------------------------------------------------------------------------
int bSleep=0;
// ホームボタン終了時にコールバック
int exit_callback(void)
{
	bSleep=1;
	sceKernelExitGame();
	return 0;
}

// スリープ時や不定期にコールバック
void power_callback(int unknown, int pwrflags)
{
	if(pwrflags & POWER_CB_POWER){
		bSleep=1;
	}else if(pwrflags & POWER_CB_RESCOMP){
		bSleep=0;
	}
	// コールバック関数の再登録
	// （一度呼ばれたら再登録しとかないと次にコールバックされない）
	int cbid = sceKernelCreateCallback("Power Callback", power_callback);
	scePowerRegisterCallback(0, cbid);
}

// ポーリング用スレッド
int CallbackThread(int args, void *argp)
{
	int cbid;
	
	// コールバック関数の登録
	cbid = sceKernelCreateCallback("Exit Callback", exit_callback);
	sceKernelRegisterExitCallback(cbid);
	cbid = sceKernelCreateCallback("Power Callback", power_callback);
	scePowerRegisterCallback(0, cbid);
	
	// ポーリング
	sceKernelPollCallbacks();
}

int SetupCallbacks(void)
{
	int thid = 0;
	
	// ポーリング用スレッドの生成
	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
		sceKernelStartThread(thid, 0, 0);
	
	return thid;
}

// ----------------------------------------------------------------------------------------

/*===================================================================*/
/*                                                                   */
/*                WinMain() : Application main                       */
/*                                                                   */
/*===================================================================*/
int xmain(int argc, char *argv)
{
	int ret = 0;
	char tmp[MAX_PATH];
	
	SetupCallbacks();
	pgInit();
	wavoutInit();
	
	int r, g, b;
	for(int i=0; i<64; i++)
	{
		r = (NesPalette[i]>>10) & 0x1F;
		g = (NesPalette[i]>>5) & 0x1F;
		b = NesPalette[i] & 0x1F;
		NesPalette[i] = (b<<10) | (g<<5) | r;
	}
	
	strcpy(lastpath, argv);
	*(strrchr(lastpath, '/')+1) = 0;
	for(;;)
	{
		wavout_enable=0;
		pgScreenFrame(2,0);
		while(!getFilePath(tmp));
		pgScreenFrame(1,0);
		pgFillvram(0);
		wavout_enable=1;
		
		InfoNES_Load(tmp);
		// Set a ROM image name
		strcpy( szRomName, tmp);
		// Load SRAM
		LoadSRAM();
		
		InfoNES_Main();
	}
	
	return 0;
}

/*===================================================================*/
/*                                                                   */
/*           LoadSRAM() : Load a SRAM                                */
/*                                                                   */
/*===================================================================*/
int LoadSRAM()
{
/*
 *  Load a SRAM
 *
 *  Return values
 *     0 : Normally
 *    -1 : SRAM data couldn't be read
 */

  FILE *fp;
  unsigned char pSrcBuf[ SRAM_SIZE ];
  unsigned char chData;
  unsigned char chTag;
  int nRunLen;
  int nDecoded;
  int nDecLen;
  int nIdx;

  // It doesn't need to save it
  nSRAM_SaveFlag = 0;

  // It is finished if the ROM doesn't have SRAM
  if ( !ROM_SRAM )
    return 0;

  // There is necessity to save it
  nSRAM_SaveFlag = 1;

  // The preparation of the SRAM file name
  strcpy( szSaveName, szRomName );
  strcpy( strrchr( szSaveName, '.' ) + 1, "srm" );

  /*-------------------------------------------------------------------*/
  /*  Read a SRAM data                                                 */
  /*-------------------------------------------------------------------*/

  // Open SRAM file
  fp = fopen( szSaveName, "rb" );
  if ( fp == NULL )
    return -1;

  // Read SRAM data
  fread( pSrcBuf, SRAM_SIZE, 1, fp );

  // Close SRAM file
  fclose( fp );

  /*-------------------------------------------------------------------*/
  /*  Extract a SRAM data                                              */
  /*-------------------------------------------------------------------*/

  nDecoded = 0;
  nDecLen = 0;

  chTag = pSrcBuf[ nDecoded++ ];

  while ( nDecLen < 8192 )
  {
    chData = pSrcBuf[ nDecoded++ ];

    if ( chData == chTag )
    {
      chData = pSrcBuf[ nDecoded++ ];
      nRunLen = pSrcBuf[ nDecoded++ ];
      for ( nIdx = 0; nIdx < nRunLen + 1; ++nIdx )
      {
        SRAM[ nDecLen++ ] = chData;
      }
    }
    else
    {
      SRAM[ nDecLen++ ] = chData;
    }
  }

  // Successful
  return 0;
}

/*===================================================================*/
/*                                                                   */
/*           SaveSRAM() : Save a SRAM                                */
/*                                                                   */
/*===================================================================*/
int SaveSRAM()
{
/*
 *  Save a SRAM
 *
 *  Return values
 *     0 : Normally
 *    -1 : SRAM data couldn't be written
 */

  FILE *fp;
  int nUsedTable[ 256 ];
  unsigned char chData;
  unsigned char chPrevData;
  unsigned char chTag;
  int nIdx;
  int nEncoded;
  int nEncLen;
  int nRunLen;
  unsigned char pDstBuf[ SRAM_SIZE ];

  if ( !nSRAM_SaveFlag )
    return 0;  // It doesn't need to save it

  /*-------------------------------------------------------------------*/
  /*  Compress a SRAM data                                             */
  /*-------------------------------------------------------------------*/

  memset( nUsedTable, 0, sizeof nUsedTable );

  for ( nIdx = 0; nIdx < SRAM_SIZE; ++nIdx )
  {
    ++nUsedTable[ SRAM[ nIdx++ ] ];
  }
  for ( nIdx = 1, chTag = 0; nIdx < 256; ++nIdx )
  {
    if ( nUsedTable[ nIdx ] < nUsedTable[ chTag ] )
      chTag = nIdx;
  }

  nEncoded = 0;
  nEncLen = 0;
  nRunLen = 1;

  pDstBuf[ nEncLen++ ] = chTag;

  chPrevData = SRAM[ nEncoded++ ];

  while ( nEncoded < SRAM_SIZE && nEncLen < SRAM_SIZE - 133 )
  {
    chData = SRAM[ nEncoded++ ];

    if ( chPrevData == chData && nRunLen < 256 )
      ++nRunLen;
    else
    {
      if ( nRunLen >= 4 || chPrevData == chTag )
      {
        pDstBuf[ nEncLen++ ] = chTag;
        pDstBuf[ nEncLen++ ] = chPrevData;
        pDstBuf[ nEncLen++ ] = nRunLen - 1;
      }
      else
      {
        for ( nIdx = 0; nIdx < nRunLen; ++nIdx )
          pDstBuf[ nEncLen++ ] = chPrevData;
      }

      chPrevData = chData;
      nRunLen = 1;
    }

  }
  if ( nRunLen >= 4 || chPrevData == chTag )
  {
    pDstBuf[ nEncLen++ ] = chTag;
    pDstBuf[ nEncLen++ ] = chPrevData;
    pDstBuf[ nEncLen++ ] = nRunLen - 1;
  }
  else
  {
    for ( nIdx = 0; nIdx < nRunLen; ++nIdx )
      pDstBuf[ nEncLen++ ] = chPrevData;
  }

  /*-------------------------------------------------------------------*/
  /*  Write a SRAM data                                                */
  /*-------------------------------------------------------------------*/

  // Open SRAM file
  fp = fopen( szSaveName, "wb" );
  if ( fp == NULL )
    return -1;

  // Write SRAM data
  fwrite( pDstBuf, nEncLen, 1, fp );

  // Close SRAM file
  fclose( fp );

  // Successful
  return 0;
}

/*===================================================================*/
/*                                                                   */
/*                  InfoNES_Menu() : Menu screen                     */
/*                                                                   */
/*===================================================================*/
int InfoNES_Menu()
{
/*
 *  Menu screen
 *
 *  Return values
 *     0 : Normally
 *    -1 : Exit InfoNES
 */

	// Nothing to do here
  return 0;
}

/*===================================================================*/
/*                                                                   */
/*               InfoNES_ReadRom() : Read ROM image file             */
/*                                                                   */
/*===================================================================*/
int InfoNES_ReadRom( const char *pszFileName )
{
/*
 *  Read ROM image file
 *
 *  Parameters
 *    const char *pszFileName          (Read)
 *
 *  Return values
 *     0 : Normally
 *    -1 : Error
 */

  FILE *fp;

  /* Open ROM file */
  fp = fopen( pszFileName, "rb" );
  if ( fp == NULL )
    return -1;

  /* Read ROM Header */
  fread( &NesHeader, sizeof NesHeader, 1, fp );
  if ( memcmp( NesHeader.byID, "NES\x1a", 4 ) != 0 )
  {
    /* not .nes file */
    fclose( fp );
    return -1;
  }

  /* Clear SRAM */
  memset( SRAM, 0, SRAM_SIZE );

  /* If trainer presents Read Triner at 0x7000-0x71ff */
  if ( NesHeader.byInfo1 & 4 )
  {
    fread( &SRAM[ 0x1000 ], 512, 1, fp );
  }

  /* Allocate Memory for ROM Image */
  ROM = (BYTE *)malloc( NesHeader.byRomSize * 0x4000 );

  /* Read ROM Image */
  fread( ROM, 0x4000, NesHeader.byRomSize, fp );

  if ( NesHeader.byVRomSize > 0 )
  {
    /* Allocate Memory for VROM Image */
    VROM = (BYTE *)malloc( NesHeader.byVRomSize * 0x2000 );

    /* Read VROM Image */
    fread( VROM, 0x2000, NesHeader.byVRomSize, fp );
  }

  /* File close */
  fclose( fp );

  /* Successful */
  return 0;
}

/*===================================================================*/
/*                                                                   */
/*           InfoNES_ReleaseRom() : Release a memory for ROM         */
/*                                                                   */
/*===================================================================*/
void InfoNES_ReleaseRom()
{
/*
 *  Release a memory for ROM
 *
 */

  if ( ROM )
  {
    free( ROM );
    ROM = NULL;
  }

  if ( VROM )
  {
    free( VROM );
    VROM = NULL;
  }
}

/*===================================================================*/
/*                                                                   */
/*      InfoNES_LoadFrame() :                                        */
/*           Transfer the contents of work frame on the screen       */
/*                                                                   */
/*===================================================================*/
void InfoNES_LoadFrame()
{
/*
 *  Transfer the contents of work frame on the screen
 *
 */
/*
	unsigned long *v0,yy;

	v0=(unsigned long *)pgGetVramAddr(0,0);
	for (yy=0; yy<NES_DISP_HEIGHT; yy++) {
		__memcpy4(v0,WorkFrame,NES_DISP_WIDTH/2);
		v0+=(LINESIZE/2-NES_DISP_WIDTH/2);
	}
*/
	static int x=0, y=0, vx=2, vy=2;
	
	x+=vx;
	y+=vy;
	
	//pgBitBlt(x,y,NES_DISP_WIDTH,NES_DISP_HEIGHT,1,WorkFrame);
	
	unsigned long *v0,*d=WorkFrame;
	v0=(unsigned long *)pgGetVramAddr(x,y);
	for (int yy=0; yy<NES_DISP_HEIGHT; yy++)
	{
		for (int xx=0; xx<NES_DISP_WIDTH; xx+=2)
			*v0++=*d++;
		v0+=(LINESIZE-NES_DISP_WIDTH)/2;
	}
	
	
	if(x<=0 | x>SCREEN_WIDTH-NES_DISP_WIDTH)  vx*=-1;
	if(y<=0 | y>SCREEN_HEIGHT-NES_DISP_HEIGHT) vy*=-1;
}

/*===================================================================*/
/*                                                                   */
/*             InfoNES_PadState() : Get a joypad state               */
/*                                                                   */
/*===================================================================*/
void InfoNES_PadState( DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem )
{
/*
 *  Get a joypad state
 *
 *  Parameters
 *    DWORD *pdwPad1                   (Write)
 *      Joypad 1 State
 *
 *    DWORD *pdwPad2                   (Write)
 *      Joypad 2 State
 *
 *    DWORD *pdwSystem                 (Write)
 *      Input for InfoNES
 *
 */

  static DWORD dwSysOld;
  DWORD dwTemp;

  /* Joypad 1 */
  sceCtrlPeekBufferPositive(&paddata, 1);
  *pdwPad1 = 0;
  if(paddata.buttons & CTRL_CIRCLE) *pdwPad1|=1<<0;
  if(paddata.buttons & CTRL_CROSS)  *pdwPad1|=1<<1;
  if(paddata.buttons & CTRL_SELECT) *pdwPad1|=1<<2;
  if(paddata.buttons & CTRL_START)  *pdwPad1|=1<<3;
  if(paddata.buttons & CTRL_UP)     *pdwPad1|=1<<4;
  if(paddata.buttons & CTRL_DOWN)   *pdwPad1|=1<<5;
  if(paddata.buttons & CTRL_LEFT)   *pdwPad1|=1<<6;
  if(paddata.buttons & CTRL_RIGHT)  *pdwPad1|=1<<7;
  *pdwPad1 = *pdwPad1 | ( *pdwPad1 << 8 );
  
  /* Joypad 2 */
  *pdwPad2 = 0;

  /* Input for InfoNES */
  *pdwSystem = paddata.buttons & CTRL_LTRIGGER;
}

/*===================================================================*/
/*                                                                   */
/*             InfoNES_MemoryCopy() : memcpy                         */
/*                                                                   */
/*===================================================================*/
void *InfoNES_MemoryCopy( void *dest, const void *src, int count )
{
/*
 *  memcpy
 *
 *  Parameters
 *    void *dest                       (Write)
 *      Points to the starting address of the copied block’s destination
 *
 *    const void *src                  (Read)
 *      Points to the starting address of the block of memory to copy
 *
 *    int count                        (Read)
 *      Specifies the size, in bytes, of the block of memory to copy
 *
 *  Return values
 *    Pointer of destination
 */

  memcpy( dest, src, count );
  return dest;
}

/*===================================================================*/
/*                                                                   */
/*             InfoNES_MemorySet() : Get a joypad state              */
/*                                                                   */
/*===================================================================*/
void *InfoNES_MemorySet( void *dest, int c, int count )
{
/*
 *  memset
 *
 *  Parameters
 *    void *dest                       (Write)
 *      Points to the starting address of the block of memory to fill
 *
 *    int c                            (Read)
 *      Specifies the byte value with which to fill the memory block
 *
 *    int count                        (Read)
 *      Specifies the size, in bytes, of the block of memory to fill
 *
 *  Return values
 *    Pointer of destination
 */

  memset( dest, c, count ); 
  return dest;
}

/*===================================================================*/
/*                                                                   */
/*                DebugPrint() : Print debug message                 */
/*                                                                   */
/*===================================================================*/
void InfoNES_DebugPrint( char *pszMsg )
{
//  _RPT0( _CRT_WARN, pszMsg );
}

/*===================================================================*/
/*                                                                   */
/*        InfoNES_SoundInit() : Sound Emulation Initialize           */
/*                                                                   */
/*===================================================================*/
void InfoNES_SoundInit( void ) {}

/*===================================================================*/
/*                                                                   */
/*        InfoNES_SoundOpen() : Sound Open                           */
/*                                                                   */
/*===================================================================*/
int InfoNES_SoundOpen( int samples_per_sync, int sample_rate ) 
{
/*
  lpSndDevice = new DIRSOUND( hWndMain );

  if ( !lpSndDevice->SoundOpen( samples_per_sync, sample_rate ) )
  {
    InfoNES_MessageBox( "SoundOpen() Failed." );
    exit(0);
  }

  // if sound mute, stop sound
  if ( APU_Mute )
  {
    if (!lpSndDevice->SoundMute( APU_Mute ) )
    {
      InfoNES_MessageBox( "SoundMute() Failed." );
      exit(0);
    }
  }
*/
  return(1);
}

/*===================================================================*/
/*                                                                   */
/*        InfoNES_SoundClose() : Sound Close                         */
/*                                                                   */
/*===================================================================*/
void InfoNES_SoundClose( void ) 
{
/*
  lpSndDevice->SoundClose();
  delete lpSndDevice;
	lpSndDevice = NULL;
*/
}

/*===================================================================*/
/*                                                                   */
/*            InfoNES_SoundOutput() : Sound Output Waves             */           
/*                                                                   */
/*===================================================================*/
void InfoNES_SoundOutput( int samples, BYTE *wave1, BYTE *wave2, BYTE *wave3, BYTE *wave4, BYTE *wave5 ) 
{
	short *buf;
	int wave;
	int sound_buf_len = 1024;
	static unsigned int nextbank=1;
	unsigned int playbank=cur_play/sound_buf_len;
	
	if (nextbank!=playbank) {
		buf = &sound_buf[nextbank*sound_buf_len*2];
		for ( int i=0; i < sound_buf_len; i++)
		{
			wave = ( wave1[i] + wave2[i] + wave3[i] + wave4[i] + wave5[i] ) / 5;
			buf[i*2] = buf[i*2+1] = (wave-128)*256;
		}
		nextbank++;
		if( (sound_buf_len * (nextbank+1)) > (SOUND_BANKLEN*4) ){ nextbank = 0; }
	}
}

/*===================================================================*/
/*                                                                   */
/*            InfoNES_StartTimer() : Start MM Timer                  */           
/*                                                                   */
/*===================================================================*/
static void InfoNES_StartTimer()
{
/*
  TIMECAPS caps;

  timeGetDevCaps( &caps, sizeof(caps) );
  timeBeginPeriod( caps.wPeriodMin );

  uTimerID = 
    timeSetEvent( caps.wPeriodMin * TIMER_PER_LINE, caps.wPeriodMin, TimerFunc, 0, (UINT)TIME_PERIODIC );

  // Calculate proper timing
  wLinePerTimer = LINE_PER_TIMER * caps.wPeriodMin;

  // Initialize timer variables
  wLines = 0;
  bWaitFlag = TRUE;

  // Initialize Critical Section Object
  InitializeCriticalSection( &WaitFlagCriticalSection );
*/
}

/*===================================================================*/
/*                                                                   */
/*            InfoNES_StopTimer() : Stop MM Timer                    */           
/*                                                                   */
/*===================================================================*/
static void InfoNES_StopTimer()
{
/*
  if ( 0 != uTimerID )
  {
    TIMECAPS caps;
    timeKillEvent( uTimerID );
    uTimerID = 0;
    timeGetDevCaps( &caps, sizeof(caps) );
    timeEndPeriod( caps.wPeriodMin * TIMER_PER_LINE );
  }
  // Delete Critical Section Object
  DeleteCriticalSection( &WaitFlagCriticalSection );
*/
}

/*===================================================================*/
/*                                                                   */
/*           TimerProc() : MM Timer Callback Function                */
/*                                                                   */
/*===================================================================*/
/*
static void CALLBACK TimerFunc( UINT nID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
  if ( NULL != m_hThread )
  {  
    EnterCriticalSection( &WaitFlagCriticalSection );
    bWaitFlag = FALSE;
    LeaveCriticalSection( &WaitFlagCriticalSection );
  }
}
*/
/*===================================================================*/
/*                                                                   */
/*            InfoNES_Wait() : Wait Emulation if required            */
/*                                                                   */
/*===================================================================*/
void InfoNES_Wait()
{
/*
  wLines++;
  if ( wLines < wLinePerTimer )
    return;
  wLines = 0;

  if ( bAutoFrameskip )
  {
    // Auto Frameskipping
    if ( !bWaitFlag )
    {
      // Increment Frameskip Counter
	    FrameSkip++;
    } 
#if 1
    else {
      // Decrement Frameskip Counter
      if ( FrameSkip > 2 )
      {
		    FrameSkip--;  
      }
    }
#endif
  }

  // Wait if bWaitFlag is TRUE
  if ( bAutoFrameskip )
  {
    while ( bWaitFlag )
      Sleep(0);
  }

  // set bWaitFlag is TRUE
  EnterCriticalSection( &WaitFlagCriticalSection );
  bWaitFlag = TRUE;
  LeaveCriticalSection( &WaitFlagCriticalSection );
*/
}


/*===================================================================*/
/*                                                                   */
/*            InfoNES_MessageBox() : Print System Message            */
/*                                                                   */
/*===================================================================*/
void InfoNES_MessageBox( char *pszMsg, ... )
{
/*
  char pszErr[ 1024 ];
  va_list args;

  va_start( args, pszMsg );
  vsprintf( pszErr, pszMsg, args );  pszErr[ 1023 ] = '\0';
  va_end( args );
  MessageBox( hWndMain, pszErr, APP_NAME, MB_OK | MB_ICONSTOP );
*/
}

