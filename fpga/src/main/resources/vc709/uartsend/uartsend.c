#include "uartsend.h"

extern int serial_fd;

void readline(char *p)
{
    char *q = p;
    do {
        uart_recv(serial_fd, (uint8_t *)q, sizeof(uint8_t));
    } while (*q++ != '\n');
    *q = '\0';
}

void write_cmd(cmd_t cmd)
{
    uart_send(serial_fd, (uint8_t *)&cmd, sizeof(cmd));
}

void write_block(int serial_fd, char *buf)
{
    int retry = -1;
    char cmd = NAK;
    
    // calculate crc
    uint16_t crc_exp = crc16((uint8_t *)buf);
    do {
        if(retry++ == TIMEOUT) {
            printf("TIMEOUT\n");
            exit(0);
        }
    
        // send file and crc
        uart_send(serial_fd, (uint8_t *)buf, CRC16_LEN);
        uart_send(serial_fd, (uint8_t *)&crc_exp, 2);
        // ACK/NAK
        uart_recv(serial_fd, (uint8_t *)&cmd, sizeof(char));
    } while (cmd != ACK);
}

void update_progress(long size, long total, double duration)
{
    u_size uploaded = format_size(size);
    u_size total_size = format_size(total);
    // u_size speed = format_size(size/duration);
    // u_time time = format_time(duration);

    printf("upload: %4.2f %s/%4.2f %s%c",
            uploaded.value, uploaded.unit,
            total_size.value, total_size.unit,
            Mux(size==total, '\n', '\r'));
}

void write_header(uint8_t *addr, long len)
{
    // send metadata
    package_t package;
    package.addr = addr;
    package.len = len;

    uart_send(serial_fd, (uint8_t *)&package, sizeof(package));
}

void write_file(FILE *fd, long len)
{
    // send file
    size_t n_bytes = 0;
    size_t size;

    char *buf = (char *)malloc(sizeof(char) * CRC16_LEN);
    clock_t start = clock();
    do {
        size = fread(buf, sizeof(char), CRC16_LEN, fd);
        if (size == -1) {
            perror("read");
            exit(1);
        }
        if (size > 0) {
            if (size < CRC16_LEN) {
                memset(buf+size, 0, CRC16_LEN - size);
            }
            n_bytes += size;
            write_block(serial_fd, buf);
            clock_t end = clock();
            update_progress(n_bytes, len, (double)(end - start)/CLOCKS_PER_SEC);
        }
    } while (size != 0);

    free(buf);
}

void write_footer(uint32_t time)
{
    uart_send(serial_fd, (uint8_t *)&time, sizeof(time));
}

int send_file(char *address, char *filename)
{
    FILE *fd = fopen(filename, "r");

    if (fd == NULL) {
        printf("open file failed.\n");
        perror("open");
        return -1;
    }

    printf("open file successfully.\n");

    // get file len
    long len;
    fseek(fd, 0L, SEEK_END);
    len = ftell(fd);
    fseek(fd, 0L, SEEK_SET);

    printf("file len: %ld\n", len);

    // parse address
    uint8_t *addr;
    sscanf(address, "%p", &addr);
    printf("start transfer at addr[%p].\n", addr);

    // send cmd, header and file
    write_cmd(UART_CMD_TRANSFER);
    write_header(addr, len);
    write_file(fd, len);
    
    fclose(fd);
    
    return 0;
}
