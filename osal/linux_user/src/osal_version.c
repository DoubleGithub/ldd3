#include "osal.h"

#ifdef COMP_VER
#define xstringify(s) stringify(s)
#define stringify(s) #s
char *osal_version_string="#@# libosal.so " xstringify(COMP_VER) " " __DATE__ " " __TIME__;
#else
char *osal_version_string="#@# libosal.so 0.0.0.0000 <- COMP_VER undefined> " __DATE__ " " __TIME__;
#endif
