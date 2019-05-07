#ifndef _ffile_h
#define _ffile_h



//
//  Reads the file
//  Determines the file size
//  Allocates a buffer
//  Reads the file into the buffer
//  closes the file
//  Returns 0 on succes
//  Returns Any error returned from call functions. ??

int  ffReadFileToBuffer(char * filename, char **buf,int *  bufsz);



//// 
// Will strip any hex nibbles (0-f) out of the line into a bit stream.  
//
//
int ffGetNextLookup(char * AsciiSrc,  char * BinOut,int *BinLen);


#define FF_MAX_COMMENT_CHARS  16

typedef struct ffGetLineStruct {
	char * BufStrt;
	int    BufLen;
	int    Index;
	int    line;
	char   CChars[FF_MAX_COMMENT_CHARS];   /* comment characters*/
} ffGetLineHndl;


#define FF_GOOD_LINE           0
#define FF_END_OF_BUFFER       1
#define FF_NEVER_GET_HERE      2

// 
// Will strip any hex nibbles (0-f) out of the line into a bit stream.  
//   i- ffdata - Handle
//   i - Buff    - input file. 
//   i - BufLen  - sice of input file
//   i - CimmentChars  - string of characters to recognize as comment 
//                       lines if first not space matches any of these values.
int ffInitGetLine(ffGetLineHndl * ffHndl,  char * Buf,int BufLen,char * CommentChars);


//
//   This function will return a pointer to the next non-commnet line in the input buffer
//   0 Buf
//   0 BufLen
int ffGetLine(ffGetLineHndl * ffHndl,  char ** Buf,int *BufLen);


#define MAX_DATED_FILE_NAME 128
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//   This function will create a File handle.
//     The filename is the base string in buf, along with
//     the date (MDHMS) and time.
//   I Base Name
//   I suffix   (.txt, .dat, none
//   I OutName  pointer to where new string will be places.
//   I sz   Size of the OutName Buffer;

int ffMakeDatedFileName(char * BaseName, char * suffix, char * OutName ,int sz);






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
int GetStringPointers(char * pline, char **ptrs,int *nptrs);


/////////////////////////////////////////////////
/////////////////////////////////////////////////
// Returns 0 if MatchStr is found inside of SrcStr
// in pSrcStr, assume NULL terminated string for now
// in pMatchStr, String to see if it is in the pSrcStr
// returns 0 if present  !0 if not present
int CheckStringMatch(char * pSrcStr,char * pMatchStr);




#endif
