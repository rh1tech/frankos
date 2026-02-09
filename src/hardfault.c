// see https://habr.com/ru/articles/437256/
#include "hardfault.h"
/** TODO for RP2350:
bool cpu_check_address(volatile const char *address)
{
    // Cortex-M0 doesn't have BusFault so we need to catch HardFault
    (void)address;
    
    // R5 will be set to 0 by HardFault handler
    // to indicate HardFault has occured
    register uint32_t result __asm("r5");

    __asm__ volatile (
        "ldr  r5, =1            \n" /* set default R5 value * /
        "ldr  r1, =0xDEADF00D   \n" /* set magic number     * /
        "ldr  r2, =0xCAFEBABE   \n" /* 2nd magic to be sure * /
        "ldrb r3, [r0]          \n" /* probe address        * /
    );

    return result;
}

__attribute__((naked)) void hardfault_handler(void)
{
    /* Get stack pointer where exception stack frame lies * /
    __asm__ volatile
    (
        /* decide if we need MSP or PSP stack * /
        "movs r0, #4                        \n" /* r0 = 0x4                   * /
        "mov r2, lr                         \n" /* r2 = lr                    * /
        "tst r2, r0                         \n" /* if(lr & 0x4)               * /
        "bne use_psp                        \n" /* {                          * /
        "mrs r0, msp                        \n" /*   r0 = msp                 * /
        "b out                              \n" /* }                          * /
        " use_psp:                          \n" /* else {                     * /
        "mrs r0, psp                        \n" /*   r0 = psp                 * /
        " out:                              \n" /* }                          * /

        /* catch intended HardFaults on Cortex-M0 to probe memory addresses * /
        "ldr     r1, [r0, #0x04]            \n" /* read R1 from the stack        * /
        "ldr     r2, =0xDEADF00D            \n" /* magic number to be found      * /
        "cmp     r1, r2                     \n" /* compare with the magic number * /
        "bne     regular_handler            \n" /* no magic -> handle as usual   * /
        "ldr     r1, [r0, #0x08]            \n" /* read R2 from the stack        * /
        "ldr     r2, =0xCAFEBABE            \n" /* 2nd magic number to be found  * /
        "cmp     r1, r2                     \n" /* compare with 2nd magic number * /
        "bne     regular_handler            \n" /* no magic -> handle as usual   * /
        "ldr     r1, [r0, #0x18]            \n" /* read PC from the stack        * /
        "add     r1, r1, #2                 \n" /* move to the next instruction  * /
        "str     r1, [r0, #0x18]            \n" /* modify PC in the stack        * /
        "ldr     r5, =0                     \n" /* set R5 to indicate HardFault  * /
        "bx      lr                         \n" /* exit the exception handler    * /
        " regular_handler:                  \n"

        /* here comes the rest of the fucking owl * /
    );
}

static uint32_t cpu_find_memory_size(char *base, uint32_t block, uint32_t maxsize) {
    char *address = base;
    do {
        address += block;
        if (!cpu_check_address(address)) {
            break;
        }
    } while ((uint32_t)(address - base) < maxsize);

    return (uint32_t)(address - base);
}
*/
uint32_t get_cpu_ram_size(void) {
    static uint32_t once = 520 << 10;
//    if (!once) {
//        once = cpu_find_memory_size((char *)0x20000000, 4096, 300*1024);
//    }
    return once;
}

#include <hardware/flash.h>
#include <pico/multicore.h>

uint32_t get_cpu_flash_size(void) {
    uint8_t rx[4] = {0};
    get_cpu_flash_jedec_id(rx);
    return 1u << rx[3];
}

void get_cpu_flash_jedec_id(uint8_t _rx[4]) {
    static uint8_t rx[4] = {0};
    if (rx[0] == 0) {
        uint8_t tx[4] = {0x9f};
        multicore_lockout_start_blocking();
        const uint32_t ints = save_and_disable_interrupts();
        flash_do_cmd(tx, rx, 4);
        restore_interrupts(ints);
        multicore_lockout_end_blocking();
    }
    *(unsigned*)_rx = *(unsigned*)rx;
}
