#ifndef _fmap_h
#define _fmap_h

//#include   <stdio.h>
//#include   <stdlib.h>
//#include   <string.h>
//#include   <fcntl.h>
/* Not technically required, but needed on some UNIX distributions */
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <sys/mman.h>
//#include <unistd.h>
//#include <sys/mman.h>       // for mmap
#ifndef llu 
#define llu  long long unsigned
#endif

#ifndef LLU 
#define LLU  long long unsigned
#endif

// Structure for holding Memory Mapped Data Structure.

typedef struct{
    uint8_t *       MallocBuffer;      /* buffer address which was malloced */
    int             fd ;               /* file handle to mapped memory */
    unsigned char * MappedBaseAddress;
    unsigned char * VirtualAddress;
    size_t PageSize ;                  /* Page Size for the System we are using */
    void * MapAddress;
    size_t MapSize ;                   /* actual Size of the memory block allocated */
}fMmapData_t;


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
// there are two ways to use this file.
//
//
// FIRST WAY
//      Call a PhyRead64 / PhyWrite64  with a Physical address
//          1) call init to create handle  
//          1) call MapPhysical
//               * Open File Handle
//          	 * Map the Memory 
//          2) do read or write 
//          3) unmap the physical memory
//               * Close the File Handle
//   --If you use the Read and Write functions below, use PA and
//        functions will convert to VAs to make the calls to.
int64_t  PhyRead64(uint64_t Paddr, uint64_t *Reg);
int64_t  PhyWrite64(uint64_t Paddr, uint64_t Val);

int32_t  PhyRead32(uint64_t Paddr, uint32_t *Reg);
int32_t  PhyWrite32(uint64_t Paddr, uint32_t Val);

int16_t  PhyRead16(uint64_t Paddr, uint16_t *Reg);
int16_t  PhyWrite16(uint64_t Paddr, uint16_t Val);

int8_t  PhyRead8(uint64_t Paddr, uint8_t *Reg);
int8_t  PhyWrite8(uint64_t Paddr, uint8_t Val);




//
// Second  WAY  -- application manages the mapping.  
//              -- reads and writes are done without closing the Handle.
//     
//          1) call will init mapping 
//               * Open File Handle
//          	 * Map the Memory 
//          2) do read or write
//          3) unmap the physical memory
//               * Close the File Handle
//
//   look at the PhyRead64 function to see the steps needed to open and close the mapping.
//      Watch the Size Parameter in the Map Function.
//
////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//  Init Memory Map data structure.  If the *pMmapData is 0 then I allocate a buffer
//  and will clean up when the close function is called
//
int64_t MMapInit(fMmapData_t** pMmapData );

int64_t MMapDelete( fMmapData_t* pMmapData);

int64_t MMapGetVirtAddress(fMmapData_t* pMmapData,uint64_t  PhyAddress,uint64_t Size);

void    MMapPrintVars(fMmapData_t * pMmapData);




extern void *   g_MappedVirtAddr;
extern void *   g_MappedVirtAddrAndOffset;
extern uint64_t g_MappedPhysAddr;
extern size_t   g_MappedSize;
extern uint64_t g_RequestPhysAddr;
extern uint64_t g_Offset;
extern int      g_fd;

void    PrintMMapVars(void);
int64_t UnMapAddressSpace( void );
int64_t MapAddressSpace(uint64_t PhysAddress, uint64_t Size);

// #####################################################################
// printf("                                                                                  \n");
// printf(" Cash lined Memory:        |_________|_________|_________|                        \n");
// printf(" Area request for Memmap:        X__________________X                             \n");
// printf("    PhysAddress                  ^                                                \n");
// printf("    Size                         |------------------|                             \n");
// printf("                                                                                  \n");
// printf("   g_MappedPhysAddr        ^                                                      \n");
// printf("   g_MappedVirtAddr        ^                                                      \n");
// printf("   g_Offset                |-----|                                                \n");
// printf("   MapSize                 |-----------------------------|                        \n");
// printf("   g_RequestPhysAddr             ^                                                \n");
// printf("                                                                                  \n");
// printf("   Address to read:   Addr = g_MappedVirtAddr + g_Offset + user requset (0 based).\n");
// printf("      or              Addr = g_MappedVirtAddr + g_Offset + (user Req Abs - g_ReqestPhysAddr)\n");
// printf("                                                                                  \n");












#endif
 
