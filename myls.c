#define _GNU_SOURCE
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
#include <sys/stat.h>
#include <time.h>
#include  "myls.h"

static int optioni, optionl, optionR = 0;


int main(int numArgs, char* args[]){
    // check options
    int path_count = 0;
    parseCommand(numArgs, args, &path_count);
    // printf("optioni = %d\n", optioni);
    // printf("optionl = %d\n", optionl);
    // printf("optionR = %d\n", optionR);
    // default operation
    if (optioni == 0 && optionl == 0 && optionR == 0){
        if (path_count == 0){
            defaultOption(".", 0);
        }
        else if (path_count == 1){
            for(int i=1; i<numArgs;i++){
                if (strncmp("-", args[i], 1)!=0){
                    defaultOption(args[i], 0);
                }
            } 
        }
        else{
            for(int i=1; i<numArgs;i++){
                if (strncmp("-", args[i], 1)!=0){
                    defaultOption(args[i], 1);
                }
            } 
        }
    }
    
    // -i option
    else if(optioni == 1 && optionl == 0 && optionR == 0){
        if (path_count == 0){
            iOption(".", 0);
        }
        else if (path_count == 1){
            for(int i=1; i<numArgs;i++){
                if (strncmp("-", args[i], 1)!=0){
                    iOption(args[i], 0);
                }
            } 
        }
        else{
            for(int i=1; i<numArgs;i++){
                if (strncmp("-", args[i], 1)!=0){
                    iOption(args[i], 1);
                }
            } 
        }
    }

    // -l option
    else if(optioni == 0 && optionl == 1 && optionR == 0){
        if (path_count == 0){
            lOption(".", 0);
        }
        else if (path_count == 1){
            for(int i=1; i<numArgs;i++){
                if (strncmp("-", args[i], 1)!=0){
                    lOption(args[i], 0);
                }
            } 
        }
        else{
            for(int i=1; i<numArgs;i++){
                if (strncmp("-", args[i], 1)!=0){
                    lOption(args[i], 1);
                }
            } 
        }
    }

    // -R option
    else if(optioni == 0 && optionl == 0 && optionR == 1){
        if (path_count == 0){
            ROption(".");
        }
        else{
            for(int i=1; i<numArgs;i++){
                if (strncmp("-", args[i], 1)!=0){
                    ROption(args[i]);
                }
            } 
        }
    }

    // -li option
    else if(optioni == 1 && optionl == 1 && optionR == 0){
        if (path_count == 0){
            ilOption(".", 0);
        }
        else if (path_count == 1){
            for(int i=1; i<numArgs;i++){
                if (strncmp("-", args[i], 1)!=0){
                    ilOption(args[i], 0);
                }
            } 
        }
        else{
            for(int i=1; i<numArgs;i++){
                if (strncmp("-", args[i], 1)!=0){
                    ilOption(args[i], 1);
                }
            } 
        }
    }

    // -Ri option
    else if(optioni == 1 && optionl == 0 && optionR == 1){
        if (path_count == 0){
            iROption(".");
        }
        else{
            for(int i=1; i<numArgs;i++){
                if (strncmp("-", args[i], 1)!=0){
                    iROption(args[i]);
                }
            } 
        }
    }

    // -lR option
    else if(optioni == 0 && optionl == 1 && optionR == 1){
        if (path_count == 0){
            lROption(".");
        }
        else{
            for(int i=1; i<numArgs;i++){
                if (strncmp("-", args[i], 1)!=0){
                    lROption(args[i]);
                }
            } 
        }
    }

    // -ilR option
    else if(optioni == 1 && optionl == 1 && optionR == 1){
        if (path_count == 0){
            ilROption(".");
        }
        else{
            for(int i=1; i<numArgs;i++){
                if (strncmp("-", args[i], 1)!=0){
                    ilROption(args[i]);
                }
            } 
        }
    }

}


void parseCommand(int numArgs, char* args[], int* path_count){
    if (numArgs == 1){
        return;
    }
    for (int i=1; i<numArgs; i++){
        int arglen = strlen(args[i]);
        if (strncmp("-", args[i],1)!=0){
            (*path_count) ++;
            continue;
        }
        for (int j=1; j<arglen; j++){
            switch (args[i][j]){
                case 'i':
                    optioni = 1;
                    break;
                case 'l':
                    optionl = 1;
                    break;
                case 'R':
                    optionR = 1;
                    break;
                default:
                    printf("myls: invalid option -- '%c' \n", args[i][j]);
                    exit(1);
            }
        }
    }
}

