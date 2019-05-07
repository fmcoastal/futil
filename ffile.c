#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <unistd.h>

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <time.h>

#include "ffile.h"

int64_t g_ffDebug = 0;

#define FFDEBUG(x)  if( (x & g_ffDebug) == x)

#define FF_DBG_GNL        0x0000000000000001
#define FF_DBG_PTR_STR    0x0000000000000002


static void ffPrintBuff(uint8_t * buffer, int32_t bufferSize,uint8_t * Address);


#ifndef TAB
#define TAB 0x0b
#endif
#ifndef CR
#define CR 0x0d
#endif
#ifndef LF
#define LF 0x0a
#endif

#define FFMIN(x,y) (x<y?x:y)
#define FFMAX(x,y) (x>y?x:y)






///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//   This function will create a File handle.
//     The filename is the base string in buf, along with
//     the date (MDHMS) and time.
//   I Base Name 
//   I suffix   (.txt, .dat, none
//   I OutName  pointer to where new string will be places.
//   I sz   Size of the OutName Buffer;

int ffMakeDatedFileName(char * BaseName, char * suffix, char * OutName ,int sz)
{
  time_t now;
  struct tm *info;
  char TimeStr[MAX_DATED_FILE_NAME] = {0};
  time(&now);
  info = localtime(&now);
  strftime(TimeStr, MAX_DATED_FILE_NAME, "%d_%H_%M_%S", info);

#if 0
  printf("%d:%s-%s \n",__LINE__,__FILE__,__FUNCTION__);
  printf("Base Name:  %s\n",BaseName);
  printf("suffix:  %s\n",suffix);
  printf("OutName:  %s    sz: %d\n",OutName,sz);
  printf("Time %s\n",TimeStr);
#endif
 
  if( (strlen(BaseName) +strlen(TimeStr)+strlen(suffix)+1) <= sz )
  {
     strcpy(OutName,BaseName);
     strcat(OutName,TimeStr);
     strcat(OutName,suffix);
  }  
  else
  {
      printf("%d:%s-%s source string not large enough\n",__LINE__,__FILE__,__FUNCTION__);
      return -1;
  }
  //  printf(" file to be created: %s\n",ptr);
  return 0;
}


// this function open sthe file "filename,  
//            allocates a buffer 
//            reads the file into the buffer
//            closes the file handle opened.
//   
int  ffReadFileToBuffer(char * filename, char **buf,int *  bufsz)
{
	int     len = 0 ;
	int      i;
	int      j;
	int     off;
        char *  buffptr;
	FILE     *fp = NULL;

	fp = fopen (filename, "r");
	if (fp == NULL) {
		printf ("Unable to open file %s\n", filename);
		return (EXIT_FAILURE);
	}
	fseek (fp, 0, SEEK_END);
	len = ftell (fp);
	if (len <= 0) {
		printf ("File %s empty\n", filename);
		return (EXIT_FAILURE);
	}
	fseek (fp, 0, SEEK_SET);
	buffptr = malloc (len);
	if (buffptr == NULL) {
		printf ("Unable to alloc memory\n");
		return (EXIT_FAILURE);
	}
	j = len;
	off = 0;
	while (j > 0) {
		i = fread (buffptr + off, 1, j, fp);
		off += i;
		j -= i;
	}
	fclose(fp);
        *buf = buffptr;
	*bufsz = len;
	return 0;

}



















int ffInitGetLine(ffGetLineHndl * ffHndl,  char * Buf,int BufLen,char * CommentChars)
{
	ffHndl->BufStrt = Buf;
	ffHndl->BufLen  = BufLen;
	ffHndl->line    = 0x00;     // line counter for errors??
	ffHndl->CChars[0] = 0x00;   // default set to an empty string
	if(strlen(CommentChars) < FF_MAX_COMMENT_CHARS)
	{
		strcpy(ffHndl->CChars,CommentChars);
	}
	return 0;
}


#define CARRIAGE_RETURN 0x0d
#define LINE_FEED 0x0a

#define FF_TEST_EOF             1
#define FF_STRIP_LEAD_ZEROS     2
#define FF_TEST_FOR_BLANK_LINE  3
#define FF_TEST_FOR_COMMENT     4
#define FF_GET_DATA             5

//#define FS_DEBUG


