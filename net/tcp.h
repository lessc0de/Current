#include "../port.h"

#if defined(BRICKS_POSIX) || defined(BRICKS_APPLE)
#include "tcp/impl/posix.h"
#else
#error "No TCP implementation available for your platform."
#endif
