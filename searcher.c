/*
TODO: ADD THIS STUFF IN search_dir_r
int search_dir_r(char *directory, char *file_type){
    WIN32_FIND_DATA fd;
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\%s", directory, file_type);
    HANDLE hFind = FindFirstFile(path, &fd);
    printf("%s\n", fd.cFileName);
    FindNextFile(hFind, &fd);
    printf("%s\n", fd.cFileName);
    FindNextFile(hFind, &fd);
    printf("%s\n", fd.cFileName);
    FindNextFile(hFind, &fd);
    printf("%s\n", fd.cFileName);
    FindNextFile(hFind, &fd);
    printf("%s\n", fd.cFileName);
    return 0;
}

*/


#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <windows.h>
#include <string.h>

unsigned static const int KB_SIZE = 1024;
unsigned static const int MB_SIZE = 1048576;
unsigned static const int GB_SIZE = 1099511627776;

int amount_of_file_calls;

unsigned int dir_size_r(char *directory, char *file_type);
void find_large_file(char *directory, unsigned int min_size_kb, char *file_type);
DWORD WINAPI recursive_thread(LPVOID param);


typedef struct {
    char directory[MAX_PATH];
    unsigned int min_size_kb;
    char file_type[32];  // adjust size to your needs
} ThreadArgs;

int main(){
    
    unsigned int size_KB;
    unsigned int full_dir_size = 0;

    char *directory = "F:/\\*";
    /*full_dir_size = dir_size_r(directory, NULL);
    full_dir_size /= 1024;
    full_dir_size /= 1024;

    printf("\nFull Directory Size %u GB\n", full_dir_size);*/
    find_large_file(directory, GB_SIZE, ".mkv");
    return 0;
}

unsigned int dir_size_r(char *directory, char *file_type) {
    unsigned int full_dir_size = 0;
    struct __stat64 file_info;
    WIN32_FIND_DATA fd;

    HANDLE hFind = FindFirstFile(directory, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        perror("Error opening directory");
        return 0;
    }

    // Strip the trailing \* to get the base directory
    char base[MAX_PATH];
    strncpy(base, directory, MAX_PATH);
    char *star = strrchr(base, '*');
    if (star) *(star - 1) = '\0';  // removes the \* at the end

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;

        char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, "%s\\%s", base, fd.cFileName);
 
        if (_stat64(full_path, &file_info) == 0) {
            unsigned int dir_size = file_info.st_size / KB_SIZE;
            full_dir_size += dir_size;
            printf("%s\n", full_path);
            printf("File Size: %u KB\n", dir_size);
            printf("Last Modified: %s", ctime(&file_info.st_mtime));
        } else {
            perror("Error reading file info");
        }
    } while (FindNextFile(hFind, &fd));

    FindClose(hFind);
    return full_dir_size;
}

void find_large_file(char *directory, unsigned int min_size_b, char *file_type){
    unsigned int full_dir_size = 0;
    struct __stat64 file_info;
    WIN32_FIND_DATA fd;

    unsigned int file_size;
    

    //TODO: fix stack overflow
    HANDLE hFind = FindFirstFile(directory, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        perror("Error opening directory");
        return;
    }

    // Strip the trailing \* to get the base directory
    char base[MAX_PATH];
    strncpy(base, directory, MAX_PATH);
    char *star = strrchr(base, '*');
    if (star) *(star - 1) = '\0';  // removes the \* at the end

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;

        char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, "%s\\%s", base, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            char recursive_path[MAX_PATH];
            snprintf(recursive_path, MAX_PATH, "%s\\*", full_path);  // re-add \*
            find_large_file(recursive_path, min_size_b, file_type);
            continue;
        }

        const char *ext = strrchr(fd.cFileName, '.');
        if (ext != NULL && _stricmp(ext, file_type) == 0 && _stat64(full_path, &file_info) == 0) {
            file_size = file_info.st_size;
            if(file_size > min_size_b){
                full_dir_size += file_size;
                printf("%s\n", full_path);
                printf("File Size: %u KB\n", file_size / KB_SIZE);
                printf("Last Modified: %s", ctime(&file_info.st_mtime));
            }
        }
        if (_stat64(full_path, &file_info) != 0) {
            //perror("Error reading file info");
            continue;
        }
    } while (FindNextFile(hFind, &fd));

    FindClose(hFind);
    amount_of_file_calls++;
    if(amount_of_file_calls % 10000 == 0){
        printf("Amount of File Calls: %i\n", amount_of_file_calls);
    }
    return;
}