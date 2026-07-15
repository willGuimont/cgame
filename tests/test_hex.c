#include "../src/cgame/coords/hex.h"
#include "test_utils.h"

static bool near_f32(const f32 a, const f32 b) { return fabsf(a - b) < 0.0001f; }

static int test_hex_create_and_equals(void) {
    const Hex a = Hex_Create(2, -3);
    const Hex b = {2, -3};
    const Hex c = {2, -2};

    ASSERT(Hex_Equals(a, b));
    ASSERT(!Hex_Equals(a, c));
    return 0;
}

static int test_hex_cube_conversion(void) {
    const Hex hex = Hex_Create(3, -5);
    const Cube cube = Hex_ToCube(hex);

    ASSERT(cube.q == 3);
    ASSERT(cube.r == -5);
    ASSERT(cube.s == 2);
    ASSERT(cube.q + cube.r + cube.s == 0);
    ASSERT(Cube_IsValid(cube));
    ASSERT(!Cube_IsValid((Cube) {.q = 1, .r = 2, .s = 3}));
    ASSERT(Hex_Equals(Hex_FromCube(cube), hex));
    return 0;
}

static int test_hex_arithmetic(void) {
    const Hex a = Hex_Create(3, -2);
    const Hex b = Hex_Create(-1, 4);

    ASSERT(Hex_Equals(Hex_Add(a, b), Hex_Create(2, 2)));
    ASSERT(Hex_Equals(Hex_Subtract(a, b), Hex_Create(4, -6)));
    ASSERT(Hex_Equals(Hex_Multiply(b, 3), Hex_Create(-3, 12)));
    return 0;
}

static int test_hex_length_and_direction_helpers(void) {
    ASSERT(Hex_Length(Hex_Create(0, 0)) == 0);
    ASSERT(Hex_Length(Hex_Create(3, -5)) == 5);

    ASSERT(Hex_Equals(Hex_Direction(0), Hex_Create(1, 0)));
    ASSERT(Hex_Equals(Hex_Direction(6), Hex_Create(1, 0)));
    ASSERT(Hex_Equals(Hex_Direction(-1), Hex_Create(0, 1)));
    ASSERT(Hex_Equals(Hex_DiagonalDirection(0), Hex_Create(2, -1)));
    ASSERT(Hex_Equals(Hex_DiagonalDirection(-1), Hex_Create(1, 1)));
    return 0;
}

static int test_hex_distance(void) {
    ASSERT(Hex_Distance(Hex_Create(0, 0), Hex_Create(0, 0)) == 0);
    ASSERT(Hex_Distance(Hex_Create(0, 0), Hex_Create(1, 0)) == 1);
    ASSERT(Hex_Distance(Hex_Create(0, 0), Hex_Create(2, -1)) == 2);
    ASSERT(Hex_Distance(Hex_Create(3, -5), Hex_Create(-2, 4)) == 9);
    ASSERT(Hex_Distance(Hex_Create(-2, 4), Hex_Create(3, -5)) == 9);
    return 0;
}

static int test_hex_rotations(void) {
    const Hex hex = Hex_Create(1, -3);

    ASSERT(Hex_Equals(Hex_RotateLeft(hex), Hex_Create(-2, -1)));
    ASSERT(Hex_Equals(Hex_RotateRight(hex), Hex_Create(3, -2)));

    Hex left = hex;
    Hex right = hex;
    for (int i = 0; i < 6; i++) {
        left = Hex_RotateLeft(left);
        right = Hex_RotateRight(right);
    }
    ASSERT(Hex_Equals(left, hex));
    ASSERT(Hex_Equals(right, hex));
    return 0;
}

static int test_hex_lerp_and_round(void) {
    ASSERT(Hex_Equals(Hex_Round((HexFractional) {.q = 0.0f, .r = 0.0f}), Hex_Create(0, 0)));
    ASSERT(Hex_Equals(Hex_Round((HexFractional) {.q = 0.6f, .r = -1.2f}), Hex_Create(1, -1)));

    const HexFractional midpoint = Hex_Lerp(Hex_Create(0, 0), Hex_Create(4, -2), 0.5f);
    ASSERT(near_f32(midpoint.q, 2.0f));
    ASSERT(near_f32(midpoint.r, -1.0f));
    ASSERT(Hex_Equals(Hex_Round(midpoint), Hex_Create(2, -1)));
    return 0;
}

static int test_hex_neighbors_pointy_top_order(void) {
    const Hex origin = Hex_Create(0, 0);

    ASSERT(Hex_Equals(Hex_Neighbor(origin, 0), Hex_Create(1, 0)));
    ASSERT(Hex_Equals(Hex_Neighbor(origin, 1), Hex_Create(1, -1)));
    ASSERT(Hex_Equals(Hex_Neighbor(origin, 2), Hex_Create(0, -1)));
    ASSERT(Hex_Equals(Hex_Neighbor(origin, 3), Hex_Create(-1, 0)));
    ASSERT(Hex_Equals(Hex_Neighbor(origin, 4), Hex_Create(-1, 1)));
    ASSERT(Hex_Equals(Hex_Neighbor(origin, 5), Hex_Create(0, 1)));
    return 0;
}

