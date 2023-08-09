#ifndef _SMEM_UTILS_H
#define _SMEM_UTILS_H

int spi_flash_program_pdl(void);
int spi_flash_program_image(u32 offset, u32 len);
void copy_globalmem_to_flash(u32 fa, u64 ga, u32 count);
void copy_flash_to_globalmem(u64 ga, u32 fa, u32 count);

#endif