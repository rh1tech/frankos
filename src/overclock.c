#include "overclock.h"
#include "graphics.h"
#include "cmd.h"
#include <pico/stdlib.h>
#include <hardware/clocks.h>

static uint32_t overclocking_khz = OVERCLOCKING * 1000;
static uint32_t last_overclocking_khz = 0;
static uint32_t vco = 0;
static uint32_t postdiv1 = 0;
static uint32_t postdiv2 = 0;

void set_overclocking(uint32_t khz) {
    overclocking_khz = khz;
}
void set_last_overclocking(uint32_t khz) {
    last_overclocking_khz = khz;
    overclocking_khz = khz;
}
uint32_t __not_in_flash() get_overclocking_khz() {
    return last_overclocking_khz ? last_overclocking_khz : overclocking_khz;
}

void overclocking_ex(uint32_t _vco, uint32_t _postdiv1, uint32_t _postdiv2) {
    /** TODO:
    if (last_overclocking_khz != overclocking_khz || vco != _vco || postdiv1 != _postdiv1 || postdiv2 != _postdiv2) {
        overclocking_khz = vco / postdiv1 / postdiv2 / 1000;
        set_sys_clock_pll(_vco, _postdiv1, _postdiv2);
        last_overclocking_khz = overclocking_khz;
        vco = _vco;
        postdiv1 = _postdiv1;
        postdiv2 = _postdiv2;
    }
    */
    goutf("CPU: %f MHz (vco: %d MHz, pd1: %d, pd2: %d)\n", overclocking_khz / 1000.0, vco / 1000000, postdiv1, postdiv2);
}

void overclocking() {
    set_sys_clock_khz(overclocking_khz, true);

    /** TODO:
   // vTaskSuspendAll();;
    if (check_sys_clock_khz(overclocking_khz, &vco, &postdiv1, &postdiv2)) {
        overclocking_ex(vco, postdiv1, postdiv2);
    //	xTaskResumeAll();;
    } else {
   // 	xTaskResumeAll();;
        FIL * f = get_stderr();
        fgoutf(f, "System clock of %d kHz cannot be achieved\n", overclocking_khz);
    }
    */
}