static int test_hex_diagonals_pointy_top_order(void) {
    const Hex origin = Hex_Create(0, 0);

    ASSERT(Hex_Equals(Hex_Diagonal(origin, 0), Hex_Create(2, -1)));
    ASSERT(Hex_Equals(Hex_Diagonal(origin, 1), Hex_Create(1, -2)));
    ASSERT(Hex_Equals(Hex_Diagonal(origin, 2), Hex_Create(-1, -1)));
    ASSERT(Hex_Equals(Hex_Diagonal(origin, 3), Hex_Create(-2, 1)));
    ASSERT(Hex_Equals(Hex_Diagonal(origin, 4), Hex_Create(-1, 2)));
    ASSERT(Hex_Equals(Hex_Diagonal(origin, 5), Hex_Create(1, 1)));
    return 0;
}

static int test_hex_ring(void) {
    Hex out[6] = {0};

    ASSERT(Hex_RingCount(-1) == 0U);
    ASSERT(Hex_RingCount(0) == 1U);
    ASSERT(Hex_RingCount(1) == 6U);

    ASSERT(Hex_Ring(Hex_Create(2, 3), 0, out, 6) == 1U);
    ASSERT(Hex_Equals(out[0], Hex_Create(2, 3)));

    ASSERT(Hex_Ring(Hex_Create(0, 0), 1, out, 6) == 6U);
    ASSERT(Hex_Equals(out[0], Hex_Create(-1, 1)));
    ASSERT(Hex_Equals(out[1], Hex_Create(0, 1)));
    ASSERT(Hex_Equals(out[2], Hex_Create(1, 0)));
    ASSERT(Hex_Equals(out[3], Hex_Create(1, -1)));
    ASSERT(Hex_Equals(out[4], Hex_Create(0, -1)));
    ASSERT(Hex_Equals(out[5], Hex_Create(-1, 0)));
    return 0;
}

static int test_hex_spiral(void) {
    Hex out[7] = {0};

    ASSERT(Hex_SpiralCount(-1) == 0U);
    ASSERT(Hex_SpiralCount(0) == 1U);
    ASSERT(Hex_SpiralCount(1) == 7U);

    ASSERT(Hex_Spiral(Hex_Create(0, 0), 1, out, 7) == 7U);
    ASSERT(Hex_Equals(out[0], Hex_Create(0, 0)));
    ASSERT(Hex_Equals(out[1], Hex_Create(-1, 1)));
    ASSERT(Hex_Equals(out[6], Hex_Create(-1, 0)));
    return 0;
}

static int test_hex_range_helpers_return_required_count_for_partial_buffers(void) {
    Hex out[2] = {0};

    ASSERT(Hex_RingCount(2) == 12U);
    ASSERT(Hex_Ring(Hex_Create(0, 0), 2, out, 2) == 2U);
    ASSERT(Hex_Equals(out[0], Hex_Create(-2, 2)));
    ASSERT(Hex_Equals(out[1], Hex_Create(-1, 2)));

    ASSERT(Hex_SpiralCount(2) == 19U);
    ASSERT(Hex_Spiral(Hex_Create(0, 0), 2, out, 2) == 2U);
    ASSERT(Hex_Equals(out[0], Hex_Create(0, 0)));
    ASSERT(Hex_Equals(out[1], Hex_Create(-1, 1)));
    return 0;
}

static int test_hex_line_draw(void) {
    Hex out[4] = {0};

    ASSERT(Hex_LineDrawCount(Hex_Create(0, 0), Hex_Create(3, -3)) == 4U);
    ASSERT(Hex_LineDraw(Hex_Create(0, 0), Hex_Create(3, -3), out, 4) == 4U);
    ASSERT(Hex_Equals(out[0], Hex_Create(0, 0)));
    ASSERT(Hex_Equals(out[1], Hex_Create(1, -1)));
    ASSERT(Hex_Equals(out[2], Hex_Create(2, -2)));
    ASSERT(Hex_Equals(out[3], Hex_Create(3, -3)));

    Hex partial[1] = {0};
    ASSERT(Hex_LineDraw(Hex_Create(0, 0), Hex_Create(3, -3), partial, 1) == 1U);
    ASSERT(Hex_Equals(partial[0], Hex_Create(0, 0)));
    return 0;
}

