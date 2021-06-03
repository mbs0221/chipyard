#include "uartsend.h"

extern int serial_fd;

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
