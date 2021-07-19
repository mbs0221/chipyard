#include "serial.h"
#include "format.h"

#include "../uartboot/include/serial.h"

#ifdef TIMEOUT
#undef TIMEOUT
#define TIMEOUT 5
#endif

void readline(char *p);
void write_cmd(cmd_t cmd);
int write_block(int serial_fd, char *buf);
void update_progress(long size, long total, int retry);
void write_header(uint8_t *addr, long len);
void write_file(FILE *fd, long len);
void write_footer(uint32_t time);
int send_file(char *address, char *filename);