//
//   This function will return a pointer to the next non-commnet line in the input buffer
//     THe pointer actually points into the data stream
//     The function looks for the LINEFEED character and replaces it with 0x00
//     The function returns the size of the line incase you want to malloc a buffer to 
//         copy the source string.
//   
int ffGetLine(ffGetLineHndl * ffHndl,  char ** Buf,int *BufLen)
{
    char * ptr = ffHndl->BufStrt;
    int  len = ffHndl->BufLen;
    int i;
    int state = FF_TEST_EOF;
    int done = 0;
    int flag ; 

    // always leave the bufferPointer at the first character after 0x0d or 0x0d 0x0a :-)


    while (done == 0)
    {

        switch (state) {
        case FF_TEST_EOF:    
#ifdef FS_DEBUG
            printf ("%s:%5d  FF_TEST_EOF\n",__FUNCTION__,__LINE__);
#endif
            if(len <= 0)                  // need this because file might end on last line of buffer
            {
                ffHndl->BufLen = 0 ;      // reset back to zero
                ffHndl->BufStrt = ptr -1; // pust the ptr back to be at the last character
                *BufLen = 0;
                *Buf = NULL;
                return( FF_END_OF_BUFFER);
            }//end Len >0
             state = FF_STRIP_LEAD_ZEROS;      
          /* break; */  //  intentional for performance because unless EOF we will FF_STRIP_LEAD_ZEROS 
        case FF_STRIP_LEAD_ZEROS:   // skip over leading blank space
#ifdef FS_DEBUG
             printf ("%s:%5d    FF_STRIP_LEAD_ZEROS\n",__FUNCTION__,__LINE__);
#endif
             while(*ptr == ' ')
             {
                 len--;
                 ptr++;
                 if(len <= 0)
                 {
                     ffHndl->BufLen = 0 ;      // reset back to zero
                     ffHndl->BufStrt = ptr -1; // pust the ptr back to be at the last character
                     *BufLen = 0;
                     *Buf = NULL;
                     return( FF_END_OF_BUFFER);
                 }//end Len >0
             }
             state = FF_TEST_FOR_BLANK_LINE;      
             /* break;  */  // intentional for performance again
         case FF_TEST_FOR_BLANK_LINE:
#ifdef FS_DEBUG
             printf ("%s:%5d    FF_TEST_FOR_BLANK_LINE\n",__FUNCTION__,__LINE__);
#endif
             if(*ptr == LINE_FEED)
             {
               ptr++;               // push past the line feed
               len--;               // dec the buffer cnt
               ffHndl->line++;      // increment the line count
               if(len <= 0)         // Test if end of buffer
               {
                   ffHndl->BufLen = 0 ;      // reset back to zero
                   ffHndl->BufStrt = ptr -1; // pust the ptr back to be at the last character
                   *BufLen = 0;
                   *Buf = NULL;
                   return( FF_END_OF_BUFFER);
               }//end Len >0
               state = FF_STRIP_LEAD_ZEROS;
             }
             else
             {
               state = FF_TEST_FOR_COMMENT;
             }
             break;
        case FF_TEST_FOR_COMMENT:      // check for comment characteri
#ifdef FS_DEBUG
             printf ("%s:%5d    FF_TEST_FOR_COMMENT  \n",__FUNCTION__,__LINE__);
#endif
             flag = FF_GET_DATA; // pre-dispose to jump to next except if we find a comment
                                 // then go back an look for another comment line
             for( i = 0 ; ffHndl->CChars[i] != 0x00; i++)
             {
             	if( ffHndl->CChars[i] == *ptr)  // we have a comment;
             	{
                   flag = FF_STRIP_LEAD_ZEROS;  // Found COmment, config to look for another comment when done
                   ffHndl->line++;      // increment the line count
                   // take me to the end of this line
             	   while( *ptr != LINE_FEED )  
             	   {
             	      ptr++;
             	      len--;
             	      if(len <= 0)  
             	      {
             	          ffHndl->BufLen = 0 ;      // reset back to zero
             	          ffHndl->BufStrt = ptr -1; // pust the ptr back to be at the last character
             	          *BufLen = 0;
             	          *Buf = NULL;
             	          return( FF_END_OF_BUFFER);
             	      } //end Len >0
             	      ffHndl->BufLen = len;  // reset back to zero
             	      ffHndl->BufStrt = ptr; // pust the ptr back to be at the last character
             	    }  // end CR search         
                    // pointing at the Line Feed - move 1 more forward and we should be looking at first char in next line 
                    ptr++;
                    len--;
                    if(len <= 0)         // Test if end of buffer
                    {
                        ffHndl->BufLen = 0 ;      // reset back to zero
                        ffHndl->BufStrt = ptr -1; // pust the ptr back to be at the last character
                        *BufLen = 0;
                        *Buf = NULL;
                        return( FF_END_OF_BUFFER);
                    }//end Len >0
             	    ffHndl->BufLen = len;  // reset back to zero
             	    ffHndl->BufStrt = ptr; // pust the ptr back to be at the last character
                    break;
             	}  // end Comment Char found             
             } // end CChar array Search
             state = flag;      
             break;

        case FF_GET_DATA:      // check for comment character
#ifdef FS_DEBUG
             printf ("%s:%5d    FF_GET_DATA -> line %d\n",__FUNCTION__,__LINE__,(ffHndl->line+1));
#endif
             *BufLen = 0;  // set the Result Length to Zero
             *Buf = ptr;   // Set the beginning of the Pointer
             while( *ptr != LINE_FEED )  
             {
                 ptr++;
                 len--;
                 (*BufLen) +=1;
                 if(len <= 0)  
                 {
                     ptr --;                 // back up the pointer
                     *ptr = 0x00;            // Terminate the string 
                     *BufLen = *BufLen-1;    // Return the length of the string - Check this! 
                     ffHndl->BufLen = 0 ;    // reset len back to zero
                     ffHndl->BufStrt = ptr;  // Set the buffer to point to the NULL.
                     return( FF_GOOD_LINE);
                 } //end Len <0 but some buffer Data;
             } // end while
             *ptr = 0x00;              // Replace the CR & Terminate the string 
             len--;                    // put to the next character
             ptr++;
             ffHndl->line++;           // increment the line count
             ffHndl->BufLen = len;     // reset len to include Next Char
             ffHndl->BufStrt = ptr;    // Set the buffer to point to the next Char.
             return( FF_GOOD_LINE);
             break;
         default:
             done = 1;
             break;
        } // end switch
    } // end while done = 0; 
    return FF_NEVER_GET_HERE ;
}


