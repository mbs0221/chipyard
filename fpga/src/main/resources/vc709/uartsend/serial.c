#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> //文件控制定义
#include <termios.h>//终端控制定义
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "../uartboot/include/serial.h"

#define DEVICE "/dev/ttyUSB0"
#define S_TIMEOUT 1

int serial_fd = 0;

//打开串口并初始化设置
int init_serial(char *device)
{
    serial_fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serial_fd < 0) {
        perror("open");
        return -1;
    }

    //串口主要设置结构体termios <termios.h>
    struct termios options;
        
    /**1. tcgetattr函数用于获取与终端相关的参数。
    *参数fd为终端的文件描述符，返回的结果保存在termios结构体中
    */

    tcgetattr(serial_fd, &options);
    /**2. 修改所获得的参数*/
    options.c_cflag |= (CLOCAL | CREAD);   //设置控制模式状态，本地连接，接收使能
    options.c_cflag &= ~CSIZE;             //字符长度，设置数据位之前一定要屏掉这个位
    options.c_cflag &= ~CRTSCTS;           //无硬件流控
    options.c_cflag |= CS8;                //8位数据长度
    options.c_cflag &= ~CSTOPB;            //1位停止位
    options.c_iflag |= IGNPAR;             //无奇偶检验位
    options.c_oflag = 0;                   //输出模式
    options.c_lflag = 0;                   //不激活终端模式
    cfsetospeed(&options, B115200);        //设置波特率
        
    /**3. 设置新属性，TCSANOW：所有改变立即生效*/
    tcflush(serial_fd, TCIFLUSH);          //溢出数据可以接收，但不读
    tcsetattr(serial_fd, TCSANOW, &options);
        
    return 0;
}
    
/**
*串口发送数据
*@fd:串口描述符
*@data:待发送数据
*@datalen:数据长度
*/
unsigned int total_send = 0 ;
int uart_send(int fd, uint8_t *data, int datalen)
{
    int len = 0;
    len = write(fd, data, datalen);//实际写入的长度
    if(len == datalen) {
        total_send += len;
        return len;
    } else {
        tcflush(fd, TCOFLUSH);//TCOFLUSH刷新写入的数据但不传送
        return -1;
    }
    return 0;
}
    
/**
*串口接收数据
*要求启动后，在pc端发送ascii文件
*/
unsigned int total_length = 0 ;
int uart_recv(int fd, uint8_t *data, int datalen)
{
    int len=0, ret = 0;
    fd_set fs_read;
    struct timeval tv_timeout;
        
    FD_ZERO(&fs_read);
    FD_SET(fd, &fs_read);

#ifdef S_TIMEOUT    
    tv_timeout.tv_sec = (10*20/115200+2);
    tv_timeout.tv_usec = 0;
    ret = select(fd+1, &fs_read, NULL, NULL, NULL);
#elif
    ret = select(fd+1, &fs_read, NULL, NULL, tv_timeout);
#endif

    //如果返回0，代表在描述符状态改变前已超过timeout时间,错误返回-1
        
    if (FD_ISSET(fd, &fs_read)) {
        len = read(fd, data, datalen);
        total_length += len ;
        return len;
    } else {
        perror("select");
        return -1;
    }
        
    return 0;
}

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

#define BYTE (1)
#define KB (1 << 10)
#define MB (KB << 10)

char* format(size_t size)
{
    char *buf = (char*)malloc(sizeof(char)*11);
    if ((size/MB) > 0) {
        sprintf(buf, "%.2f MB", 1.0*size/MB);
    } else if ((size/KB) > 0) {
        sprintf(buf, "%.2f KB", 1.0*size/KB);
    } else {
        sprintf(buf, "%.2f B", 1.0*size);
    }
    return buf;
}

void update_progress(size_t n_bytes, long len)
{
    char *cur_size = format(n_bytes);
    char *total = format(len);
    printf("upload %s, total %s.%c", cur_size, total, n_bytes==len?'\n':'\r');
    free(total);
    free(cur_size);
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
            write_block(serial_fd, buf);
            n_bytes += size;
            update_progress(n_bytes, len);
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

char msg[256];

int main(int argc, char *argv[])
{
    for (int i = 0; i < argc; i++)
        printf("argv[%d]: %s\n", i, argv[i]);
        
    // init connection
    if (init_serial(argv[1]) != 0) {
        printf("open serial failed.\n");
        exit(-1);
    }
    printf("open serial successfully.\n");
    
    if (argc == 4) {
        // ./serial tty, address, filename
        send_file(argv[2], argv[3]);
        printf("transfer finished.\n");
    } else {
        // ./serial tty
        write_cmd(UART_CMD_END);
        uint32_t time;
        sscanf(argv[2], "%d", &time);
        write_footer(time);
    }

    close(serial_fd);

    return 0;
}
