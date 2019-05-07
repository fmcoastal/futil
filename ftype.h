#ifndef _ftype_h
#define _ftype_h

#define __FX86__

#ifdef __FX86__
#define CVMX_SYNC
#endif



#ifndef uint64_t
#define uint64_t long long unsigned
#endif
#ifndef int64_t
#define int64_t long long 
#endif
#ifndef uint32_t
#define uint32_t  unsigned int
#endif
#ifndef int32_t
#define int32_t  int
#endif
#ifndef uint16_t
#define uint16_t unsigned short
#endif
#ifndef int16_t
#define int16_t short
#endif
#ifndef uint8_t
#define uint8_t unsigned char
#endif
#ifndef int8_t
#define int8_t char
#endif


#ifndef ll 
#define ll long long
#endif
#ifndef llu
#define llu  long long unsigned
#endif
#ifndef lu
#define lu  long unsigned
#endif




#ifdef USE_UART_IO 
// this should be defined in uart_irq.c
extern uint8_t g_UartBuffer;
extern uint64_t g_uart_index;
extern int8_t g_UartBuffer[];

#define fsprintf(_x,...) { \
     sprintf(g_UartBuffer,_x,##__VA_ARGS__);\
     uart_write_string(g_uart_index,g_UartBuffer);\
}
#else
#define fsprintf printf
#endif

#endif
 
