#include <stdint.h>
extern uint32_t test_primask;
static inline uint32_t __get_PRIMASK(void) {return test_primask;}
static inline void __disable_irq(void) {test_primask=1;}
static inline void __set_PRIMASK(uint32_t x) {test_primask=x;}
