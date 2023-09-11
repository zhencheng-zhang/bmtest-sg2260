#ifndef __SMP_H__
#define __SMP_H__

#define MAIN_BOOT_CORE	1
#define DEFAULT_STACK_SIZE 4096
#define SMP_CONTEXT_SIZE_SHIFT 7
#define SMP_CONTEXT_SIZE (1 << SMP_CONTEXT_SIZE_SHIFT)
#define SMP_CONTEXT_SP_OFFSET 0
#define SMP_CONTEXT_FN_OFFSET 8
#define SMP_CONTEXT_PRIV_OFFSET 16
#define SMP_CONTEXT_STATCKSIZE_OFFSET 24
#define CONFIG_SMP_NUM	64

#ifndef __ASSEMBLER__

void wake_up_other_core(int core_id, void (*fn)(void *priv),
			void *priv, void *sp, int stack_size);
void jump_to(long jump_addr, long hartid, long dtb_addr, long dynamic_info_addr);

#endif

#endif