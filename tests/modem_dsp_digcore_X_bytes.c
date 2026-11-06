#include <stdint.h>
#include <stdio.h>
#include "baseband.h"

#define NUM_PACKETS 5
#define PACKET_SIZE 32

static uint8_t rx_dma_buffer[256] __attribute__((aligned(64)));
static uint8_t tx_dma_buffer[256] __attribute__((aligned(64)));

static inline void riscv_fence(void) {
    asm volatile ("fence rw, rw" ::: "memory");
}

int main(void) {
    uint32_t rx_phy_addr = (uint32_t)(uintptr_t)rx_dma_buffer;
    uint32_t tx_phy_addr = (uint32_t)(uintptr_t)tx_dma_buffer;

    // 1. Initial Baseband configuration
    baseband_configure(BASEBAND_CONFIG_RADIO_MODE, BASEBAND_MODE_BLE);
    baseband_configure(BASEBAND_CONFIG_SHR, 0);
    
    reg_write8(BASEBAND_ENABLE_RX_I, 1);
    reg_write8(BASEBAND_ENABLE_RX_Q, 1);
    reg_write8(BASEBAND_TRIM_G0, 0x0F);
    reg_write8(BASEBAND_TRIM_G1, 0x0F);
    riscv_fence();

    // 2. Set up Digital Loopback Mode
    reg_write32(BASEBAND_ADDITIONAL_DATA, rx_phy_addr);
    reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_DEBUG, 0b11, PACKET_SIZE));
    riscv_fence();

    printf("\n================ Starting %d-Packet DMA Loopback Test ================\n", NUM_PACKETS);

    for (int pkt = 0; pkt < NUM_PACKETS; pkt++) {
        // Prepare unique pattern per packet (Pkt 0: 0xA0.., Pkt 1: 0xB0.., etc.)
        uint8_t base_pattern = 0xA0 + (pkt << 4);
        for (int i = 0; i < PACKET_SIZE; i++) {
            tx_dma_buffer[i] = (uint8_t)(base_pattern + i);
            rx_dma_buffer[i] = 0x00;
        }
        riscv_fence();

        // Arm RX DMA for current packet
        baseband_receive(rx_phy_addr);
        riscv_fence();

        // Trigger TX DMA transaction
        baseband_send(tx_phy_addr, PACKET_SIZE);
        riscv_fence();

        // Await hardware completion
        uint32_t rx_msg = baseband_rxfinish_message();
        riscv_fence();

        // Print sample bytes from landing buffer
        volatile uint8_t *rx_buf = (volatile uint8_t *)rx_dma_buffer;
        printf("Packet [%d/%d] Completed (rx_msg=0x%08X) -> First byte: 0x%02X, Last byte: 0x%02X\n",
               pkt + 1, NUM_PACKETS, rx_msg, rx_buf[0], rx_buf[PACKET_SIZE - 1]);
    }

    printf("================ Multi-Packet Test Complete ================\n");

    return 0;
}
