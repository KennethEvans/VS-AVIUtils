// CopyImplementation.cpp : Provides the implementation of AVIConvert that uses
// AVIFileCreateStream, AVIStreamRead, and AVIStreamWrite

#include "stdafx.h"

// Determine what will be done for processing
// DO_DECOMPRESSION does decompressVideo, else copyStream for VIDEO
// copyStream is used for all others besides VIDEO
// DO_VIDEO and DO_AUDIO determine whether to do or skip VIDEO or AUDIO
#define DO_VIDEO 1
#define DO_DECOMPRESSION 0
#define DO_AUDIO 1

#define PATH_MAX _MAX_PATH
#define DEBUG 0

// Global variables
extern char aviFileName1[PATH_MAX];
extern char aviFileName2[PATH_MAX];
extern int aviFileSpecified1;
extern int aviFileSpecified2;

// Function prototypes;
int CopyImplemetation();
int parseCommand(int argc, char **argv);
void usage(void);

int CopyImplementation() {
	HRESULT hr; 
	BOOL libOpened=FALSE;
	PAVIFILE pFile1=NULL; 
	PAVIFILE pFile2=NULL;
	PAVISTREAM *pStreams=NULL;
	AVIFILEINFO fileInfo;
	AVISTREAMINFO streamInfo; 
	int i;
	int nStreams=0;

	// Initialize library
	AVIFileInit();
	libOpened=TRUE;

	// Check file size before opening the source file
	int fileLen1 = getFileSize(aviFileName1);

	// Open source file
	hr = AVIFileOpen(&pFile1,aviFileName1,OF_SHARE_DENY_WRITE,0L); 
	if(hr != AVIERR_OK) { 
		errMsg("Unable to open %s\n  [Error 0x%08x %s]",
			aviFileName1, hr, getErrorCode(hr)); 
		goto ABORT; 
	}

	// Print warning if the file size is < 1 GB
	if(fileLen1 >= 1024*1024*1024) {
		errMsg("\n*** Warning: Input file size [%.2f GB] is 1 GB or greater\n"
			"This may cause problems", fileLen1/1024./1024./1024.);
	}

	// Get source info
	hr = AVIFileInfo(pFile1,&fileInfo,sizeof(fileInfo));
	if(hr != AVIERR_OK) { 
		errMsg("Unable to get info from %s\n  [Error 0x%08x %s]",
			aviFileName1, hr, getErrorCode(hr)); 
		goto ABORT; 
	}
	nStreams=fileInfo.dwStreams;
	printf("\nInput: %s\n\n", aviFileName1);
	printFileInfo(fileInfo);

	// Open destination file
	hr = AVIFileOpen(&pFile2, aviFileName2, OF_WRITE|OF_CREATE, 0L);
	if(hr != AVIERR_OK) { 
		errMsg("Unable to open %s\n  [Error 0x%08x %s]",
			aviFileName2, hr, getErrorCode(hr)); 
		goto ABORT; 
	}

	// Allocate the streams
	if(nStreams <= 0) {
		errMsg("No streams in %s",aviFileName1); 
		goto ABORT;
	}
	pStreams=new PAVISTREAM[nStreams];
	if(!pStreams) {
		errMsg("Cannot allocate streams array"); 
		goto ABORT;
	}

	// Loop over streams
	for (i = 0; i < nStreams; i++) {
		// Get the stream
		printf("\nStream %d:\n",i);
		pStreams[i] = NULL; 
		hr = AVIFileGetStream(pFile1, &pStreams[i], 0L, i) ;
		if(hr != AVIERR_OK || pStreams[i] == NULL) {
			errMsg("Cannot open stream %d [Error 0x%08x %s]",
				i, hr, getErrorCode(hr)); 
			continue; 
		}

		// Get stream info
		hr = AVIStreamInfo(pStreams[i], &streamInfo, sizeof(streamInfo)); 
		if(hr != AVIERR_OK) { 
			errMsg("Unable to get stream info for stream %d [Error 0x%08x %s]",
				i, hr, getErrorCode(hr)); 
			goto ABORT; 
		}
		printStreamInfo(streamInfo);

		if (streamInfo.fccType == streamtypeVIDEO) { 
#if DO_VIDEO
			printf("\n  Processing VIDEO\n");
#  if DO_COMPRESSION
			hr = decompressVideo(pFile2, pStreams[i], NULL);
#  else
			hr = copyStream(pFile2, pStreams[i]);
#  endif
			if(hr != AVIERR_OK) {
				errMsg("Convert failed [Error 0x%08x %s]",
					hr, getErrorCode(hr)); 
				goto ABORT; 
			}
#endif
			continue;
		} else if (streamInfo.fccType == streamtypeAUDIO) { 
#if DO_AUDIO
			printf("\n  Processing AUDIO\n");
			hr = copyStream(pFile2, pStreams[i]);
			if(hr != AVIERR_OK) {
				errMsg("Copy failed");
				goto ABORT; 
			}
#endif
			continue;
		}  else if (streamInfo.fccType == streamtypeMIDI) {  
			printf("\n  Processing MIDI\n");
			hr = copyStream(pFile2, pStreams[i]);
			if(hr != AVIERR_OK) {
				errMsg("Copy failed");
				goto ABORT; 
			}
			continue;
		}  else if (streamInfo.fccType == streamtypeTEXT) {  
			printf("\n  Processing TEXT\n");
			hr = copyStream(pFile2, pStreams[i]);
			if(hr != AVIERR_OK) {
				errMsg("Copy failed");
				goto ABORT; 
			}
			continue;
		} else {
			printf("\n  Processing unknown fccType [%d]\n",streamInfo.fccType);
			hr = copyStream(pFile2, pStreams[i]);
			if(hr != AVIERR_OK) {
				errMsg("Copy failed");
				goto ABORT; 
			}
			printf("\n Unknown fccType [");
			printFourCcCode(streamInfo.fccType, "]\n");
			printf("]\n");
			continue;
		}
	}
	goto END;

END:
	// Normal exit
#if 0
	printf("\nBefore AVIFileRelease:\n");
	printFileInfo(pFile2);
	printf("\n");
	printAudioInfo(pFile2);
#endif

	// Close files
	if(pFile1) AVIFileRelease(pFile1);
	if(pFile2) AVIFileRelease(pFile2);

#if 0
	// Gets get a memory error
	printf("\nAfter AVIFileRelease:\n");
	printFileInfo(pFile2);
	printf("\n");
	printAudioInfo(pFile2);
#endif

	// Release AVIFile library 
	if(libOpened) AVIFileExit();
	// Free space
	if(pStreams) delete [] pStreams;
	// Print warning if the file size is < 1 GB
	if(fileLen1 >= 1024*1024*1024) {
		errMsg("\n*** Warning: Input file size [%.2f GB] is 1 GB or greater\n"
			"This may have caused problems\n"
			"Check the output file\n",
			fileLen1/1024./1024./1024.);
	}

	printf("\nAll done\n");
	// Wait for a prompt so the console doesn't go away
	printf("Type return to continue:\n");
	_gettchar();
	return 0;

ABORT:
	// Abnormal exit
	// Close files
	if(pFile1) AVIFileRelease(pFile1);
	if(pFile2) AVIFileRelease(pFile2);
	// Release AVIFile library 
	if(libOpened) AVIFileExit();
	// Free space
	if(pStreams) delete [] pStreams;

	printf("\nAborted\n");
	// Wait for a prompt so the console doesn't go away
	printf("Type return to continue:\n");
	_gettchar();
	return 1;
}
