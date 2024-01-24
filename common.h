//> Chunks of Bytecode common-h
#ifndef mu_common_h
#define mu_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//> A Virtual Machine define-debug-trace
#define NAN_BOXING // Optimization define-nan-boxing
#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION
//^ A Virtual Machine define-debug-trace

// Garbage Collection
#define DEBUG_STRESS_GC
#define DEBUG_LOG_GC

// Local Variables
#define UINT8_COUNT (UINT8_MAX + 1)


#endif
#undef DEBUG_PRINT_CODE
#undef DEBUG_TRACE_EXECUTION
#undef DEBUG_STRESS_GC
#undef DEBUG_LOG_GC
