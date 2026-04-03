#pragma once

#ifndef STOPWATCH_H
#define STOPWATCH_H
#include <defines.h>

typedef struct stopwatch{
    f64 last;
    f64 delta;
} stopwatch_t;

#define Stopwatch(func) __stopwatch_##func
void Stopwatch(update)(stopwatch_t* stopwatch);

#endif //STOPWATCH_H