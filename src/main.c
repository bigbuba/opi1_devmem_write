#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define SETUP_CLOCK

#if defined SETUP_CLOCK
#include "io.h"
#include "clk.h"
#endif



// H3, SRAM A2
#define SRAM_A2_SIZE        (12 * 4096)
#define SRAM_A2_ADDR        0x00040000

// H3, ARISC params
#define ARISC_PARA_SIZE     2048
#define ARISC_PARA_OFFSET   (SRAM_A2_SIZE - ARISC_PARA_SIZE)

// H3, CPUCFG register
#define CPUCFG_SIZE         (1 * 4096)
#define CPUCFG_ADDR         0x01F01C00




int main(int argc, char **argv)
{
    char buf[4096] = {0};
    int mem_fd = 0;
    int src_fd = 0;

#if defined SETUP_CLOCK
    unsigned int vrt_offset[4] = {0};
    unsigned int * vrt_block_addr[4] = {0};
    off_t phy_block_addr[4] = {0};
#else
    unsigned int vrt_offset[2] = {0};
    unsigned int * vrt_block_addr[2] = {0};
    off_t phy_block_addr[2] = {0};
#endif



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
    vrt_offset[0] = SRAM_A2_ADDR % 4096;
    phy_block_addr[0] = SRAM_A2_ADDR - vrt_offset[0];

    // make a block of phy memory visible in our user space
    vrt_block_addr[0] = mmap(
       NULL,                    // Any adddress in our space
       SRAM_A2_SIZE,            // Map length
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
    vrt_offset[1] = CPUCFG_ADDR % 4096;
    phy_block_addr[1] = CPUCFG_ADDR - vrt_offset[1];

    // make a block of phy memory visible in our user space
    vrt_block_addr[1] = mmap(
       NULL,                    // Any adddress in our space
       CPUCFG_SIZE,            // Map length
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

#if defined SETUP_CLOCK
    // calculate phy memory block start
    vrt_offset[2] = CCM_BASE % 4096;
    phy_block_addr[2] = CCM_BASE - vrt_offset[2];

    // make a block of phy memory visible in our user space
    vrt_block_addr[2] = mmap(
       NULL,                    // Any adddress in our space
       CCM_SIZE,                // Map length
       PROT_READ | PROT_WRITE,  // Enable reading & writting to mapped memory
       MAP_SHARED,              // Shared with other processes
       mem_fd,                  // File to map
       phy_block_addr[2]        // PHY offset
    );

    // exit program if mmap is failed
    if (vrt_block_addr[2] == MAP_FAILED)
    {
       printf("mmap error %d\n", (int)vrt_block_addr[2]);//errno also set!
       exit(-1);
    }

    // adjust offset to correct value
    vrt_block_addr[2] += (vrt_offset[2]/4);


    // calculate phy memory block start
    vrt_offset[3] = R_PRCM_BASE % 4096;
    phy_block_addr[3] = R_PRCM_BASE - vrt_offset[3];

    // make a block of phy memory visible in our user space
    vrt_block_addr[3] = mmap(
       NULL,                    // Any adddress in our space
       R_PRCM_SIZE,             // Map length
       PROT_READ | PROT_WRITE,  // Enable reading & writting to mapped memory
       MAP_SHARED,              // Shared with other processes
       mem_fd,                  // File to map
       phy_block_addr[3]        // PHY offset
    );

    // exit program if mmap is failed
    if (vrt_block_addr[3] == MAP_FAILED)
    {
       printf("mmap error %d\n", (int)vrt_block_addr[3]);//errno also set!
       exit(-1);
    }

    // adjust offset to correct value
    vrt_block_addr[3] += (vrt_offset[3]/4);
#endif

    // no need to keep phy memory file open after mmap
    close(mem_fd);

    printf("mmaping done. \n\n");




    // -------------------------------------------------------------------------
    // --- assert reset for the ar100 ---
    // -------------------------------------------------------------------------

    *vrt_block_addr[1] = 0UL;




#if defined SETUP_CLOCK
    // -------------------------------------------------------------------------
    // --- enable clocking for ARISC core ---
    // -------------------------------------------------------------------------

#if 1
    // Set PLL6 to 300MHz
    unsigned int reg = *(vrt_block_addr[2] + PLL6_CTRL_REG);
    SET_BITS_AT(reg, 2, 0, 1); // M = 1
    SET_BITS_AT(reg, 2, 4, 1); // K = 1
    SET_BITS_AT(reg, 5, 8, 24); // N = 24
    SET_BITS_AT(reg, 2, 16, 0); // POSTDIV = 0
    reg |= PLL6_CTRL_ENABLE | PLL6_CTRL_CLK_OUTEN;
    *(vrt_block_addr[2] + PLL6_CTRL_REG) = reg;

    // Switch AR100 to PLL6
    reg = *(vrt_block_addr[3] + AR100_CLKCFG_REG);
    reg &= ~AR100_CLKCFG_SRC_MASK;
    reg |= AR100_CLKCFG_SRC_PLL6;
    reg &= ~AR100_CLKCFG_POSTDIV_MASK;
    reg |= AR100_CLKCFG_POSTDIV(1);
    reg &= ~AR100_CLKCFG_DIV_MASK;
    reg |= AR100_CLKCFG_DIV(0);
    *(vrt_block_addr[3] + AR100_CLKCFG_REG) = reg;
#else
    // Switch AR100 to LOSC
    unsigned int reg = *(vrt_block_addr[3] + AR100_CLKCFG_REG);
    reg &= ~AR100_CLKCFG_SRC_MASK;
    reg |= AR100_CLKCFG_SRC_LOSC;
    *(vrt_block_addr[3] + AR100_CLKCFG_REG) = reg;
#endif
#endif




    // -------------------------------------------------------------------------
    // --- clean up SRAM A2 block ---
    // -------------------------------------------------------------------------

    memset(vrt_block_addr[0], 0, SRAM_A2_SIZE);




    // -------------------------------------------------------------------------
    // --- open ARISC code blob and writing it to the phy memory ---
    // -------------------------------------------------------------------------

    printf("writing code blob to the devmem.. \n");

    // opening blob
    if ( (src_fd = open("./arisc-fw.code", O_RDONLY|O_SYNC) ) < 0 )
    {
       printf("can't open code blob file \n");
       exit(-1);
    }

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
    }

    close(src_fd);

    printf("code writing done. \n\n");




    // -------------------------------------------------------------------------
    // --- open ARISC parameters blob and writing it to the phy memory ---
    // -------------------------------------------------------------------------

    printf("writing ARISC parameters to the devmem.. \n");

    // opening blob
    if ( (src_fd = open("./arisc-fw.para", O_RDONLY|O_SYNC) ) < 0 )
    {
       printf("can't open parameters source file \n");
       exit(-1);
    }

    // reading from blob file
    if ( read(src_fd, buf, ARISC_PARA_SIZE) < 0 )
    {
        printf("parameters file read failed \n");
        exit(-1);
    }

    // writing to the devmem
    memcpy( (vrt_block_addr[0] + ARISC_PARA_OFFSET/4), buf, ARISC_PARA_SIZE );

    close(src_fd);

    printf("writing parameters done. \n\n");




    // -------------------------------------------------------------------------
    // --- de-assert reset for the ar100 ---
    // -------------------------------------------------------------------------

    *vrt_block_addr[1] = 1UL;




    // -------------------------------------------------------------------------
    // --- unmmaping ---
    // -------------------------------------------------------------------------

    printf("unmmaping /dev/mem .. \n");

    munmap(vrt_block_addr[0], SRAM_A2_SIZE);
    munmap(vrt_block_addr[1], CPUCFG_SIZE);

#if defined SETUP_CLOCK
    munmap(vrt_block_addr[2], CCM_SIZE);
    munmap(vrt_block_addr[3], R_PRCM_SIZE);
#endif

    printf("unmmaping done.\n\n");




    // -------------------------------------------------------------------------
    // --- exit --
    // -------------------------------------------------------------------------

    printf("all work is done.\n");

    return 0;
}
