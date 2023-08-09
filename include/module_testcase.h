#ifndef __MODULE_TESTCASE_H__
#define __MODULE_TESTCASE_H__

//return 0 mean success, others fail
typedef int (*module_func_t)(void);

//only support level0-2
#define module_testcase(level,fn) \
            static module_func_t __module_##fn \
	                          __attribute__((used)) \
                            __attribute__((section (".module" level ".testcase" ))) = fn


#endif
