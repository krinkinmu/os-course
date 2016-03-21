#ifndef __ERROR_H__
#define __ERROR_H__

#define ENOMEM  1
#define ENOENT  2
#define ENOTSUP 3
#define EBUSY   4
#define EEXIST  5
#define EINVAL  6
#define EIO     7
#define ENOEXEC 8
#define ENOSYS  9

#ifndef __ASM_FILE__

const char *errstr(int errc);

#endif /*__ASM_FILE__*/

#endif /*__ERROR_H__*/
