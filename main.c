#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

// http://d4c027c89b30561298bd-484902fe60e1615dc83faa972a248000.r12.cf3.rackcdn.com/supporting_materials/Broadcom%20BCM2835.pdf
// bcm2835 datasheet page 6 virtual to physical address
#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000)

#define GPIO_INPUT  0
#define GPIO_OUTPUT 1
#define GPIO_LOW    0
#define GPIO_HIGH   1

// PT2262 datasheet
#define CLOCK_CYC 80 //in us
#define SMALL_WAIT (4*CLOCK_CYC)
#define BIG_WAIT (12*CLOCK_CYC)

#define REMOTE_PIN 27
#define REPEAT_SIGNAL 5

volatile uint32_t *gpio;
void gpio_init();

// bcm2835 datasheet page 89
void inline gpio_sio(uint8_t p, uint8_t state)
{
    *(gpio+((p)/10)) &= ~(7<<(((p)%10)*3));
    if (state == GPIO_OUTPUT)
        *(gpio+((p)/10)) |=  (1<<(((p)%10)*3));
}

void inline gpio_set(uint8_t gpio_port, uint8_t state)
{
    if (state == GPIO_LOW)
        *(gpio+10) = 1<<gpio_port;
    else
        *(gpio+7)  = 1<<gpio_port;
}

void write_zero(void) {
    gpio_set(REMOTE_PIN, GPIO_HIGH);
    usleep(SMALL_WAIT);
    gpio_set(REMOTE_PIN, GPIO_LOW);
    usleep(BIG_WAIT);

    gpio_set(REMOTE_PIN, GPIO_HIGH);
    usleep(SMALL_WAIT);
    gpio_set(REMOTE_PIN, GPIO_LOW);
    usleep(BIG_WAIT);
}

void write_one(void) {
    gpio_set(REMOTE_PIN, GPIO_HIGH);
    usleep(BIG_WAIT);
    gpio_set(REMOTE_PIN, GPIO_LOW);
    usleep(SMALL_WAIT);

    gpio_set(REMOTE_PIN, GPIO_HIGH);
    usleep(BIG_WAIT);
    gpio_set(REMOTE_PIN, GPIO_LOW);
    usleep(SMALL_WAIT);
}

void write_float(void) {
    gpio_set(REMOTE_PIN, GPIO_HIGH);
    usleep(SMALL_WAIT);
    gpio_set(REMOTE_PIN, GPIO_LOW);
    usleep(BIG_WAIT);

    gpio_set(REMOTE_PIN, GPIO_HIGH);
    usleep(BIG_WAIT);
    gpio_set(REMOTE_PIN, GPIO_LOW);
    usleep(SMALL_WAIT);
}

void write_sync(void) {
    gpio_set(REMOTE_PIN, GPIO_HIGH);
    usleep(SMALL_WAIT);
    gpio_set(REMOTE_PIN, GPIO_LOW);
    usleep(31*SMALL_WAIT);
}

// 5 address bits (housecode)
void write_to_system(uint8_t system)
{
    int i;
    for (i = 0; i < 5; i++) {
        system & (1<<i) ? write_zero() : write_float();
    }
}

// 5 adress bits (unitcode)
void write_to_unit(char unit)
{
    int i;
    for (i = 0; i < 5; i++) {
        unit == 'A'+i || unit == 'a'+i ? write_zero() : write_float();
    }
}

// 2 data bits (on or off)
void write_mode(uint8_t mode)
{
    if (mode == 0) {
        write_float();
        write_zero();
    } else {
        write_zero();
        write_float();
    }
}

void write_setting(uint8_t system, char unit, uint8_t mode)
{
    int i;
    for (i = 0; i < REPEAT_SIGNAL; i++) {
        write_to_system(system);
        write_to_unit(unit);
        write_mode(mode);
        write_sync();
    }
}

int8_t stoi(volatile char *buf)
{
    int8_t tmp = 0;
    while (*buf >= '0' && *buf <= '9') {
        tmp = (tmp * 10) + ((*buf++) - '0');
    }
    return tmp;
}

int main(int argc, char **argv)
{
    gpio_init();
    gpio_sio(REMOTE_PIN, GPIO_OUTPUT);
    write_setting(stoi(*(argv+1)), **(argv+2), stoi(*(argv+3)));

    return 0;
}

void gpio_init()
{
    int  mem_fd;
    void *gpio_map;

    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
        printf("can't open /dev/mem \n");
        exit(-1);
    }

    gpio_map = mmap(NULL,
                    getpagesize(),
                    PROT_READ|PROT_WRITE,
                    MAP_SHARED,
                    mem_fd,
                    GPIO_BASE);
    close(mem_fd);

    if (gpio_map == MAP_FAILED) {
        printf("mmap error %d\n", (int)gpio_map);
        exit(-1);
    }

    gpio = (volatile uint32_t *) gpio_map;
}
