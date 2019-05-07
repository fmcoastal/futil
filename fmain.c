#include <stdint.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>   // for memset()
#include <stdio.h>    // for printf()
#include <fcntl.h>    // for open()
#include <pthread.h>  // for thread Items
#include <stdlib.h>  // for itoa()
#include <getopt.h>


#undef  USE_THREADS
#define USE_THREADS
#define USE_MENU
#define USE_SERIAL
#define USE_SCRIPT


#include "ffile.h"
#include "frbuff.h"

#ifdef  USE_SERIAL
#include "fln_serial.h"
#endif



uint64_t g_main_debug = 0;
#define  MAIN_DBG(x) if((g_main_debug & x) == x)
//#define  MAIN_DBG_MEASUREMENT_PARMS     0x02
//#define  MAIN_DBG_DUMP_INPUTS           0x01


#define SERIAL_DBG(x) if((g_main_debug & x) == x)
#define SERIAL_DBG_GETSTRINGPOINTERS  0x10000


////////////////////////////////////////////////////////////////////////
//   Include a CLI MENU
#ifdef USE_MENU
void    doMenu(void);
int     g_Menu = 0;         // if = 1, run a Menu interface (do_Menu);
#endif

////////////////////////////////////////////////////////////////////////
//   Include CLI Script Commnads
#ifdef USE_SCRIPT           
int     g_CLIScript = 0;    // if value, execute script  base on Number;
#endif


#ifdef USE_THREADS
//----------------------------------
pthread_t g_tid[2];             // posix thread array
#endif


frbuff *  g_pTxSerial = NULL;   // Serial Buffer for talking to Tx
frbuff *  g_pRxSerial = NULL;   // Serial Buffer to catch RXx
uint64_t  g_Verbose = 0;

int  g_RxThreadDisableRBuff = 0 ;   // makes Rx thread not write to RxBuff. 
int  g_RxThreadPrint        = 0 ;   // makes Rx thread print to StdOut
int  g_RxThreadLog          = 0 ;   // makes Rx thread log to a file if file handle open


//  MAX_DATED_FILE_NAME  is defined in ffile.h
FILE *g_fpOut        = NULL;
char  g_LogFileBase[MAX_DATED_FILE_NAME] = {0};
char  g_LogFileName[MAX_DATED_FILE_NAME] = {"log"};


int   RunScriptFile(char * filename);
 

// the struct below is passed to the two threads. 
typedef struct 
{
   int done;
   int fd;

}datablock;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//  Thread Functions
//
//  tx Thread Function
int tx(void* arg);
//  rx Thread Function
int rx(void* arg);
//  WorkerThread:
//     looks at the Thread ID 
//     determines which worker thread to run
void *WorkerThread(void *arg);



#define error_message printf
#define llu long long unsigned 


void cleanup()
{
    if(g_fpOut != NULL)  
    {
       fflush(g_fpOut); // flush any lingering data
       fclose(g_fpOut);
    }
    RbuffClose(g_pTxSerial);
    RbuffClose(g_pRxSerial);
}


