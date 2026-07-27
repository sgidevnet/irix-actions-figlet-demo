/*
 * __BEGIN_DECLS and __END_DECLS come from glibc's <sys/cdefs.h>. They are not
 * standard, IRIX has no equivalent header, and a great deal of Unix software
 * assumes them anyway. figlet's utf8.h is one such case: without these the
 * declarations inside it never parse, and the first visible symptom is an
 * implicit declaration of utf8_to_wchar several hundred lines away.
 *
 * Forced ahead of every translation unit with -include, so no upstream file
 * needs editing.
 */
#ifndef IRIX_CDEFS_SHIM_H
#define IRIX_CDEFS_SHIM_H

#ifndef __BEGIN_DECLS
#define __BEGIN_DECLS
#define __END_DECLS
#endif

/* IRIX declares alloca here rather than in stdlib.h. */
#include <alloca.h>

#endif /* IRIX_CDEFS_SHIM_H */
