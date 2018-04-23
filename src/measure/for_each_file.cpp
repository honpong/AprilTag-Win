#include "for_each_file.h"

#include <sys/stat.h>
#include <string>

#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#endif

void for_each_file(const char *file_or_dir, std::function<void (const char *file)> call)
{
    struct stat st;
    if (stat(file_or_dir, &st) != 0)
        return;
    if ((st.st_mode&S_IFMT) != S_IFDIR)
        call(file_or_dir);
    else {
        std::string path(file_or_dir);
#ifdef WIN32
        HANDLE h;
        WIN32_FIND_DATA data;
        if ((h = FindFirstFile((path + (*path.rbegin() ==  '/' ? "*" : "/*")).c_str(), &data)) != INVALID_HANDLE_VALUE)
            do if (data.cFileName[0] != '.')
                for_each_file((path + (*path.rbegin() != '/' && *path.rbegin() != '\\' ? "\\" : "") + data.cFileName).c_str(), call);
            while (FindNextFile(h, &data));
        FindClose(h);
#else
        DIR *dir = opendir(file_or_dir);
        if (dir) {
            for (struct dirent *dirent; (dirent = readdir(dir)); )
                if (dirent->d_name[0] != '.')
                    for_each_file((path + (*path.rbegin() != '/' ? "/" : "") + dirent->d_name).c_str(), call);
            closedir(dir);
        }
#endif
    }
}
