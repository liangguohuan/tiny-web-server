#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include "functions.h"


/* declare functions */
int isalnum(int c);

void mlog(int level, const char *format, ...) {
    char *tag;
    char color[12];
    switch (level) {
        case 0: tag = "TRAC"  ; break ;
        case 1: tag = "DEBUG" ; break ;
        case 2: tag = "INFO"  ; break ;
        case 3: tag = "WARN"  ; break ;
        case 4: tag = "ERROR" ; break ;
        case 5: tag = "FATAL" ; break ;
        case 6: tag = "ALL"   ; break ;
        default: break        ;
    }
    sprintf(color, "\x1b[38;5;%dm", level);
    fprintf(stdout, "%s", color);
    fprintf(stdout, "[%s] ", tag);
    fprintf(stdout, "%s", "\x1b[0m");
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    fprintf(stdout, "\n");
}

int file_exists (char * fileName)
{
    struct stat buf;
    int i = stat ( fileName, &buf );
    if ( i == 0 )
    {
        return 1;
    }
    return 0;

}

void format_size(char *buf, struct stat *stat){
    if(S_ISDIR(stat->st_mode)){
        sprintf(buf, "%s", "[DIR]");
    } else {
        off_t size = stat->st_size;
        if(size < 1024){
            sprintf(buf, "%lu", size);
        } else if (size < 1024 * 1024){
            sprintf(buf, "%.1fK", (double)size / 1024);
        } else if (size < 1024 * 1024 * 1024){
            sprintf(buf, "%.1fM", (double)size / 1024 / 1024);
        } else {
            sprintf(buf, "%.1fG", (double)size / 1024 / 1024 / 1024);
        }
    }
}

int url_encode(char *dest, const char *src) {
    char *d;
    int i;
    for(i=0, d=dest; src[i]; i++) {
        if(isalnum(src[i])) *(d++) = src[i];
        else {
            sprintf(d, "%%%02X", src[i]);
            d += 3;
        }
    }
    *d = 0;
    return d-dest;
}

void url_decode(char *src, char *dest, int max) {
    char *p = src;
    char code[3] = { 0 };
    while(*p && --max) {
        if(*p == '%') {
            memcpy(code, ++p, 2);
            *dest++ = (char)strtoul(code, NULL, 16);
            p += 2;
        } else {
            *dest++ = *p++;
        }
    }
    *dest = '\0';
}


char *get_fmtime(char *filePath)
{
    struct stat attrib;
    stat(filePath, &attrib);
    char date[30];
    strftime(date, 30, "%a, %d %b %Y %H:%M:%S GMT", localtime(&(attrib.st_mtime)));

    char *str_to_ret = malloc(sizeof(date));
    sprintf(str_to_ret, "%s", date);
    return str_to_ret;
}

char *now_rfc()
{
    char outstr[30];
    time_t t;
    struct tm *tmp;
    t = time(NULL);
    tmp = localtime(&t);
    strftime(outstr, sizeof(outstr), "%a, %d %b %Y %H:%M:%S GMT", tmp);

    char *str_to_ret = malloc(sizeof(outstr));
    sprintf(str_to_ret, "%s", outstr);
    return str_to_ret;
}

long now() {
    return (unsigned long)time(NULL);
}

double time_nanos() {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    char date[20];
    double ret;
    sprintf(date, "%ld.%ld", ts.tv_sec, ts.tv_nsec);
    sscanf(date, "%lf", &ret);
    return ret;
}

char *read_file(char *filename)
{
   char *buffer = NULL;
   int string_size, read_size;
   FILE *handler = fopen(filename, "r");

   if (handler)
   {
       fseek(handler, 0, SEEK_END);
       string_size = ftell(handler);
       rewind(handler);

       buffer = (char*) malloc(sizeof(char) * (string_size + 1) );
       read_size = fread(buffer, sizeof(char), string_size, handler);
       buffer[string_size] = '\0';

       if (string_size != read_size)
       {
           free(buffer);
           buffer = NULL;
       }

       fclose(handler);
    }
    return buffer;
}

void write_file(char *filename, char *content, char *mode) {
    FILE* fp;
    fp = fopen(filename, mode);
    fwrite (content , strlen(content), 1, fp);
    fclose(fp);
}

char *join(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1);

    if (result)
    {
        strcpy(result, s1);
        strcat(result, s2);
    }

    return result;
}

char *str_replace(char *orig, char *rep, char *with) {
    char *result;
    char *ins;
    char *tmp;
    int len_rep;
    int len_with;
    int len_front;
    int count;

    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL;
    if (!with)
        with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep;
    }
    strcpy(tmp, orig);
    return result;
}
