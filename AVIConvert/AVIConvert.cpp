// AVIConvert.cpp : Defines the entry point for the console application.

#include "stdafx.h"

#define APP_OK 0
#define APP_ERROR 1

#define PATH_MAX _MAX_PATH
#define DEBUG 0

// Function prototypes
int parseCommand(int argc, char **argv);
void usage(void);
HRESULT copyStream(PAVISTREAM pAvi);
HRESULT convertVideo(PAVISTREAM gapavi);

// Global variables
char aviFileName1[PATH_MAX];
char aviFileName2[PATH_MAX];
int aviFileSpecified1=0;
int aviFileSpecified2=0;
PAVIFILE pFile1=NULL; 
PAVIFILE pFile2=NULL;
PAVISTREAM *gapavi=NULL;

int _tmain(int argc, _TCHAR* argv[])
{
	HRESULT hr; 
	BOOL libOpened=FALSE;
	AVIFILEINFO aviInfo;
	AVISTREAMINFO avis; 
#if 0
	double sps,ssps,length,slength;
#endif
	int i;
	int nStreams=0,gcpavi=0;

	// Title
	printf("\nAVIDump\n");

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
	libOpened=TRUE;

	// Open source file
	hr=AVIFileOpen(&pFile1,aviFileName1,OF_SHARE_DENY_WRITE,0L); 
	if(hr != 0) { 
		errMsg("Unable to open %s [Error 0x%08x %s]",
			aviFileName1, hr, getErrorCode(hr)); 
		goto ABORT; 
	}

	// Get source info
	hr=AVIFileInfo(pFile1,&aviInfo,sizeof(aviInfo));
	if(hr != 0) { 
		errMsg("Unable to get info from %s [Error 0x%08x %s]",
			aviFileName1, hr, getErrorCode(hr)); 
		goto ABORT; 
	}
	nStreams=aviInfo.dwStreams;

	// Open destination file (create new one)
	hr=AVIFileOpen(&pFile2, aviFileName2, OF_CREATE, 0L);
	if(hr != 0) { 
		errMsg("Unable to open %s [Error 0x%08x %s]",
			aviFileName2, hr, getErrorCode(hr)); 
		goto ABORT; 
	}

	printf("\nInput: %s\n\n", aviFileName1);
	printFileInfo(aviInfo);

	// Open the streams
	if(nStreams <= 0) {
		errMsg("No streams in %s",aviFileName1); 
		goto ABORT;
	}
	gapavi=new PAVISTREAM[nStreams];
	if(!gapavi) {
		errMsg("Cannot allocate streams array"); 
		goto ABORT;
	}

	// Loop over streams
	int nFrames = 0;
	int firstFrame = 0;
	for (i = 0; i < nStreams; i++) {
		printf("\nStream %d:\n",i);
		gapavi[i] = NULL; 
		hr=AVIFileGetStream(pFile1,&gapavi[i],0L,i - gcpavi) ;
		if(hr != AVIERR_OK || gapavi[i] == NULL) {
			errMsg("Cannot open stream %d",i); 
			continue; 
		}
		// Get stream info
		hr=AVIStreamInfo(gapavi[i],&avis,sizeof(avis)); 
		if(hr != 0) { 
			errMsg("Unable to get stream info for stream %d",i); 
			goto ABORT; 
		}
		printStreamInfo(avis);

		if (avis.fccType == streamtypeVIDEO) { 
			printf("\n  Processing VIDEO\n");
			//hr = convertVideo(gapavi[i]);
			hr = copyStream(gapavi[i]);
			if(hr != AVIERR_OK) {
				errMsg("Convert failed [Error 0x%08x %s]",
					hr, getErrorCode(hr)); 
				goto ABORT; 
			}
			continue;
		} else if (avis.fccType == streamtypeAUDIO) { 
			printf("\n  Processing AUDIO\n");
			hr = copyStream(gapavi[i]);
			if(hr != AVIERR_OK) {
				errMsg("Copy failed");
				goto ABORT; 
			}
			continue;
		}  else if (avis.fccType == streamtypeMIDI) {  
			printf("\n  Processing MIDI\n");
			hr = copyStream(gapavi[i]);
			if(hr != AVIERR_OK) {
				errMsg("Copy failed");
				goto ABORT; 
			}
			continue;
		}  else if (avis.fccType == streamtypeTEXT) {  
			printf("\n  Processing TEXT\n");
			hr = copyStream(gapavi[i]);
			if(hr != AVIERR_OK) {
				errMsg("Copy failed");
				goto ABORT; 
			}
			continue;
		} else {
			printf("\n  Processing unknown fccType [%d]\n",avis.fccType);
			hr = copyStream(gapavi[i]);
			if(hr != AVIERR_OK) {
				errMsg("Copy failed");
				goto ABORT; 
			}
			printf("  Unknown fccType [%d]\n",avis.fccType);
			continue;
		}
	}

	AVIFILEINFO aviInfo2;	
	hr=AVIFileInfo(pFile2,&aviInfo2,sizeof(aviInfo2));
	if(hr != 0) { 
		errMsg("Unable to get info from %s [Error 0x%08x %s]",
			aviFileName2, hr, getErrorCode(hr)); 
		goto ABORT; 
	}

	printf("\nOutput: %s\n\n", aviFileName2);
	printFileInfo(aviInfo2);

	goto END;

END:
	// Normal exit
	// Close files
	if(pFile1) AVIFileRelease(pFile1);
	if(pFile2) AVIFileRelease(pFile2);
	// Release AVIFile library 
	if(libOpened) AVIFileExit();
	// Free space
	if(gapavi) delete [] gapavi;
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
	if(gapavi) delete [] gapavi;

	printf("\nAborted\n");
	// Wait for a prompt so the console doesn't go away
	printf("Type return to continue:\n");
	_gettchar();
	return 1;
}

