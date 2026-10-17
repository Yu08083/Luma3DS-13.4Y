#include <stdlib.h>
#include "theme_image.h"

#define PNGLE_NO_GAMMA_CORRECTION
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_STDIO
#define MINIZ_NO_MALLOC
#include "../../../../third_party/theme/miniz.c"
#define calloc ThemeImage_Calloc
#define free ThemeImage_Free
#include "../../../../third_party/theme/pngle.c"
#undef calloc
#undef free
#include "../../../../third_party/theme/picojpeg.c"
