#include "format.h"

inline u_time format_time(long time)
{
    u_time u = {
        .h = time / HOUR % 24,
        .m = time / MINITE % 60,
        .s = time / SECOND % 60,
    };
    return u;
}

inline u_size format_size(double size)
{
    u_size u = {
        .value = VALUE(size),
        .unit = UNIT(size)
    };
    return u;
}