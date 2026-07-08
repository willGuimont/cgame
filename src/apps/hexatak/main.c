#include "app.h"

#ifdef __SANITIZE_ADDRESS__
const char *__lsan_default_options(void) { return "suppressions=" ROOT_DIR "/asan_suppressions.txt"; }
#endif

i32 main(void) { return App_Run(); }
