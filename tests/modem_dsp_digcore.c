// // #include <stdio.h>
// // #include <stdint.h>
// // #include <stdbool.h>

// // // Include the project's native BSP header
// // #include "baseband.h"

// // // Configuration sub-opcodes for BASEBAND_CONFIG (if not defined in baseband.h)
// // #ifndef BASEBAND_CONFIG_RADIO_MODE
// // #define BASEBAND_CONFIG_RADIO_MODE          0
// // #endif
// // #ifndef BASEBAND_CONFIG_BLE_CHANNEL_INDEX
// // #define BASEBAND_CONFIG_BLE_CHANNEL_INDEX   4
// // #endif

// // // Define message queue offsets relative to BASEBAND_INST (0x8000)
// // #define REG_RX_ERROR_MSG        (*(volatile uint32_t *)(BASEBAND_INST + 0x34))
// // #define REG_RX_FINISH_MSG       (*(volatile uint32_t *)(BASEBAND_INST + 0x38))
// // #define REG_TX_ERROR_MSG        (*(volatile uint32_t *)(BASEBAND_INST + 0x3C))

// // // Dedicated DMA receive buffer in DRAM/SRAM
// // static uint8_t rx_dma_buffer[256] __attribute__((aligned(8)));

// // static inline void riscv_fence(void) {
// //     asm volatile ("fence" ::: "memory");
// // }

// // int main(void) {
// //     uint32_t rx_msg = 0;
// //     uint32_t rx_err = 0;
// //     uint32_t timeout = 0;
// //     bool data_mismatch = false;
    
// //     uintptr_t dram_target = (uintptr_t)rx_dma_buffer;

// //     printf(" SCuMv26 Baseband RX Integration Test\n");
// //     printf(" MMIO Base Address: 0x%04X\n", BASEBAND_INST);
// //     printf(" DMA Target Buffer: 0x%08lX\n", (unsigned long)dram_target);

// //     // Optional Step 0: Pre-configure radio parameters if required by test environment
// //     // reg_write32(BASEBAND_INST + 0x04, 0); // BLE Mode
// //     // reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_CONFIG, BASEBAND_CONFIG_RADIO_MODE, 0));
// //     // reg_write32(BASEBAND_INST + 0x04, 37); // BLE Channel 37
// //     // reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_CONFIG, BASEBAND_CONFIG_BLE_CHANNEL_INDEX, 0));
// //     // riscv_fence();

// //     // 1. Pass DMA target memory address to additionalData (0x8004)
// //     reg_write32(BASEBAND_INST + 0x04, (uint32_t)dram_target);
// //     riscv_fence();

// //     // 2. Issue RECEIVE_START command word to inst (0x8000)
// //     // Uses the BSP macro: BASEBAND_INSTRUCTION(primary, secondary, data)
// //     reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_RECEIVE, 0, 0));
// //     riscv_fence();

// //     printf("[CORE] Polling rxFinishMessage at 0x%04X...\n", BASEBAND_INST + 0x38);

// //     // 3. Poll Decoupled message queues
// //     while (1) {
// //         riscv_fence();

// //         // Check for error on 0x8034
// //         rx_err = REG_RX_ERROR_MSG;
// //         if (rx_err != 0) {
// //             printf("[ERROR] RX Failed with error code: 0x%08X\n", rx_err);
// //             reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_RECEIVE_EXIT, 0, 0));
// //             return -1;
// //         }

// //         // Reading 0x8038 automatically pops/clears the finish message queue
// //         rx_msg = REG_RX_FINISH_MSG;
// //         if (rx_msg != 0) {
// //             printf("[CORE] Modem signaled RX complete! Msg: 0x%08X\n", rx_msg);
// //             break;
// //         }

// //         if (++timeout > 10000U) {
// //             printf("[ERROR] Timed out waiting for modem response!\n");
// //             reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_RECEIVE_EXIT, 0, 0));
// //             return -2;
// //         }
// //     }

// //     // 4. Memory Fence to guarantee DMA consistency before reading DRAM
// //     riscv_fence();

// //     // 5. Inspect and verify received packet payload against expected pattern
// //     uint32_t bytes_to_read = rx_msg & 0xFFFF; // Extract byte count from finish word
// //     if (bytes_to_read == 0) bytes_to_read = 32;

// //     printf("[CORE] Reading and verifying %u payload bytes from DRAM:\n", bytes_to_read);
// //     for (uint32_t i = 0; i < bytes_to_read; i++) {
// //         uint8_t actual_byte = rx_dma_buffer[i];
        
// //         // Standard incrementing test vector (0x01, 0x02, 0x03, ...)
// //         // Adjust expected_byte if your testbench transmits a specific fixed header or seed
// //         uint8_t expected_byte = (uint8_t)(i + 1); 

// //         printf("  [%02u] Expected: 0x%02X | Received: 0x%02X", i, expected_byte, actual_byte);

// //         if (actual_byte != expected_byte) {
// //             printf(" <-- MISMATCH!");
// //             data_mismatch = true;
// //         }
// //         printf("\n");
// //     }

