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
#include <process.h>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#define LONG_PATH 4096

unsigned static const int KB_SIZE = 1024;
unsigned static const int MB_SIZE = 1048576;
unsigned static const int GB_SIZE = 1073741824;

int amount_of_file_calls;

unsigned int dir_size_r(char *directory, char *file_type);
void find_large_file(char *start_dir, unsigned long long min_size_b, char *file_type);
DWORD WINAPI recursive_thread(LPVOID param);


typedef struct {
    char *directory;
    unsigned int min_size_kb;
    char *file_type;  // adjust size to your needs
} ThreadArgs;

unsigned __stdcall RecursiveThreadFunc(void *param){
    ThreadArgs* args = (ThreadArgs*)param;
    find_large_file(args->directory, args->min_size_kb, args->file_type);
    return 0;
}

int main(){
    
    unsigned int size_KB;
    unsigned int full_dir_size = 0;

    char *directory = "\\\\?\\F:\\*";

    find_large_file(directory, GB_SIZE * 30, ".mkv");
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

void find_large_file(char *start_dir, unsigned long long min_size_b, char *file_type) {
    int stack_size = 100000;
    char **dir_stack = malloc(sizeof(char*) * stack_size);
    if (!dir_stack) { printf("malloc failed\n"); return; }
    int stack_top = 0;

    dir_stack[stack_top++] = _strdup(start_dir);

    struct __stat64 *file_info = malloc(sizeof(struct __stat64));
    WIN32_FIND_DATA *fd = malloc(sizeof(WIN32_FIND_DATA));
    char *full_path = malloc(LONG_PATH);
    char *recursive_path = malloc(LONG_PATH);
    if (!file_info || !fd || !full_path || !recursive_path) { printf("malloc failed\n"); return; }

    while (stack_top > 0) {
        char *directory = dir_stack[--stack_top];

        HANDLE hFind = FindFirstFile(directory, fd);
        if (hFind == INVALID_HANDLE_VALUE) {
            free(directory);
            continue;
        }

        char *base = _strdup(directory);
        char *star = strrchr(base, '*');
        if (star) *(star - 1) = '\0';

        BOOL found = TRUE;
        while (found) {
            if (strcmp(fd->cFileName, ".") != 0 && strcmp(fd->cFileName, "..") != 0) {
                snprintf(full_path, LONG_PATH, "%s\\%s", base, fd->cFileName);

                if (fd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (!(fd->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                        snprintf(recursive_path, LONG_PATH, "%s\\*", full_path);

                        // Grow stack if needed
                        if (stack_top >= stack_size - 1) {
                            stack_size *= 2;
                            dir_stack = realloc(dir_stack, sizeof(char*) * stack_size);
                            if (!dir_stack) { printf("realloc failed\n"); return; }
                        }
                        dir_stack[stack_top++] = _strdup(recursive_path);
                    }
                } else {
                    const char *ext = strrchr(fd->cFileName, '.');
                    if (ext != NULL && _stricmp(ext, file_type) == 0 && _stat64(full_path, file_info) == 0) {
                        unsigned long long file_size = file_info->st_size;
                        if (file_size > min_size_b) {
                            printf("%s\n", full_path);
                            printf("File Size: %llu KB\n", file_size / KB_SIZE);
                            printf("Last Modified: %s", ctime(&file_info->st_mtime));
                        }
                    }
                }
            }
            found = FindNextFile(hFind, fd);
            amount_of_file_calls++;
            if(amount_of_file_calls % 10000 == 0){
                printf("Amount of File Calls: %i\n", amount_of_file_calls);
            }
        }

        FindClose(hFind);
        free(base);
        free(directory);
    }

    free(dir_stack);
    free(file_info);
    free(fd);
    free(full_path);
    free(recursive_path);
}