#include "uartsend.h"
#include "string.h"

extern int serial_fd;

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
    
    if (argc == 9) {
        send_file(argv[2], argv[3]);
        send_file(argv[4], argv[5]);
        send_file(argv[6], argv[7]);
        write_cmd(UART_CMD_END);
        write_footer(atoi(argv[8]));
        printf("upload finished.\n");
    }

    close(serial_fd);

    return 0;
}
