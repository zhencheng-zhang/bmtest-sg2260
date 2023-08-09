#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>

void print_trace(void);
void hang(int ret);

#define ASSERT(_cond)                                   \
  do {                                                  \
	if (!(_cond)) {                                     \
		printf("ASSERT %s: %d: %s: %s\n",                 \
			 __FILE__, __LINE__, __func__, #_cond);     \
		print_trace();                                    \
		fflush(stdout);                                   \
		hang(-1);                                         \
	}                                                   \
  } while(0)

#define DBG_ASSERT(_cond)  \
{                         \
  if (!_cond) {           \
    while (1) {           \
        ;                 \
    }                     \
  }                       \
}

#define FW_ERR(fmt, ...)		printf("FW_ERR: " fmt, ##__VA_ARGS__)
#define FW_INFO(fmt, ...)		printf("FW: " fmt, ##__VA_ARGS__)
#ifdef TRACE_FIRMWARE
#define FW_TRACE(fmt, ...)		printf("FW_TRACE: " fmt, ##__VA_ARGS__)
#else
#define FW_TRACE(fmt, ...)
#endif
#ifdef DEBUG_FIRMWARE
#define FW_DBG(fmt, ...)		printf("FW_DBG: " fmt, ##__VA_ARGS__)
#else
#define FW_DBG(fmt, ...)
#endif

/* The log output macros print output to the console. These macros produce
 * compiled log output only if the LOG_LEVEL defined in the makefile (or the
 * make command line) is greater or equal than the level required for that
 * type of log output.
 * The format expected is the same as for INFO(). For example:
 * INFO("Info %s.\n", "message")	-> INFO:	Info message.
 * WARN("Warning %s.\n", "message") -> WARNING: Warning message.
 */

#define LOG_LEVEL_NONE			0
#define LOG_LEVEL_ERROR			10
#define LOG_LEVEL_NOTICE		20
#define LOG_LEVEL_WARNING		30
#define LOG_LEVEL_INFO			40
#define LOG_LEVEL_VERBOSE		50

#define LOG_LEVEL 40

#if LOG_LEVEL >= LOG_LEVEL_NOTICE
# define NOTICE(...)	printf("NOTICE: " __VA_ARGS__)
#else
# define NOTICE(...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_ERROR
# define ERROR(...)	printf("ERROR: " __VA_ARGS__)
#else
# define ERROR(...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARNING
# define WARN(...)	printf("WARNING: " __VA_ARGS__)
#else
# define WARN(...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
# define INFO(...)	printf("INFO: " __VA_ARGS__)
#else
# define INFO(...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
# define VERBOSE(...)	printf("VERBOSE: " __VA_ARGS__)
#else
# define VERBOSE(...)
#endif

#endif /* _DEBUG_H_ */
