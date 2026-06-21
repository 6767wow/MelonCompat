#include "rosev_lexer.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

char* rosev_trim_in_place(char* text)
{
    char* start = text;
    char* end;

    if (text == NULL)
        return text;

    while (*start != '\0' && isspace((unsigned char)*start))
        ++start;

    if (*start == '\0')
    {
        *text = '\0';
        return text;
    }

    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end))
        *end-- = '\0';

    if (start != text)
        memmove(text, start, strlen(start) + 1);

    return text;
}

int rosev_starts_with(const char* text, const char* prefix)
{
    if (text == NULL || prefix == NULL)
        return 0;

    while (*prefix != '\0')
    {
        if (*text++ != *prefix++)
            return 0;
    }

    return 1;
}

uint64_t rosev_text_hash(const char* text)
{
    uint64_t hash = 0xcbf29ce484222325ULL;
    if (text == NULL)
        return hash;

    while (*text != '\0')
    {
        hash = rosev_mix64(hash, (uint8_t)*text);
        ++text;
    }

    return hash;
}