void print_usage(void)
{
   printf("Main - Test Eye \n");
#ifdef USE_SERIAL
   printf("-p <serialport>   EG: /dev/ttyS0  (default)\n");
#endif
   printf("-i                Interactive (Terminal Server)  \n");
#ifdef USE_SCRIPT
   printf("-t <TestFileName>  \n");
#endif
   printf("-v                Verbose Flag  \n");
   printf("-d <0xDebugFlags> Debug Flags  \n");
#ifdef USE_MENU
   printf("-m                CLI menu  \n");
#endif
   printf("example: \n");
   printf("./fmain  -p /dev/ttyS0 -t test  -p9090 -v \n");
   printf("./fmain  -m \n");
   printf("\n");
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//
//  main function  
int main (int argc, char ** argv)

{
#ifdef USE_SERIAL
    int  fd = 0;      // file handle to serial device
//    char *portname = "/dev/ttyUSB1";
//    char *portname = "/dev/ttyS0";
    char portname[80]; 
#endif
    char test_file[80] = {0}; 
    int i = 0;
    int err;
    datablock db;    
    int option;
    int result = 0;
    int UsageFlag = 0;
    
    char logfile[128] = {0};  // default Log file name


// Initialize Default Values
    memset((void*)&db,0,sizeof(datablock));
#ifdef USE_SERIAL
    strcpy(portname, "/dev/ttyS0");
#endif

while ((option = getopt(argc, argv,"mvrl:b:s:d:t:")) != -1) {
       switch (option) {

            case 'l' : 
                strcpy(test_file,optarg);   // Test File - Need "t:" in string above
                break;
#ifdef USE_MENU
            case 'm' : 
                UsageFlag++ ;               // Flag we had a good input
                g_Menu = 1 ;                // run a CLI menu after Init Finished
                break;
#endif
            case 'v' : g_Verbose=1 ;             // configure to bind w/ SO_REUSEADDR
                break;
#ifdef USE_SERIAL
            case 's' : strcpy(portname,optarg);  // Serial Port
                break;
#endif
#ifdef USE_SCRIPT           
            case 't' : 
                UsageFlag++ ;               // Flag we had a good input
                g_CLIScript = 1;            // Which command
                strcpy(test_file,optarg);   // Test File - Need "t:" in string above
                break;
#endif
            case 'd' : g_main_debug = atoi(optarg);  // #of times to send File Data
                break;
            case 'h' :                               //  Help
                print_usage();
//                exit(EXIT_FAILURE);
                return(1);
       }
   }

   if( UsageFlag == 0)
   {
      print_usage();
      return 2;
   }

   printf("Command Line Arguments:\n");
#ifdef USE_SERIAL
   printf("       %16s SerialPort\n",portname);
#endif
#ifdef USE_SCRIPT           
   printf("       %16s Test File Name\n",test_file);
#endif
   printf("       %16d Verbose\n",(int)g_Verbose);
   printf("  0x%016llx   Debug\n",(llu)g_main_debug);


//
//  Open an Output LOG File with "LogName" base and a time stamp at the end  
//
   if( 0 ==  ffMakeDatedFileName(g_LogFileBase,".txt", g_LogFileName , MAX_DATED_FILE_NAME))
   {
       //got a name
       g_fpOut = fopen(g_LogFileName, "w");
       if( g_fpOut == NULL)    
       {
           printf ("Unable to open file %s\n", g_LogFileName);
           cleanup();
           return (-5);
       }
   }

//  Create Buffers for the Serial port
     RbuffInitialize ( &g_pTxSerial, &result, sizeof(char),4096, 32);  //
     if( result != 0)
     {
         printf("Failed to allocate Tx Buffer  %d \n",result);
         cleanup();
         return -1;
     }
     RbuffInitialize ( &g_pRxSerial, &result, sizeof(char),4096, 32);  //
     if( result != 0)
     {
         printf("Failed to allocate Rx Buffer  %d \n",result);
         cleanup();
         return -1;
     }
 
#ifdef USE_SERIAL
//   Opent the Serial Port
    fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        error_message ("error %d opening %s: %s", errno, portname, strerror (errno));
        return -1;
    }
    db.fd = fd;  //  handle for the threads to access serial port.   

    set_interface_attribs (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (fd, 0);                // set no blocking

#endif

#ifdef USE_THREADS
//  Launch Tx and Rx Threads
    while(i < 2)
    {
        err = pthread_create(&(g_tid[i]), NULL, &WorkerThread, &db);
        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
        else
            printf("\n Thread created successfully\n");
        i++;
    }

    usleep(100000);  // for now sleep till threads run.  future, rework and have thread signal it is running!    
#endif

#ifdef USE_MENU
    if(g_Menu == 1)
    {
        doMenu();
    }
#endif

#ifdef USE_SCRIPT
    if (g_CLIScript == 1)
    {
        RunScriptFile(test_file);
    }
/*    else if (g_CLIScript == 2)
    {
        A second CLI Function
    }
*/    
#endif
    db.done = 1;   // flag the threads to terminate 
//    while(db.done == 0) {}    

    sleep(5);    // swag hope the treads shut down in 5 seconds

    cleanup();   // Cleanup the rest
#ifdef USE_SERIAL
    if( fd != 0) close(fd);  // the handle to the serail porti
#endif
    return 0;

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//  Thread Functions
//
//  tx Thread Function
int tx(void* arg)
{
    datablock * pdb =(datablock *)arg;
    char c;
    char * pc = &c ;
    int r;

    printf("Starting Tx Thread\n");
    printf("  Serial Handle: %d\n",pdb->fd);
    while( pdb->done == 0 )
    {
#ifdef KEYBOARD
       c = getchar();
//       printf("%c",c);   // local echo??
       write (pdb->fd, &c, 1);           // send 7 character greeting
       if( c == 0x1b) pdb->done = 1;
#else
       r = RBuffFetch( g_pTxSerial,(void **) &pc); //fetch value out of ring buffer */
       if ( r == 0)
       {
           if(g_main_debug != 0)
           {
                printf("tx %c  0x%x\n",*pc,*pc);           
           }
           write (pdb->fd, pc, 1);           // send 7 character greeting
       }
       else
       {
           usleep(500);    // let the OS have the machine if noting to go out.
       }
       
#endif
    }
    printf("Ending Tx Thread\n");
    return 1;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//  Thread Functions
//
//  rx Thread Function
int rx(void* arg)
{
    int  n;
    int  i;
    int  r;
    char buf [100];
    datablock * pdb =(datablock *)arg;
    printf("Starting Rx Thread\n");

    while( pdb->done == 0) // this will be set by teh back door
    {
       n = read (pdb->fd, buf, sizeof(buf));  // read up to 100 characters if ready to read
       for( i = 0 ; i < n ; i++)
       {
          if ( g_RxThreadDisableRBuff == 0)
          {
              r =  RBuffPut(g_pRxSerial, (void *) (&buf[i]) );    /* put value int ring buffer */
              if ( r != 0)
              {     
                  printf("%d:%s-%s   Error writing Rx Serial Buff :%d \n",__LINE__,__FILE__,__FUNCTION__,r);
              }
          }
          if(g_RxThreadPrint == 1)
          {
              printf("%c",buf[i]);
          }
          if((g_RxThreadLog == 1) && ( g_fpOut != NULL))
          {
              fprintf(g_fpOut,"%c",buf[i]);
          }
 
       }
    }
    printf("End of  Rx Thread\n");
    return 1;
}


#ifdef USE_THREADS
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//  Thread Functions
//
//  WorkerThread:
//     looks at the Thread ID 
//     determines which worker thread to run
void *WorkerThread(void *arg)
{
pthread_t id = pthread_self();
 
   if(pthread_equal(id,g_tid[0]))
   {
       // tx
       tx(arg);
   }
   else
  {
      // rx
      rx(arg);
  }
  return NULL;

}
#endif





////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
int doTerminalServer(void)
{
    char c;
    int r;
    int d = 0;

    g_RxThreadDisableRBuff = 1; // have Rx thread not stuff RX Buffer
    g_RxThreadPrint = 1;        // have Rx thread print responses from the target
    while( d == 0 )
    {
       c = getchar();
//       printf("%c",c);   // local echo??
       
       if( c == 0x1b)
       {  
           d = 1;
       }
       else
       {
          r =  RBuffPut(g_pTxSerial, (void *) &c );    /* put value int ring buffer */
 
       }
    } //end while
    g_RxThreadPrint = 0;  // disable  Rx tread print responses from the target
    g_RxThreadDisableRBuff = 0; // have Rx thread not stuff RX Buffer

}

#ifdef USE_MENU

#ifdef USE_STATISTICS
extern int g_StatLoops;
#endif 
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
void doMenuMenu(void)
{
printf("\n");
printf("c             - canned statistics \n");
printf("f <filename>  - Open a LogFile\n");
printf("fc            - Close Current Log File\n");
#ifdef USE_STATISTICS
printf("l <value>     - loop count   now=%d \n",g_StatLoops);
#endif
printf("q             - quit program \n");
printf("r             - Random Eye Test \n");
printf("s <filename>  - run a script file \n");
printf("t             - terminal Server \n");
printf("\n");
}
#define ARG_SIZE   64    // max size of an Individual ARGC
#define MAX_ARGS   5    // max number of Command Line Arguments.

int g_argc = 0;
char g_arg0[ARG_SIZE];
char g_arg1[ARG_SIZE];
char g_arg2[ARG_SIZE];
char g_arg3[ARG_SIZE];
char g_arg4[ARG_SIZE];

char g_MenuPrompt[] ={"Yes?> "};

#define CMD_BUFFER_SIZE 256
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
void doMenu(void)
{
int r;
int d = 0;
char CmdLine[CMD_BUFFER_SIZE] = { 0 };
//int  GetLineResults ;
int Cmd;
#define FILE_STRING_LENGTH 64
char logFileName[FILE_STRING_LENGTH];


    doMenuMenu();

    while (d == 0)
    {
       printf("%s",g_MenuPrompt);
       fgets(CmdLine,CMD_BUFFER_SIZE,stdin);
       g_arg0[0] = 0x00;  // if g_argc == 0 we will run last cmd. - stop this behavior
       g_argc = sscanf(CmdLine,"%s%s%s%s%s",g_arg0,g_arg1,g_arg2,g_arg3,g_arg4);

       Cmd = g_arg0[0];
       switch(Cmd) {

       case 'c':
       case 'C':
           printf("Canned Test \n");
           doStatistics(); 
 
           // do script file
           break;

       case 'f':
       case 'F':
           if((g_arg0[1] == 'c') || ( g_arg0[1] == 'C'))
           {
               printf("Close Log File \n");
               if(g_fpOut != NULL) 
               {
                   fflush(g_fpOut); // flush any lingering data
                   fclose(g_fpOut); // close the old, then open the new
                   g_fpOut = NULL;
               }
           }
           else
           {
               printf("Create new Log File \n");
               if(g_fpOut != NULL) // see if file currently open
               {
                   fflush(g_fpOut); // flush any lingering data
                   fclose(g_fpOut); // close the old, then open the new
                   g_fpOut = NULL;
               }
               if( ffMakeDatedFileName(g_LogFileBase,".txt",g_LogFileName,MAX_DATED_FILE_NAME) == 0 )
               {
                   //got a name
                   g_fpOut = fopen(g_LogFileName, "w");
                   if( g_fpOut == NULL)
                   {
                       printf ("Unable to open file %s\n", g_LogFileName);
                       cleanup();
                       return (-5);
                   }
                   printf("Created %s\n",logFileName);
               }
               else
               {
                   printf ("Unable to crate File name and Open File\n");
               }
           }
           break;
 
#ifdef USE_STATISTICS
       case 'l':    // set number of loops
       case 'L':
           if(g_argc >=2)
           {
               g_StatLoops = atoi(g_arg1);
           }
           else
           {  
               printf("Must enter with command for now \n");
           }
           printf("Number of Loops %d \n",g_StatLoops);
           break;
#endif 
       case 'q':
       case 'Q':
           d = 1;
           break;
       case 'r':
       case 'R':
           printf("Test Random Number \n");
            printf("New Line %d\n", (rand() % 4 ) );
            printf("New QLM  %d\n", (rand() % 5 ) );
 
           // do script file
           break;
       case 's':
       case 'S':
           printf("Do Script File : not implemented \n");
           if ( g_argc >= 2 )
              RunScriptFile(g_arg1);
           break;
       case 't':
       case 'T':  
           printf("  Entering  Terminal Server exit...   <ESC Enter> to exit \n");
           doTerminalServer();
           printf("\n--Exiting  Terminal Server\n\n");
           break;
       default:
           doMenuMenu();
           break;
       } // end switch
    }  // end while 

}
#endif




