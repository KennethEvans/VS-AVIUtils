// AVIDump.cpp : Defines the entry point for the console application.

#include "stdafx.h"

#define APP_OK 0
#define APP_ERROR 1

#define PATH_MAX _MAX_PATH
#define OUTPUT_FILE "AVIDump.txt"
#define USE_FILE 0
#define DEBUG 0

// Function prototypes
int parseCommand(int argc, char **argv);
void usage(void);
char aviFileName[PATH_MAX];
int aviFileSpecified=0;
extern char errorString[1024];  // Danger fixed size

// Global variables
int useFile = 0;

int _tmain(int argc, _TCHAR* argv[]) {
	HRESULT hr; 
	PAVIFILE pFile=NULL; 
	BOOL libOpened=FALSE;
	AVIFILEINFO fileInfo;
	AVISTREAMINFO streamInfo; 
	PAVISTREAM *pStreams=NULL;
	int i;
	int nStreams=0;
	char *fileName = NULL;
	FILE *out = NULL;

	// Title
	printf("\nAVIDump\n");

	// Parse command line
	if(parseCommand(argc,argv) != APP_OK) {
		return 1;
	}
	if(!aviFileSpecified) {
		errMsg("No AVI file specified\n");
		goto ABORT;
	}

	if(useFile) {
		// Redirect stdout
		int len = strlen(OUTPUT_FILE) + strlen(aviFileName) + 2;
		fileName = new char[len];
		fileName[0] = '\0';
		strcat(fileName, aviFileName);
		strcat(fileName, ".");
		strcat(fileName, OUTPUT_FILE);
		out = freopen(fileName, "w", stdout);
		if(out == NULL) {
			errMsg("Cannot redirect stdout\n");
			goto ABORT;
		}
		// Reprint title
		printf("\nAVIDump\n");
	}

	printf("\n%s\n\n",aviFileName);

	// Initialize library
	AVIFileInit();
	libOpened=TRUE;

	// Open file
	hr=AVIFileOpen(&pFile,aviFileName,OF_SHARE_DENY_WRITE,0L); 
	if(hr != AVIERR_OK) { 
		errMsg("Unable to open %s [Error 0x%08x %s]",
			aviFileName, hr, getErrorCode(hr)); 
		goto ABORT; 
	}

	// Get info
	hr=AVIFileInfo(pFile,&fileInfo,sizeof(fileInfo));
	if(hr != AVIERR_OK) { 
		errMsg("Unable to get info from %s [Error 0x%08x %s]",
			aviFileName, hr, getErrorCode(hr)); 
		goto ABORT; 
	}
	nStreams=fileInfo.dwStreams;
	printFileInfo(fileInfo);

	// Open the streams
	if(nStreams <= 0) {
		errMsg("No streams in %s",aviFileName); 
		goto ABORT;
	}
	pStreams=new PAVISTREAM[nStreams];
	if(!pStreams) {
		errMsg("Cannot allocate streams array"); 
		goto ABORT;
	}
	for (i = 0; i < nStreams; i++) {
		printf("\nStream %d:\n",i);
		pStreams[i] = NULL;
		// Do in reverse order, getting all fcc types
		hr=AVIFileGetStream(pFile,&pStreams[i],0L,i) ;
		if(hr != AVIERR_OK || pStreams[i] == NULL) {
			errMsg("Cannot open stream %d",i); 
			continue; 
		}
		// Get stream info
		hr=AVIStreamInfo(pStreams[i],&streamInfo,sizeof(streamInfo)); 
		if(hr != AVIERR_OK) { 
			errMsg("Unable to get stream info for stream %d",i); 
			goto ABORT; 
		}
		printStreamInfo(streamInfo);

		// Handle specific types
		if (streamInfo.fccType == streamtypeVIDEO) { 
			getBitmapInfo(pStreams[i]);
		} else if (streamInfo.fccType == streamtypeAUDIO) { 
			continue;
		}  else if (streamInfo.fccType == streamtypeMIDI) {  
			continue;
		}  else if (streamInfo.fccType == streamtypeTEXT) {  
			continue;
		} else {
			continue;
		}
		printf("\n");
	}
	goto END;

ABORT:
	// Abnormal exit
	if(out != NULL) {
		printf("\nAborted\n");
		out = freopen("CON", "w", stdout);
		printf("Output file is: %s", fileName);
	}
	printf("\nAborted\n");
	goto CLEANUP;

END:
	// Normal exit
	if(out != NULL) {
		printf("\nAll done\n");
		out = freopen("CON", "w", stdout);
		printf("Output file is: %s", fileName);
	}
	printf("\nAll done\n");

CLEANUP:
	// Close file
	if(pFile) AVIFileRelease(pFile);
	// Release AVIFile library 
	if(libOpened) AVIFileExit();
	// Free space
	if(pStreams) delete [] pStreams;
	if(fileName) delete [] fileName;

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
		case 'f':
			useFile = 1;
			break;
		default:
			fprintf(stderr,"\n\nInvalid option: %s\n",argv[i]);
			usage();
			return APP_ERROR;
			}
		} else {
			if(!aviFileSpecified) {
				strcpy(aviFileName,argv[i]);
				aviFileSpecified=1;
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
		"\nUsage: AVIDump [Options] filename\n"
		"  Prints AVI file info\n"
		"\n"
		"  Options:\n"
		"    filename     Name of an AVI file\n"
		"    -h help      This message\n"
		);
}

