
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "stdlib.h"
#include "iostream" 
//#include <sys/types.h>
#include "sys/stat.h"
#include "windows.h"

#include <ctime>

#define printv(x, ...) if(verbose==1) printf(x, __VA_ARGS__);


// declarations
bool initDAQ(int activeDAQ, const char *fw);
bool acquirenDAQ(int n, int activeDAQ, const daqRaylinePrms &rlprms, const daqSequencePrms &seqprms, int numChannls, int verbose, const char *m_dataPath, const char*m_tempPath, int numTiggers, int sumTriggers, int remote, int remoteCount, const char *remotePath,int memMode);
bool acquireDAQ(int activeDAQ, const daqRaylinePrms &rlprms, const daqSequencePrms &seqprms, int numChannls, int verbose, const char *m_dataPath, const char*m_tempPath, int numTiggers, int sumTriggers, int remote, int remoteCount, const char *remotePath,int blockNum,int memMode);
bool reshapeData(const char *path, const char *tempPath, int numTriggers, int numPoints, int numChannels, int sumTriggers, int blockNum, int x, int y, int z, int u);
//bool writeBlock(int *_buffer, const char *path, const char *tempPath, int numTriggers, int numPoints, int numChannels, int sumTriggers, int blockNum, int x, int y, int z, int u);
//bool writeBlock(short int *_buffer, const char *path, const char *tempPath, int numTriggers, int numPoints, int numChannels, int sumTriggers, int blockNum, int x, int y, int z, int u);
bool stopDAQ(int activeDAQ);
bool listDAQs();
bool helpDAQ();

template <class bufferType> bool writeBlock(const char *path, int numTriggers, int numPoints, int numChannels, int sumTriggers, int blockNum, int x, int y, int z, int u)
{
		char fullPath[256]="";
    FILE *fid = NULL;


	// write output buffer to disk
	sprintf(fullPath, "%s\\block%05d.DAQx", path, blockNum);
	fid = fopen(fullPath, "wb");
	if(fid == NULL)
	{
	    printf("ERROR: Could not open path = %s\n", fullPath);
		return false;
	}

		printf("Writing the header of blockfile...\n");


	fwrite(&blockNum, sizeof(int), 1, fid );
	fwrite(&sumTriggers, sizeof(int), 1, fid );
	fwrite(&numTriggers, sizeof(int), 1, fid );
	fwrite(&numChannels, sizeof(int), 1, fid );
	fwrite(&numPoints, sizeof(int), 1, fid );
	fwrite(&x, sizeof(int), 1, fid );  // robot position in um
	fwrite(&y, sizeof(int), 1, fid );
	fwrite(&z, sizeof(int), 1, fid );
	fwrite(&u, sizeof(int), 1, fid );  // robot angle in millidegrees
  

	fclose(fid);


}


template <class bufferType> bool writeBlock(const char *path, bufferType *_buffer, int numTriggers, int numPoints, int numChannels, int sumTriggers, int blockNum, int x, int y, int z, int u)
{
		char fullPath[256]="";
    FILE *fid = NULL;


	// write output buffer to disk
	sprintf(fullPath, "%s\\block%05d.DAQx", path, blockNum);
	fid = fopen(fullPath, "ab");
	if(fid == NULL)
	{
	    printf("ERROR: Could not open path = %s\n", fullPath);
		return false;
	}

	//writing data...
	if(sumTriggers == 1)
	{
	fwrite(_buffer, sizeof(bufferType)*numPoints, numChannels, fid );
	}
	else
	{
	fwrite(_buffer, sizeof(bufferType)*numPoints*numTriggers, numChannels, fid );
	}


	fclose(fid);


}