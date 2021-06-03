#ifndef _FORMAT_H_
#define _FORMAT_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define SECOND      1
#define MINITE      (SECOND * 60)
#define HOUR        (MINITE * 60)

typedef struct _u_time{
    /* data */
    int h, m, s;
} u_time;

u_time format_time(long time);

#define KB 1024.0
#define MB (KB * 1024.0)
#define GB (MB * 1024.0)

#define Mux(cond, t, f) ((cond) ? (t) : (f))

#define VALUE(sz)  Mux(sz>=GB, (sz/GB), Mux(sz>=MB, (sz/MB), Mux(sz>=KB, (sz/KB), (sz))))
#define UNIT(sz)  Mux(sz>=GB, "GB", Mux(sz>=MB, "MB", Mux(sz>=KB, "KB", "B"))) 

typedef struct _u_size{
    /* data */
    char *unit;
    double value;
} u_size;

u_size format_size(double size);

#endif