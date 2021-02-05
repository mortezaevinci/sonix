// jc_sonix.cpp : main project file.

// things needed to do in order to get this compile under visual studio 2010
// 1. create a console project (not win32)
// 2. add to properties - c/c++ - general - additional include directories : C:/sonix/sdk_6.0.3_(00.036.203)/daq/inc
// 3. add to properties - linker - input - additional dependencies: C:\sonix\sdk_6.0.3_(00.036.203)\daq\lib\daq.lib
// 4. copy /fw folder from the sdk daq folder to project folder
// 5. copy daq.dll from sdk daq/bin folder to the project executable folder, e.g. /release or /debug
// 6. PDB ERRORS
// for pdb errors in the output windows, thanks http://social.msdn.microsoft.com/Forums/uk/vsdebug/thread/f2d31010-dc89-4290-8869-11bf4a037ea0
// Those perticular pdb's ( for ntdll.dll, mscoree.dll, kernel32.dll, etc ) are for the windows API and shouldn't be needed for simple apps.
// However, if you cannot find pdb's for your own compiled projects, I suggest making sure the Project Properties > Configuration Properties > Debugging > Working Directory uses the value from Project Properties > Configuration Properties > General > Output Directory .


// Seems sith the sdk, one first connects to a single device, then initializes it, does downloading of data, disconnect, then goes to next device
//
#include "stdafx.h"
#include "jc_sonix.h"

using namespace System;
using namespace std; 


// help functions
int doesFileExist(char * fileName);
int removeTempFiles(const char *tempPath);

