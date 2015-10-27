#include "for_each_file.h"

#include <sys/stat.h>
#include <dirent.h>
#include <err.h>
#include <string>

void for_each_file(const char *file_or_dir, std::function<void (const char *file)> call)
{
    bool is_dir = ({ struct stat st; stat(file_or_dir, &st) == 0 && S_ISDIR(st.st_mode); });
    if (!is_dir)
        call(file_or_dir);
    else {
        std::string path(file_or_dir);
        DIR *dir = opendir(file_or_dir);
        if (!dir)
            warn("opening %s", file_or_dir);
        else
            for (struct dirent *dirent; (dirent = readdir(dir)); )
                if (dirent->d_name[0] != '.')
                    for_each_file((path + "/" + dirent->d_name).c_str(), call);
        closedir(dir);
    }
}

