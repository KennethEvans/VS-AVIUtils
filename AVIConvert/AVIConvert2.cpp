// AVIConvert.cpp : Defines the entry point for the console application.

#include "stdafx.h"

#define APP_OK 0
#define APP_ERROR 1

#define PATH_MAX _MAX_PATH
#define DEBUG 0

// Function prototypes
int parseCommand(int argc, char **argv);
void usage(void);

// Global variables
char aviFileName1[PATH_MAX];
char aviFileName2[PATH_MAX];
int aviFileSpecified1=0;
int aviFileSpecified2=0;

int _tmain(int argc, _TCHAR* argv[])
{
	HRESULT hr; 
	BOOL libOpened = FALSE;
	PAVIFILE pFile1 = NULL; 
	AVIFILEINFO fileInfo;
	AVISTREAMINFO streamInfo; 
	PAVISTREAM *pStreams = NULL;
	LPAVICOMPRESSOPTIONS *pOpts = NULL;
	int i;
	int nStreams = 0;
	int aborted = 0;

	// Title
	printf("\nAVIConvert\n");

	// Parse command line
	if(parseCommand(argc,argv) != APP_OK) {
		return 1;
	}
	if(!aviFileSpecified1) {
		errMsg("No AVI source file specified\n");
		goto ABORT;
	}
	if(!aviFileSpecified2) {
		errMsg("No AVI destination file specified\n");
		goto ABORT;
	}
	printf("\nSource: %s\n",aviFileName1);
	printf("Dest: %s\n",aviFileName2);

	// Initialize library
	AVIFileInit();
	libOpened = TRUE;

	// Open source file
	hr=AVIFileOpen(&pFile1, aviFileName1, OF_SHARE_DENY_WRITE, 0L); 
	if(hr != AVIERR_OK) { 
		errMsg("Unable to open %s\n  [Error 0x%08x %s]",
			aviFileName1, hr, getErrorCode(hr)); 
		goto ABORT; 
	}

	// Get source info
	hr = AVIFileInfo(pFile1, &fileInfo, sizeof(fileInfo));
	if(hr != AVIERR_OK) { 
		errMsg("Unable to get info from %s\n  [Error 0x%08x %s]",
			aviFileName1, hr, getErrorCode(hr)); 
		goto ABORT; 
	}
	nStreams = fileInfo.dwStreams;
	printf("\nInput: %s\n\n", aviFileName1);
	printFileInfo(fileInfo);

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

	// Allocate the opts
	pOpts = new LPAVICOMPRESSOPTIONS[nStreams];
	if(!pStreams) {
		errMsg("Cannot allocate streams array"); 
		goto ABORT;
	}

	// Loop over streams
	for (i = 0; i < nStreams; i++) {
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

		// Create the AVICOMPRESSOPTIONS
		pOpts[i] = new  AVICOMPRESSOPTIONS;
		ZeroMemory(pOpts[i], sizeof(AVICOMPRESSOPTIONS));
		//(*pOpts[i]).fccType = streamInfo.fccType;

#if 0
		if(i == 0) {
			// Print AVICOMPRESSOPTIONS
			printf("\n  Default AVICOMPRESSOPTIONS\n");
			printCompressOptions(*pOpts[i]);
		
#endif

		if (streamInfo.fccType == streamtypeVIDEO) { 
			printf("\n  Processing VIDEO\n");
			// Create a new empty editable stream
			PAVISTREAM pStream2 = NULL;
#if 1
			hr = CreateEditableStream(&pStream2, NULL);
			if(hr != AVIERR_OK) { 
				errMsg("Unable to get create new video stream [Error 0x%08x %s]",
					i, hr, getErrorCode(hr)); 
				continue;
			}
#endif
#if 0
			hr = AVIStreamCreate(&pStream2, 0, 0, NULL);
#endif
			if(hr != AVIERR_OK) { 
				errMsg("Unable to get create new video stream [Error 0x%08x %s]",
					hr, getErrorCode(hr)); 
				continue;
			}
			hr = decompressVideo(pStreams[i], pStream2);
			if(hr != AVIERR_OK) { 
				errMsg("Unable to convert video [Error 0x%08x %s]",
					i, hr, getErrorCode(hr)); 
				continue;
			}
			// ????????????????????????????????????????????????????????????????????????????
			AVIStreamRelease(pStream2);
			pStreams[i] = pStream2;
			continue;
		} else if (streamInfo.fccType == streamtypeAUDIO) { 
			printf("\n  Processing AUDIO\n");
			printf("    Doing nothing, input stream will be used\n");
			continue;
		}  else if (streamInfo.fccType == streamtypeMIDI) {  
			printf("\n  Processing MIDI\n");
			printf("    Doing nothing, input stream will be used\n");
			continue;
		}  else if (streamInfo.fccType == streamtypeTEXT) {  
			printf("\n  Processing TEXT\n");
			printf("    Doing nothing, input stream will be used\n");
			continue;
		} else {
			printf("\n  Processing unknown fccType [");
			printFourCcCode(streamInfo.fccType, "]\n");
			printf("    Doing nothing, input stream will be used\n");
			continue;
		}
	}

	// Save the file
	printf("\nSaving file...\n");
	hr = AVISaveV(aviFileName2, NULL, NULL, nStreams, pStreams, pOpts);
	if(hr != AVIERR_OK) {
		errMsg("Error saving file [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		goto ABORT; 
	} else {
		printf("\nSaved %s\n", aviFileName2);
	}

	goto END;

ABORT:
	aborted = 1;

END:
	// Close files
	if(pFile1) AVIFileRelease(pFile1);

	// Release AVIFile library 
	if(libOpened) AVIFileExit();
	// Free space
	if(pStreams) delete [] pStreams;

	if(aborted) {
		printf("\nAborted\n");
	} else {
		printf("\nAll done\n");
	}

	// Wait for a prompt so the console doesn't go away
	printf("Type return to continue:\n");
	_gettchar();

	return aborted;
}

int parseCommand(int argc, char **argv) {
	for(int i=1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch(argv[i][1]) {
#if 0
			case 's':
				doServer=1;
				serverName=argv[++i];
				break;
#endif
			case 'h':
				usage();
				return APP_OK;
			default:
				fprintf(stderr,"\n\nInvalid option: %s\n",argv[i]);
				usage();
				return APP_ERROR;
			}
		} else if(!aviFileSpecified1) {
			strcpy(aviFileName1,argv[i]);
			aviFileSpecified1=1;
		} else if(!aviFileSpecified2) {
			strcpy(aviFileName2,argv[i]);
			aviFileSpecified2=1;
		} else {
			errMsg("\n\nInvalid option: %s\n",argv[i]);
			usage();
			return APP_ERROR;
		}
	}
	return APP_OK;
}

void usage(void) {
	printf(
		"\nUsage: AVIConvert [Options] srcfilename destfilename\n"
		"  Converts AVI files for Pinnacle Studio 8\n"
		"\n"
		"  Options:\n"
		"    srcfilename  Name of an AVI file to convert\n"
		"    destfilename Name of converted file\n"
		"    -h help      This message\n"
		);
}
