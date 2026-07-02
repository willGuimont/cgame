#ifndef CGAME_STRING_VIEW_H
#define CGAME_STRING_VIEW_H

#include <ctype.h>
#include <string.h>

typedef struct {
    const char *data;
    size_t count;
} StringView;

static StringView SV_Create(const char *cstr) {
    return (StringView) {
            .data = cstr,
            .count = strlen(cstr),
    };
}

static void SV_ChopRight(StringView *sv, size_t n) {
    if (n > sv->count)
        n = sv->count;
    sv->count -= n;
}

static void SV_ChopLeft(StringView *sv, size_t n) {
    if (n > sv->count)
        n = sv->count;
    sv->count -= n;
    sv->data += n;
}

static void SV_TrimRight(StringView *sv) {
    while (sv->count > 0 && isspace(sv->data[sv->count - 1])) {
        SV_ChopRight(sv, 1);
    }
}

static void SV_TrimLeft(StringView *sv) {
    while (sv->count > 0 && isspace(sv->data[0])) {
        SV_ChopLeft(sv, 1);
    }
}

static void SV_Trim(StringView *sv) {
    SV_TrimLeft(sv);
    SV_TrimRight(sv);
}

static StringView SV_ChopByDelim(StringView *sv, const char delim) {
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) {
        i += 1;
    }

    if (i < sv->count) {
        const StringView result = {
                .data = sv->data,
                .count = i,
        };
        SV_ChopLeft(sv, i + 1);
        return result;
    }

    const StringView result = *sv;
    SV_ChopLeft(sv, sv->count);
    return result;
}

#define SV_FMT "%.*s"
#define SV_ARG(s) (s).count, (s).data

#endif // CGAME_STRING_VIEW_H
