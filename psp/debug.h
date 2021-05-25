#include <stdio.h>
#include <stdarg.h>

#ifndef DEBUG_H
#define DEBUG_H

//#if defined PSPE || defined DEBUG
void DebugPrint(const char* str, ...);
//#else
//inline void DebugPrint(const char* str, ...) {}
//#endif

#ifdef PSPE
int pspeDebugWrite(const char* str,size_t length);
int pspeDebugPrintf(const char *format, ...); 
#endif

#endif