// int argc, int** argv) // 
int main(array<System::String ^> ^args)
{
	printf("realDAQ version .66\n\n");
	printf("");

    int  i = 0, retVal = 0;
    char m_firmwarePath[256] = DAQ_FIRMWARE_PATH;
    const char m_dataPath[256] = "";
    const char m_tempPath[256] = "";
    const char remotePath[256] = "";
    int  m_channels = 128;
	int  verbose = 1, remote = 0, remoteCount = 0;

	// acquisition setup
	unsigned int channelsDAQ[4] = {0,0,0,0};
    daqRaylinePrms rlprms;
    daqSequencePrms seqprms;
    bool sampling = SAMPLING;
    int const numChannls = 128;
	int sumTriggers = 0;  // if 0 output triggered data in stacked format, 1 = add triggered data before saving

    // sampling and decimation
    if (sampling)
    {
        rlprms.sampling = 80;   // DAQ sampling frequency 80 -> 80 [MHz]
        rlprms.decimation = 0;  // no decimation for 80MHz sampling
    }
    else
    {
        rlprms.sampling = 40;   // DAQ sampling frequency 40 -> 40 [MHz]
        rlprms.decimation = 0;  // Fs = sampling / (1+decimation) e.g. decimation = 1 -> Fs=20 MHz
    }

	// turn on all channels
    channelsDAQ[0] = 0xffffffff;
    channelsDAQ[1] = 0xffffffff;
    channelsDAQ[2] = 0xffffffff;
    channelsDAQ[3] = 0xffffffff;

    rlprms.channels = channelsDAQ;
    rlprms.gainDelay = 2;
    rlprms.gainOffset = 0;
    rlprms.rxDelay = 6;
    rlprms.lineDuration = 100;  // line duration in micro seconds
    rlprms.numSamples   = 2000 / (1 + rlprms.decimation); // assuming 3000 samples at 40MHz

    seqprms.freeRun = 0;
    seqprms.hpfBypass = 0;
    seqprms.divisor = 11;           // data size = 16GB / 2^divisor
    seqprms.externalTrigger = 0;
    seqprms.externalClock = 0;  // set to true if external clock is provided

	// lnaGain: LNA gain [0:1:2] corresponds to [16dB, 18dB, 21dB].
    // pgaGain: PGA gain [0:1:3] corresponds to [21dB, 24dB, 27dB, 30dB].
	// biasCurrent: switch gain [0:1:7] where 0 completely turns off the switch
    seqprms.lnaGain = 0;            // 16dB, 18dB, 21dB
    seqprms.pgaGain = 0;            // 21dB, 24dB, 27dB, 30dB
    seqprms.biasCurrent = 1;        // 0,1,2,...,7

	// fixedTGC: defines whether the DAQ should use flat TGC or adjustable TGC.
	// fixedTGCLevel: [0:1:100] fixed TGC value. This value is only used if fixedTGC parameter is set to be true.
	// TGCcurve: If fixedTGC is set to be false, a TGC curve needs to be defined.
	// Currently, this curve is defines by 3 points with X, Y values ranging from 0 to 1,
	// where 1 corresponds to maximum value of TGC for Y and maximum acquisition depth for X.
    seqprms.fixedTGC = 1;
    if (seqprms.fixedTGC == 1)
    {
        seqprms.fixedTGCLevel = 0;
    }
    else
    {
        // set TGC curve
        daqTgcSetX(2, 0.75f);
        daqTgcSetX(1, 0.5f);
        daqTgcSetX(0, 0.25f);

        daqTgcSetY(2, 0.75f);
        daqTgcSetY(1, 0.5f);
        daqTgcSetY(0, 0.25f);
    }

	// run command from command line
	// http://msdn.microsoft.com/en-us/library/e3awe53k.aspx

    String^ delimStr = "=";
    array<Char>^ delimiter = delimStr->ToCharArray( );
    array<String^>^ words;
	int v = 0;
	for each (String ^s in args)
	{
		words = s->Split( delimiter );
		if(words[0]->CompareTo(L"help") == 0)
		{
			helpDAQ();
		}
		if(words[0]->CompareTo(L"list") == 0)
		{
			listDAQs();
		}
		if(words[0]->CompareTo(L"init") == 0)
		{
			v = Int32::Parse(words[1]);
			initDAQ(v, m_firmwarePath);
		}
		if(words[0]->CompareTo(L"remote") == 0)
		{
			v = Int32::Parse(words[1]);
			remote = v;
		}
		if(words[0]->CompareTo(L"acquire") == 0)
		{
			// NOTES:
			// if only a single trigger is used then, file size appears as 12bytes or 1k to windows even though correct size on disk
			// eg 2048 pts and divisor of 15 -> daq returns after 1 trigger, but files are corrupt at 1k
			// eg for divsor of 14, and 2048 samples -> windows reports 1/2 size of actual file after 2 triggers.
			// cannot use the 2^x as a number of points
			//
			// if the numPoints*numchannels*2 is greater than n-7, where n = numchannels*numPoints*2, then file size fails

			// test arguments
			double div = (double)pow(2, (double)seqprms.divisor);
			double bufferSize = (double)4*(double)4294967296/div;
			int frameSize = rlprms.numSamples * numChannls * 2;
			double numFrames = bufferSize/(double)frameSize;
			int numTriggers = (int)numFrames;
			if((double)numTriggers < numFrames)
			{
				//numTriggers += 1;
			}
            printf("bufferSize = %g\n", bufferSize);
            printf("frameSize = %d\n", frameSize);
            printf("numTriggers (computed) = %g\n", numFrames);
            printf("numTriggers (used) = %d\n", numTriggers);
			// ensure line duration is properly set
			float lineDuration = (float)rlprms.numSamples / (float)40;
            printf("lineDuration (expected) = %g\n", lineDuration);
			if(lineDuration > (float)rlprms.lineDuration)
			{
				rlprms.lineDuration = (int)(lineDuration) + (int)1;
		        printf("lineDuration (fixed) = %g\n", (float)rlprms.lineDuration);
			}
			else
			{
		        printf("lineDuration (ok) = %g\n", (float)rlprms.lineDuration);
			}
			if(numFrames >= 1)
			{
				v = Int32::Parse(words[1]);
				if(remote == 1)
				{
					for(remoteCount = 1; remoteCount <= 99999; remoteCount++)
					{
						acquireDAQ(v, rlprms, seqprms, numChannls, verbose, m_dataPath, m_tempPath, numTriggers, sumTriggers, remote, remoteCount, remotePath);
					}
				}
				else
				{
					acquireDAQ(v, rlprms, seqprms, numChannls, verbose, m_dataPath, m_tempPath, numTriggers, sumTriggers, remote, remoteCount, remotePath);
				}
			}
		}
		if(words[0]->CompareTo(L"sum") == 0)
		{
			v = Int32::Parse(words[1]);
			sumTriggers = v;
		}
		if(words[0]->CompareTo(L"stop") == 0)
		{
			v = Int32::Parse(words[1]);
			stopDAQ(v);
 		}
		// augment acquisition settings
		if(words[0]->CompareTo(L"lineDuration") == 0)
		{
			v = Int32::Parse(words[1]);
			rlprms.lineDuration = v;  // line duration in micro seconds
		}
		if(words[0]->CompareTo(L"numSamples") == 0)
		{
			v = Int32::Parse(words[1]);
		    rlprms.numSamples = v / (1 + rlprms.decimation);
		}
		if(words[0]->CompareTo(L"externalTrigger") == 0)
		{
			v = Int32::Parse(words[1]);
			seqprms.externalTrigger = v;
		}
		if(words[0]->CompareTo(L"divisor") == 0)             // data size = 16GB / 2^divisor
		{
			v = Int32::Parse(words[1]);
			if(v > 0 && v <= 16)
			{
				seqprms.divisor = v;
			}
		}
		if(words[0]->CompareTo(L"freeRun") == 0)             // data size = 16GB / 2^divisor
		{
			v = Int32::Parse(words[1]);
			seqprms.freeRun = v;
		}
		if(words[0]->CompareTo(L"numFrames") == 0)             // data size = 16GB / 2^divisor
		{
			v = Int32::Parse(words[1]);
			// numFramesRequested = v;
		}
		if(words[0]->CompareTo(L"verbose") == 0)
		{
			v = Int32::Parse(words[1]);
			verbose = v;
		}
		if(words[0]->CompareTo(L"fw") == 0)
		{
			// String ^myString = openFileDialog1->FileName;
			array<Byte> ^chars = System::Text::Encoding::ASCII->GetBytes(words[1]);
			pin_ptr<Byte> charsPointer = &(chars[0]);
			const char *nativeCharsPointer = reinterpret_cast<char *>(static_cast<unsigned char *>(charsPointer));
			// std::String native(nativeCharsPointer, chars->Length);
			strcpy_s((char *)m_firmwarePath, 256, nativeCharsPointer);
			//pin_ptr<const wchar_t> wch = PtrToStringChars(words[1]);
			//strcpy_s(m_firmwarePath, 256, wch);
		}
		if(words[0]->CompareTo(L"data") == 0)
		{
			array<Byte> ^chars = System::Text::Encoding::ASCII->GetBytes(words[1]);
			pin_ptr<Byte> charsPointer = &(chars[0]);
			const char *nativeCharsPointer = reinterpret_cast<char *>(static_cast<unsigned char *>(charsPointer));
			// std::String native(nativeCharsPointer, chars->Length);
			strcpy_s((char *)m_dataPath, 256, nativeCharsPointer);
			//pin_ptr<const wchar_t> wch = PtrToStringChars(words[1]);
			// strcpy_s(m_dataPath, 256, wch);
		}
		if(words[0]->CompareTo(L"tempPath") == 0)
		{
			array<Byte> ^chars = System::Text::Encoding::ASCII->GetBytes(words[1]);
			pin_ptr<Byte> charsPointer = &(chars[0]);
			const char *nativeCharsPointer = reinterpret_cast<char *>(static_cast<unsigned char *>(charsPointer));
			strcpy_s((char *)m_tempPath, 256, nativeCharsPointer);
		}
		if(words[0]->CompareTo(L"remotePath") == 0)
		{
			array<Byte> ^chars = System::Text::Encoding::ASCII->GetBytes(words[1]);
			pin_ptr<Byte> charsPointer = &(chars[0]);
			const char *nativeCharsPointer = reinterpret_cast<char *>(static_cast<unsigned char *>(charsPointer));
			// std::String native(nativeCharsPointer, chars->Length);
			strcpy_s((char *)remotePath, 256, nativeCharsPointer);
			//pin_ptr<const wchar_t> wch = PtrToStringChars(words[1]);
			// strcpy_s(m_dataPath, 256, wch);
		}
	}

    return 0;
}


