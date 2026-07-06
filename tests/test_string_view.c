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
    const StringView view = StringView_Create("hello");
    if (check_sv_equals(view, "hello"))
        return 1;

    const StringView empty = StringView_Create("");
    ASSERT(empty.count == 0U);
    return 0;
}

static int test_sv_chop_left_and_right_handle_bounds(void) {
    StringView view = StringView_Create("abcdef");

    StringView_ChopLeft(&view, 0U);
    if (check_sv_equals(view, "abcdef"))
        return 1;

    StringView_ChopLeft(&view, 2U);
    if (check_sv_equals(view, "cdef"))
        return 1;

    StringView_ChopRight(&view, 1U);
    if (check_sv_equals(view, "cde"))
        return 1;

    StringView_ChopRight(&view, 100U);
    ASSERT(view.count == 0U);

    StringView exact_left = StringView_Create("abc");
    StringView_ChopLeft(&exact_left, 3U);
    ASSERT(exact_left.count == 0U);

    StringView exact_right = StringView_Create("abc");
    StringView_ChopRight(&exact_right, 3U);
    ASSERT(exact_right.count == 0U);

    StringView empty = StringView_Create("");
    StringView_ChopLeft(&empty, 5U);
    StringView_ChopRight(&empty, 5U);
    ASSERT(empty.count == 0U);
    return 0;
}

static int test_sv_trim_left_and_right_remove_whitespace(void) {
    StringView left_trim = StringView_Create(" \t\nvalue");
    StringView_TrimLeft(&left_trim);
    if (check_sv_equals(left_trim, "value"))
        return 1;

    StringView right_trim = StringView_Create("value\t \n");
    StringView_TrimRight(&right_trim);
    if (check_sv_equals(right_trim, "value"))
        return 1;

    StringView both_trim = StringView_Create(" \t value \n");
    StringView_Trim(&both_trim);
    if (check_sv_equals(both_trim, "value"))
        return 1;

    StringView all_whitespace = StringView_Create(" \t\n");
    StringView_Trim(&all_whitespace);
    ASSERT(all_whitespace.count == 0U);

    StringView already_trimmed = StringView_Create("value");
    StringView_Trim(&already_trimmed);
    if (check_sv_equals(already_trimmed, "value"))
        return 1;

    StringView empty = StringView_Create("");
    StringView_TrimLeft(&empty);
    StringView_TrimRight(&empty);
    StringView_Trim(&empty);
    ASSERT(empty.count == 0U);

    StringView keeps_internal = StringView_Create("  a b  ");
    StringView_Trim(&keeps_internal);
    if (check_sv_equals(keeps_internal, "a b"))
        return 1;
    return 0;
}

static int test_sv_chop_by_delim_splits_and_updates_input(void) {
    StringView csv = StringView_Create("alpha,beta");
    const StringView first = StringView_ChopByDelim(&csv, ',');
    if (check_sv_equals(first, "alpha"))
        return 1;
    if (check_sv_equals(csv, "beta"))
        return 1;

    const StringView second = StringView_ChopByDelim(&csv, ',');
    if (check_sv_equals(second, "beta"))
        return 1;
    ASSERT(csv.count == 0U);

    StringView starts_with_delim = StringView_Create(",next");
    const StringView empty = StringView_ChopByDelim(&starts_with_delim, ',');
    ASSERT(empty.count == 0U);
    if (check_sv_equals(starts_with_delim, "next"))
        return 1;

    StringView ends_with_delim = StringView_Create("tail,");
    const StringView tail = StringView_ChopByDelim(&ends_with_delim, ',');
    if (check_sv_equals(tail, "tail"))
        return 1;
    ASSERT(ends_with_delim.count == 0U);

    StringView repeated = StringView_Create("a,,b");
    const StringView r1 = StringView_ChopByDelim(&repeated, ',');
    const StringView r2 = StringView_ChopByDelim(&repeated, ',');
    const StringView r3 = StringView_ChopByDelim(&repeated, ',');
    if (check_sv_equals(r1, "a"))
        return 1;
    ASSERT(r2.count == 0U);
    if (check_sv_equals(r3, "b"))
        return 1;
    ASSERT(repeated.count == 0U);

    StringView no_delim = StringView_Create("nodelem");
    const StringView all = StringView_ChopByDelim(&no_delim, ',');
    if (check_sv_equals(all, "nodelem"))
        return 1;
    ASSERT(no_delim.count == 0U);

    StringView delim_only = StringView_Create(",");
    const StringView only = StringView_ChopByDelim(&delim_only, ',');
    ASSERT(only.count == 0U);
    ASSERT(delim_only.count == 0U);

    StringView with_nul = StringView_Create("abc");
    const StringView with_nul_result = StringView_ChopByDelim(&with_nul, '\0');
    if (check_sv_equals(with_nul_result, "abc"))
        return 1;
    ASSERT(with_nul.count == 0U);

    const StringView exhausted = StringView_ChopByDelim(&with_nul, ',');
    ASSERT(exhausted.count == 0U);
    ASSERT(with_nul.count == 0U);
    return 0;
}

static int expect_sv_arg(const int count, const char *data, const StringView expected) {
    ASSERT(count >= 0);
    ASSERT((size_t) count == expected.count);
    ASSERT(data == expected.data);
    return 0;
}

static int test_sv_macros_are_defined_as_expected(void) {
    ASSERT(strcmp(STRING_VIEW_FMT, "%.*s") == 0);

    const StringView view = StringView_Create("macro");
    if (expect_sv_arg(STRING_VIEW_ARG(view), view))
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
