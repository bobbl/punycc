/* The first 16 bytes of memory are reserved for this functions

   offset length name
   0000   4      iov.iov_base   pointer to buffer
   0004   4      iov.iov_len    length of buffer
   0008   4      nwritten       number of bytes written/read
   000C   4      buf            read buffer for getchar
*/


void *malloc(unsigned long) _Pragma("PunyC emit \x01\x00\x20\x00\x41\xff\xff\x03\x6a\x41\x10\x76\x40\x00\x41\x10\x74");
/* Very simple memory allocation: allocate a whole 64 KiByte page on every call.
   Or multiple pages if the allocated size is larger than 64 KiByte.

    01          1 argument      // void *malloc(unsigned long nbytes)
    00          no locals

    20 00       local.get 0     // npages = (nbytes+0xFFFF) >> 16
    41 FF FF 03 i32.const 0xFFFF
    6A          i32.add
    41 10       i32.const 16
    76          i32.shr_u

    40 00       memory.grow     // allocate the computed number of pages

    41 10       i32.const 16    // return ((previous memory size) << 16)
    74          i32.shl
*/


void exit(int) _Pragma("PunyC emit \x01\x00\x20\x00\x10\x02\x41\x00");
/*
    01          1 argument      // void exit(int exitcode)
    00          no locals

    20 00       local.get 0     // exitcode
    10 02       call 2          // proc_exit()

                                // unreachable, but neccessary to avoid error:
    41 00       i32.const 0     // return 0
*/


int write(int, char *, int) _Pragma("PunyC emit \x03\x00\x41\x00\x20\x01\x36\x02\x00\x41\x04\x20\x02\x36\x02\x00\x20\x00\x41\x00\x41\x01\x41\x08\x10\x01");
/*
    03          3 arguments     // int write(int fd, char *buf, int len)
    00          no locals

    41 00       i32.const 0
    20 01       local.get 1
    36 02 00    i32.store       // iov.iov_base = buf
    41 04       i32.const 4
    20 02       local.get 2
    36 02 00    i32.store       // iov.iov_len  = len
    20 00       local.get 0     // fd           file descriptor
    41 00       i32.const 0     // &iovs        pointer to an array at memory location 0
    41 01       i32.const 1     // iovs_len     iovs has only one entry
    41 08       i32.const 8     // &nwritten    address where the number of 
                                                writtem bytes is stores
    10 01       call 1          // fd_write()
*/


int putchar(int) _Pragma("PunyC emit \x01\x00\x41\x00\x41\x0c\x36\x02\x00\x41\x04\x41\x01\x36\x02\x00\x41\x0c\x20\x00\x3a\x00\x00\x41\x01\x41\x00\x41\x01\x41\x08\x10\x01\x1a\x20\x00");
/*
    (func $putchar (param i32) (result i32)
        ;; Creating a new io vector within linear memory
        (i32.store (i32.const 0) (i32.const 12))  ;; iov.iov_base = buf
        (i32.store (i32.const 4) (i32.const 1))   ;; iov.iov_len  = 1

        (i32.store8 (i32.const 12) (local.get 0)) ;; buf = ch

        (call $fd_write
            (i32.const 1) ;; file_descriptor - 1 for stdout
            (i32.const 0) ;; *iovs - The pointer to the iov array, which is stored at memory location 0
            (i32.const 1) ;; iovs_len - We're printing 1 string stored in an iov - so one.
            (i32.const 8) ;; nwritten - A place in memory to store the number of bytes written
        )
        drop            ;; discard result of $fd_write
        (local.get 0)   ;; return ch
    )
*/

int getchar() _Pragma("PunyC emit \x00\x01\x01\x7f\x41\x00\x41\x0c\x36\x02\x00\x41\x04\x41\x01\x36\x02\x00\x41\x00\x41\x00\x41\x01\x41\x08\x10\x00\x21\x00\x41\x08\x28\x02\x00\x41\x01\x46\x04\x7f\x41\x0c\x2d\x00\x00\x05\x41\x7f\x0b");
/*
 00             no parameters
 01             1 local variable
 01 7f          local i32
 41 00          i32.const 0
 41 0c          i32.const 12
 36 02 00       i32.store
 41 04
 41 01
 36 02 00
 41 00
 41 00
 41 01
 41 08
 10 00     call 0
 21 00     local.set 0
 41 08     i32.const 8
 28 02 00  i32.load
 41 01     i32.const 1
 46        i32.eq
 04 7f     if (result i32)
 41 0c       i32.const 12
 2d 00 00    i32.load8_u
 05        else
 41 7f     i32.const -1
 0b
*/

/*
    (func $getchar (result i32)
        (local i32)

        ;; Creating a new io vector within linear memory
        (i32.store (i32.const 0) (i32.const 12))  ;; iov.iov_base = buf
        (i32.store (i32.const 4) (i32.const 1))   ;; iov.iov_len  = 1

        ;; r = fd_read
        (local.set 0
            (call $fd_read
                (i32.const 0) ;; file_descriptor - 0 for stdin
                (i32.const 0) ;; *iovs - The pointer to the iov array, which is stored at memory location 0
                (i32.const 1) ;; iovs_len - We're printing 1 string stored in an iov - so one.
                (i32.const 8) ;; nwritten - A place in memory to store the number of bytes written
            )
        )

        ;; return (r==1) ? ch : -1
        (if (result i32)
            (i32.eq (i32.load (i32.const 8)) (i32.const 1))
            (then (i32.load8_u (i32.const 12)))
            (else (i32.const -1))
        )
    )
*/
