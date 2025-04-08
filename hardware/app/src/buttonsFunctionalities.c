#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>		//printf()
#include <stdlib.h>		//exit()
#include <string.h>
#include <stdbool.h>
#include <unistd.h>     //usleep()

static bool isInitialized = false;

void ButtonFunctionalities_init(void)
{
    assert(!isInitialized);
    isInitialized = true;
}

void ButtonFunctionalities_cleanup(void)
{
    assert(isInitialized);
    isInitialized = false;
}