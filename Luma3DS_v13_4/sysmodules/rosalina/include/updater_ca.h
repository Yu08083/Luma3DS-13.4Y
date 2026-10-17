#pragma once
#include <stddef.h>
#include <stdint.h>
typedef struct UpdateCa { const uint8_t *der; size_t bytes; } UpdateCa;
extern const UpdateCa updateCaRoots[2];