bool initDAQ(int activeDAQ, const char *fw)
{
    bool sampling = SAMPLING;
	char err[256];
    printf("initializing DAQ...\n");
   
	  if (daqIsInitializing())
    {
		       daqStopInit();
		fprintf(stderr,"DAQ was already initializing. It is being stopped now. \n");
        return false;
    }

	daqSetFirmwarePath(fw);


	
	//daqInit();	
	
    if (!daqDriverInit())
    {
        fprintf(stderr, "DAQ driver not initialized\n");
        return false;
    }
    else
    {
        printf("found DAQ module\n");
    }
   // if (daqIsInitializing())
   // {
   //     daqStopInit();
  //  }

    if (daqIsConnected())
    {

        printf("programming DAQ...\n");

     	   if (!daqInit())
        {
            daqGetLastError(err, 256);
            fprintf(stderr, err);
            return false;
        }
		
        printf("finished programming DAQ module\n");
    }
   

    return true;
}

// capture data
bool acquireDAQ(int activeDAQ, const daqRaylinePrms &rlprms, const daqSequencePrms &seqprms, int numChannls, int verbose, const char*m_dataPath, const char *m_tempPath, int numTriggers, int sumTriggers, int remote, int remoteCount, const char *remotePath)
{
    char err[256];
	int x=0, y=0, z=0, u=0, blockNum=0, arraySize=0;
	char fullRemotePath[256];
	FILE *fid = NULL;
	struct stat * buf;
	int n = 0;

    if (verbose)
    {
        printf("DAQ statistics:\n\n");
        printf("line duration = %d micro sec\n", rlprms.lineDuration);
        printf("number of channels = %d\n", numChannls);
        printf("number of triggers = %d\n", numTriggers);
        printf("samples per channel = %d\n", rlprms.numSamples);
        printf("frame size = %d bytes\n", (rlprms.numSamples * numChannls * 2));
    }

	if(verbose == 1)
	{
		printf("Acquiring Data...\n");
	}

	//  wait for command
	if(remote == 1)
	{
		// wait for flie with remoteCount to appear in the transfer directory
		sprintf(fullRemotePath, "%sac%05d.dat", remotePath, remoteCount);
  	    printf("Waiting for %s\n", fullRemotePath);
		while(1)
		{
			if(doesFileExist(fullRemotePath) == 1)
			{
				// printf("Waiting for %s\n", fullRemotePath);
				fid = fopen(fullRemotePath, "rb");
				if(fid == NULL)
				{
				    printf("ERROR: Could not open path = %s\n", fullRemotePath);
					return false;
				}
				// first byte is the size of the array - a labveiw thing
				n = fread(&arraySize, sizeof(int), 1, fid );
				n = fread(&blockNum, sizeof(int), 1, fid );
				n = fread(&x, sizeof(int), 1, fid );
				n = fread(&y, sizeof(int), 1, fid );
				n = fread(&z, sizeof(int), 1, fid );
				n = fread(&u, sizeof(int), 1, fid );
				fclose(fid);
				printf("Found it: %d, %d, %d, %d, %d\n", blockNum, x, y, z, u);
				break;
			}
		}
	}

	// check that it is connected
	
	daqDriverInit();
	
	daqInit();

	//	 printf("connected? %d   initialized? %d running? %d",daqIsConnected(),daqIsInitialized(),daqIsRunning());

       if (!daqIsConnected())
    {
		if(verbose == 1)
		{
        fprintf(stderr, "Error: The device is not connected!\n");
		}
        return false;
    }

   if (!daqIsInitialized())
    {
		if(verbose == 1)
		{
        fprintf(stderr, "Error: The device is not initialized!\n");
		}
        return false;
    }

   if (daqIsRunning())
    {
		if(verbose == 1)
		{
        fprintf(stderr, "Error: DAQ is already running.!\n");
		}
        return false;
    }

    if (!daqRun(seqprms, rlprms))
    {
		if(verbose == 1)
		{
        printf("Error: Could not upload DAQ sequence\n");
        daqGetLastError(err, 256);
        printf(err);
		}
        return false;
    }

    // tell labview that data has been captured
	if(remote == 1)
	{
		sprintf(fullRemotePath, "%sre%05d.dat", remotePath, remoteCount);
  	    printf("Sending %s\n", fullRemotePath);
		fid = fopen(fullRemotePath, "wb");
		if(fid == NULL)
		{
		    printf("ERROR: Could not open path = %s\n", fullRemotePath);
			return false;
		}
		fwrite(&numTriggers, sizeof(int), 1, fid );
		fclose(fid);
	}

	// wait until done
    printf("Running");
	while(daqIsRunning())
    {
       	if(verbose == 1)
		{
		//	printf(".");
		}
    }
    printf("\n");

	// download data to folder
    if (!daqDownload(m_tempPath))
    {
		if(verbose == 1)
		{
	//		daqGetLastError(err, 256);
	//		fprintf(stderr, err);
		}
  //      return false;
    }

    printf("Downloading");
	while (daqIsDownloading())
    {
       	if(verbose == 1)
		{
			printf(".");
		}
    }
    printf("\n");

	// save data into a nice format, header + data for all channels
    if (reshapeData(m_dataPath, m_tempPath, numTriggers, rlprms.numSamples, numChannls, sumTriggers, blockNum, x, y, z, u))
    {
		if(verbose == 1)
		{

		}
    }

	// remove the temp files
	removeTempFiles(m_tempPath);

	if(verbose == 1)
	{
	    printf("Acquisition Done!\n");
	}

    return true;
}