#define fsprint printf
#define LLU long long unsigned

static void ffPrintBuff(uint8_t * buffer, int32_t bufferSize,uint8_t * Address)
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
                fsprint("\n%016x",(unsigned)((unsigned long)Address & ~0x000000000f));
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
            fsprint("\n%016llx",((LLU)Address & ~0x0f));
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


////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// Breaks "pline" into an array of strings
//      -- replaces TAB,SPACE,CR,LF,with 0x00
//   in pline
//   in  ptrs - either null Ind code will allocate buffer
//            - or an array of char* with nptrs the length of the array
//   out- updated ptrs and nptrs
//
//   remember to free ptrs if you pass in NULL
// This function
//     1) takes a string
//     2) counts the number of strings
//     3) allocates a buffer of char * of the correct Size if not passed in
//     4) terminates each string int the line,
//     5) fills in the pointer array with the number of elements found
//
#define  STRIP_SPACE_AND_TABS 0
#define  FIND_END_PARANTHESE  1
#define  FIND_NEXT_SPACE      2
int GetStringPointers(char * pline, char **ptrs,int *nptrs)
{
    int done ;
    int i    ;
    int s    ;
    int ptrCount ;
    char ** strptr;

//  parse the command line into an array of pointers to strings "str[]"
//  first pass, null terminate strings, and count number of strings

    FFDEBUG(FF_DBG_PTR_STR)
    {
        for( i = 0 ; (*(pline+i) != 0x00)  ; i++)
        {
             if(*(pline+i) < 0x20)
             {
                 printf("%2d . 0x%02x\n",i,*(pline+i));
             }
             else
             {
                 printf("%2d %c 0x%02x\n",i,*(pline+i),*(pline+i));
             }
         }
         printf("------\n");
    }

    ptrCount = 0;
    done = 0;
    i = 0 ;
    s = STRIP_SPACE_AND_TABS;

    while( done == 0)
    {
       switch (s)
       {
       case STRIP_SPACE_AND_TABS:      //Strip Lead Space and TABs

            while( (*(pline+i) ==  ' ') || (*(pline+i) ==  TAB))
           {
               FFDEBUG(FF_DBG_PTR_STR) printf(".");
               i++;
           }

           if( ((*(pline+i)) == LF ) || ( (*(pline+i)) == 0x00 ) || ((*(pline+i)) == CR ) ) // end of linei??
           {
               done = 1;
           }
           else if ((*(pline+i)) == '"')   // quote field ??
           {
               i++;                        // move to next Char
               ptrCount++;              // flag we have something to point to
               s = FIND_END_PARANTHESE;    // set next State
               FFDEBUG(FF_DBG_PTR_STR) printf("\nNextState: %d %s\n",s,"FIND_END_PARANTHESE");
           }
           else                            // regular text Line!
           {
               ptrCount++;              // flag we have something to point to
               s = FIND_NEXT_SPACE;      // set next State
               FFDEBUG(FF_DBG_PTR_STR) printf("\nNextState: %d %s\n",s,"FIND_NEXT_SPACE");
           }
           break;
       case FIND_NEXT_SPACE:

           while( (*(pline+i) !=  ' ') && (*(pline+i) !=  TAB)  && (*(pline+i) !=  0x00)  )
           {
               FFDEBUG(FF_DBG_PTR_STR) printf("%c",*(pline+i));
               i++;
           }
           FFDEBUG(FF_DBG_PTR_STR) printf("\n");
           if( ((*(pline+i)) == LF ) || ( (*(pline+i)) == 0x00 ) || ((*(pline+i)) == CR ) )
           {
               done = 1;                   // blow Out.
           }
           else    // space or tab
           {
               i++;                       // move to next Char
               s = STRIP_SPACE_AND_TABS;  // set next State
               FFDEBUG(FF_DBG_PTR_STR) printf("\nNextState: %d %s\n",s,"STRIP_SPACE_AND_TABS");
           }
           break;
       case FIND_END_PARANTHESE:

           while( (*(pline+i) !=  '"') && (*(pline+i) !=  LF) && (*(pline+i) !=  0x00)  )
           {
               FFDEBUG(FF_DBG_PTR_STR) printf("%c",*(pline+i));
               i++;
           }

           if( ((*(pline+i)) == LF ) || ((*(pline+i)) == 0x00 ))
           {
              done = 1;                   // blow Out.
           }
           else    // must be '"'
           {
               i++;                       // move to next Char
               s = STRIP_SPACE_AND_TABS;  // set next State
               FFDEBUG(FF_DBG_PTR_STR) printf("\nNextState: %d %s\n",s,"STRIP_SPACE_AND_TABS");
           }
           break;
        } // end switch
    }// end while

    FFDEBUG(FF_DBG_PTR_STR) printf(" pline %s\n",pline);
    FFDEBUG(FF_DBG_PTR_STR) printf(" ptrCount %d\n",ptrCount);

       if( ptrs == NULL)
       {
           ptrs = (char **) malloc( sizeof(char*) * ptrCount);
           if( ptrs == NULL)
           {
               printf("%d:%s-%s  unable to malloc pointers\n",__LINE__,__FILE__,__FUNCTION__);
               *nptrs = 0;
               return 0 ;
           }
           *nptrs = ptrCount;
       }
       else
       {
           // since we were pased an array, adujust ptrs we collect as to not exceed the size passed
           *nptrs = FFMIN( *nptrs ,ptrCount);
       }
       strptr = ptrs;  // init the array we are writing to

     ptrCount = 0;
     done = 0;
     i = 0 ;
     s = STRIP_SPACE_AND_TABS;

      while( done == 0)
      {
         switch (s)
         {
         case STRIP_SPACE_AND_TABS:      //Strip Lead Space and TABs

             while( (*(pline+i) ==  ' ') || (*(pline+i) ==  TAB)) {i++;}

             if( ((*(pline+i)) == LF ) || ( (*(pline+i)) == 0x00 ) || ((*(pline+i)) == CR ) ) // end of linei??
             {
                 *(pline+i) = 0x00 ;     // replace the LF with NULL
                 done = 1;
             }
             else if ((*(pline+i)) == '"')   // quote field ??
             {
                 i++;                        // move to next Char
                 if(ptrCount < *nptrs)     // Check that there is space available
                 {
                     *strptr++ = pline+i;   // save the ptr
                     ptrCount++;           // inc the counter
                 }
                 else
                 {
                     done = 1;    // blow out passed array is smaller than Args
                 }
                 s = FIND_END_PARANTHESE;    // set next State
                 FFDEBUG(FF_DBG_PTR_STR) printf("\nNextState: %d %s\n",s,"FIND_END_PARANTHESE");
             }
             else                            // regular text Line!
             {
                 if(ptrCount < *nptrs)     // check that there is space available
                 {
                     *strptr++ = pline+i;  // save the ptr
                     ptrCount++;          // inc the counter
                 }
                 else
                 {
                     done = 1;          // blow out passed array is smaller than Args
                 }
                 s = FIND_NEXT_SPACE;      // set next State
                 FFDEBUG(FF_DBG_PTR_STR) printf("\nNextState: %d %s\n",s,"FIND_NEXT_SPACE");
             }
             break;
         case FIND_NEXT_SPACE:

             while( (*(pline+i) !=  ' ') && (*(pline+i) !=  TAB) && (*(pline+i) !=  0x00)) {i++;}

             if( ((*(pline+i)) == LF ) || ( (*(pline+i)) == 0x00 ) || ((*(pline+i)) == CR ) )
             {
                 *(pline+i) = 0x00 ;     // replace the LF with NULL
                 done = 1;                   // blow Out.
             }
             else    // space or tab
             {
                 *(pline+i) = 0x00 ;     // flip to NULL
                 i++;                       // move to next Char
                 s = STRIP_SPACE_AND_TABS;  // set next State
                 FFDEBUG(FF_DBG_PTR_STR)  printf("\nNextState: %d %s\n",s,"STRIP_SPACE_AND_TABS");
             }
             break;
         case FIND_END_PARANTHESE:

             while( (*(pline+i) !=  '"') && (*(pline+i) !=  LF) && (*(pline+i) !=  0x00)  ) {i++;}

             if( ((*(pline+i)) == LF ) || ((*(pline+i)) == 0x00 ))
             {
                *(pline+i) = 0x00 ;     // flip to NULL
                done = 1;                   // blow Out.
             }
             else    // must be '"'
             {
                 *(pline+i) = 0x00 ;     // flip to NULL
                 i++;                       // move to next Char
                 s = STRIP_SPACE_AND_TABS;  // set next State
                 FFDEBUG(FF_DBG_PTR_STR) printf("\nNextState: %d %s\n",s,"STRIP_SPACE_AND_TABS");
             }
             break;
          } // end switch
      }// end while

    return ptrCount;  // ptrCount should be equal to *nptr if everything is oorrect.

}


