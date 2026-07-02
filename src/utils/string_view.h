#pragma once

#include <ctype.h>
#include <string.h>

typedef struct {
    const char *data;
    size_t count;
} StringView;

static StringView StringView_Create(const char *cstr) {
    return (StringView) {
            .data = cstr,
            .count = strlen(cstr),
    };
}

static void StringView_ChopRight(StringView *sv, size_t n) {
    if (n > sv->count)
        n = sv->count;
    sv->count -= n;
}

static void StringView_ChopLeft(StringView *sv, size_t n) {
    if (n > sv->count)
        n = sv->count;
    sv->count -= n;
    sv->data += n;
}

static void StringView_TrimRight(StringView *sv) {
    while (sv->count > 0 && isspace((unsigned char) sv->data[sv->count - 1])) {
        StringView_ChopRight(sv, 1);
    }
}

static void StringView_TrimLeft(StringView *sv) {
    while (sv->count > 0 && isspace((unsigned char) sv->data[0])) {
        StringView_ChopLeft(sv, 1);
    }
}

static void StringView_Trim(StringView *sv) {
    StringView_TrimLeft(sv);
    StringView_TrimRight(sv);
}

static StringView StringView_ChopByDelim(StringView *sv, const char delim) {
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) {
        i += 1;
    }

    if (i < sv->count) {
        const StringView result = {
                .data = sv->data,
                .count = i,
        };
        StringView_ChopLeft(sv, i + 1);
        return result;
    }

    const StringView result = *sv;
    StringView_ChopLeft(sv, sv->count);
    return result;
}

#define STRING_VIEW_FMT "%.*s"
#define STRING_VIEW_ARG(s) (s).count, (s).data
