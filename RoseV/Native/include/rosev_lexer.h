#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

char* rosev_trim_in_place(char* text);
int rosev_starts_with(const char* text, const char* prefix);
uint64_t rosev_text_hash(const char* text);
uint64_t rosev_mix64(uint64_t state, uint64_t value);

#ifdef __cplusplus
}
#endif