// load in and reshape data to a nice format
bool reshapeData(const char *path, const char *tempPath, int numTriggers, int numPoints, int numChannels, int sumTriggers, int blockNum, int x, int y, int z, int u)
{
	// load in the first file
	int i = 0, j = 0, k=0;
	char fullPath[256] = "";
	FILE *fid = NULL;
	int n = 0;
	int header[19];
	int adjustedPoints = (numTriggers*numPoints)-32;
	int sum=0;

	// allocate space for the data
	size_t bufferSize = 0, _bufferSize = 0;
	short int *buffer = NULL;
	int *_buffer = NULL;
	bufferSize = (size_t)2*(size_t)numPoints*(size_t)numChannels*(size_t)numTriggers;
	buffer = (short int *)malloc(bufferSize);
	if(buffer == NULL)
		{
		    printf("ERROR: Could allocate the buffer\n");
			return false;
		}
	memset(buffer, 0, bufferSize);

	// output buffer -> int32
	if(sumTriggers == 1)
	{
	_bufferSize = (size_t)4*(size_t)numPoints*(size_t)numChannels;
	}
	else
	{
	_bufferSize = (size_t)4*(size_t)numPoints*(size_t)numChannels*(size_t)numTriggers;
	}
	_buffer = (int *)malloc(_bufferSize);
	if(_buffer == NULL)
		{
		    printf("ERROR: Could allocate the buffer\n");
			return false;
		}
	memset(_buffer, 0, _bufferSize);


	for(i=0; i < numChannels; i++)
	{
		sprintf(fullPath, "%s\\CH%03d.daq", tempPath, i);
		fid = fopen(fullPath, "rb");
		if(fid == NULL)
		{
		    printf("ERROR: Could not open path = %s\n", fullPath);
			return false;
		}
		n = fread( header, sizeof(int), 19, fid );
		if(n != 19)
		{
		    printf("ERROR: header too small for %s\n", fullPath);
			return false;
		}
	/*	for(j=0; j < 19; j++)
		{
			printf("header[%d] = %d\n", j, header[j]);
		}
		*/
		// read in the data
		n = fread( &buffer[i*numPoints*numTriggers], sizeof(short int), adjustedPoints, fid );
		if(n != adjustedPoints)
		{
		    printf("ERROR: buffer[size of %d] read = %d not equal to %d for %s\n", bufferSize, n, adjustedPoints, fullPath);
			//return false;
		}
		// printf("OK: loaded %s\n", fullPath);
		if(sumTriggers == 1)
		{
			// sum up triggered data into first numPoints
			for(j = 0; j < numPoints; j++)
			{
				sum = 0;
				for(k = 0; k < numTriggers; k++)
				{
				sum = sum + (int)buffer[i*numPoints*numTriggers + k*numPoints + j];
				}
				_buffer[j+i*numPoints] = sum;
			}
		}
		else
			// copy triggered data
			for(j = 0; j < numPoints*numTriggers; j++)
			{
			_buffer[j+i*numPoints*numTriggers] = (int)buffer[j+i*numPoints*numTriggers];
			}

		fclose(fid);
	}

	// write output buffer to disk
	sprintf(fullPath, "%s\\block%05d.dat", path, blockNum);
	fid = fopen(fullPath, "wb");
	if(fid == NULL)
	{
	    printf("ERROR: Could not open path = %s\n", fullPath);
		return false;
	}
	fwrite(&blockNum, sizeof(int), 1, fid );
	fwrite(&sumTriggers, sizeof(int), 1, fid );
	fwrite(&numTriggers, sizeof(int), 1, fid );
	fwrite(&numChannels, sizeof(int), 1, fid );
	fwrite(&numPoints, sizeof(int), 1, fid );
	fwrite(&x, sizeof(int), 1, fid );  // robot position in um
	fwrite(&y, sizeof(int), 1, fid );
	fwrite(&z, sizeof(int), 1, fid );
	fwrite(&u, sizeof(int), 1, fid );  // robot angle in millidegrees
	if(sumTriggers == 1)
	{
	fwrite(_buffer, 4*numPoints, numChannels, fid );
	}
	else
	{
	fwrite(_buffer, 4*numPoints*numTriggers, numChannels, fid );
	}
	fclose(fid);

	free(_buffer);
	free(buffer);

	return true;
}



