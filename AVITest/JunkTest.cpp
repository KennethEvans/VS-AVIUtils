// JunkTest.cpp : implementation file

CRect and CSize were replaced by SIZE and RECT

#include "stdafx.h"

#define OUT_FILE_NAME _T("junktest.avi")

void JunkTest() {
	HRESULT hr;
	PAVIFILE pFile;
	PAVISTREAM pVideoStream;

	SIZE sz;
	sz.cx = 320;
	sz.cy = 240;
	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect. right = sz.cx;
	rect.bottom = sz.cy;
	int   nFrames  = 30;

	AVISTREAMINFO vidinfo;
	memset( &vidinfo, 0, sizeof(vidinfo) );
	vidinfo.fccType      = streamtypeVIDEO;
	vidinfo.fccHandler   = MKFOURCC('M','S','V','C');
	vidinfo.dwRate       = 15;
	vidinfo.dwScale      = 1;
	vidinfo.rcFrame      = rect;
	vidinfo.dwLength     = nFrames;

	BITMAPINFOHEADER bi;
	bi.biSize            = 40;
	bi.biWidth           = sz.cx;
	bi.biHeight          = sz.cy;
	bi.biPlanes          = 1;
	bi.biBitCount        = 16;
	bi.biCompression     = MKFOURCC('M','S','V','C');
	bi.biSizeImage       = 0;  // differs per frame, in general, so setting to 0
	bi.biXPelsPerMeter   = 0;
	bi.biYPelsPerMeter   = 0;
	bi.biClrUsed         = 0;
	bi.biClrImportant    = 0;

	AVIFileInit();
	hr = AVIFileOpen( &pFile, OUT_FILE_NAME, OF_WRITE | OF_CREATE, NULL );
	if(hr != AVIERR_OK) {
		printf("AVIFileOpen failed\n");
	}
	hr = AVIFileCreateStream( pFile, &pVideoStream, &vidinfo );
	if(hr != AVIERR_OK) {
		printf("AVIFileCreateStream failed\n");
	}
	hr = AVIStreamSetFormat( pVideoStream, 0, &bi, sizeof(bi) );
	if(hr != AVIERR_OK) {
		printf("AVIStreamSetFormat failed\n");
	}

	int nFrame = 0;
	for ( int i = 0; i < 30; i++ ) {
		BYTE* pFrame = new BYTE[sz.cx*sz.cy*3];
		int nIdx = 0;
		int x, y;
		for (y = 0; y < sz.cy/4; y++)
			for (x = 0; x < sz.cx/4; x++) {
				// draw one solid color 4x4 block
				pFrame[nIdx++] = 0;
				pFrame[nIdx++] = 0;
				pFrame[nIdx++] = 0;
				pFrame[nIdx++] = 0;
				*((SHORT*)(pFrame+nIdx)) = ((x-i)^y) + (i<<10);
				nIdx += 2;
			}
			pFrame[nIdx++] = 0;  // end of blocks
			pFrame[nIdx++] = 0;
			hr = AVIStreamWrite( pVideoStream, nFrame++, 1, pFrame, nIdx,
				AVIIF_KEYFRAME, NULL, NULL ); // all keyframes
			if(hr != AVIERR_OK) {
				printf("AVIStreamWrite failed for y=%d, x=%d\n", y, x);
			}
			delete [] pFrame;
	}

	AVIStreamRelease( pVideoStream ); 
	AVIFileRelease( pFile );
	AVIFileExit();

	printf("Wrote %s\n", OUT_FILE_NAME);
	printf("All done\n");
}
