#include "mmio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Address map
#define BASEBAND_INST 0x8000
#define BASEBAND_ADDITIONAL_DATA 0x8004

#define BASEBAND_STATUS0 0x8008
#define BASEBAND_STATUS1 0x800C
#define BASEBAND_STATUS2 0x8010
#define BASEBAND_STATUS3 0x8014
#define BASEBAND_STATUS4 0x8018

#define BASEBAND_TRIM_G0 0x801C
#define BASEBAND_TRIM_G1 0x801D
#define BASEBAND_TRIM_G2 0x801E
#define BASEBAND_TRIM_G3 0x801F
#define BASEBAND_TRIM_G4 0x8020
#define BASEBAND_TRIM_G5 0x8021
#define BASEBAND_TRIM_G6 0x8022
#define BASEBAND_TRIM_G7 0x8023

#define BASEBAND_I_VGA_ATTEN_VALUE 0x8024
#define BASEBAND_I_VGA_ATTEN_RESET 0x8026
#define BASEBAND_I_VGA_ATTEN_USE_AGC 0x8027
#define BASEBAND_I_VGA_ATTEN_SAMPLE_WINDOW 0x8028
#define BASEBAND_I_VGA_ATTEN_IDEAL_P2P 0x8029
#define BASEBAND_I_VGA_ATTEN_TOLERANCE_P2P 0x802A

#define BASEBAND_I_BPF_CHP_0_1 0x802B
#define BASEBAND_I_BPF_CHP_2_3 0x802C
#define BASEBAND_I_BPF_CHP_4_5 0x802D
#define BASEBAND_I_BPF_CLP_0_1 0x802E
#define BASEBAND_I_BPF_CLP_2 0x802F

#define BASEBAND_Q_VGA_ATTEN_VALUE 0x8030
#define BASEBAND_Q_VGA_ATTEN_RESET 0x8032
#define BASEBAND_Q_VGA_ATTEN_USE_AGC 0x8033
#define BASEBAND_Q_VGA_ATTEN_SAMPLE_WINDOW 0x8034
#define BASEBAND_Q_VGA_ATTEN_IDEAL_P2P 0x8035
#define BASEBAND_Q_VGA_ATTEN_TOLERANCE_P2P 0x8036

#define BASEBAND_Q_BPF_CHP_0_1 0x8037
#define BASEBAND_Q_BPF_CHP_2_3 0x8038
#define BASEBAND_Q_BPF_CHP_4_5 0x8039
#define BASEBAND_Q_BPF_CLP_0_1 0x803A
#define BASEBAND_Q_BPF_CLP_2 0x803B

#define BASEBAND_I_DCO_USE_DCO 0x803C
#define BASEBAND_I_DCO_RESET 0x803D
#define BASEBAND_I_DCO_GAIN 0x803E

#define BASEBAND_Q_DCO_USE_DCO 0x803F
#define BASEBAND_Q_DCO_RESET 0x8040
#define BASEBAND_Q_DCO_GAIN 0x8041

#define BASEBAND_DCO_TUNING_1 0x8042
#define BASEBAND_DCO_TUNING_2 0x8043

#define BASEBAND_MUX_DBG_IN 0x8046
#define BASEBAND_MUX_DBG_OUT 0x8048

#define BASEBAND_ENABLE_RX_I 0x804A
#define BASEBAND_ENABLE_RX_Q 0x804B
#define BASEBAND_ENABLE_VCO_LO 0x804C

#define BASEBAND_LUT_CMD 0x8050

#define BASEBAND_TXFINISH_MESSAGE 0x8050
#define BASEBAND_RXERROR_MESSAGE 0x8054
#define BASEBAND_RXFINISH_MESSAGE 0x8058
#define BASEBAND_TXERROR_MESSAGE 0x805C

#define BASEBAND_FIR_CMD 0x8060

#define BASEBAND_I_VGA_ATTEN_GAIN_INC 0x8064
#define BASEBAND_I_VGA_ATTEN_GAIN_DEC 0x8065

#define BASEBAND_Q_VGA_ATTEN_GAIN_INC 0x8066
#define BASEBAND_Q_VGA_ATTEN_GAIN_DEC 0x8067

// Instruction macro
#define BASEBAND_INSTRUCTION(primaryInst, secondaryInst, data) ((primaryInst & 0xF) + ((secondaryInst & 0xF) << 4) + ((data & 0xFFFFFF) << 8))

// Primary instructions
#define BASEBAND_CONFIG 0
#define BASEBAND_SEND 1
#define BASEBAND_RECEIVE 2
#define BASEBAND_RECEIVE_EXIT 3
#define BASEBAND_DEBUG 15

// Secondary instructions
#define BASEBAND_CONFIG_RADIO_MODE 0
#define BASEBAND_CONFIG_CRC_SEED 1
#define BASEBAND_CONFIG_ACCESS_ADDRESS 2
#define BASEBAND_CONFIG_SHR 3
#define BASEBAND_CONFIG_BLE_CHANNEL_INDEX 4
#define BASEBAND_CONFIG_LRWPAN_CHANNEL_INDEX 5

// Radio modes
#define BASEBAND_MODE_BLE 0
#define BASEBAND_MODE_LRWPAN 1

// LUT Control
#define LUT_VCO_MOD 0
#define LUT_VCO_CT_BLE 1
#define LUT_VCO_CT_LRWPAN 2
#define LUT_AGC_I 3
#define LUT_AGC_Q 4
#define LUT_DCO_I 5
#define LUT_DCO_Q 6

#define LUT_COMMAND(lut, address, value) ((lut & 0xF) + ((address & 0x3F) << 4) + ((value & 0x3FFFFF) << 10))


/*
    BASEBAND STATUS
*/

/*
    STATUS 0
    [2:0] Assembler State
    [5:3] Disassembler State
    [7:6] TX State
    [10:8] RX Controller State
    [12:11] TX Controller State
    [15:13] Controller State
    [23:16] ADC I data
    [31:24] ADC Q data
*/
typedef struct {
    uint8_t assembler_state;
    uint8_t disassembler_state;
    uint8_t tx_state;
    uint8_t rx_controller_state;
    uint8_t tx_controller_state;
    uint8_t controller_state;
    uint8_t adc_i_data;
    uint8_t adc_q_data;
} baseband_status0_t;

void baseband_get_status0(baseband_status0_t* status);

uint32_t baseband_status0();
uint32_t baseband_status1();
uint32_t baseband_status2();
uint32_t baseband_status3();
uint32_t baseband_status4();

/*
    COMMAND FUNCTIONS
*/
// Load and send <bytes> bytes of data from address <addr>
void baseband_send(uint32_t addr, uint32_t bytes);

// Configure baseband constant <target> (from secondary instruction set) to value <value>
void baseband_configure(uint8_t target, uint32_t value);

// Try and receive data. Any found data will be stored at address <addr>
void baseband_receive(uint32_t addr);

// Exit receive mode
void baseband_receive_exit();

// Function that tests (send + check) the baseband debug command
void baseband_debug(uint32_t addr, uint8_t num_bytes);

uint8_t baseband_read_adc_i();
uint8_t baseband_read_adc_q();

/*
    INTERRUPTS
 */

uint32_t baseband_rxerror_message();
uint32_t baseband_txerror_message();
uint32_t baseband_rxfinish_message();

