#pragma once
#include <stdbool.h>
#include <stdint.h>

#define UPDATE_CLOCK_MIN_SECONDS INT64_C(1792249200)

int64_t UpdateClock_FromDate(int year,int month,int day,int hour,int minute,int second);
bool UpdateClock_ParseHttpDate(const char *text,int64_t *seconds);
