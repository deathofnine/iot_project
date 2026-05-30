#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

static int fd_led;

void led_init(void)
{
    fd_led = open("/dev/100ask_led", O_RDWR);
    if (fd_led < 0)
    {
    }
}

void led_control(int on)
{
    char buf[2];
    buf[0] = 0;

    if (on)
    {
        buf[1] = 0;
    }
    else
    {
        buf[1] = 1;
    }
    write(fd_led, buf, 2);
}
