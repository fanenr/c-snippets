#ifndef APUE_H
#define APUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAXLINE 4096

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

extern void err_sys(const char *fmt, ...) __attribute__((noreturn));

extern void err_quit(const char *fmt, ...) __attribute__((noreturn));

extern void err_dump(const char *fmt, ...) __attribute__((noreturn));

extern void err_ret(const char *fmt, ...);

extern void set_fl(int fd, int flags);

extern void clr_fl(int fd, int flags);

#endif