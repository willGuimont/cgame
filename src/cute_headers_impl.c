#define _GNU_SOURCE

#define CUTE_ASEPRITE_IMPLEMENTATION
#define CUTE_C2_IMPLEMENTATION
#define CUTE_PNG_IMPLEMENTATION
#define SPRITEBATCH_IMPLEMENTATION
#define CUTE_TILED_IMPLEMENTATION

#ifndef __EMSCRIPTEN__
#define CUTE_NET_IMPLEMENTATION
#define CUTE_SOUND_IMPLEMENTATION
#define CUTE_SYNC_IMPLEMENTATION

#ifdef _WIN32
#define CUTE_SYNC_WINDOWS
#elifdef CUTE_SYNC_SDL
/* Use the SDL backend when the including build explicitly selects it. */
#else
#define CUTE_SYNC_POSIX
#endif
#endif

#if !defined(__EMSCRIPTEN__) &&                                                                                        \
        (defined(_WIN32) || defined(__APPLE__) || defined(__ANDROID__) || defined(CGAME_CUTE_TLS_S2N))
#define CUTE_TLS_IMPLEMENTATION
#endif

#ifdef CGAME_CUTE_TLS_S2N
#define CUTE_TLS_S2N
#endif

typedef struct cute_tiled_chunk_t cute_tiled_chunk_t;

#include <cute_aseprite.h>
#include <cute_c2.h>
#include <cute_png.h>
#include <cute_spritebatch.h>
#include <cute_tiled.h>
#include <cute_tls.h>

#ifndef __EMSCRIPTEN__
#include <cute_net.h>
#include <cute_sound.h>
#include <cute_sync.h>
#endif