void defaultOption(char *path, int showpath){
    DIR *directory;
    int n = 0;
    directory = opendir(path);
    if (directory == NULL){
        fprintf(stderr, "myls: cannot access '%s': %s\n ", path, strerror(errno));
        return;
    }
    // get number of non-hidden files and directories
    n = count_dpList(directory);

    rewinddir(directory);
    // get an array of dirent struct
    struct dirent *dpList = get_dpList(directory, n);
    sort_dpList(dpList, n);
    if (showpath==1){
        printf("%s: \n", path);
    }
    for(int i=0; i<n ;i++){
        printf("%s\n", dpList[i].d_name);
    }
    free(dpList);
    closedir(directory);
}


void iOption(char *path, int showpath){
    DIR *directory;
    struct stat fileStat;
    int n = 0;
    directory = opendir(path);
    if (directory == NULL){
        fprintf(stderr, "myls: cannot access '%s': %s\n ", path, strerror(errno));
        return;
    }
    n = count_dpList(directory);

    // add these dirent structs to an array
    // so we can sort them
    rewinddir(directory);
    // get an array of dirent struct
    struct dirent *dpList = get_dpList(directory, n);
    sort_dpList(dpList, n);

    // first, find the maximum length of digits among inodes with function max_digits_length
    int inodeDigit = find_inode_length(dpList, n, path);

    // then, print information
    if (showpath == 1){
        printf("%s: \n", path);
    }
    for (int i=0; i<n; i++){
        char abs_path[MAX_NAME_LENGTH];
        strcpy(abs_path, path);
        strcat(abs_path, "/");
        strcat(abs_path, dpList[i].d_name);
        stat(abs_path, &fileStat);
        printf("%*ld %s\n", inodeDigit, fileStat.st_ino, dpList[i].d_name);
    }
    free(dpList);
    closedir(directory);
}

void lOption(char *path, int showpath){
    DIR *directory;
    struct stat fileStat;
    struct passwd *pw;
    struct group *grp;
    int n = 0;
    directory = opendir(path);
    if (directory == NULL){
        fprintf(stderr, "myls: cannot access '%s': %s\n ", path, strerror(errno));
        return;
    }
    n = count_dpList(directory);
    // add these dirent structs to an array
    // so we can sort them
    rewinddir(directory);
    // get an array of dirent struct
    struct dirent *dpList = get_dpList(directory, n);
    sort_dpList(dpList, n);

    // determine width of values
    int hardlinkDigit, sizeDigit, ownerLength, groupLength;
    find_length(dpList, n, path, &hardlinkDigit, &sizeDigit, &ownerLength, &groupLength);

    if (showpath == 1){
        printf("%s: \n", path);
    }  
    for (int i=0; i<n; i++){
        char abs_path[MAX_NAME_LENGTH];
        strcpy(abs_path, path);
        strcat(abs_path, "/");
        strcat(abs_path, dpList[i].d_name);
        stat(abs_path, &fileStat);
        // type and permission
        // Reference : https://stackoverflow.com/a/10323127
        printf( (S_ISDIR(fileStat.st_mode)) ? "d":"-");
        printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
        printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
        printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
        printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
        printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
        printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
        printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
        printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
        printf( (fileStat.st_mode & S_IXOTH) ? "x " : "- ");
        // hardlink
        printf("%*ld ", hardlinkDigit, fileStat.st_nlink);
        // owner name
        pw = getpwuid(fileStat.st_uid);
        printf("%*s ", ownerLength, pw->pw_name);
        // group name
        grp = getgrgid(fileStat.st_gid);
        printf("%*s ", groupLength, grp->gr_name);
        // size in byte
        printf("%*ld ", sizeDigit, fileStat.st_size);
        // time 
        // reference: http://www.cplusplus.com/reference/ctime/strftime/
        // reference: https://stackoverflow.com/a/26307720/12358813
        char timebuffer[80];
        strftime(timebuffer, 80, "%b %2d %Y %2H:%2M", localtime(&(fileStat.st_mtime)));
        printf("%s ", timebuffer);
        // file name
        printf("%s\n", dpList[i].d_name);
    }
    closedir(directory);
    free(dpList);
}

void ROption(char* path){
    DIR *directory;
    struct stat fileStat;
    int n = 0;
    directory = opendir(path);
    if (directory == NULL){
        fprintf(stderr, "myls: cannot access '%s': %s\n ", path, strerror(errno));
        return;
    }
    n = count_dpList(directory);

    rewinddir(directory);
    struct dirent *dpList = get_dpList(directory, n);
    sort_dpList(dpList, n);

    defaultOption(path, 1);

    // recursive step
    for (int i=0; i<n; i++){
        char abs_path[MAX_NAME_LENGTH];
        strcpy(abs_path, path);
        strcat(abs_path, "/");
        strcat(abs_path, dpList[i].d_name);
        stat(abs_path, &fileStat);
        if (S_ISDIR(fileStat.st_mode)){
            printf("\n");
            ROption(abs_path);
        }
    }
    closedir(directory);
    free(dpList);
}


