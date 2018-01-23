#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>




// H3, SRAM A2
#define BLOCK_1_SIZE    (12 * 4096)
#define BLOCK_1_START   0x00040000

// H3, CPUCFG register
#define BLOCK_2_SIZE    (1 * 4096)
#define BLOCK_2_START   0x01F01C00



#if 0
void delay(unsigned long loops)
{
    __asm__ volatile ("1:\n" "subs %0, %1, #1\n"
              "bne 1b":"=r" (loops):"0"(loops));
}
#endif



int main(int argc, char **argv)
{
    char buf[4096] = {0};
    int mem_fd = 0;
    int src_fd = 0;
    unsigned int vrt_offset[2] = {0};
    unsigned int * vrt_block_addr[2] = {0};
    off_t phy_block_addr[2] = {0};
    unsigned int size = 0;




    // -------------------------------------------------------------------------
    // --- mmaping ---
    // -------------------------------------------------------------------------

    printf("opening and mmaping /dev/mem ..\n");

    // opening devmem
    if ( (mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0 )
    {
       printf("can't open /dev/mem file \n");
       exit(-1);
    }

    // calculate phy memory block start
    vrt_offset[0] = BLOCK_1_START % 4096;
    phy_block_addr[0] = BLOCK_1_START - vrt_offset[0];

    // make a block of phy memory visible in our user space
    vrt_block_addr[0] = mmap(
       NULL,                    // Any adddress in our space
       BLOCK_1_SIZE,            // Map length
       PROT_READ | PROT_WRITE,  // Enable reading & writting to mapped memory
       MAP_SHARED,              // Shared with other processes
       mem_fd,                  // File to map
       phy_block_addr[0]        // PHY offset
    );

    // exit program if mmap is failed
    if (vrt_block_addr[0] == MAP_FAILED)
    {
       printf("mmap error %d\n", (int)vrt_block_addr[0]);//errno also set!
       exit(-1);
    }

    // adjust offset to correct value
    vrt_block_addr[0] += (vrt_offset[0]/4);


    // calculate phy memory block start
    vrt_offset[1] = BLOCK_2_START % 4096;
    phy_block_addr[1] = BLOCK_2_START - vrt_offset[1];

    // make a block of phy memory visible in our user space
    vrt_block_addr[1] = mmap(
       NULL,                    // Any adddress in our space
       BLOCK_2_SIZE,            // Map length
       PROT_READ | PROT_WRITE,  // Enable reading & writting to mapped memory
       MAP_SHARED,              // Shared with other processes
       mem_fd,                  // File to map
       phy_block_addr[1]        // PHY offset
    );

    // exit program if mmap is failed
    if (vrt_block_addr[1] == MAP_FAILED)
    {
       printf("mmap error %d\n", (int)vrt_block_addr[1]);//errno also set!
       exit(-1);
    }

    // adjust offset to correct value
    vrt_block_addr[1] += (vrt_offset[1]/4);

    // no need to keep phy memory file open after mmap
    close(mem_fd);

    printf("mmaping done. \n\n");




    // -------------------------------------------------------------------------
    // --- open ARISC code blob and writing it to the phy memory ---
    // -------------------------------------------------------------------------

    printf("writing to the devmem.. \n");

    // opening blob
    if ( (src_fd = open("./arisc-fw.code", O_RDONLY|O_SYNC) ) < 0 )
    {
       printf("can't open source file \n");
       exit(-1);
    }

    // assert reset for the ar100
    *vrt_block_addr[1] = 0UL;

    // reading from blob file, writing to the devmem
    for ( unsigned short b = 0, len = 0; b < 12; ++b )
    {
        if ( (len = read(src_fd, buf, 4096)) < 0 )
        {
            printf("block %d read failed \n", b);
            break;
        }

        if ( len <= 0 ) break;

        memcpy( (vrt_block_addr[0] + (b*4096)/4), buf, len );

        size += len;
    }

    // de-assert reset for the ar100
    *vrt_block_addr[1] = 1UL;

    close(src_fd);

    printf("writing done. \n\n");




    // -------------------------------------------------------------------------
    // --- unmmaping ---
    // -------------------------------------------------------------------------

    printf("unmmaping /dev/mem .. \n");

    munmap(vrt_block_addr[0], BLOCK_1_SIZE);
    munmap(vrt_block_addr[1], BLOCK_2_SIZE);

    printf("unmmaping done.\n\n");




    // -------------------------------------------------------------------------
    // --- exit --
    // -------------------------------------------------------------------------

    printf("all work is done.\n");

    return 0;
}
