// ConvertVideo.cpp : Defines the convert video method.

#include "stdafx.h"

// Test using the AVISTreamGet methods that get a DIB
#define DO_GET 1

// Do the decompression or skip it
#define DO_DECOMPRESS 1

// Get the stream from the pFile2 argument if true
// Otherwise use the pStream2 stream input
#define USE_FILE 1

// Use PBITMAPINFOHEADER(AVIGETFRAMEF_BESTDISPLAYFMT) for AVIStreamGetFrameOpen
// Overrides other output formats
#define USE_BEST 0

// Use YUY2 as the output format for AVIStreamGetFrameOpen
#define USE_YUY2 0
// Use current test as the output format for AVIStreamGetFrameOpen
#define USE_TEST 1

// Function prototypes
BOOL writeDIB(int i, LPTSTR szFileName, HANDLE hDIB);
BOOL writeDIB(LPTSTR szFileName, HANDLE hDIB);

HRESULT decompressVideo(PAVIFILE pFile2, PAVISTREAM pStream1,PAVISTREAM pStream2) {
	if(!pStream1) {
		errMsg("decompressVideo: stream 1 is null"); 
		return AVIERR_BADPARAM; 
	}
	//if(!pStream2) {
	//	errMsg("decompressVideo: stream 2 is null"); 
	//	return AVIERR_BADPARAM; 
	//}

	HRESULT hrReturnVal = AVIERR_OK;
	HRESULT hr;
	DWORD res;
	HIC hic = NULL;
	BITMAPINFO bmi1, bmi2;
	BITMAPINFOHEADER bmih1, bmih2;
	char *pBuf1 = NULL;
	char *pBuf2 = NULL;

	long biSize = sizeof(bmi1);
	ZeroMemory(&bmi1, biSize);
	hr = AVIStreamReadFormat(pStream1, 0, &bmi1, &biSize);
	if(hr != AVIERR_OK) {
		errMsg("Cannot get BITMAPINFO [Error 0x%08x %s]", hr, getErrorCode(hr)); 
		return hr; 
	}

	// Get the stream info from the input stream
	AVISTREAMINFO streamInfo1;
	ZeroMemory(&streamInfo1, sizeof(streamInfo1));
	hr=AVIStreamInfo(pStream1,&streamInfo1,sizeof(streamInfo1)); 
	if(hr != AVIERR_OK) { 
		errMsg("Cannot read stream info 1 [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		return hr; 
	}

	// Determine compression
	bmih1 = bmi1.bmiHeader;
	printBmiHeaderInfo("    ", bmih1);
	DWORD biCompression = bmih1.biCompression;
	printf("  Compression: ");
	printFourCcCode(biCompression, "\n");

	// Open the decompressor
	hic = ICDecompressOpen(ICTYPE_VIDEO, biCompression, &bmih1, NULL);
	if(hic) {
		printf("  ICDecompressOpen successful\n");
	} else {
		printf("  ICDecompressOpen unsuccessful\n");
		goto CLEANUP;
	}

	// Get the output format
	res = ICDecompressGetFormat(hic, &bmi1, &bmi2);
	if(res != ICERR_OK) {
		printf("Default output format not available\n");
		goto CLEANUP;
	}
	bmih2 = bmi2.bmiHeader;
	printf("  Default output format:\n");
	printBmiHeaderInfo("    ", bmih2);

#if USE_YUY2 
	bmih2.biCompression = mmioFOURCC('Y','U','Y','2');
	bmih2.biBitCount = 16;
	bmih2.biSizeImage =
		bmih2.biWidth * bmih2.biHeight * bmih2.biBitCount / 8;
	printf("  New output format:\n");
	printBmiHeaderInfo("    ", bmih2);
#elif USE_TEST
	bmih2.biSize = sizeof(bmih2);
	bmih2.biWidth = streamInfo1.rcFrame.right - streamInfo1.rcFrame.left;
	bmih2.biHeight = streamInfo1.rcFrame.bottom - streamInfo1.rcFrame.top;
	bmih2.biPlanes = 1;
	bmih2.biBitCount = 24;
	bmih2.biCompression = BI_RGB;
	bmih2.biSizeImage = 0;
	bmih2.biXPelsPerMeter = 0;
	bmih2.biYPelsPerMeter = 0;
	bmih2.biClrUsed = 0;
	bmih2.biClrImportant  = 0;
#endif

#if USE_FILE
	// Create the output stream
	hr = AVIFileCreateStream(pFile2, &pStream2, &streamInfo1);
	if(hr != AVIERR_OK) {
		errMsg("Cannot create stream [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		hrReturnVal = hr;
		goto CLEANUP;
	}

	// Set the format
	hr = AVIStreamSetFormat(pStream2, 0, &bmi2, sizeof(bmi2));
	if(hr != AVIERR_OK) {
		errMsg("Cannot set format [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		hrReturnVal = hr;
		goto CLEANUP;
	}
#endif

#if 1
	// Get the buffer sizes for the input
	LONG sizeMin;
	LONG sizeMax;
	LONG nSizeErrors;
	getBufferSizes(pStream1, &sizeMin, &sizeMax, &nSizeErrors);
	printf("  Required buffer sizes for input stream\n");
	printf("  dwSuggestedBufferSize=%d sizeMin=%d sizeMax=%d nSizeErrors=%d\n",
		streamInfo1.dwSuggestedBufferSize, sizeMin, sizeMax, nSizeErrors);
#endif

#if DO_GET
	// Try getting the frames as bitmaps
	printf("  Getting frames as DIB\n");
	int nGetProcessed = 0;
	int nGetOK = 0;
	int nGetErrors = 0;
	int	maxGetErrors = 10;

	// Calculate index of frame near 1 minute.
	int minuteFrame = (int)((double)60. * streamInfo1.dwRate /
		(double)streamInfo1.dwScale);
	// Use NULL to use as is, otherwise specify a LPBITMAPINFOHEADER
#if USE_BEST
	PGETFRAME pgf = AVIStreamGetFrameOpen(pStream1,
		PBITMAPINFOHEADER(AVIGETFRAMEF_BESTDISPLAYFMT));
#else
	PGETFRAME pgf = AVIStreamGetFrameOpen(pStream1, &bmih2);
#endif
	if(pgf == NULL) {
		errMsg("AVIStreamGetFrameOpen failed\n");
		goto END_GET;
	}

	LPVOID ptr = NULL;
	for(LONG i = AVIStreamStart(pStream1); i < AVIStreamEnd(pStream1); i++) {
		nGetProcessed++;
		ptr = AVIStreamGetFrame(pgf, i);
		if(ptr == NULL) {
			if(++nGetErrors < maxGetErrors) {
				errMsg("AVIStreamGetFrame failed at frame %d",i);
			}
			continue;
		}
		if(i == minuteFrame) {
			printf("Saving DIB for frame %d\n", i);
			writeDIB(i, streamInfo1.szName, ptr);
		}
		nGetOK++;
	}

	// Close the pointer
	hr = AVIStreamGetFrameClose(pgf);
	if(hr != AVIERR_OK) { 
		errMsg("AVIStreamClose failed [Error 0x%08x %s]",
			hr, getErrorCode(hr)); 
		goto END_GET;
	}

END_GET:
	printf("  %d get frames processed, %d OK, %d errors\n\n",
		nGetProcessed, nGetOK, nGetErrors);
	//ULONG releaseCount1 = AVIStreamRelease(pStream1);
	//printf("\n  After Stream AVIStreamRelease: release count 1 is %d\n",
	//	releaseCount1);
#endif

#if DO_DECOMPRESS
	// Loop over the frames
	printf("  Processing %d frames starting at frame %d...\n",
		streamInfo1.dwLength, AVIStreamStart(pStream1));
	// The input buffer will be allocated with a determined size for each frame
	LONG bufSize1 = 0;
	// The output buffer is the size of the image
	LONG bufSize2 = bmih2.biSizeImage;
	pBuf2 = new char[bufSize2];
	LONG nFrames = AVIStreamStart(pStream1);
	// Set the start pBuf1
	DWORD nKeyFrames = 0;
	DWORD nFramesProcessed = 0;
	DWORD maxErrors = 10;
	DWORD nErrors = 0;
	LONG frameOut = AVIStreamStart(pStream1);
	for(LONG i = AVIStreamStart(pStream1); i < AVIStreamEnd(pStream1); i++) {
		// Find bufSize1
		hr = AVIStreamRead(pStream1, i, 1, NULL, 0, &bufSize1, NULL);
		if(hr != AVIERR_OK && hr != AVIERR_ERROR) {
			if(++nErrors < maxErrors) {
				errMsg("Error reading stream size at frame %d [Error 0x%08x %s]",
					i, hr, getErrorCode(hr)); 
			}
			goto ABORT_FRAME;
		}
		// Allocate space
		pBuf1 = new char[bufSize1];
#if 0
		if(bufSize1 != streamInfo1.dwSuggestedBufferSize) {
			printf("Non-sandard frame size %d at frame %d\n", bufSize1, i);
		}
#endif

		// Get the frame
		hr = AVIStreamRead(pStream1, i, 1, pBuf1, bufSize1, NULL, NULL);
		if(hr != AVIERR_OK) {
			if(++nErrors < maxErrors) {
				errMsg("Error reading stream at frame %d [Error 0x%08x %s]",
					i, hr, getErrorCode(hr)); 
			}
			goto ABORT_FRAME;
		}

		// Write to the output stream
		if(AVIStreamIsKeyFrame(pStream1, i)) {
			nKeyFrames++;
			hr = AVIStreamWrite(pStream2, frameOut, 1, pBuf1, bufSize1,
				AVIIF_KEYFRAME, NULL, NULL);
		} else  {
			hr = AVIStreamWrite(pStream2, frameOut, 1, pBuf1, bufSize1,
				0, NULL, NULL);
		}
		if(hr != AVIERR_OK) {
			if(++nErrors < maxErrors) {
				errMsg("Error writing stream at frame %d [Error 0x%08x %s]",
					i, hr, getErrorCode(hr)); 
			}
			goto ABORT_FRAME;
		}

		frameOut++;

ABORT_FRAME:
		delete [] pBuf1;
		pBuf1 = NULL;
		nFrames++;
		nFramesProcessed++;
	}

	//ERRORS:
	//	printf("More than %d errors, aborting\n", maxErrors);
	//	hrReturnVal = hr;

	printf("\n");
	printf("  AVIStreamStart=%d AVIStreamEnd=%d\n",
		AVIStreamStart(pStream1), AVIStreamEnd(pStream1));
	printf("  dwStart=%d dwLength=%d\n",
		streamInfo1.dwStart, streamInfo1.dwLength);
	printf("  %d frames processed, %d frames written, %d key frames\n",
		nFramesProcessed, frameOut - AVIStreamStart(pStream1), nKeyFrames);
	printf("  %d frame errors\n", nErrors);
#endif

CLEANUP:
#if USE_FILE
	if(pStream2) {
		ULONG releaseCount = AVIStreamRelease(pStream2);
#  if 1
		printf("\n  After Stream AVIStreamRelease: release count is %d",
			releaseCount);
#  endif
	}
#endif

	if(hic) ICClose(hic);
	hic = NULL;
	if(pBuf1) delete [] pBuf1;
	pBuf1 = NULL;
	if(pBuf2) delete [] pBuf2;
	pBuf2 = NULL;

	return AVIERR_OK;
}

BOOL writeDIB(int i, LPTSTR szFileName, HANDLE hDIB) {
	if (!szFileName || strlen(szFileName) == 0 || !hDIB) {
		return FALSE;
	}
	int len = strlen(szFileName) + 1 + 8 + 4 + 1;
	char *fileName = new char[len];
	sprintf(fileName, "%s-%08d.bmp", szFileName, i);
	int outLen = strlen(fileName);
	if(outLen > len - 1) {
		errMsg("Bad length [%d], expected %d)", outLen, len - 1);
		return FALSE;
	}
	printf("Writing %s\n", fileName);
	return writeDIB(fileName, hDIB);
}

// WriteDIB		- Writes a DIB to file
// Returns		- TRUE on success
// szFile		- Name of file to write to
// hDIB			- Handle of the DIB
BOOL writeDIB(LPTSTR szFileName, HANDLE hDIB) {
	if (!szFileName || strlen(szFileName) == 0 || !hDIB) {
		return FALSE;
	}
	FILE *file = fopen(szFileName, "wb");
	if(!file) {
		return FALSE;
	}

	LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)hDIB;
	int nColors = 1 << lpbi->biBitCount;

	// Fill in the fields of the file header 
	BITMAPFILEHEADER hdr;
	ZeroMemory(&hdr, sizeof(hdr));
	hdr.bfType		= ((WORD) ('M' << 8) | 'B');	// is always "BM"
	hdr.bfSize		= GlobalSize (hDIB) + sizeof( hdr );
	hdr.bfReserved1 	= 0;
	hdr.bfReserved2 	= 0;
	hdr.bfOffBits		= (DWORD) (sizeof( hdr ) + lpbi->biSize +
		nColors * sizeof(RGBQUAD));

	// Write the file header 
	fwrite(&hdr, sizeof(hdr), 1, file);

	// Write the DIB header and the bits 
	fwrite(lpbi, GlobalSize(hDIB), 1, file);

	// Close the file
	fclose(file);

	return TRUE;
}
