#ifndef _FUNCTIONS_H
#define _FUNCTIONS_H

void mlog(int level, const char *format, ...);
int file_exists (char *fileName);
void format_size(char *buf, struct stat *stat);
/* not support zh char */
int url_encode(char *dest, const char *src);
/* not support zh char */
void url_decode(char *src, char *dest, int max);
/* call free() after unsed */
char *get_fmtime(char *filePath);
/* call free() after unsed */
char *now_rfc();
long now();
double time_nanos();
char *read_file(char *filename);
void write_file(char *filename, char *content, char *mode);
/* call free() after unsed */
char *join(const char *s1, const char *s2);
/* call free() after unsed */
char *str_replace(char *orig, char *rep, char *with);

#endif