// store data to disk
bool stopDAQ(int activeDAQ)
{
  if (daqIsInitializing())
    {
        daqStopInit();
         fprintf(stdout,"Initialization cancelled");
        return true;
    }

	if (daqIsDownloading())
    {
        daqStopDownload();
        fprintf(stdout,"Download cancelled");
        return true;
    }

    if (daqIsRunning())
    {
		daqStop();
	}
    fprintf(stdout, "DAQ stopped.\n");
    return true;
}

bool listDAQs()
{
	int i = 0, retVal = 0;
	char device[256];
	// get all daqs in the system, up to eight
    for(i = 0; i <= 7; i++) 
	{
		retVal = daqGetDeviceList(i, device, 256);
		if(!retVal)
		{
			break;
		}
		else
		{
			printf("device id = %d; device port = %s\n", i, device);
		}
    }
	return true;
}


bool helpDAQ()
{
	Console::WriteLine(L"JC_SONIX HELP\n\n");

    Console::WriteLine(L"VERSION: 0.3\n\n");

	Console::WriteLine(L"GENERAL:\n");
	Console::WriteLine(L" Use this command line program to communicate with Ultrasonix SonixDAQ.\n");
	Console::WriteLine(L" Step 1. Use list to view all active DAQs on the system.\n");
	Console::WriteLine(L" Step 2. Use init to initialize each DAQ by ID number.\n");
	Console::WriteLine(L" Step 3. Use acquire with additional arguments to capture and download the data to disk.\n");
	Console::WriteLine(L" Optional. Use stop to halt a running DAQ, e.g. after a Ctrl-C during an acquire.\n\n");

	Console::WriteLine(L"COMMAND LINE ARGUMENTS\n");
	Console::WriteLine(L" acquire=N : acquires and downloads data from DAQ N.\n");
	Console::WriteLine(L" data=X : fully qualified path data folder (e.g. c:/sonix/data).\n");
	Console::WriteLine(L" divisor=X : buffer size = 16GB / 2^divisor [1, 16].\n");
	Console::WriteLine(L" externalTrigger=X : 0 = ignore; 1 = use ext trigger.\n");
	Console::WriteLine(L" freerun=X : 0 = ignore; 1 = free run DAQ.\n");
	Console::WriteLine(L" fw=X : fully qualified path of firmware folder (e.g. c:/sonix/fw).\n");
	Console::WriteLine(L" init=N : initializes DAQ N.\n");
	Console::WriteLine(L"          find N with list argument.\n");
	Console::WriteLine(L" lineDuration=X : line duration in microseconds.\n");
	Console::WriteLine(L" list : prints list of active DAQs\n");
	Console::WriteLine(L" numSamples=X : number of sample to aquire on one channel per trigger.\n");
	Console::WriteLine(L" stop=N : stops a running acquisition on DAQ N.\n");
	Console::WriteLine(L" verbose=X : 0 = silent; 1 = show information.\n");
	return true;
}

int doesFileExist(char * fileName)
{
   struct stat buf;
   int i = stat ( fileName, &buf );
   /* File found */
   if ( i == 0 )
     {
       return 1;
     }
   return 0;
}


int removeTempFiles(const char *tempPath)
{
	int i = 0;
	char fullPath[256] = "";

	for(i = 0; i <= 127; i++)
	{
		sprintf(fullPath, "%s\\CH%03d.daq", tempPath, i);
		if (remove(fullPath) == -1)
		{
			printf("ERROR: Could not remove %s\n", fullPath);
	    }
	}
	return 0;
}