void ilOption(char *path, int showpath){
    DIR *directory;
    struct stat fileStat;
    struct passwd *pw;
    struct group *grp;
    int n = 0;
    directory = opendir(path);
    if (directory == NULL){
        fprintf(stderr, "myls: cannot access '%s': %s\n ", path, strerror(errno));
        return;
    }
    n = count_dpList(directory);
    // add these dirent structs to an array
    // so we can sort them
    rewinddir(directory);
    // get an array of dirent struct
    struct dirent *dpList = get_dpList(directory, n);
    sort_dpList(dpList, n);

    // determine width of values
    int hardlinkDigit, sizeDigit, ownerLength, groupLength;
    find_length(dpList, n, path, &hardlinkDigit, &sizeDigit, &ownerLength, &groupLength);
    int inodeDigit = find_inode_length(dpList, n, path);
    if (showpath == 1){
        printf("%s: \n", path);
    }  
    for (int i=0; i<n; i++){
        char abs_path[MAX_NAME_LENGTH];
        strcpy(abs_path, path);
        strcat(abs_path, "/");
        strcat(abs_path, dpList[i].d_name);
        stat(abs_path, &fileStat);

        // inode
        printf("%*ld ", inodeDigit, fileStat.st_ino);
        // type and permission
        // Reference : https://stackoverflow.com/a/10323127
        printf( (S_ISDIR(fileStat.st_mode)) ? "d":"-");
        printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
        printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
        printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
        printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
        printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
        printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
        printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
        printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
        printf( (fileStat.st_mode & S_IXOTH) ? "x " : "- ");
        // hardlink
        printf("%*ld ", hardlinkDigit, fileStat.st_nlink);
        // owner name
        pw = getpwuid(fileStat.st_uid);
        printf("%*s ", ownerLength, pw->pw_name);
        // group name
        grp = getgrgid(fileStat.st_gid);
        printf("%*s ", groupLength, grp->gr_name);
        // size in byte
        printf("%*ld ", sizeDigit, fileStat.st_size);
        // time 
        // reference: http://www.cplusplus.com/reference/ctime/strftime/
        // reference: https://stackoverflow.com/a/26307720/12358813
        char timebuffer[80];
        strftime(timebuffer, 80, "%b %2d %Y %2H:%2M", localtime(&(fileStat.st_mtime)));
        printf("%s ", timebuffer);
        // file name
        printf("%s\n", dpList[i].d_name);
    }
    closedir(directory);
    free(dpList);
}

void iROption(char* path){
    DIR *directory;
    struct stat fileStat;
    int n = 0;
    directory = opendir(path);
    if (directory == NULL){
         fprintf(stderr, "myls: cannot access '%s': %s\n ", path, strerror(errno));
         return;
     }
     n = count_dpList(directory);

    // add these dirent structs to an array
    // so we can sort them
    rewinddir(directory);
    // get an array of dirent struct
     struct dirent *dpList = get_dpList(directory, n);
    sort_dpList(dpList, n);

    // print information
    iOption(path, 1);

    // recursive step
    for (int i=0; i<n; i++){
        char abs_path[MAX_NAME_LENGTH];
        strcpy(abs_path, path);
        strcat(abs_path, "/");
        strcat(abs_path, dpList[i].d_name);
        stat(abs_path, &fileStat);
        if (S_ISDIR(fileStat.st_mode)){
            printf("\n");
            iROption(abs_path);
        }
    }
    free(dpList);
    closedir(directory);
}


void lROption(char* path){
    DIR *directory;
    struct stat fileStat;
    int n = 0;
    directory = opendir(path);
    if (directory == NULL){
         fprintf(stderr, "myls: cannot access '%s': %s\n ", path, strerror(errno));
         return;
     }
     n = count_dpList(directory);

    // add these dirent structs to an array
    // so we can sort them
    rewinddir(directory);
    // get an array of dirent struct
     struct dirent *dpList = get_dpList(directory, n);
    sort_dpList(dpList, n);

    // print information
    lOption(path, 1);

    // recursive step
    for (int i=0; i<n; i++){
        char abs_path[MAX_NAME_LENGTH];
        strcpy(abs_path, path);
        strcat(abs_path, "/");
        strcat(abs_path, dpList[i].d_name);
        stat(abs_path, &fileStat);
        if (S_ISDIR(fileStat.st_mode)){
            printf("\n");
            lROption(abs_path);
        }
    }
    free(dpList);
    closedir(directory);
}

