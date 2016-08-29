#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#define max_file_size 8*1024*1024 // max file size 8MB

// search information linked list 
typedef struct search_info{
    struct search_info *next;
    unsigned int wgt;
    char filename[1];
} search_info;

unsigned int boyer_moore_sunday_algorithm(char *des_text, char *pattern, unsigned int *skip, unsigned int text_len,
                                          unsigned int pattern_len);
void iterate_file_in_folder(char *folder_path, char *pattern[], int pattern_num, search_info *info);
void destroy_info(search_info *info);
void insert_into_search_info(search_info *info, search_info *node);

int main(int argc, char *argv[]) {
    char *folder_path = argv [1];
    // this software will not generate any index file
    // skip dummy cmd parameter index filename
    int offset;
    //skip dummy cmd parameter "-s"
    if (strcmp(argv[3],"-s") == 0) {
        offset = 5;
    }else
        offset = 3;
    int pattern_num = argc - offset;
    char *pattern[argc-offset];
    for (int i = 0; i < pattern_num; i++)
        pattern[i] = argv[i+offset];
    // initialise dummy head of linked list
    search_info *info = (search_info *) malloc(sizeof(search_info));
    info -> next = NULL;
    info -> wgt = 0;
    info -> filename[0] = 'a';
    // search all files in the folder and insert the result into linked list
    iterate_file_in_folder(folder_path, pattern,pattern_num, info);
    // skip dummy head of linked list
    search_info *p = info->next;
    // output search result
    while(p!=NULL) {
        printf("%s\n",p -> filename);
        p = p->next;
    }
    destroy_info(info);
    return 0;
}

unsigned int boyer_moore_sunday_algorithm(char *text, char *pattern, unsigned int *skip, unsigned int text_len,
                                          unsigned int pattern_len) {
    //boyer-moore-sunday algorthm to match pattern in destination text file
    //return matching count
    unsigned int pos = 0;
    unsigned int counter = 0;
    while(pos <= (text_len - pattern_len)) {
        // align at left side
        unsigned int i = pos;
        unsigned int j = 0;
        while(j < pattern_len) {
            // from left to right
            if(tolower(text[i]) != pattern[j]) {
                // no matching detected
                // check skip array to update position in text file
                pos += skip[tolower(text[pos+pattern_len])];
                break;
            }
            // update j and i
            j++;
            i++;
        }
        if (j == pattern_len) {
            // one matching case detected
            // update counter
            // update position in text file
            counter += 1;
            pos += pattern_len;
        }
    }
    return counter;
}

