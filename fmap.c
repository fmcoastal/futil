#include   <stdint.h>
#include   <stdio.h>
#include   <stdlib.h>
#include   <string.h>
#include   <fcntl.h>
/* Not technically required, but needed on some UNIX distributions */
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __linux__

#include <sys/mman.h>
#include <unistd.h>
#endif

#include "fmap.h"
#include "fprintbuff.h"


//#define FMAP_DEBUG

#ifdef __FOCTEON__
#include "/usr/local/Cavium_Networks/OCTEON-SDK/executive/cvmx-asm.h"
#endif
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
 /*  Init Memory Map data structure.  If the *pMmapData is 0 then I allocate a buffer
 *  and will clean up when the close function is called
 */

int64_t MMapInit(fMmapData_t** pMmapData )
{  
#ifdef FMAP_DEBUG     
   printf("%d:%s-%s \n",__LINE__,__FILE__,__FUNCTION__);
#endif
   fMmapData_t* buffer;
   if(*pMmapData == 0)  // No buffer so I will allocate */
   {
      buffer = (fMmapData_t*) malloc (sizeof (fMmapData_t));
      if ( buffer == NULL)
      {
           return (1);
      }
      *pMmapData = buffer;
      buffer->MallocBuffer     = (uint8_t *)buffer;  /* save the address of the buffer */
   }
   else
   {
        buffer                 = *pMmapData;     // Set the
        buffer->MallocBuffer   = 0 ;             /* Initialization was called with a buffer */
   }
   buffer->fd                  = 0;              // file handle for /dev/mem 
   buffer->MappedBaseAddress   = NULL;           // virtual Address Mapped
   buffer->VirtualAddress      = NULL;           // Mapped address + offset of Target Adddress
   buffer->PageSize            = 0;
   buffer->MapAddress          = 0;
   buffer->MapSize             = 0;
   return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// this function creates a virtual address from a physical address and size.
//   The virtual address to access is pMmapData->VirutalAddress */
int64_t MMapGetVirtAddress(fMmapData_t* pMmapData,uint64_t  PhyAddress,uint64_t Size)
{
#ifdef FMAP_DEBUG     
     printf("%d:%s-%s \n",__LINE__,__FILE__,__FUNCTION__);
#endif
#ifdef __linux__
     pMmapData->PageSize = getpagesize();
#endif
#ifdef FMAP_DEBUG     
     printf("\n  Page Size    0x%016llx\n",(LLU)pMmapData->PageSize);
#endif
     // figure out the size to map -- if the physical address were page aligned,
     //                          you would not need to add the Extra "page size"
     pMmapData->MapSize    = Size / pMmapData->PageSize  + pMmapData->PageSize;
     // the line below gets me the Physical  address aligned to the page boundry 
     //      before the Target Address
     pMmapData->MapAddress = (void *)(PhyAddress & ~(pMmapData->PageSize-1));
#ifdef FMAP_DEBUG     
     printf("  Map Size     0x%016llx\n" ,(LLU)pMmapData->MapSize);
     printf("  Map Address  0x%016llx\n" ,(LLU)pMmapData->MapAddress);
#endif


// #####################################################################

#ifdef FMAP_DEBUG     
    printf("    --Call the fopen on the /dev/mem file\n");
#endif
    pMmapData->fd = open("/dev/mem", O_RDWR);

    if( pMmapData->fd <= 0)
        {
         printf("  failed to open file /dev/mem \n\n");
         return -1;
        }

#ifdef FMAP_DEBUG     
     printf("  fd                  0x%08x\n",pMmapData->fd);
     printf("  ptr before mmap()   %p\n"    ,pMmapData->MappedBaseAddress);
#endif

// #####################################################################
#ifdef __linux__
     pMmapData->MappedBaseAddress = mmap(NULL, pMmapData->MapSize, PROT_READ|PROT_WRITE, MAP_SHARED, pMmapData->fd,(uint64_t)pMmapData->MapAddress);
#ifdef FMAP_DEBUG     
     printf("  ptr after mmap()    %p \n",pMmapData->MappedBaseAddress);
#endif
     if (pMmapData->MappedBaseAddress == MAP_FAILED)
          {
              printf(" mmap returned MAP_FAILED\n");
              pMmapData->PageSize = 0;
              pMmapData->MapSize = 0;
              pMmapData->MapAddress = 0;
              close(pMmapData->fd);
              pMmapData->fd = 0;
              pMmapData->MappedBaseAddress = 0;
        
              return -2;   /*evol no clean up of data structure & File Handle */
          }
#endif
 //   printf("\n  --buffer at mmaped address \n");
 //   PrintBuff(pMmapData->MappedBaseAddress,128,pMmapData->MappedBaseAddress);
    //status = cvmx_bootmem_free_named(Name);
    //
    // Create the address of the Named space my addding the offset of
    // the origineal buffer back to the base page
    pMmapData->VirtualAddress = pMmapData->MappedBaseAddress + (size_t )(PhyAddress & (pMmapData->PageSize-1));
#ifdef FMAP_DEBUG     
    printf("  Virtual Address %p\n", pMmapData->VirtualAddress);
    PrintBuff(pMmapData->VirtualAddress,128,pMmapData->VirtualAddress);
#endif
    return 0;
}




int64_t MMapDelete( fMmapData_t* pMmapData)
{
#ifdef FMAP_DEBUG     
      printf("%d:%s-%s \n",__LINE__,__FILE__,__FUNCTION__);
#endif
#ifdef __linux__
     /* unmap the physical Memory */
     /* if for some reason we called init and close back to back Mapped Base Address will be 0
      * do not call munmap */
     if (pMmapData->MappedBaseAddress != 0)
     {     
         munmap( pMmapData->MappedBaseAddress , pMmapData->MapSize);
     }
    /* if file handle open, close it */
    if(pMmapData->fd > 0)   close(pMmapData->fd);
#endif

    /* if we allocated the buffer then free it */
    if( pMmapData->MallocBuffer != 0)
    {
        free(pMmapData->MallocBuffer);
    }
    return (0);
}
void MMapPrintVars(fMmapData_t * pMmapData)
{
     printf("\n");
     printf(" 0x%016llx  PageSize                  \n",(llu)pMmapData->PageSize);
     printf(" 0x%016llx  address request to mmap() \n",(llu)pMmapData->MapAddress);
     printf(" 0x%016llx  size request to mmap()    \n",(llu)pMmapData->MapSize);
     printf(" 0x%016llx  fd                        \n",(llu)pMmapData->fd);
     printf(" 0x%016llx  MappedVirtualAddress      \n",(llu)pMmapData->MappedBaseAddress);
     printf(" 0x%016llx  VirtualAddress            \n",(llu)pMmapData->VirtualAddress);
     printf(" 0x%016llx  MallocBuffer              \n",(llu)pMmapData->MallocBuffer);
     printf("\n");
}


     

int64_t  PhyRead64(uint64_t Paddr, uint64_t *Reg)
{
   fMmapData_t* pMPhy = NULL;   // Make sure you set this to NULL
   int64_t    r;

   // Allocate a handle for Reading and Writing Physical Memory
   r = MMapInit(& pMPhy );
   if( r != 0)
   {
        printf("%d:%s-%s Failed to allocate fMmapData_t Structure  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        return r;
   }
   // map the Physical Address
   r =  MMapGetVirtAddress( pMPhy, Paddr,sizeof(int64_t));
   if( r != 0)
   {
        printf("%d:%s-%s Failed to Map physical Address  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        MMapDelete(pMPhy);
        return r;
   }
   // Read The Data
   //
   *Reg = *((uint64_t *)pMPhy->VirtualAddress);    
   // Free teh pMPhy handle
   MMapDelete(pMPhy);
   return 0;
}

int64_t  PhyWrite64(uint64_t Paddr, uint64_t Val)
{
   fMmapData_t* pMPhy = NULL;   // Make sure you set this to NULL
   int64_t    r;

   // Allocate a handle for Reading and Writing Physical Memory
   r = MMapInit(& pMPhy );
   if( r != 0)
   {
        printf("%d:%s-%s Failed to allocate fMmapData_t Structure  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        return r;
   }
   // map the Physical Address
   r =  MMapGetVirtAddress( pMPhy, Paddr,sizeof(int64_t));
   if( r != 0)
   {
        printf("%d:%s-%s Failed to Map physical Address  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        MMapDelete(pMPhy);
        return r;
   }
   // Write The Data
   //
   *((uint64_t *)pMPhy->VirtualAddress) = Val;    

   // Free teh pMPhy handle
   MMapDelete(pMPhy);
   return 0;
}





int32_t  PhyRead32(uint64_t Paddr, uint32_t *Reg)
{
   fMmapData_t* pMPhy = NULL;   // Make sure you set this to NULL
   int64_t    r;

   printf("%d:%s-%s Function not debugged \n",__LINE__,__FILE__,__FUNCTION__);
   // Allocate a handle for Reading and Writing Physical Memory
   r = MMapInit(& pMPhy );
   if( r != 0)
   {
        printf("%d:%s-%s Failed to allocate fMmapData_t Structure  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        return r;
   }
   // map the Physical Address
   r =  MMapGetVirtAddress( pMPhy, Paddr,sizeof(int64_t));
   if( r != 0)
   {
        printf("%d:%s-%s Failed to Map physical Address  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        MMapDelete(pMPhy);
        return r;
   }
   // Read The Data
   //
   *Reg = *((uint64_t *)pMPhy->VirtualAddress);    
   // Free teh pMPhy handle
   MMapDelete(pMPhy);
   return 0;
}

int32_t  PhyWrite32(uint64_t Paddr, uint32_t Val)
{
   fMmapData_t* pMPhy = NULL;   // Make sure you set this to NULL
   int64_t    r;

   printf("%d:%s-%s Function not debugged \n",__LINE__,__FILE__,__FUNCTION__);
   // Allocate a handle for Reading and Writing Physical Memory
   r = MMapInit(& pMPhy );
   if( r != 0)
   {
        printf("%d:%s-%s Failed to allocate fMmapData_t Structure  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        return r;
   }
   // map the Physical Address
   r =  MMapGetVirtAddress( pMPhy, Paddr,sizeof(int64_t));
   if( r != 0)
   {
        printf("%d:%s-%s Failed to Map physical Address  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        MMapDelete(pMPhy);
        return r;
   }
   // Write The Data
   //
   *((uint32_t *)pMPhy->VirtualAddress) = Val;    

   // Free teh pMPhy handle
   MMapDelete(pMPhy);
   return 0;
}


int16_t  PhyRead16(uint64_t Paddr, uint16_t *Reg)
{
   fMmapData_t* pMPhy = NULL;   // Make sure you set this to NULL
   int64_t    r;
   
   printf("%d:%s-%s Function not debugged \n",__LINE__,__FILE__,__FUNCTION__);

   // Allocate a handle for Reading and Writing Physical Memory
   r = MMapInit(& pMPhy );
   if( r != 0)
   {
        printf("%d:%s-%s Failed to allocate fMmapData_t Structure  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        return r;
   }
   // map the Physical Address
   r =  MMapGetVirtAddress( pMPhy, Paddr,sizeof(int64_t));
   if( r != 0)
   {
        printf("%d:%s-%s Failed to Map physical Address  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        MMapDelete(pMPhy);
        return r;
   }
   // Read The Data
   //
   *Reg = *((uint16_t *)pMPhy->VirtualAddress);    
   // Free teh pMPhy handle
   MMapDelete(pMPhy);
   return 0;
}

int16_t  PhyWrite16(uint64_t Paddr, uint16_t Val)
{
   fMmapData_t* pMPhy = NULL;   // Make sure you set this to NULL
   int64_t    r;

   printf("%d:%s-%s Function not debugged \n",__LINE__,__FILE__,__FUNCTION__);
   // Allocate a handle for Reading and Writing Physical Memory
   r = MMapInit(& pMPhy );
   if( r != 0)
   {
        printf("%d:%s-%s Failed to allocate fMmapData_t Structure  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        return r;
   }
   // map the Physical Address
   r =  MMapGetVirtAddress( pMPhy, Paddr,sizeof(int64_t));
   if( r != 0)
   {
        printf("%d:%s-%s Failed to Map physical Address  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        MMapDelete(pMPhy);
        return r;
   }
   // Write The Data
   //
   *((uint16_t *)pMPhy->VirtualAddress) = Val;    

   // Free teh pMPhy handle
   MMapDelete(pMPhy);
   return 0;
}


int8_t  PhyRead8(uint64_t Paddr, uint8_t *Reg)
{
   fMmapData_t* pMPhy = NULL;   // Make sure you set this to NULL
   int64_t    r;

   printf("%d:%s-%s Function not debugged \n",__LINE__,__FILE__,__FUNCTION__);
   // Allocate a handle for Reading and Writing Physical Memory
   r = MMapInit(& pMPhy );
   if( r != 0)
   {
        printf("%d:%s-%s Failed to allocate fMmapData_t Structure  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        return r;
   }
   // map the Physical Address
   r =  MMapGetVirtAddress( pMPhy, Paddr,sizeof(int64_t));
   if( r != 0)
   {
        printf("%d:%s-%s Failed to Map physical Address  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        MMapDelete(pMPhy);
        return r;
   }
   // Read The Data
   //
   *Reg = *((uint8_t *)pMPhy->VirtualAddress);    
   // Free teh pMPhy handle
   MMapDelete(pMPhy);
   return 0;
}

int8_t  PhyWrite8(uint64_t Paddr, uint8_t Val)
{
   fMmapData_t* pMPhy = NULL;   // Make sure you set this to NULL
   int64_t    r;

   printf("%d:%s-%s Function not debugged \n",__LINE__,__FILE__,__FUNCTION__);
   // Allocate a handle for Reading and Writing Physical Memory
   r = MMapInit(& pMPhy );
   if( r != 0)
   {
        printf("%d:%s-%s Failed to allocate fMmapData_t Structure  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        return r;
   }
   // map the Physical Address
   r =  MMapGetVirtAddress( pMPhy, Paddr,sizeof(int64_t));
   if( r != 0)
   {
        printf("%d:%s-%s Failed to Map physical Address  %d\n",__LINE__,__FILE__,__FUNCTION__,(int)r);
        MMapDelete(pMPhy);
        return r;
   }
   // Write The Data
   //
   *((uint8_t *)pMPhy->VirtualAddress) = Val;    

   // Free teh pMPhy handle
   MMapDelete(pMPhy);
   return 0;
}






















#if 0

void *   g_MappedVirtAddr = 0;
void *   g_MappedVirtAddrAndOffset = 0;
uint64_t g_MappedPhysAddr = 0;
size_t   g_MappedSize = 0;
uint64_t g_RequestPhysAddr = 0;
uint64_t g_Offset = 0;
int      g_fd = 0;


void PrintMMapVars(void)
{
     printf("\n");
     printf(" 0x%016llx  g_fd                      \n",(llu)g_fd);
     printf(" 0x%016llx  address request to  mmap()\n",(llu)g_RequestPhysAddr);
     printf(" 0x%016llx  size request to  mmap()   \n",(llu)g_MappedSize);
     printf(" 0x%016llx  g_MappedPhysAddr\n",(llu)g_MappedPhysAddr);
     printf(" 0x%016llx  g_MappedSize \n",(llu)g_MappedSize);
     printf(" 0x%016llx  g_Offset\n",(llu)g_Offset);
     printf(" 0x%016llx g_MappedVirtAddr\n",(llu)g_MappedVirtAddr);
     printf(" 0x%016llx g_MappedVirtAddrAndOffset\n",(llu)g_MappedVirtAddrAndOffset);
     printf("\n");
}


int64_t MapAddressSpace(uint64_t PhysAddress, uint64_t Size)
{
    size_t   PageSize;

    if (g_MappedVirtAddr  != 0)
    {
        printf(" Freeing Virtual Address space & \n");
        UnMapAddressSpace();
    }

#ifdef FMAP_DEBUG
// #####################################################################
 printf("                                                                                  \n");
 printf(" Cash lined Memory:        |_________|_________|_________|                        \n");
 printf(" Area request for Memmap:        X__________________X                             \n");
 printf("    PhysAddress                  ^                                                \n");
 printf("    Size                         |------------------|                             \n");
 printf("                                                                                  \n");
 printf("   g_MappedVirtAddr        ^                                                      \n");
 printf("   g_MappedPhysAddr        ^                                                      \n");
 printf("   g_Offset                |-----|                                                \n");
 printf("   MapSize                 |-----------------------------|                        \n");
 printf("   g_RequestPhysAddr             ^                                                \n");
 printf("                                                                                  \n");
 printf("   Address to read:   Addr = g_MappedVirtAddr + g_Offset + user requset (0 based).\n");
 printf("      or              Addr = g_MappedVirtAddr + g_Offset + (user Req Abs - g_ReqestPhysAddr)\n");
 printf("                                                                                  \n");
     printf("Call the fopen on the /dev/mem file\n");
#endif
     g_fd = open("/dev/mem", O_RDWR);

     if(g_fd < 0)
     {
         printf(" failed to open file /dev/mem \n\n");
         return -1;
     }

     g_RequestPhysAddr = PhysAddress;  // save the physical base addresss
#ifdef __linux__
     PageSize = getpagesize();
#endif

#ifdef FMAP_DEBUG
     // Must Mmap must be on on Cach line boundry.
     printf("\n");
//   printf("INFO: Size of pointer is %llu bytes\n", (llu)sizeof(void*));
     printf(" 0x%016llx  g_fd                      \n",(llu)g_fd);
     printf(" 0x%016llx  address request to  mmap()\n",(llu)g_RequestPhysAddr);
     printf(" 0x%016llx  size request to  mmap()   \n",(llu)Size);
     printf(" 0x%016llx  Page Size                 \n",(llu)PageSize);
#endif
    // need to rework the size and base address here    
    /* offset for mmap() must be page aligned */
    // pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
     g_MappedPhysAddr  = PhysAddress  & ~( PageSize - 1); // this is cache alligned
     g_Offset          = PhysAddress - g_MappedPhysAddr;  
     if( Size < PageSize)
     {
         Size  = PageSize ;  // we may map more than we need but ....
     }  
     g_MappedSize    = (Size & (~(PageSize-1))) + PageSize;
#ifdef FMAP_DEBUG     
     printf(" 0x%016llx  g_MappedPhysAddr\n",(llu)g_MappedPhysAddr);
     printf(" 0x%016llx  g_MappedSize \n",(llu)g_MappedSize);
     printf(" 0x%016llx  g_Offset\n",(llu)g_Offset);
#endif
 
#ifdef __linux__
     g_MappedVirtAddr = mmap(NULL, g_MappedSize, PROT_READ | PROT_WRITE, MAP_SHARED, g_fd, g_MappedPhysAddr);

      if (g_MappedVirtAddr == MAP_FAILED)
     {
         printf(" mmap returned MAP_FAILED\n");
         g_MappedVirtAddrAndOffset = 0;
         return (-1);
     }
#endif
     g_MappedVirtAddrAndOffset = g_MappedVirtAddr + g_Offset;
// watch this sync function :-)
#ifndef __linux__
     CVMX_SYNC;
#endif
#ifdef FMAP_DEBUG     
     printf(" 0x%016llx  g_MappedVirtAddr\n",(llu)g_MappedVirtAddr);
     printf(" 0x%016llx  g_MappedVirtAddrAndOffset\n",(llu)g_MappedVirtAddrAndOffset);
#endif
     return 0;
}

int64_t UnMapAddressSpace( void )
{
#ifdef __linux__
int result; 
     
    if(g_fd > 0){
          //int munmap(void *start, size_t length);
          result = munmap((void *)g_MappedVirtAddr, g_MappedSize);
          if(result != 0)
		printf("function munmap failed %d\n",result);
          close(g_fd);
     }
#endif
    g_MappedVirtAddr = NULL; 
    g_MappedPhysAddr = 0;
    g_MappedSize = 0;
    g_RequestPhysAddr = 0;
    g_Offset = 0;
    g_fd = 0;
    g_MappedVirtAddrAndOffset = 0;
    return 0;
}


#ifdef MAIN

int main( int argc, char ** argv);
void PrintBuff8 (uint8_t  * buffer, int64_t bufferSize,uint8_t  * Address);

void  PrintHelp(void)
{
    printf("\n  Usage:  ");
    printf("\n           -o<fname>        Output file name Default:nopop.txt");
    printf("\n           -d<debugValue>   DebugValue");
    printf("\n             0x01 - display xxxx ");
    printf("\n             0x02 - display xxxx ");

    printf("\n           -?           help");
    printf("\n Notes:");
    printf("\n      The file will be parsed based on the ',' char");
    printf("\n      The file will be parsed based on the string in the line");
    printf("\n\n");  
}

void  PrintCmdHelp(void)
{
    printf("\n  ");
    printf("\n  m <add> <window sz>  - map physical memory");
    printf("\n  d <add> <size>       - dump memeory ");
    printf("\n  r                    - read memory "  );
    printf("\n  h                    This Message");
    printf("\n  q                    Quit");
    printf("\n\n");  
}
#define SINGLE_ARGUMENT_SIZE  64
#define COMMAND_LINE_SIZE     256
/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/
int main(int argc, char **argv){
    int64_t  result = 0;
    char cmdString[COMMAND_LINE_SIZE];
    char ArgV0[SINGLE_ARGUMENT_SIZE];
    char ArgV1[SINGLE_ARGUMENT_SIZE];
    char ArgV2[SINGLE_ARGUMENT_SIZE];
    char ArgV3[SINGLE_ARGUMENT_SIZE];
    char ArgV4[SINGLE_ARGUMENT_SIZE];
    int  ArgC;
    int  done = 0;
    char  prompt[80];
    uint64_t AddressOffset = 0;
    int64_t  RWSize = 1;
    uint8_t   data8[1024];
    uint64_t  data64[1024];
    uint64_t MapSize    = 0x400;
    uint64_t MapAddress = 0x1fc00000;
    
    strcpy(prompt,"input>");
    for( done=0; done==0;)
    {
        printf("%s",prompt);
        gets( cmdString);
        ArgC = sscanf(cmdString,"%s %s %s %s %s",ArgV0,ArgV1,ArgV2,ArgV3,ArgV4);
#if 0
        printf("ArgC   %d\n",ArgC); 
        printf("ArgV0  %s\n",ArgV0); 
        printf("ArgV1  %s\n",ArgV1); 
        printf("ArgV2  %s\n",ArgV2); 
        printf("ArgV3  %s\n",ArgV3); 
#endif

        switch (*(ArgV0 + 0))
        {
        case 'h':
            PrintHelp();
            break;
        case 'd':   // for now map and unmap
        case 'D':
           if (ArgC >= 2)
           {
               AddressOffset = fatox(ArgV1);
           }
           if (ArgC >= 3)
           {
              RWSize =fatox(ArgV2);  
           }
//           printf("0x%08llx AddressOffset \n",(llu)AddressOffset);
//           printf("0x%08llx Size          \n",(llu)RWSize);
//           result =  MapAddressSpace( 0x1fc000100, 0x1000);
           Read8( AddressOffset,&(data8[0]),RWSize);
           PrintBuff8( &(data8[0]), (int64_t) RWSize, (uint8_t *)(g_RequestPhysAddr +AddressOffset));
//           UnMapAddressSpace();
            break;

        case 'r':   // for now map and unmap
        case 'R':
           if (ArgC >= 2)
           {
               AddressOffset = fatox(ArgV1);
           }

 //          result =  MapAddressSpace( 0x1fc000100, 0x1000);

           if(((*(ArgV0 + 1)) == '6') || (*(ArgV0 + 1) == 0x00)) // 64 & default
           {
           Read64( AddressOffset ,&(data64[0]),1);
           printf("R64: 0x%016llx = 0x%016llx\n",(g_RequestPhysAddr +AddressOffset) ,*data64);
           }
           else if((*(ArgV0 + 1)) == '3')
           {

           }
           else if((*(ArgV0 + 1)) == '1')
           {

           }
           else if((*(ArgV0 + 1)) == '8')
           {
           Read8( AddressOffset ,&(data8[0]),1);
           printf("R8: 0x%016llx = 0x%02x\n",(g_RequestPhysAddr +AddressOffset) ,*data8);
           }

//           UnMapAddressSpace();
            break;

        case 'q':
        case 'Q':
            done=1;
            break;

        case 'm':
        case 'M':
            if (ArgC >= 2)
           {
               MapAddress = fatox(ArgV1);
           }
           if (ArgC >= 3)
           {
              MapSize =fatox(ArgV2);  
           }
            printf("\n");
            printf(" -- Map Memory  -- \n");
            printf(" 0x%016llx MapAddress \n",MapAddress);
            printf(" 0x%016llx MapSize    \n",MapSize);
            result =  MapAddressSpace((uint64_t)MapAddress , (uint64_t) MapSize);
            if(result == 0) 
                printf("Success\n");
            else 
                printf("failed to MAP space\n");
            break;

        case 'u':
        case 'U':
            printf("\n");
            printf(" -- unmap Memory  -- \n");
            printf(" 0x%016llx g_MappedVirtAddr\n",g_MappedVirtAddr);
            UnMapAddressSpace( );
            break;

        case 'z':
        case 'Z':
            PrintMMapVars();
            break;

        default:
            break;
        } // end switch
    } // end for 
    
    UnMapAddressSpace( );
    return 0;
}



int64_t Read8(uint64_t Address, uint8_t *pdata,int64_t  Size)
{
   int64_t i;
   int64_t result;
    
   if((result =  MapAddressSpace( Address, (Size * sizeof(uint8_t)))) != 0)
   {
        printf(" Rd8: failed to map memory space %lld\n",result);
        return result;
   }
   for( i = 0 ; i < Size ; i++)
   {
      *(pdata+i) = *((uint8_t *)g_MappedVirtAddr + g_Offset /* + Address*/ +i );
   }
   UnMapAddressSpace();
   return 0;
} // end Read8

int64_t Write8(uint64_t Address, uint8_t *pdata,int64_t  Size)
{
   int64_t i;
   int64_t result;
    
   if((result =  MapAddressSpace( Address, (Size * sizeof(uint8_t)))) != 0)
   {
        printf(" Wr8: failed to map memory space %lld\n",result);
        return result;
   }

   for( i = 0 ; i < Size ; i++)
   {
      *((uint8_t *)g_MappedVirtAddr + g_Offset /* + Address */ +i ) = *(pdata+i); 
   }
   UnMapAddressSpace();
   return 0;
} // end Write8






int64_t Read64(uint64_t Address, uint64_t *pdata,int64_t  Size)
{
   int64_t i;
   int64_t result;
    
   if((result =  MapAddressSpace( Address, (Size * sizeof(uint64_t)))) != 0)
   {
        printf(" Rd64: failed to map memory space %lld\n",result);
        return result;
   }

   for( i = 0 ; i < Size ; i++)
   {
       *(pdata+i) = *((uint64_t *)g_MappedVirtAddr + g_Offset /*+ Address*/ +i);
   }
   UnMapAddressSpace();
   return 0;
} //end Read64

int64_t Write64(uint64_t Address, uint64_t *pdata,int64_t  Size)
{
   int64_t i;
#ifdef INFUNCT
   int64_t result;
    
   if((result =  MapAddressSpace( Address, (Size * sizeof(uint64_t)))) != 0)
   {
        printf(" Wr64: failed to map memory space %lld\n",result);
        return result;
   }
#endif

   for( i = 0 ; i < Size ; i++)
   {
       *((uint64_t *)g_MappedVirtAddr + g_Offset /*+ Address*/ +i) = *(pdata+i) ;
   }
#ifdef INFUNCT
   UnMapAddressSpace();
#endif
   return 0;
} //end Write64









#define fsprint printf
//
//   
void PrintBuff8(uint8_t * buffer, int64_t bufferSize,uint8_t * Address)
{
    uint8_t * tmpptr0  = buffer; 
    uint8_t * tmpptr1  = tmpptr0; 
    int64_t  i          = 0 ;
    int64_t  m          = 0 ;
    int64_t  n          = 0 ;
    int64_t  j          = 0 ;
    int64_t  PrintCount = 0 ;   // used as counter to denote when to print \nadderss
    int64_t  BlankCnt   = 0 ;
   
    
    // align the lead
    BlankCnt = (unsigned long)Address & 0x000000000f;
    
    // print the lead
    if( BlankCnt != 0)  // if 0 jump to main body 
    {        
        for ( PrintCount = 0 ; PrintCount < BlankCnt ; PrintCount++ )
        {
            if( PrintCount == 0) // space between fields
            {
                fsprint("\n%08x",(unsigned)((unsigned long)Address & ~0x000000000f)); 
                tmpptr1 = tmpptr0;
            }
            if( (PrintCount % 8) == 0)
            {
                fsprint(" ");
            }    
            fsprint("   ");
        }
        PrintCount--;  // remove last increment of printcount
        // print PrintCount data
        for ( m = 0  ; (PrintCount < 0xf) && (i < bufferSize); i++, m++,PrintCount++)
        {
            if(PrintCount % 8 == 0)
            {
                fsprint(" ");
            }    
            fsprint(" %02x",(unsigned char)(*tmpptr0++));
            Address++;
        }
        
        // special case here when count is less than one line and not starting at zero
        if ( i == bufferSize) 
        {
            // print out the last space 
            for (      ; (PrintCount < 0x0f) ; PrintCount++ )
            {
                if( PrintCount  % 8 == 0)
                {
                    fsprint(" ");
                }    
                fsprint("   ");
            }
            // print PrintCount text space
            for ( PrintCount = 0 ; (PrintCount < BlankCnt) ; PrintCount++ )
            {
                if( PrintCount == 0)   // space between fields 
                {
                    fsprint(" ");
                }
                else if( PrintCount  % 8 == 0)
                {
                    fsprint(" ");
                }    
                fsprint(" ");
            }             
            // print PrintCount characters
            for( n = 0 ; (n < m) ; n++)
            {
                if( n == 0 ) printf(" ");
                if((*tmpptr1 >=0x20) && (*tmpptr1 <= 0x7e))
                    fsprint("%c",*tmpptr1);
                else
                    fsprint(".");
                tmpptr1++;
            }
            printf("\n");
            return;
        } // end i == bufferSize
        
        // print PrintCount text space
        for ( PrintCount = 0 ; (PrintCount < BlankCnt) ; PrintCount++ )
        {
            if( PrintCount == 0)   // space between fields 
            {
                fsprint(" ");
            }
            else if( PrintCount  % 8 == 0)
            {
                fsprint(" ");
            }    
            fsprint(" ");
        }
        // print PrintCount characters
        n = 0;
        for( n = 0 ; (PrintCount <= 0xf) && (n < m) ; n++,PrintCount++)
        {
            if((*tmpptr1 >=0x20) && (*tmpptr1 <= 0x7e))
                fsprint("%c",*tmpptr1);
            else
                fsprint(".");
            tmpptr1++;
        }
    }
    
    // print the body    
    PrintCount = 0;
    for (   ; i < bufferSize ; i++)
    {
        if( PrintCount == 0 )
        {
            fsprint("\n%016llx",((llu)Address & ~0x0f));
            tmpptr1 = tmpptr0;
        }
        if(PrintCount % 8 == 0)
        {
            fsprint(" ");
        }
        fsprint(" %02x",(unsigned char)(*tmpptr0++));
        Address++;
        PrintCount ++;
        if( PrintCount  > 0x0f)  
        {
            PrintCount = 0;
            for( j = 0 ; j < 16 ; j++)
            {
                if( j == 0 ) printf("  ");
                if((*tmpptr1 >=0x20) && (*tmpptr1 <= 0x7e))
                    fsprint("%c",*tmpptr1);
                else
                    fsprint(".");
                tmpptr1++;
            }
        }
    }
    
    // print out the last space 
    m = PrintCount;
    for (      ; (PrintCount <= 0x0f) ; PrintCount++ )
    {
        if( PrintCount  % 8 == 0)
        {
            fsprint(" ");
        }    
        fsprint("   ");
    }
    
    // print PrintCount characters
    for( n = 0 ; (n < m) ; n++)
    {
        if( n == 0 ) printf("  ");
        if((*tmpptr1 >=0x20) && (*tmpptr1 <= 0x7e))
            fsprint("%c",*tmpptr1);
        else
            fsprint(".");
        tmpptr1++;
    }
    fsprint("\n");
}


//////////////////////////////////////////////////////////////////////////
//
uint64_t fatox(int8_t* string)
{
    uint64_t result = 0;
    int64_t i = 0;
    int64_t done = 0;
    uint8_t in;
    int64_t length = strlen(string);
     
    while( (done == 0) && (i < length))
    {
        in = *(string+i);
        if( in == 0x00)
        {
            done = 1;
        }
        else
        {
            if (( in >= '0') &&( in <= '9'))
            {
                result <<= 4;
                result += in & 0x0f;
            }
            else if( (( in >= 'a') &&( in <= 'f')) ||
                     (( in >= 'A') &&( in <= 'F')) )
            {
                result <<= 4;
                result += ((in & 0x0f) + 9);
            }
            else if( ( in == 'x') || ( in == 'X'))
            {
            }
            else
            {
                done = 1; // unrecognized character
            }
            i++;
        }
    }
    return result;
}

#endif
#endif
 
