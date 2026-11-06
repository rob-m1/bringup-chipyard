#include <stdint.h>
#include <stdio.h>
#include "baseband.h"

static uint8_t rx_dma_buffer[256] __attribute__((aligned(64)));
static uint8_t tx_dma_buffer[256] __attribute__((aligned(64)));

static inline void riscv_fence(void) {
    asm volatile ("fence rw, rw" ::: "memory");
}

int main(void) {
    uint32_t rx_phy_addr = (uint32_t)(uintptr_t)rx_dma_buffer;
    uint32_t tx_phy_addr = (uint32_t)(uintptr_t)tx_dma_buffer;

    // 1. Fill TX buffer with test pattern
    for (int i = 0; i < 32; i++) {
        tx_dma_buffer[i] = (uint8_t)(0xA0 + i);
        rx_dma_buffer[i] = 0x00;
    }
    riscv_fence();

    // 2. Baseband config
    baseband_configure(BASEBAND_CONFIG_RADIO_MODE, BASEBAND_MODE_BLE);
    baseband_configure(BASEBAND_CONFIG_SHR, 0);
    
    reg_write8(BASEBAND_ENABLE_RX_I, 1);
    reg_write8(BASEBAND_ENABLE_RX_Q, 1);
    reg_write8(BASEBAND_TRIM_G0, 0x0F);
    reg_write8(BASEBAND_TRIM_G1, 0x0F);
    riscv_fence();

    // 3. Digital Loopback Mode
    reg_write32(BASEBAND_ADDITIONAL_DATA, rx_phy_addr);
    reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_DEBUG, 0b11, 32));
    riscv_fence();

    // 4. Start RX DMA & trigger TX
    baseband_receive(rx_phy_addr);
    riscv_fence();

    baseband_send(tx_phy_addr, 32);
    riscv_fence();

    // 5. Poll finish message
    uint32_t rx_msg = baseband_rxfinish_message();
    riscv_fence();

    // 6. Print raw RX DRAM buffer bytes to console
    volatile uint8_t *rx_buf = (volatile uint8_t *)rx_dma_buffer;
    
    printf("\n================ Baseband RX DMA Dump ================\n");
    for (int i = 0; i < 32; i++) {
        printf("RX[%02d] = 0x%02X\n", i, rx_buf[i]);
    }
    printf("======================================================\n");

    return 0;
}