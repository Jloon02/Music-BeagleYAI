#include "hal/rotary_encoder.h"
#include "hal/gpio.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include <time.h>

// Pin config info Rotary Encoder
#define GPIO_CHIP           GPIO_CHIP_2     // A & B
#define GPIO_CHIP_PUSH      GPIO_CHIP_0     // Push down
#define GPIO_LINE_NUMBER_A      7           // A
#define GPIO_LINE_NUMBER_B      8           // B
#define GPIO_LINE_NUMBER_PUSH   10

// Statemachine Data Structures
struct stateEvent {
    struct state* pNextState;
};

struct state {
    struct stateEvent a_rising;
    struct stateEvent b_rising;

    struct stateEvent a_falling;
    struct stateEvent b_falling;
};

struct state states[] = {
    // Rest
    {
        .a_rising = {&states[0]},
        .b_rising = {&states[0]},

        .a_falling = {&states[1]},
        .b_falling = {&states[3]},
    },
    // State 1
    {
        .a_rising = {&states[0]},
        .b_rising = {&states[1]},

        .a_falling = {&states[1]},
        .b_falling = {&states[2]},
    },
    // State 2
    {
        .a_rising = {&states[3]},
        .b_rising = {&states[1]},

        .a_falling = {&states[2]},
        .b_falling = {&states[2]},
    },
    // State 3
    {
        .a_rising = {&states[3]},
        .b_rising = {&states[0]},

        .a_falling = {&states[2]},
        .b_falling = {&states[3]},
    }
};

static bool isInitialized = false;
static bool keepRunning = false;
static pthread_t rotary_thread; // Thread handle
static bool click = false;

struct GpioLine* s_lineRotA = NULL;
struct GpioLine* s_lineRotB = NULL;
struct GpioLine* s_linePUSH = NULL;

struct state* pCurrentState = &states[0];
struct state* pSecondState = NULL;
struct state* pSecondLastState = NULL;

// For higher level to know if knob has been turned
static bool isTurnedClockwise = false;
static bool isTurnedCounterClockwise = false;

// ─────────────────────────────────────────────
// ──────────── Helper Functions ───────────────
// ─────────────────────────────────────────────

static void check_for_action()
{
    if (pCurrentState != &states[0] || !pSecondState || !pSecondLastState) return;

    if (pSecondState == &states[1] && pSecondLastState == &states[3]) {
        isTurnedClockwise = true;
    } else if (pSecondState == &states[3] && pSecondLastState == &states[1]) {
        isTurnedCounterClockwise = true;
    }

    pSecondState = NULL;
    pSecondLastState = NULL;
}

static void handle_state_transition(bool isRising, bool isLineA)
{
    // Check what the state will become
    struct stateEvent* event = isLineA
        ? (isRising ? &pCurrentState->a_rising : &pCurrentState->a_falling)
        : (isRising ? &pCurrentState->b_rising : &pCurrentState->b_falling);

    if (!event || !event->pNextState) return;

    pSecondLastState = pCurrentState;
    pCurrentState = event->pNextState;

    if (!pSecondState && pCurrentState != &states[0]) {
        pSecondState = pCurrentState;
    }

    check_for_action();
}

static void handle_push_button(int currPush, int* prevPush, bool* click)
{
    if (currPush != *prevPush) {
        if (currPush == 0 && !(*click)) {
            *click = true;
            printf("Rotary_Encoder: Clicked BUTTON\n");
        } 

        *prevPush = currPush;
    }
}

bool Rotary_encoder_isCW(void) {
    bool toReturn = isTurnedClockwise;
    isTurnedClockwise = false;
    return toReturn;
}

bool Rotary_encoder_isCCW(void) {
    bool toReturn = isTurnedCounterClockwise;
    isTurnedCounterClockwise = false;
    return toReturn;
}

bool Rotary_encoder_get_click(void) 
{
    return click;
}

void Rotary_encoder_set_click(bool status)
{
    click = status;
}

// ─────────────────────────────────────────────
// ──────────── Rotary Thread Logic ────────────
// ─────────────────────────────────────────────

static void* rotary_encoder_running(void* arg)
{
    assert(isInitialized);
    (void)arg;

    int prevA = gpiod_line_get_value((struct gpiod_line*)s_lineRotA);
    int prevB = gpiod_line_get_value((struct gpiod_line*)s_lineRotB);
    int prevPush = gpiod_line_get_value((struct gpiod_line*)s_linePUSH);

    while (keepRunning) 
    {
        struct timespec sleep_time = { .tv_sec = 0, .tv_nsec = 1 * 1000 * 1000 }; // 1ms
        nanosleep(&sleep_time, NULL);

        int currA = gpiod_line_get_value((struct gpiod_line*)s_lineRotA);
        int currB = gpiod_line_get_value((struct gpiod_line*)s_lineRotB);
        int currPush = gpiod_line_get_value((struct gpiod_line*)s_linePUSH);

        // 1 is rising, that's we we do currA == 1 to check for isRising
        if (currA != prevA) {
            handle_state_transition(currA == 1, true);
            prevA = currA;
        }

        if (currB != prevB) {
            handle_state_transition(currB == 1, false);
            prevB = currB;
        }

        handle_push_button(currPush, &prevPush, &click);
    }

    return NULL;
}

// ─────────────────────────────────────────────
// ─────────────── Init & Cleanup ──────────────
// ─────────────────────────────────────────────

void Rotary_encoder_init(void)
{
    assert(!isInitialized);
    s_lineRotA = Gpio_openForEvents(GPIO_CHIP, GPIO_LINE_NUMBER_A);
    s_lineRotB = Gpio_openForEvents(GPIO_CHIP, GPIO_LINE_NUMBER_B);
    s_linePUSH = Gpio_openForEvents(GPIO_CHIP_PUSH, GPIO_LINE_NUMBER_PUSH);

    gpiod_line_request_input((struct gpiod_line*)s_lineRotA, "Rotary A");
    gpiod_line_request_input((struct gpiod_line*)s_lineRotB, "Rotary B");
    gpiod_line_request_input((struct gpiod_line*)s_linePUSH, "Push Button");

    isInitialized = true;
    keepRunning = true;

    if (pthread_create(&rotary_thread, NULL, rotary_encoder_running, NULL) != 0) {
        perror("Failed to create rotary encoder thread");
        exit(EXIT_FAILURE);
    }
}

void Rotary_encoder_cleanup(void)
{
    assert(isInitialized);
    keepRunning = false;
    pthread_join(rotary_thread, NULL);

    Gpio_close(s_lineRotA);
    Gpio_close(s_lineRotB);
    Gpio_close(s_linePUSH);

    isInitialized = false;
}