static int test_hex_pointy_pixel_conversions(void) {
    const HexLayout layout = {.size = 10.0f, .origin = {.x = 100.0f, .y = 50.0f}};
    const Hex hex = Hex_Create(2, -1);
    const HexPoint pixel = Hex_PointyToPixel(layout, hex);

    ASSERT(near_f32(pixel.x, 125.98076f));
    ASSERT(near_f32(pixel.y, 35.0f));
    ASSERT(Hex_Equals(Hex_PointyFromPixel(layout, pixel), hex));

    const HexPoint corner = Hex_PointyCornerOffset(layout, 0);
    ASSERT(near_f32(corner.x, 8.660254f));
    ASSERT(near_f32(corner.y, -5.0f));
    return 0;
}

static int test_hex_flat_pixel_conversions(void) {
    const HexLayout layout = {.orientation = HEX_ORIENTATION_FLAT, .size = 10.0f, .origin = {.x = 100.0f, .y = 50.0f}};
    const Hex hex = Hex_Create(2, -1);
    const HexPoint pixel = Hex_FlatToPixel(layout, hex);

    ASSERT(near_f32(pixel.x, 130.0f));
    ASSERT(near_f32(pixel.y, 50.0f));
    ASSERT(Hex_Equals(Hex_FlatFromPixel(layout, pixel), hex));

    const HexPoint corner = Hex_FlatCornerOffset(layout, 0);
    ASSERT(near_f32(corner.x, 10.0f));
    ASSERT(near_f32(corner.y, 0.0f));

    const HexPoint corner1 = Hex_FlatCornerOffset(layout, 1);
    ASSERT(near_f32(corner1.x, 5.0f));
    ASSERT(near_f32(corner1.y, 8.660254f));

    return 0;
}

static int test_hex_generic_pixel_conversions(void) {
    const HexLayout layout_pointy = {
            .orientation = HEX_ORIENTATION_POINTY, .size = 10.0f, .origin = {.x = 100.0f, .y = 50.0f}};
    const HexLayout layout_flat = {
            .orientation = HEX_ORIENTATION_FLAT, .size = 10.0f, .origin = {.x = 100.0f, .y = 50.0f}};
    const Hex hex = Hex_Create(2, -1);

    // Test Pointy via generic
    const HexPoint pixel_p = Hex_ToPixel(layout_pointy, hex);
    ASSERT(near_f32(pixel_p.x, 125.98076f));
    ASSERT(near_f32(pixel_p.y, 35.0f));
    ASSERT(Hex_Equals(Hex_FromPixel(layout_pointy, pixel_p), hex));

    const HexPoint corner_p = Hex_CornerOffset(layout_pointy, 0);
    ASSERT(near_f32(corner_p.x, 8.660254f));
    ASSERT(near_f32(corner_p.y, -5.0f));

    // Test Flat via generic
    const HexPoint pixel_f = Hex_ToPixel(layout_flat, hex);
    ASSERT(near_f32(pixel_f.x, 130.0f));
    ASSERT(near_f32(pixel_f.y, 50.0f));
    ASSERT(Hex_Equals(Hex_FromPixel(layout_flat, pixel_f), hex));

    const HexPoint corner_f = Hex_CornerOffset(layout_flat, 0);
    ASSERT(near_f32(corner_f.x, 10.0f));
    ASSERT(near_f32(corner_f.y, 0.0f));

    return 0;
}

static int test_hex_direction_indices_wrap(void) {
    const Hex origin = Hex_Create(0, 0);

    ASSERT(Hex_Equals(Hex_Neighbor(origin, 6), Hex_Neighbor(origin, 0)));
    ASSERT(Hex_Equals(Hex_Neighbor(origin, -1), Hex_Neighbor(origin, 5)));
    ASSERT(Hex_Equals(Hex_Diagonal(origin, 8), Hex_Diagonal(origin, 2)));
    ASSERT(Hex_Equals(Hex_Diagonal(origin, -2), Hex_Diagonal(origin, 4)));
    return 0;
}

int main(void) {
    int failures = 0;
    RUN_TEST(test_hex_create_and_equals);
    RUN_TEST(test_hex_cube_conversion);
    RUN_TEST(test_hex_arithmetic);
    RUN_TEST(test_hex_length_and_direction_helpers);
    RUN_TEST(test_hex_distance);
    RUN_TEST(test_hex_rotations);
    RUN_TEST(test_hex_lerp_and_round);
    RUN_TEST(test_hex_neighbors_pointy_top_order);
    RUN_TEST(test_hex_diagonals_pointy_top_order);
    RUN_TEST(test_hex_ring);
    RUN_TEST(test_hex_spiral);
    RUN_TEST(test_hex_range_helpers_return_required_count_for_partial_buffers);
    RUN_TEST(test_hex_line_draw);
    RUN_TEST(test_hex_pointy_pixel_conversions);
    RUN_TEST(test_hex_flat_pixel_conversions);
    RUN_TEST(test_hex_generic_pixel_conversions);
    RUN_TEST(test_hex_direction_indices_wrap);
    return failures > 0 ? 1 : 0;
}
