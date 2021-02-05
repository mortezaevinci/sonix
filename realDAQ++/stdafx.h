// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// TODO: reference additional headers your program requires here
#include <daq.h>
#include <daq_def.h>


#ifndef DAQ_FIRMWARE_PATH
    #define DAQ_FIRMWARE_PATH "/fw/"
#endif

#ifndef SAMPLING
    #define SAMPLING false;
#endif

#ifndef DAQ_DATA_PATH
    #define DAQ_DATA_PATH "/data"
#endif