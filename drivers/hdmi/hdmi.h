#ifndef HDMI_H
#define HDMI_H

#include <inttypes.h>
#include <stdbool.h>
#include "hardware/dma.h"

#define VIDEO_DMA_IRQ (DMA_IRQ_0)

#ifndef HDMI_BASE_PIN
#define HDMI_BASE_PIN (12)
#endif

#ifndef HDMI_PIN_RGB_notBGR
#define HDMI_PIN_RGB_notBGR (1)
#endif
#ifndef HDMI_PIN_invert_diffpairs
#define HDMI_PIN_invert_diffpairs (1)
#endif

#ifndef PIO_VIDEO
#define PIO_VIDEO pio1
#endif
#ifndef PIO_VIDEO_ADDR
#define PIO_VIDEO_ADDR pio1
#endif

#ifndef beginHDMI_PIN_data
#define beginHDMI_PIN_data (HDMI_BASE_PIN + 2)
#endif

#ifndef beginHDMI_PIN_clk
#define beginHDMI_PIN_clk (HDMI_BASE_PIN)
#endif

struct video_mode_t {
    int h_total;
    int h_width;
    int freq;
    int vgaPxClk;
};

// Initialize HDMI hardware (PIO, DMA, palette conversion table).
// Call with defer_irq=true to defer IRQ handler setup to Core 1.
void graphics_init(bool defer_irq);

// Set the active scanline source buffer.
void graphics_set_buffer(uint8_t *buffer);

// Request a buffer swap at the next vsync.
void graphics_request_buffer_swap(uint8_t *buffer);

// Returns monotonically increasing frame counter.
uint32_t get_frame_count(void);

// Initialize the pair-encoded 16-color palette.
// colors[0..15] are RGB888 values. Fills the 256-entry conv_color table
// so that byte value b maps to TMDS(left=colors[b>>4], right=colors[b&0xF]).
void graphics_init_pair_palette(const uint32_t colors[16]);

// Initialize the HDMI DMA IRQ handler on the calling core.
// Call from Core 1 after graphics_init() was called with defer_irq=true.
void graphics_init_irq_on_this_core(void);

#endif // HDMI_H
