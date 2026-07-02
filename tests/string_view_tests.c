#include "test_utils.h"
#include "utils/string_view.h"

#include <string.h>

static int check_sv_equals(const StringView view, const char *expected) {
    const size_t expected_len = strlen(expected);
    ASSERT(view.count == expected_len);
    ASSERT(strncmp(view.data, expected, view.count) == 0);
    return 0;
}

static int test_sv_initializes_count_from_cstr(void) {
    const StringView view = SV_Create("hello");
    if (check_sv_equals(view, "hello"))
        return 1;

    const StringView empty = SV_Create("");
    ASSERT(empty.count == 0U);
    return 0;
}

static int test_sv_chop_left_and_right_handle_bounds(void) {
    StringView view = SV_Create("abcdef");

    SV_ChopLeft(&view, 0U);
    if (check_sv_equals(view, "abcdef"))
        return 1;

    SV_ChopLeft(&view, 2U);
    if (check_sv_equals(view, "cdef"))
        return 1;

    SV_ChopRight(&view, 1U);
    if (check_sv_equals(view, "cde"))
        return 1;

    SV_ChopRight(&view, 100U);
    ASSERT(view.count == 0U);

    StringView exact_left = SV_Create("abc");
    SV_ChopLeft(&exact_left, 3U);
    ASSERT(exact_left.count == 0U);

    StringView exact_right = SV_Create("abc");
    SV_ChopRight(&exact_right, 3U);
    ASSERT(exact_right.count == 0U);

    StringView empty = SV_Create("");
    SV_ChopLeft(&empty, 5U);
    SV_ChopRight(&empty, 5U);
    ASSERT(empty.count == 0U);
    return 0;
}

static int test_sv_trim_left_and_right_remove_whitespace(void) {
    StringView left_trim = SV_Create(" \t\nvalue");
    SV_TrimLeft(&left_trim);
    if (check_sv_equals(left_trim, "value"))
        return 1;

    StringView right_trim = SV_Create("value\t \n");
    SV_TrimRight(&right_trim);
    if (check_sv_equals(right_trim, "value"))
        return 1;

    StringView both_trim = SV_Create(" \t value \n");
    SV_Trim(&both_trim);
    if (check_sv_equals(both_trim, "value"))
        return 1;

    StringView all_whitespace = SV_Create(" \t\n");
    SV_Trim(&all_whitespace);
    ASSERT(all_whitespace.count == 0U);

    StringView already_trimmed = SV_Create("value");
    SV_Trim(&already_trimmed);
    if (check_sv_equals(already_trimmed, "value"))
        return 1;

    StringView empty = SV_Create("");
    SV_TrimLeft(&empty);
    SV_TrimRight(&empty);
    SV_Trim(&empty);
    ASSERT(empty.count == 0U);

    StringView keeps_internal = SV_Create("  a b  ");
    SV_Trim(&keeps_internal);
    if (check_sv_equals(keeps_internal, "a b"))
        return 1;
    return 0;
}

static int test_sv_chop_by_delim_splits_and_updates_input(void) {
    StringView csv = SV_Create("alpha,beta");
    const StringView first = SV_ChopByDelim(&csv, ',');
    if (check_sv_equals(first, "alpha"))
        return 1;
    if (check_sv_equals(csv, "beta"))
        return 1;

    const StringView second = SV_ChopByDelim(&csv, ',');
    if (check_sv_equals(second, "beta"))
        return 1;
    ASSERT(csv.count == 0U);

    StringView starts_with_delim = SV_Create(",next");
    const StringView empty = SV_ChopByDelim(&starts_with_delim, ',');
    ASSERT(empty.count == 0U);
    if (check_sv_equals(starts_with_delim, "next"))
        return 1;

    StringView ends_with_delim = SV_Create("tail,");
    const StringView tail = SV_ChopByDelim(&ends_with_delim, ',');
    if (check_sv_equals(tail, "tail"))
        return 1;
    ASSERT(ends_with_delim.count == 0U);

    StringView repeated = SV_Create("a,,b");
    const StringView r1 = SV_ChopByDelim(&repeated, ',');
    const StringView r2 = SV_ChopByDelim(&repeated, ',');
    const StringView r3 = SV_ChopByDelim(&repeated, ',');
    if (check_sv_equals(r1, "a"))
        return 1;
    ASSERT(r2.count == 0U);
    if (check_sv_equals(r3, "b"))
        return 1;
    ASSERT(repeated.count == 0U);

    StringView no_delim = SV_Create("nodelem");
    const StringView all = SV_ChopByDelim(&no_delim, ',');
    if (check_sv_equals(all, "nodelem"))
        return 1;
    ASSERT(no_delim.count == 0U);

    StringView delim_only = SV_Create(",");
    const StringView only = SV_ChopByDelim(&delim_only, ',');
    ASSERT(only.count == 0U);
    ASSERT(delim_only.count == 0U);

    StringView with_nul = SV_Create("abc");
    const StringView with_nul_result = SV_ChopByDelim(&with_nul, '\0');
    if (check_sv_equals(with_nul_result, "abc"))
        return 1;
    ASSERT(with_nul.count == 0U);

    const StringView exhausted = SV_ChopByDelim(&with_nul, ',');
    ASSERT(exhausted.count == 0U);
    ASSERT(with_nul.count == 0U);
    return 0;
}

static int expect_sv_arg(const size_t count, const char *data, const StringView expected) {
    ASSERT(count == expected.count);
    ASSERT(data == expected.data);
    return 0;
}

static int test_sv_macros_are_defined_as_expected(void) {
    ASSERT(strcmp(SV_FMT, "%.*s") == 0);

    const StringView view = SV_Create("macro");
    if (expect_sv_arg(SV_ARG(view), view))
        return 1;
    return 0;
}

int main(void) {
    int failures = 0;
    RUN_TEST(test_sv_initializes_count_from_cstr);
    RUN_TEST(test_sv_chop_left_and_right_handle_bounds);
    RUN_TEST(test_sv_trim_left_and_right_remove_whitespace);
    RUN_TEST(test_sv_chop_by_delim_splits_and_updates_input);
    RUN_TEST(test_sv_macros_are_defined_as_expected);
    return failures > 0 ? 1 : 0;
}
