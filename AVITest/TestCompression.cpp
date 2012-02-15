// TestCompression.cpp : Experiment with compression.

#include "stdafx.h"

#define TEST4

#ifdef TEST0
#  define FILE_NAME_IN "junktest.avi"
#  define FILE_NAME_OUT "junktest.test.avi"
#elif defined(TEST1)
#  define FILE_NAME_IN "Reel 1- 4.avi"
#  define FILE_NAME_OUT "Reel 1- 4.test.avi"
#elif defined(TEST2)
#  define FILE_NAME_IN "Reel 5-8.avi"
#  define FILE_NAME_OUT "Reel 5-8.test.avi"
#elif defined(TEST3)
#  define FILE_NAME_IN "Reel 9-11.avi"
#  define FILE_NAME_OUT "Reel 9-11.test.avi"
#elif defined(TEST4)
#  define FILE_NAME_IN "Reel 12.avi"
#  define FILE_NAME_OUT "Reel 12.test.avi"
#elif defined(TEST5)
#  define FILE_NAME_IN "Reel 13.avi"
#  define FILE_NAME_OUT "Reel 13.test.avi"
#elif defined(TEST6)
#  define FILE_NAME_IN "Rapids Boating.avi"
#  define FILE_NAME_OUT "Rapids Boating.test.avi"
#endif

void testCompression() {
	HRESULT hr;
	PAVISTREAM *pStreams=NULL;

	PAVISTREAM pStream = NULL;
	LPAVICOMPRESSOPTIONS *pOpts = NULL;
	PAVIFILE pFile = NULL;

	printf("\nInput:  %s\n", FILE_NAME_IN);
	printf("Output: %s\n", FILE_NAME_OUT);

	AVIFileInit();

	// Open source file
	hr = AVIFileOpen(&pFile, FILE_NAME_IN, OF_SHARE_DENY_WRITE,0L); 
	if(hr != 0) { 
		errMsg("Unable to open %s\n  [Error 0x%08x %s]",
			FILE_NAME_IN, hr, getErrorCode(hr)); 
		goto ABORT; 
	}

	// Get the number of streams
	DWORD nStreams = 0;
	hr = getNStreams(pFile, &nStreams);
	if(hr != 0) { 
		errMsg("Unable to get number of streams from %s\n  [Error 0x%08x %s]",
			FILE_NAME_IN, hr, getErrorCode(hr)); 
		goto ABORT; 
	}

	// Allocate the streams
	if(nStreams <= 0) {
		errMsg("No streams in %s", FILE_NAME_IN); 
		goto ABORT;
	}
	pStreams=new PAVISTREAM[nStreams];
	if(!pStreams) {
		errMsg("Cannot allocate streams array"); 
		goto ABORT;
	}

	// Allocate the opts
	pOpts = new LPAVICOMPRESSOPTIONS[nStreams];
	if(!pStreams) {
		errMsg("Cannot allocate streams array"); 
		goto ABORT;
	}

	// Loop over streams
	AVISTREAMINFO streamInfo;
	for (DWORD i = 0; i < nStreams; i++) {
		// Get the stream
		printf("\nStream %d\n", i);
		pStreams[i] = NULL; 
		hr=AVIFileGetStream(pFile,&pStreams[i],0L,i) ;
		if(hr != AVIERR_OK || pStreams[i] == NULL) {
			errMsg("Unable to get info from %s\n  [Error 0x%08x %s]",
				FILE_NAME_IN, hr, getErrorCode(hr)); 
			goto ABORT;
		}

		// Get stream info
		hr = AVIStreamInfo(pStreams[i], &streamInfo, sizeof(streamInfo)); 
		if(hr != 0) { 
			errMsg("Unable to get stream info for stream %d [Error 0x%08x %s]",
				i, hr, getErrorCode(hr)); 
			goto ABORT; 
		}
		printStreamInfo(streamInfo);

		// Create the AVICOMPRESSOPTIONS
		pOpts[i] = new  AVICOMPRESSOPTIONS;
		ZeroMemory(pOpts[i], sizeof(AVICOMPRESSOPTIONS));
		(*pOpts[i]).fccType = streamInfo.fccType;

		// Print AVICOMPRESSOPTIONS
		printf("\n  AVICOMPRESSOPTIONS\n");
		printCompressOptions(*pOpts[i]);
	}

	// Save the file
	printf("\nSaving file...\n");
	hr = AVISaveV(FILE_NAME_OUT, NULL, NULL, nStreams, pStreams, pOpts);
	if(hr != AVIERR_OK) {
		errMsg("Error saving file [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		goto ABORT; 
	} else {
		printf("\nSaved %s\n", FILE_NAME_OUT);
	}

#if 0
	// Print AVICOMPRESSOPTIONS
	printf("\nAVICOMPRESSOPTIONS after\n");
	printCompressOptions(opts);
#endif

	goto CLEANUP;

ABORT:

CLEANUP:
	if(pFile) AVIFileRelease(pFile);
	AVIFileExit();
	// Free space
	if(pStreams) {
		delete [] pStreams;
		pStreams = NULL;
	}
	if(pOpts) {
		for(DWORD i = 0; i < nStreams; i++) {
			if(pOpts[i]) delete pOpts[i];
			pOpts[i] = NULL;
		}
		delete [] pOpts;
		pOpts = NULL;
	}

	printf("\nAll done\n");
}
