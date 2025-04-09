#include <assert.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "hal/buttons.h"
#include "hal/gpio.h"

// Pin config info Rotary Encoder
#define GPIO4_CHIP      GPIO_CHIP_1     // Button Right
#define GPIO6_CHIP      GPIO_CHIP_2     // Button left
#define GPIO4_LINE      38
#define GPIO6_LINE      17

static bool keepRunning = false;
static pthread_t buttons_thread; // Thread handle
static pthread_mutex_t buttonsMutex = PTHREAD_MUTEX_INITIALIZER;

struct GpioLine* s_line_right = NULL;
struct GpioLine* s_line_left = NULL;
static bool isInitialized = false;

static bool right_clicked = false;
static bool left_clicked = false;

bool Buttons_get_right(void) 
{
    pthread_mutex_lock(&buttonsMutex);
    bool value = right_clicked;
    pthread_mutex_unlock(&buttonsMutex);
    return value;
}

bool Buttons_get_left(void) 
{
    pthread_mutex_lock(&buttonsMutex);
    bool value = left_clicked;
    pthread_mutex_unlock(&buttonsMutex);
    return value;
}

void Buttons_set_right(bool status)
{
    pthread_mutex_lock(&buttonsMutex);
    right_clicked = status;
    pthread_mutex_unlock(&buttonsMutex);
}

void Buttons_set_left(bool status)
{
    pthread_mutex_lock(&buttonsMutex);
    left_clicked = status;
    pthread_mutex_unlock(&buttonsMutex);
}

static void buttons_handle_click(int currPush, int* prevPush, bool* click) {
    if (currPush != *prevPush) {
        if (currPush == 0 && !(*click)) {
            *click = true;
        } 
    }
    *prevPush = currPush;
}

static void* buttons_running(void* arg)
{
    (void)arg;

    int prevRight = gpiod_line_get_value((struct gpiod_line*)s_line_right);
    int prevLeft = gpiod_line_get_value((struct gpiod_line*)s_line_left);
    
    while (keepRunning) {
        int currRight = gpiod_line_get_value((struct gpiod_line*)s_line_right);
        int currLeft = gpiod_line_get_value((struct gpiod_line*)s_line_left);

        pthread_mutex_lock(&buttonsMutex);
        buttons_handle_click(currRight, &prevRight, &right_clicked);
        buttons_handle_click(currLeft, &prevLeft, &left_clicked);
        pthread_mutex_unlock(&buttonsMutex);

        usleep(10000); // 10 ms delay to avoid busy-waiting
    }

    return NULL;
}

void Buttons_init(void)
{
    assert(!isInitialized);
    s_line_right = Gpio_openForEvents(GPIO4_CHIP, GPIO4_LINE);
    s_line_left = Gpio_openForEvents(GPIO6_CHIP, GPIO6_LINE);

    gpiod_line_request_input((struct gpiod_line*)s_line_right, "Right Button");
    gpiod_line_request_input((struct gpiod_line*)s_line_left, "Left Button");
    isInitialized = true;
    keepRunning = true;

    if (pthread_create(&buttons_thread, NULL, buttons_running, NULL) != 0) {
        perror("Failed to create buttons thread");
        exit(EXIT_FAILURE);
    }
}

void Buttons_cleanup(void)
{
    assert(isInitialized);
    keepRunning = false;
    pthread_join(buttons_thread, NULL);
    Gpio_close(s_line_right);
    Gpio_close(s_line_left);
    isInitialized = false;
}