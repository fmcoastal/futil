#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>   // for memset()
#include <stdio.h>    // for printf()
#include <fcntl.h>    // for open()
#include <pthread.h>  // for thread Items
#include <stdint.h>
#include <sys/ioctl.h>
#include "fln_serial.h" // f linux serial .h

#ifdef SERIAL_EXAMPLE
#include <getopt.h>
#endif


//----------------------------------
  
/* The values for speed are B115200, B230400, B9600, B19200, 
 * B38400, B57600, B1200, B2400, B4800, etc. The values for 
 * parity are 0 (meaning no parity), PARENB|PARODD (enable 
 * parity and use odd), PARENB (enable parity and use even), 
 * PARENB|PARODD|CMSPAR (mark parity), and PARENB|CMSPAR 
 * (space parity).
 *
 * "Blocking" sets whether a read() on the port waits for the 
 * specified number of characters to arrive. Setting no blocking 
 * means that a read() returns however many characters are 
 * available without waiting for more, up to the buffer limit.
 */

#define error_message printf



// set the Parameters fo the Serial Port
// in  fd - fopen handle
// in  speed
// in  parity

//
// pdev -> handle to use
// devStr -> absolute path to serial Device
//
int OpenSerialDevice( ser_hndl_t * pdev, char * devStr)
{
    int  fd;
    int option;
    char portname[80];
    char test_file[80];
    int i = 0;
    int err;
    datablock db;

//  Init the data structure
    pdev->fd = 0;

// Start Init Here

    fd = open ( devStr, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        error_message ("error %d opening %s: %s", errno, portname, strerror (errno));
        return -1;
    }

    pdev->fd = fd;  //  handle for the threads to access serial port.
    strcpy(pdev->Device,devStr); // name
    pdev->exit_thread = 0;

    set_interface_attribs (pdev, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (pdev, 0);                // set no blocking


    return 0;
}


//
// pdev -> handle to use
//
int CloseSerialDevice( ser_hndl_t * pdev)
{
//  Init the data structure
    if ( pdev->fd != 0)
    {
      close( pdev->fd);
    }
    return 0;
}



int set_interface_attribs (ser_hndl_t * pdev, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (pdev->fd, &tty) != 0)
        {
                error_message ("error %d from tcgetattr", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (pdev->fd, TCSANOW, &tty) != 0)
        {
                error_message ("error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}


//
// Determines if a serial port read should block or not
// serail port will wait .5 seconds before timeout and return
// in fd - fopen handle
//
void set_blocking (ser_hndl_t * pdev, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (pdev->fd, &tty) != 0)
        {
                error_message ("error %d from tggetattr", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (pdev->fd, TCSANOW, &tty) != 0)
                error_message ("error %d setting term attributes", errno);
}


// http://www.linuxquestions.org/questions/programming-9/manually-controlling-rts-cts-326590/
int setRTS( ser_hndl_t * pdev, int level)
{
    int status;

    if (ioctl(pdev->fd, TIOCMGET, &status) == -1) {
        printf("setRTS(): TIOCMGET");
        return 0;
    }
    if (level)
        status |= TIOCM_RTS;
    else
        status &= ~TIOCM_RTS;
    if (ioctl(pdev->fd, TIOCMSET, &status) == -1) {
        printf("setRTS(): TIOCMSET");
        return 0;
    }
    return 1;
}

int setDTR( ser_hndl_t * pdev, int level)
{
    int status;

    if (ioctl( pdev->fd, TIOCMGET, &status) == -1) {
        printf("setDTR(): TIOCMGET");
        return 0;
    }
    if (level)
        status |= TIOCM_DTR;
    else
        status &= ~TIOCM_DTR;
    if (ioctl(pdev -> fd, TIOCMSET, &status) == -1) {
        printf("setDTR(): TIOCMSET");
        return 0;
    }
    return 1;
}

int getCTS( ser_hndl_t * pdev )
{
    int status;

    if (ioctl(pdev->fd, TIOCMGET, &status) == -1) {
        printf("getCTS(): TIOCMGET");
        return 0;
    }
    if (status & TIOCM_CTS)
        return 1;
    else
        return 0;
}

int getDSR( ser_hndl_t * pdev )
{
    int status;

    if (ioctl(pdev->fd, TIOCMGET, &status) == -1) {
        printf("getCTS(): TIOCMGET");
        return 0;
    }
    if (status & TIOCM_DSR)
        return 1;
    else
        return 0;
}





/*
 
...
char *portname = "/dev/ttyUSB1";
 ...
int fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
if (fd < 0)
{
        error_message ("error %d opening %s: %s", errno, portname, strerror (errno));
        return;
}

set_interface_attribs (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
set_blocking (fd, 0);                // set no blocking

write (fd, "hello!\n", 7);           // send 7 character greeting

usleep ((7 + 25) * 100);             // sleep enough to transmit the 7 plus
                                     // receive 25:  approx 100 uS per char transmit
char buf [100];
int n = read (fd, buf, sizeof buf);  // read up to 100 characters if ready to read

close(fd);

*/
//#define EXAMPLE  -- this needs to be defined in fln_serial.h

#ifdef SERIAL_EXAMPLE

void print_usage(void)
{
   printf("Serial Eye - Test Eye \n");
   printf("-p <serialport>   EG: /dev/ttyS0\n");
   printf("-t <TestFileName>  \n");
   printf("-v                Verbose Flag  \n");
   printf("-d <0xDebugFlags> Debug Flags  \n");
   printf("example: \n");
   printf("./main  -p /dev/ttyS0 -t test  -p9090 -v \n");
   printf("\n");
}


pthread_t g_tid[2];

// http://www.thegeekstuff.com/2012/04/create-threads-in-linux/
// void * struct passed as part of the Fork Worker thread
// typedef struct
// {
//     int done;
//     int fd;
// }datablock;
//

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

//    write (fd, "hello!\n", 7);           // send 7 character greeting

//    usleep ((7 + 25) * 100);             // sleep enough to transmit the 7 plus
                                     // receive 25:  approx 100 uS per char transmit
//   n = read (fd, buf, sizeof buf);  // read up to 100 characters if ready to read

//   printf("xxx:%s\n:xxx\n",buf);
 

int tx(void* arg)
{
    datablock * pdb =(datablock *)arg;
    char c;
    printf("Starting Tx Thread\n");
    printf("  Serial Handle: %d\n",pdb->pdev->fd);
    while( pdb->pdev->exit_thread == 0 )
    {
       c = getchar();
//       printf("%c",c);   // local echo??
       write (pdb->pdev->fd, &c, 1);           // send 7 character greeting
       if( c == 0x1b) pdb->pdev->exit_thread = 1;
    }
    return 1;
}

int rx(void* arg)
{
    int  n;
    int  i;
    char buf [100];
    datablock * pdb =(datablock *)arg;
    printf("Starting Rx Thread\n");

    while( pdb->pdev->exit_thread == 0) // this will be set by teh back door
    {
       n = read (pdb->pdev->fd, buf, sizeof(buf));  // read up to 100 characters if ready to read
       for( i = 0 ; i < n ; i++)
       printf("%c",buf[i]);
    }
    return 1;
}


// could make his a cute little structure!

void *WorkerThread(void *arg)
{
pthread_t id = pthread_self();
 
   if(pthread_equal(id,g_tid[0]))
   {
       // tx
       tx(arg);
   }
   else  if(pthread_equal(id,g_tid[1]))
  {
      // rx
      rx(arg);
  }
  else
  {
     // launch worker threads here ---
     error_message ("No function assigned to spawned \"fork\" \n");
  }
  return NULL;
}


uint64_t g_Debug = 0;
uint64_t g_Verbose = 0;
#define llu long long unsigned 
//
//
int main (int argc, char ** argv)
{

//    char *portname = "/dev/ttyUSB1";
//    char *portname = "/dev/ttyS0";
    int option;
    char portname[80];
    char test_file[80];
    int i = 0;
    int err;
    datablock db;    
    ser_hndl_t  serial_device = {0};
    ser_hndl_t  * p_ser = & serial_device;
    int result;


// Initialize Default Values
    memset((void*)&db,0,sizeof(datablock));
    strcpy(portname, "/dev/ttyS0");
    strcpy(test_file, "test.txt");

while ((option = getopt(argc, argv,"vrl:b:s:d:t:")) != -1) {
       switch (option) {
            case 'v' : g_Verbose=1 ;     // configure to bind w/ SO_REUSEADDR
                break;
            case 's' : strcpy(portname,optarg);      // Serial Port
                break;
            case 't' : strcpy(test_file,optarg);     // Test File
                break;
            case 'd' : g_Debug = atoi(optarg);// #of times to send File Data
                break;
            case 'h' :                               // Print Help
            default:
                print_usage();
//                exit(EXIT_FAILURE);
                return(0);
       }
   }
   printf("Command Line Arguments:\n");
   printf("  %16s SerialPort\n",portname);
   printf("  %16s Test File Name\n",test_file);
   printf("  %d   Verbose\n",g_Verbose);
   printf("  0x%016llx   Debug\n",(llu)g_Debug);

// Start Init Here
    result = OpenSerialDevice( p_ser , portname );
    if ( result != 0) // error Initializing
    {
        error_message ("error %d opening %s: %s", errno, portname, strerror (errno));
        return -1;
    }
    set_interface_attribs ( p_ser , B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking ( p_ser , 0);                // set no blocking


    // patch up the
    db.pdev= p_ser;  //  handle for the threads to access serial port.

    while(i < 2)
    {
        err = pthread_create(&(g_tid[i]), NULL, &WorkerThread, &db);
        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
        else
            printf("\n Thread created successfully\n");

        i++;
    }

    while(db.pdev->exit_thread == 0) {}    

    sleep(5);
    CloseSerialDevice( p_ser );
    return 0;

}
#endif
