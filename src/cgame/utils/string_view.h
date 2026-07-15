#pragma once

#include <ctype.h>
#include <limits.h>
#include <string.h>
#include "common.h"

typedef struct {
    const char *data;
    usize count;
} StringView;

static StringView StringView_Create(const char *cstr) {
    return (StringView) {
            .data = cstr ? cstr : "",
            .count = cstr ? strlen(cstr) : 0,
    };
}

static void StringView_ChopRight(StringView *sv, usize n) {
    if (n > sv->count)
        n = sv->count;
    sv->count -= n;
}

static void StringView_ChopLeft(StringView *sv, usize n) {
    if (n > sv->count)
        n = sv->count;
    sv->count -= n;
    sv->data += n;
}

static void StringView_TrimRight(StringView *sv) {
    while (sv->count > 0 && isspace((u8) sv->data[sv->count - 1])) {
        StringView_ChopRight(sv, 1);
    }
}

static void StringView_TrimLeft(StringView *sv) {
    while (sv->count > 0 && isspace((u8) sv->data[0])) {
        StringView_ChopLeft(sv, 1);
    }
}

static void StringView_Trim(StringView *sv) {
    StringView_TrimLeft(sv);
    StringView_TrimRight(sv);
}

static StringView StringView_ChopByDelim(StringView *sv, const char delim) {
    usize i = 0;
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

static i32 StringView_PrintLen(const StringView sv) { return sv.count > (usize) INT_MAX ? INT_MAX : (i32) sv.count; }

#define STRING_VIEW_FMT "%.*s"
#define STRING_VIEW_ARG(s) StringView_PrintLen(s), (s).data