void getBitmapInfo(PAVISTREAM pStream) {
	printf("\n  BITMAPINFOHEADER:\n");
	if(!pStream) {
		errMsg("PAVISTREAM input is null"); 
		return; 
	}

	HRESULT hr;
	HIC hic;
	BITMAPINFO *pBmi = new BITMAPINFO();;
	long biSize = sizeof(*pBmi);
	ZeroMemory(pBmi, biSize);
	// First determine biSize
	// Not really necessary since we allocated a full BITMAPINFO anyway
	// The returned size should be less than or equal to the size of BITMAPINFO
	hr = AVIStreamReadFormat(pStream, 0, NULL, &biSize);
#if DEBUG
	printf("DEBUG: biSize=%d *pBmi=%d\n", biSize, sizeof(*pBmi));
	printf("DEBUG: WORD=%d DWORD=%d LONG=%d\n",
		sizeof(WORD), sizeof(DWORD), sizeof(LONG));
	printf("DEBUG: BITMAPINFO=%d BITMAPINFOHEADER=%d RGBTRIPLE=%d\n",
		sizeof(BITMAPINFO), sizeof(BITMAPINFOHEADER), sizeof(RGBTRIPLE));
#endif
	// Then actually read it
	hr = AVIStreamReadFormat(pStream, 0, pBmi, &biSize);
	if(hr != AVIERR_OK) {
		errMsg("Cannot get BITMAPINFO [Error 0x%08x %s]", hr, getErrorCode(hr)); 
		return; 
	}
	BITMAPINFOHEADER header = pBmi->bmiHeader;
	printBmiHeaderInfo("    ", header);

	// Get decompressor information
	DWORD biCompression = header.biCompression;
	printf("    Decompressor Information for compression: ");
	printFourCcCode(biCompression, "\n");

#if 1
	printAvailableDecompressors(pBmi);
#endif

#if 1
	hic = ICLocate(ICTYPE_VIDEO, 0L, &header, NULL, ICMODE_DECOMPRESS); 
	if(hic) {
		printf("      Can decompress\n");
		ICClose(hic); 
	} else {
		printf("      Cannot decompress\n");
	}
#endif

#if 1
	hic = ICOpen(ICTYPE_VIDEO, biCompression, ICMODE_DECOMPRESS);
	if(hic) {
		printf("      ICOpen for ICMODE_DECOMPRESS successful\n");
		ICClose(hic); 
	} else {
		printf("      ICOpen for ICMODE_DECOMPRESS unsuccessful\n");
	}
#endif

#if 1
	hic = ICDecompressOpen(ICTYPE_VIDEO, biCompression, &header, NULL);
	if(hic) {
		printf("      ICDecompressOpen for ICMODE_DECOMPRESS successful\n");
		ICClose(hic); 
	} else {
		printf("      ICDecompressOpen for ICMODE_DECOMPRESS unsuccessful\n");
	}
#endif

#if 1
	BITMAPINFOHEADER headerOut;
	memcpy(&headerOut, &header, sizeof(headerOut));
	headerOut.biCompression = mmioFOURCC('c','v','i','d');
	hic = ICDecompressOpen(ICTYPE_VIDEO, biCompression, &header, &headerOut);
	if(hic) {
		printf("      ICDecompressOpen for ICMODE_DECOMPRESS "
			"to cvid successful\n");
		ICClose(hic); 
	} else {
		printf("      ICDecompressOpen for ICMODE_DECOMPRESS "
			"to cvid unsuccessful\n");
	}
#endif

#if 1
	BITMAPINFO bmiOut;
	DWORD res = ICDecompressGetFormat(hic, pBmi, &bmiOut);
	if(res == ICERR_OK) { 
		printf("      ICDecompressGetFormat failed ");
	}

	hic = ICOpen(ICTYPE_VIDEO, biCompression, ICMODE_QUERY); 
	if (hic) {
		DWORD res = ICDecompressQuery(hic, pBmi, NULL);
		if(res != ICERR_OK) { 
			printf("      Cannot handle ");
			printFourCcCode(biCompression, "\n");
		} else {
			printf("      Can handle ");
			printFourCcCode(biCompression, "\n");
			// Get the size of the default format
			BITMAPINFO bmiOut;
			// Get the default
			DWORD res = ICDecompressGetFormat(hic, pBmi, &bmiOut);
			printf("        ");
			printFourCcCode(biCompression, ": "); 
			if(res != ICERR_OK) {
				printf("Default output format not available\n");
			} else {
				printf("Default output format:\n");
				printBmiHeaderInfo("          ", bmiOut.bmiHeader);
			}
		}

		// Try cvid in the default format
		if(res == ICERR_OK) {
			BITMAPINFO bmiOut2;
			memcpy(&bmiOut2, &bmiOut, sizeof(bmiOut2));
			DWORD newCompression = mmioFOURCC('c','v','i','d');
			bmiOut.bmiHeader.biCompression = newCompression;
			DWORD res = ICDecompressGetFormat(hic, &bmiOut, &bmiOut2);
			printf("        ");
			printFourCcCode(biCompression, ": "); 
			if(res != ICERR_OK) {
				printf("cvid2 output format not available\n");
			} else {
				printf("cvid2 output format:\n");
				printBmiHeaderInfo("          ", bmiOut2.bmiHeader);
			}
		}

		// Cleanup
		ICClose(hic); 
	}
#endif

	// Cleanup
	if(pBmi) delete pBmi;
}