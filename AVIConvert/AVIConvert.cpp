// AVIConvert.cpp : Converts AVI files stream by stream

#include "stdafx.h"

#define APP_OK 0
#define APP_ERROR 1

#define PATH_MAX _MAX_PATH

// Function prototypes
int CopyImplementation();
int SaveImplementation();
int parseCommand(int argc, char **argv);
void usage(void);

// Global variables
char aviFileName1[PATH_MAX];
char aviFileName2[PATH_MAX];
int aviFileSpecified1 = 0;
int aviFileSpecified2 = 0;

enum METHOD {COPY, SAVE};
METHOD method = SAVE;

int _tmain(int argc, _TCHAR* argv[]) {
	// Title
	printf("\nAVIConvert\n");

	// Parse command line
	if(parseCommand(argc,argv) != APP_OK) {
		return 1;
	}
	if(!aviFileSpecified1) {
		errMsg("No AVI source file specified\n");
		return 1;
	}
	if(!aviFileSpecified2) {
		errMsg("No AVI destination file specified\n");
		return 1;
	}

	// Use the specified method
	if(method == COPY) {
		printf("Using Copy method\n");
		printf("\nSource: %s\n",aviFileName1);
		printf("Dest: %s\n",aviFileName2);
		return CopyImplementation();
	} else if(method == SAVE) {
		printf("Using Save method\n");
		printf("\nSource: %s\n",aviFileName1);
		printf("Dest: %s\n",aviFileName2);
		return SaveImplementation();
	}
}

int parseCommand(int argc, char **argv) {
	for(int i=1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch(argv[i][1]) {
			case 'c':
				method = COPY;
			case 's':
				method = SAVE;
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
		"    -h           This message\n"
		"    -c           Use copy method\n"
		"    -s           Use save method method\n"
		"    -h help      This message\n"
		"    srcfilename  Name of an AVI file to convert\n"
		"    destfilename Name of converted file\n"
		);
}
