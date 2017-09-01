#include "resource.h"

#if defined(WIN32) || defined(WIN64)
#include <windows.h>
#endif

extern "C" const char voc[];
extern "C" const int voc_size;

const char *load_vocabulary(size_t& size)
{
#if defined(WIN32) || defined(WIN64)
    HMODULE handle = ::GetModuleHandle("vocabulary");
    HRSRC rc = ::FindResource(handle, MAKEINTRESOURCE(IDR_VOC_FILE), MAKEINTRESOURCE(VOC_FILE));
    HGLOBAL rc_data = ::LoadResource(handle, rc);
    size = ::SizeofResource(handle, rc);
    return static_cast<const char*>(::LockResource(rc_data));
#else
    size = voc_size;
    return voc;
#endif
}
