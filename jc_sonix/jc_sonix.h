
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "stdlib.h"
#include "iostream" 
//#include <sys/types.h>
#include "sys/stat.h"



// declarations
bool initDAQ(int activeDAQ, const char *fw);
bool acquireDAQ(int activeDAQ, const daqRaylinePrms &rlprms, const daqSequencePrms &seqprms, int numChannls, int verbose, const char *m_dataPath, const char*m_tempPath, int numTiggers, int sumTriggers, int remote, int remoteCount, const char *remotePath);
bool reshapeData(const char *path, const char *tempPath, int numTriggers, int numPoints, int numChannels, int sumTriggers, int blockNum, int x, int y, int z, int u);
bool stopDAQ(int activeDAQ);
bool listDAQs();
bool helpDAQ();