// //     // 6. Stop RX state machine cleanly
// //     reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_RECEIVE_EXIT, 0, 0));
// //     riscv_fence();

// //     // 7. Final Pass/Fail verdict
// //     if (data_mismatch) {
// //         printf("[FAIL] Test Failed: Payload byte mismatch detected!\n");
// //         return -3;
// //     }

// //     printf("[PASS] Test Completed Successfully!\n");

// //     return 0;
// // }
// // int main(void) {
// //     uint32_t test_val = 0xDEADBEEF;
    
// //     // Write test value to additionalData (0x8004)
// //     reg_write32(BASEBAND_INST + 0x04, test_val);
// //     riscv_fence();

// //     // Read back value
// //     uint32_t read_val = reg_read32(BASEBAND_INST + 0x04);
// //     riscv_fence();

// //     // Return status codes mapped to exact readback values
// //     if (read_val == test_val)    return 0;  // 0  -> Perfect match (Read/Write RAM)
// //     if (read_val == 0x00000000)  return 10; // 10 -> Returned 0x0 (Write-Only / Command Queue)
// //     if (read_val == 0xFFFFFFFF)  return 20; // 20 -> Unmapped / Bus Error
    
// //     return 30;                              // 30 -> Returned non-zero hardware value
// // }


// // #include <stdint.h>
// // #include "baseband.h"

// // static inline void riscv_fence(void) {
// //     asm volatile ("fence" ::: "memory");
// // }

// // int main(void) {
// //     // 1. Write dummy DMA target address to argument register 0x8004
// //     reg_write32(BASEBAND_INST + 0x04, 0x80001000);
// //     riscv_fence();

// //     // 2. Issue BASEBAND_RECEIVE_EXIT command word to command queue 0x8000
// //     reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_RECEIVE_EXIT, 0, 0));
// //     riscv_fence();

// //     // 3. Return 0 cleanly if both writes completed on the bus
// //     return 0;
// // }

// #include <stdio.h>
// #include <stdint.h>
// #include <stdbool.h>
// #include "baseband.h"

// #define REG_RX_ERROR_MSG   (*(volatile uint32_t *)(BASEBAND_INST + 0x34))
// #define REG_RX_FINISH_MSG  (*(volatile uint32_t *)(BASEBAND_INST + 0x38))

// static uint8_t rx_dma_buffer[256] __attribute__((aligned(8)));

// static inline void riscv_fence(void) {
//     asm volatile ("fence" ::: "memory");
// }

// int main(void) {
//     uint32_t rx_msg = 0;
//     uint32_t rx_err = 0;
    
//     // 1. Set DMA target address
//     reg_write32(BASEBAND_INST + 0x04, (uint32_t)(uintptr_t)rx_dma_buffer);
//     riscv_fence();

//     // 2. Start RX
//     reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_RECEIVE, 0, 0));
//     riscv_fence();

//     // 3. Wait for hardware finish message (0x8038)
//     rx_msg = REG_RX_FINISH_MSG;
//     riscv_fence();

//     // 4. Verify received payload in DRAM
//     // Testbench sent pattern: 1, 2, 3, 4 ...
//     for (uint32_t i = 0; i < 32; i++) {
//         uint8_t expected = (uint8_t)(i + 1);
//         if (rx_dma_buffer[i] != expected) {
//             return 10 + i; // Return byte index where mismatch occurred
//         }
//     }

//     // 5. Clean stop
//     reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_RECEIVE_EXIT, 0, 0));
//     riscv_fence();

//     return 0; // PASS: Full RX DMA functional path verified!
// }

#include <stdint.h>
#include "baseband.h"

#define REG_RX_ERROR_MSG   (*(volatile uint32_t *)(BASEBAND_INST + 0x34))
#define REG_RX_FINISH_MSG  (*(volatile uint32_t *)(BASEBAND_INST + 0x38))

static uint8_t rx_dma_buffer[256] __attribute__((aligned(8)));

static inline void riscv_fence(void) {
    asm volatile ("fence" ::: "memory");
}

int main(void) {
    uint32_t rx_msg = 0;
    
    // 1. Pass DMA target address (0x8004)
    reg_write32(BASEBAND_INST + 0x04, (uint32_t)(uintptr_t)rx_dma_buffer);
    riscv_fence();

    // 2. Issue BASEBAND_RECEIVE start command (0x8000)
    reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_RECEIVE, 0, 0));
    riscv_fence();

    // 3. Blocking read on completion queue 0x8038
    // Unblocks as soon as one of the testbench packet bursts completes
    rx_msg = REG_RX_FINISH_MSG;
    riscv_fence();

    // 4. Verify received payload integrity in DRAM
    for (uint32_t i = 0; i < 32; i++) {
        uint8_t expected = (uint8_t)(i + 1);
        if (rx_dma_buffer[i] != expected) {
            return 100 + i; // Return byte offset if data mismatch occurs
        }
    }

    // 5. Clean exit from RX mode
    reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_RECEIVE_EXIT, 0, 0));
    riscv_fence();

    return 0; // PASS
}