void iterate_file_in_folder(char *folder_path, char *pattern[], int pattern_num, search_info *info) {
    // iterate every file in the folder and matching pattern
    DIR *folder;
    struct dirent *p_dir;
    struct stat file;
    unsigned int *pattern_len = (unsigned int*) malloc (sizeof(unsigned int) * pattern_num);
    unsigned int **skip = (unsigned int **) malloc(sizeof(unsigned int*) * pattern_num);
    search_info *cur = info;
    // initialise skip array
    for (int i = 0; i < pattern_num ; i++) {
        pattern_len[i] = (unsigned int)strlen(pattern[i]);
        skip[i] = (unsigned int*) malloc (sizeof(unsigned int) * 256);
        for (int j = 0; j < 256; j ++) {
            skip[i][j] = pattern_len[i] + 1;
        }
    }
    for (int i = 0; i < pattern_num; i++) {
        for (int j = 0; j < pattern_len[i]; j++) {
            pattern[i][j] = (char) tolower(pattern[i][j]);
            skip[i][pattern[i][j]] = pattern_len[i] - j;
        }
    }
    //iterate file in folder
    if ((folder = opendir(folder_path)) != NULL) {
        if(!chdir(folder_path)) {
            while ((p_dir = readdir(folder)) != NULL) {
                // do not access current folder and upper folder
                if (strcmp(".", p_dir->d_name) == 0 || strcmp("..", p_dir->d_name) == 0)
                    continue;
                lstat(p_dir->d_name, &file);
                // if file size is less than 8MB
                // read only onece
                int des_file = open(p_dir->d_name, O_RDONLY);
                unsigned int counter = 0;

                if (file.st_size <= max_file_size) {
                    char *text = (char *) calloc(file.st_size + 1, sizeof(char));
                    //int des_file = open(p_dir->d_name, O_RDONLY);
                    read(des_file, text, file.st_size);
                    for (int i = 0; i < pattern_num; i++) {
                        unsigned int tmp = boyer_moore_sunday_algorithm(text, pattern[i], skip[i], file.st_size,
                                                                        pattern_len[i]);
                        if (tmp != 0)
                            counter += tmp;
                        else {
                            counter = 0;
                            break;
                        }
                    }

                    free(text);
                }
                    // file size is over 8MB, read twice
                else {
                    char *text = (char *) calloc(max_file_size + 1, sizeof(char));
                    read(des_file, text, max_file_size);
                    unsigned int next_read_pos;
                    unsigned int **count = (unsigned int **) calloc(2, sizeof(unsigned int *));
                    count[0] = (unsigned int *) calloc(pattern_num, sizeof(unsigned int));
                    count[1] = (unsigned int *) calloc(pattern_num, sizeof(unsigned int));
                    // find nearst \t and save file split pos
                    for (next_read_pos = max_file_size - 1; next_read_pos >= 0; next_read_pos--) {
                        if (text[next_read_pos] == '\n') {
                            text[next_read_pos] = '\0';
                            //printf("split point:%d",next_read_pos);
                            break;
                        }
                    }
                    // find pattern in the first part of file
                    for (int i = 0; i < pattern_num; i++) {
                        
                        count[0][i] = boyer_moore_sunday_algorithm(text, pattern[i], skip[i], next_read_pos+1,
                                                                   pattern_len[i]);
                    }
                    free(text);
                    // read rest part
                    lseek(des_file, next_read_pos, SEEK_SET);
                    text = (char *)calloc(file.st_size - next_read_pos ,sizeof(char));
                    read(des_file,text,file.st_size - next_read_pos -1);
                    for (int i = 0; i < pattern_num; i++) {
                        count[1][i] = boyer_moore_sunday_algorithm(text, pattern[i], skip[i], file.st_size - next_read_pos - 1,
                                                                   pattern_len[i]);
                    }
                    // count the total number
                    for (int i = 0; i < pattern_num; i++) {
                        unsigned int tmp = 0;
                        if((tmp = count[1][i] + count[0][i]) != 0)
                            counter += tmp;
                        else {
                            counter = 0;
                            break;
                        }
                    }
                    free(count);
                    free(text);
                }
                // if counter is not zero
                // record the filename and the number of pattern matching times
                if (counter != 0) {
                    search_info *p = (search_info *) calloc(
                            sizeof(search_info *) + sizeof(int) + strlen(p_dir->d_name) + 1, sizeof(char));
                    p->wgt = counter;
                    p->next = NULL;
                    strcpy(p->filename, p_dir->d_name);
                    insert_into_search_info(cur, p);
                }
                close(des_file);
            }
        }
        closedir(folder);
    }
    free(pattern_len);
    free(skip);
}
void insert_into_search_info(search_info *info, search_info *node) {
    // insert into linked list order by wgt
    // if wgt is the same, order by lexicological order
    search_info *pre = info;
    search_info *p = info -> next;
    while (p != NULL) {
        if (node->wgt < p->wgt) {
            pre = p;
            p = p->next;
            continue;
        }
        else if (node->wgt == p->wgt) {
            if (strcmp(node->filename, p->filename) > 0) {
                pre = p;
                p = p->next;
                continue;
            }
            else {
                pre->next = node;
                node->next = p;
                return;
            }
        } else {
            pre->next = node;
            node->next = p;
            return;
        }
    }
    pre->next = node;
}

void destroy_info(search_info *info) {
    // free alloctaed memory of search_info list
    if(info->next !=NULL)
        destroy_info(info->next);
    free(info);
}