/////////////////////////////////////////////////
/////////////////////////////////////////////////
//  RETURNS 0 if MatchStr is in SrcStr
// in pSrcStr, assume NULL terminated string for now
// in pMatchStr, String to see if it is in the pSrcStr
// returns 0 if present  !0 if not present
int CheckStringMatch(char * pSrcStr,char * pMatchStr)
{
   int i = 0 ; // i source Index;
   int m = 0 ; // MStr Match
   int len = strlen(pMatchStr) ; // include the NULL in the count
   int d = 0;
   int r;
   char c;

    while(d == 0)
    {
    c =  *(pSrcStr + i);
    if ( c  == 0x00)
    {
        r = 1 ;   // pattern not in this string
        d = 1 ;   // flag to blow out
    }
    if ( c != (*(pMatchStr + m++ )) )  // see if hte chars are mathcing
    {
       m = 0 ;
    }

    if( m == len)  // pattern is in this line
    {
        d = 1;
        r = 0;
    }
    i++;
    }

   return r;
}











//
// Will strip any hex nibbles (0-f) out of the line into a bit stream.
//
//
#define HIGH_NIBBLE 0
#define LOW_NIBBLE  1
int ffGetNextLookup(char * AsciiSrc,  char * BinOut,int *BinLen)
{
        int    hex       = 0;
        char * ptr       = AsciiSrc;
        int    OutLen    = 0;
        int    NextNibble = HIGH_NIBBLE;  // low nibble =1 when doing bits 3-0
        char * outptr    = BinOut;

        FFDEBUG(FF_DBG_GNL) { printf("%4d:%s AsciiSrc = \"%s\"\n",__LINE__,__FUNCTION__,AsciiSrc);}

        while ( *ptr != 0x00)
        {
                // Check if hex
                if( (( *ptr >= '0') && (*ptr <= '9'))    ||
                    (( *ptr >= 'a') && (*ptr <= 'f')) ||
                    (( *ptr >= 'A') && (*ptr <= 'F'))  )
                {
                        hex |= ( (*ptr)& 0x0f);   /* this takes care of 0-9 */
                        if ((( *ptr >= 'a') && (*ptr <= 'f')) ||
                                (( *ptr >= 'A') && (*ptr <= 'F')) )
                        {
                                hex += 9;
                        }
                        if( NextNibble == LOW_NIBBLE) // if true write the data to output stream
                        {
                                if(OutLen < *BinLen)
                                {
                                        *BinOut++ = hex;
                                }
                                hex = 0;
                                NextNibble = HIGH_NIBBLE;
                        }
                        else    // the High Nibble
                        {
                                hex = ( (hex << 4) & 0xf0) ;              // shift the data
                                if(OutLen < *BinLen)
                                {
                                       *BinOut = hex;  // write the upper nibbledo not increment ptr till lower nibble
                                        OutLen ++;     // yes there will be one more byte in the output buffer
                                }
                                NextNibble = LOW_NIBBLE;    // next car will be the low nibble
                        }
                } // end Hex Check
                ptr++;           // get the next Bit
        } // end while not eol or 0x00
        *ptr = 0;
        *BinLen = OutLen;
        FFDEBUG(FF_DBG_GNL)
        {
           printf("Number of Nibble  %d\n",((OutLen*2) - ( NextNibble == LOW_NIBBLE ? 1:0) ) );
           printf("byte length of output nibble string %d\n",OutLen);
           ffPrintBuff((unsigned char *)outptr,OutLen,NULL);
        }
        return 0;
}



