#include <stdint.h>
#include "baseband.h"

#define NUM_PACKETS 5
#define BEAT_BYTES  8U

// 0x1 = Baseband digital loopback (framing/whitening/CRC + DMA)
// 0x2 = Modem CDR loopback (includes FSKRX CDR; requires LO/ADC clock)
#define LOOPBACK_MASK 0x2U

#define BLE_ADV_ACCESS_ADDR 0x8E89BED6U
#define BLE_ADV_CRC_INIT    0x00555555U

static const uint8_t packet_sizes[NUM_PACKETS] = {4, 8, 16, 32, 128};
static uint8_t dma_buffer[512] __attribute__((aligned(64)));

static inline void riscv_fence(void) {
    asm volatile ("fence rw, rw" ::: "memory");
}

static inline void issue_baseband_debug(uint32_t addr, uint32_t total_bytes, uint8_t mask) {
    reg_write32(BASEBAND_ADDITIONAL_DATA, addr);
    reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_DEBUG, mask, total_bytes));
}

// Poll STATUS0[15:13] (controller_state) until it returns to s_idle (0)
static int wait_controller_idle(uint32_t max_polls) {
    for (uint32_t i = 0; i < max_polls; i++) {
        uint32_t st0 = reg_read32(BASEBAND_STATUS0);
        uint32_t ctrl_state = (st0 >> 13) & 0x7U;
        if (ctrl_state == 0U) {
            return 0;
        }
    }
    return -1; // Timed out stuck in s_debug
}

int main(void) {
    uint32_t buf_phy_addr = (uint32_t)(uintptr_t)dma_buffer;

    // 1. Configure BLE PHY constants via ISA commands (uses 0x8000/0x8004 only)
    baseband_configure(BASEBAND_CONFIG_RADIO_MODE, BASEBAND_MODE_BLE);
    baseband_configure(BASEBAND_CONFIG_ACCESS_ADDRESS, BLE_ADV_ACCESS_ADDR);
    baseband_configure(BASEBAND_CONFIG_CRC_SEED, BLE_ADV_CRC_INIT);
    baseband_configure(BASEBAND_CONFIG_BLE_CHANNEL_INDEX, 37);
    riscv_fence();

    for (int pkt = 0; pkt < NUM_PACKETS; pkt++) {
        uint8_t total_size = packet_sizes[pkt];
        uint8_t base_pattern = 0xA0 + (pkt << 4);

        // Exact RTL destination offset: (addr + totalBytes + beatBytes) & ~(beatBytes - 1)
        uint32_t rx_offset = (total_size + BEAT_BYTES) & ~(BEAT_BYTES - 1U);

        // 2. Populate TX region [0 .. total_size - 1]
        dma_buffer[0] = 0x02;               // BLE PDU header byte 0
        dma_buffer[1] = total_size - 2;     // BLE PDU payload length
        for (uint32_t i = 2; i < total_size; i++) {
            dma_buffer[i] = (uint8_t)(base_pattern + i);
        }

        // 3. Clear RX region + 3-byte BLE CRC + set overrun canary
        for (uint32_t i = 0; i < total_size + 3U; i++) {
            dma_buffer[rx_offset + i] = 0x00;
        }
        dma_buffer[rx_offset + total_size + 3U] = 0xFF;
        riscv_fence();

        // 4. Fire DEBUG_CMD with explicit loopback mask
        issue_baseband_debug(buf_phy_addr, total_size, LOOPBACK_MASK);
        riscv_fence();

        // Give FSM at least a few cycles to leave s_idle (0) -> s_debug before polling
        for (int k = 0; k < 32; k++) {
            asm volatile ("nop");
        }

        // 5. Wait for Controller FSM to finish s_debug and return to s_idle
        if (wait_controller_idle(200000U) != 0) {
            uint32_t st0 = reg_read32(BASEBAND_STATUS0);
            return ((pkt + 1) << 24) | (0xFF << 16) | (st0 & 0xFFFFU);
        }
        riscv_fence();

        // 6. Pop RX finish message (transitions RXChainController s_rxFinish -> s_idle)
        uint32_t rx_msg = baseband_rxfinish_message();
        (void)rx_msg;
        riscv_fence();

        volatile uint8_t *rx_buf = (volatile uint8_t *)&dma_buffer[rx_offset];

        // 7. Verify exact byte-for-byte match
        for (uint32_t i = 0; i < total_size; i++) {
            if (rx_buf[i] != dma_buffer[i]) {
                return ((pkt + 1) << 16) | ((i + 1) << 8) | rx_buf[i];
            }
        }

        // 8. Verify no overrun past rx_offset + total_size + 3U (PDU + 3B BLE CRC)
        if (dma_buffer[rx_offset + total_size + 3U] != 0xFF) {
            return ((pkt + 1) << 16) | 0xEE00 | dma_buffer[rx_offset + total_size + 3U];
        }
    }

    return 0;
}