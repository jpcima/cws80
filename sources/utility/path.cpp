#include "utility/path.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

std::string path_filename(const char *path)
{
    char fname[_MAX_FNAME], ext[_MAX_EXT];
    _splitpath(path, nullptr, nullptr, fname, ext);
    size_t nfname = strlen(fname), next = strlen(ext);
    std::string result;
    result.reserve(nfname + next);
    result.append(fname, nfname);
    result.append(ext, next);
    return result;
}

std::string path_directory(const char *path)
{
    char drive[_MAX_DRIVE], dir[_MAX_DIR];
    _splitpath(path, drive, dir, nullptr, nullptr);
    size_t ndrive = strlen(drive), ndir = strlen(dir);
    std::string result;
    result.reserve(ndrive + ndir);
    result.append(drive, ndrive);
    result.append(dir, ndir);
    return result;
}

#else
#include "utility/scope_guard.h"
#include <libgen.h>

std::string path_filename(const char *path)
{
    char *tmp = strdup(path);
    if (!tmp)
        throw std::bad_alloc();
    SCOPE(exit)
    {
        free(tmp);
    };
    return basename(tmp);
}

std::string path_directory(const char *path)
{
    char *tmp = strdup(path);
    if (!tmp)
        throw std::bad_alloc();
    SCOPE(exit)
    {
        free(tmp);
    };
    return dirname(tmp);
}

#endif