// this should take an input file and display the contents
// write the file out without any comment lines

#if 0

FILE * g_fpOut  = NULL;

void cleanup(void)
{
   if(g_fpOut != NULL)  fclose(g_fpOut);
}

#define LINE_SIZE 1024
#define RESULT_BUFFER_SIZE 8
int  main(int argc, char ** argv)
{
    int    ArgC;
    char** ArgV;
//    char   CmdString[2048];   // for getting Character input

    int    result=0;

    char   Infile[2048];        // input file to read in
    char   *buf;                // buffer where file is read into
    int    bufsz;               // size of the buffer
    char  *buf_ptr_allocated;   // copy of buffer pointer the file was read into.

    ffGetLineHndl FileData;     // structure for handling Get Line functions
    ffGetLineHndl * pFileData;  // pointer to the above structure
    char * pLine;               // pointer filled in by get next line function 
    int    LineSz;              // size of line if you want to copy out the line data

    int    LineCount;           // counter for the number of non-comment lines

    pFileData = &FileData;

#if 1

    strcpy(Infile,"eye_auto.txt");
#else
     printf("\n Input File Name:");
     GetInput(&ArgC, &ArgV);
     if(ArgC != 1)
     {
          printf("\n Did not recognize input as Filename  \n");
          return -1;
     }
     strcpy(Infile,ArgV[0]);
#endif


     g_fpOut = fopen (outFileName, "w");
     if (g_fpOut == NULL) 
     {
        printf ("Unable to open file %s\n", outFileName);
        cleanup();
        return (-5);
     }



       
      // input should be a FileName
      result = ffReadFileToBuffer( Infile, &buf, &bufsz);
      if( result != 0)
      {
          printf(" Unable to open FIle  %s  Error:%d\n",Infile,result );
          cleanup();
          return -2;
      }
      buf_ptr_allocated = buf ;

     printf("Input File Size:%d\n",bufsz);


    //  Init the get line function 
    //  int ffInitGetLine(ffGetLineHndl * ffHndl,  char * Buf,int BufLen,char * CommentChars);
      result = ffInitGetLine(  pFileData ,  buf, bufsz ,"#");
      if( result != 0)
      {
          printf(" Unable to int ffInitGetLine   Error:%d\n",result );
          free (buf_ptr_allocated);
          cleanup();
          return -3;
      }
     
      // process the work  
      LineCount = 0 ;
      while (1)
      {
         // initialize the ggGetLine file
         result =  ffGetLine( pFileData,  &pLine, &LineSz);
         if ( result == FF_END_OF_BUFFER)
         {
             printf(" End of trace file :%d\n",result );
             free (buf_ptr_allocated);
             cleanup();
             return 0;
         }
         LineCount++;
         // Prccess the line data here
         printf("%d  %s\n",LineCount,pLine);
      } // end while done == 0


     cleanup();
     return 0 ;

}  // doNSPLookupFile




#endif