int parseCommand(int argc, char **argv)
{
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
		} else {
			if(!aviFileSpecified1) {
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
	}
	return APP_OK;
}

void usage(void)
{
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

HRESULT copyStream(PAVISTREAM pAvi) {
	if(!pAvi) {
		errMsg("PAVISTREAM input is null"); 
		return AVIERR_BADPARAM; 
	}

	HRESULT hrReturnVal = AVIERR_OK;
	HRESULT hr;
	PAVISTREAM pAviOut;
	AVISTREAMINFO avisOut;
	ZeroMemory(&avisOut, sizeof(avisOut));

	// Get the format
	char *frameFormat = NULL;
	LONG frameFormatSize = 0;
	hr = AVIStreamReadFormat(pAvi, 0, NULL, &frameFormatSize);
	if(hr != AVIERR_OK) {
		errMsg("Cannot read format size [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		hrReturnVal = hr;
		goto CLEANUP;
	}
	frameFormat = new char[frameFormatSize];
	hr = AVIStreamReadFormat(pAvi, 0,
		frameFormat, &frameFormatSize);
	if(hr != AVIERR_OK) {
		errMsg("Cannot read format [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		hrReturnVal = hr;
		goto CLEANUP;
	}

	// Create the stream
	hr = AVIFileCreateStream(pFile2, &pAviOut, &avisOut);
	if(hr != AVIERR_OK) {
		errMsg("Cannot create stream [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		hrReturnVal = hr;
		goto CLEANUP;
	}

	// Set the format
	hr = AVIStreamSetFormat(pAviOut, 0, frameFormat, frameFormatSize);
	if(hr != AVIERR_OK) {
		errMsg("Cannot set format [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		hrReturnVal = hr;
		goto CLEANUP;
	}

#if DEBUG
	printf("frameFormatSize=%d\n", frameFormatSize);
	printf("BITMAPINFOHEADER=%d PCMWAVEFORMAT=%d WAVEFORMAT=%d WAVEFORMATEX=%d\n",
		sizeof(BITMAPINFOHEADER), sizeof(PCMWAVEFORMAT),
		sizeof(WAVEFORMAT), sizeof(WAVEFORMATEX));
#endif

	// Loop over the frames
	char *frame = NULL;
	LONG frameSize = 0;
	LONG n = 1;
	hr = AVIERR_OK;
	while(hr == AVIERR_OK) {
		hr = AVIStreamRead(pAvi, n, 1, NULL, 0, &frameSize, NULL);
		if(hr != AVIERR_OK && hr != AVIERR_ERROR) {
			errMsg("Error reading stream size [Error 0x%08x %s]",
				hr, getErrorCode(hr)); 
		}
		if(hr != AVIERR_OK) {
			break;
		}
		frame = new char[frameSize];
		hr = AVIStreamRead(pAvi, n, 1, frame, frameSize, NULL, NULL);
		if(hr != AVIERR_OK) {
			errMsg("Error reading stream [Error 0x%08x %s]",
				hr, getErrorCode(hr)); 
			hrReturnVal = hr;
			goto CLEANUP;
		}
		if(AVIStreamIsKeyFrame(pAvi, n))		    {
			AVIStreamWrite(pAviOut, n, 1, frame, frameSize,
				AVIIF_KEYFRAME, NULL, NULL);
		} else  {
			AVIStreamWrite(pAviOut, n, 1, frame, frameSize,
				0, NULL, NULL);
		}
		delete [] frame;
		frame = NULL;
		n++;
	}
	printf("  %d frames written\n", n);

CLEANUP:
	AVIStreamRelease(pAviOut);
	if(frameFormat) delete [] frameFormat;
	if(frame) delete [] frame;

	return hrReturnVal;
}

HRESULT convertVideo(PAVISTREAM pAvi) {
	if(!pAvi) {
		errMsg("PAVISTREAM input is null"); 
		return AVIERR_BADPARAM; 
	}

	HRESULT hr;
	DWORD res;
	HIC hic;
	BITMAPINFO bmi;
	long biSize = sizeof(bmi);
	ZeroMemory(&bmi, biSize);
	hr = AVIStreamReadFormat(pAvi, 0, &bmi, &biSize);
	if(hr != AVIERR_OK) {
		errMsg("Cannot get BITMAPINFO [Error 0x%08x %s]", hr, getErrorCode(hr)); 
		return hr; 
	}

	// Determine compression
	BITMAPINFOHEADER header = bmi.bmiHeader;
	printBmiHeaderInfo("    ", header);
	DWORD biCompression = header.biCompression;
	printf("  Compression: ");
	printFourCcCode(biCompression, "\n");

	// Open the decompressor
	hic = ICDecompressOpen(ICTYPE_VIDEO, biCompression, &header, NULL);
	if(hic) {
		printf("  ICDecompressOpen successful\n");
	} else {
		printf("  ICDecompressOpen unsuccessful\n");
		goto CLEANUP;
	}

	// Get the output format
	BITMAPINFO bmiOut;
	res = ICDecompressGetFormat(hic, &bmi, &bmiOut);
	if(res != ICERR_OK) {
		printf("Default output format not available\n");
		goto CLEANUP;
	} else {
		printf("  Default output format:\n");
		printBmiHeaderInfo("    ", bmiOut.bmiHeader);
	}

CLEANUP:
	ICClose(hic);
	return AVIERR_OK;
}
