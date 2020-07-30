#ifndef _MYLS_H_
#define _MYLS_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_NAME_LENGTH 4096

void parseCommand(int numArgs, char* args[], int* path_count);

void defaultOption(char *path, int showpath);

void iOption(char *path, int showpath);

void lOption(char *path, int showpath);

void ROption(char* path);

void ilOption(char *path, int showpath);

void iROption(char* path);

void lROption(char* path);

void ilROption(char* path);

int count_dpList(DIR* directory);

struct dirent* get_dpList(DIR*directory, int n);

void sort_dpList(struct dirent dpList[], int n);

// allocate inode of files or directories in dpList to inodeList
// also return the maximum length of digits among those inodes
int find_inode_length (struct dirent *dpList, int n, char* path);

void find_length(struct dirent *dpList, int n, char* path, int *hardlink, int *size, int *onwer, int *group);
#endif 