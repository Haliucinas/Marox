#ifndef MAROX_ASSERT_H
#define MAROX_ASSERT_H

#include "marox.h"
#include "util.h"

#ifndef NDEBUG

#define KASSERT(cond)                                   \
do {                                                    \
    if (!(cond)) {                                        \
        kprintf("Failed Assertion: %s at %s:%s:%d\n",   \
                #cond, __FILE__, __func__, __LINE__);   \
        khalt();                                        \
    }                                                   \
} while (0)

#else /* NDEBUG */

#define KASSERT(cond)

#endif /* NDEBUG */

#endif /* MAROX_ASSERT_H */