void ilROption(char* path){
    DIR *directory;
    struct stat fileStat;
    int n = 0;
    directory = opendir(path);
    if (directory == NULL){
         fprintf(stderr, "myls: cannot access '%s': %s\n ", path, strerror(errno));
         return;
     }
     n = count_dpList(directory);

    // add these dirent structs to an array
    // so we can sort them
    rewinddir(directory);
    // get an array of dirent struct
     struct dirent *dpList = get_dpList(directory, n);
    sort_dpList(dpList, n);

    // print information
    ilOption(path, 1);

    // recursive step
    for (int i=0; i<n; i++){
        char abs_path[MAX_NAME_LENGTH];
        strcpy(abs_path, path);
        strcat(abs_path, "/");
        strcat(abs_path, dpList[i].d_name);
        stat(abs_path, &fileStat);
        if (S_ISDIR(fileStat.st_mode)){
            printf("\n");
            ilROption(abs_path);
        }
    }
    free(dpList);
    closedir(directory);
}

void sort_dpList(struct dirent dpList[], int n){
    for(int i=0; i<n; i++){
        int min = i;
        for(int j=i+1; j<n; j++){
            if(strcmp(dpList[min].d_name,dpList[j].d_name)>0){
                min = j;
            }
        }
        struct dirent temp = dpList[i];
        dpList[i] = dpList[min];
        dpList[min] = temp;
    }
}


int count_dpList(DIR* directory){
    int count = 0;
    struct dirent *dp;
    while((dp = readdir(directory)) != NULL){
        char *name = dp->d_name;
        // skip hidden file
        if (strncmp(name, "." ,1) == 0){
            continue;
        }
        count ++;
    }
    return count;
}


struct dirent* get_dpList(DIR*directory, int n){
    struct dirent *dpList = malloc(n * sizeof(struct dirent));
    struct dirent *dp;
    int i = 0;
    while((dp = readdir(directory)) != NULL){
        char *name = dp->d_name;
        // skip hidden file
        if (strncmp(name, "." ,1) == 0){
            continue;
        }
        dpList[i] = *dp;
        i ++;
    }
    return dpList;

}


int find_inode_length (struct dirent *dpList, int n, char* path){
    struct stat fileStat;
    int digit = 0;
    long maxinode = 0;
    for (int i=0; i<n; i++){
        // use absolute path get inode of file or directory
        // https://stackoverflow.com/a/2153755
        char abs_path[MAX_NAME_LENGTH];
        strcpy(abs_path, path);
        strcat(abs_path, "/");
        strcat(abs_path, dpList[i].d_name);
        stat(abs_path, &fileStat);
        long inode = fileStat.st_ino;
        if (inode  > maxinode){
            maxinode = inode;
        }
    }
    while(maxinode / 10 > 0){
        digit ++;
        maxinode /= 10;
    }
    return digit + 1;
}


void find_length(struct dirent *dpList, int n, char* path, int *hardlink, int *size, int *onwer, int *group){
    struct stat fileStat;
    struct group *grp;
    struct passwd *pw;
    int hardlink_length = 0;
    int size_length = 0;
    int owner_length = 0;
    int group_length = 0;

    long maxhardlink = 0;
    long maxsize = 0;

    for (int i=0; i<n ;i++){
        char abs_path[MAX_NAME_LENGTH];
        strcpy(abs_path, path);
        strcat(abs_path, "/");
        strcat(abs_path, dpList[i].d_name);
        stat(abs_path, &fileStat);
        if (fileStat.st_nlink > maxhardlink){
            maxhardlink = fileStat.st_nlink;
        }
        if (fileStat.st_size > maxsize){
            maxsize = fileStat.st_size;
        }
        pw = getpwuid(fileStat.st_uid);
        if (strlen(pw->pw_name) > owner_length){
            owner_length = strlen(pw->pw_name);
        }
        grp = getgrgid(fileStat.st_gid);
        if (strlen(grp->gr_name) > group_length){
            group_length = strlen(grp->gr_name);
        }
    }
    while (maxhardlink / 10 > 0){
        hardlink_length ++;
        maxhardlink /= 10;
    }
    while(maxsize / 10 > 0){
        size_length ++;
        maxsize /= 10;
    }
    *hardlink = hardlink_length+1;
    *size = size_length+1;
    *onwer = owner_length;
    *group = group_length;
    return;
}