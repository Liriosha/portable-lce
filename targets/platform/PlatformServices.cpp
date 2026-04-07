#include "PlatformServices.h"
#include "StdFileIO.h"

static StdFileIO s_stdFileIO;

IPlatformFileIO& PlatformFileIO = s_stdFileIO;