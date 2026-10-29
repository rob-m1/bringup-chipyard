#include "baseband.h"


// Command functions
// Load and send <bytes> bytes of data from address <addr>
void baseband_send(uint32_t addr, uint32_t bytes) {
  reg_write32(BASEBAND_ADDITIONAL_DATA, addr);
  reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_SEND, 0, bytes));
}

// Configure baseband constant <target> (from secondary instruction set) to value <value>
void baseband_configure(uint8_t target, uint32_t value) {
  reg_write32(BASEBAND_ADDITIONAL_DATA, value);
  reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_CONFIG, target, 0));
}

// Try and receive data. Any found data will be stored at address <addr>
void baseband_receive(uint32_t addr) {
  reg_write32(BASEBAND_ADDITIONAL_DATA, addr);
  reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_RECEIVE, 0, 0));
}

// Exit receive mode
void baseband_receive_exit() {
  reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_RECEIVE_EXIT, 0, 0));
}

uint32_t baseband_status0() {
  return reg_read32(BASEBAND_STATUS0);
}

void baseband_get_status0(baseband_status0_t* status) {
    uint32_t status0 = baseband_status0();
    status->assembler_state = (status0 >> 0) & 0x7;
    status->disassembler_state = (status0 >> 3) & 0x7;
    status->tx_state = (status0 >> 6) & 0x3;
    status->rx_controller_state = (status0 >> 8) & 0x7;
    status->tx_controller_state = (status0 >> 11) & 0x3;
    status->controller_state = (status0 >> 13) & 0x7;
    status->adc_i_data = (status0 >> 16) & 0xFF;
    status->adc_q_data = (status0 >> 24) & 0xFF;
}

uint8_t baseband_read_adc_i() {
    return (baseband_status0() >> 16) & 0xFF;
}

uint8_t baseband_read_adc_q() {
    return (baseband_status0() >> 24) & 0xFF;
}

uint32_t baseband_status1() {
  return reg_read32(BASEBAND_STATUS1);
}

uint32_t baseband_status2() {
  return reg_read32(BASEBAND_STATUS2);
}

uint32_t baseband_status3() {
  return reg_read32(BASEBAND_STATUS3);
}

uint32_t baseband_status4() {
  return reg_read32(BASEBAND_STATUS4);
}


// TODO: Find a way to check valid flags to avoid hanging
uint32_t baseband_rxerror_message() {
  return reg_read32(BASEBAND_RXERROR_MESSAGE);
}

uint32_t baseband_txerror_message() {
  return reg_read32(BASEBAND_TXERROR_MESSAGE);
}

uint32_t baseband_rxfinish_message() {
  return reg_read32(BASEBAND_RXFINISH_MESSAGE);
}


void baseband_set_lut(uint8_t lut, uint8_t address, uint32_t value) {
  reg_write32(BASEBAND_LUT_CMD, LUT_COMMAND(lut, address, value));
}

// Function written for writing to tuning MMIO, but technically can be used for all MMIO although unchecked
// For partial write (4 bits), offset is used to shift data to correct position
void baseband_tuning_set(uint32_t addr, uint32_t data, uint32_t bit_size, uint32_t offset) {
  // For partial writes, need to read, mask, write-back
  if (bit_size == 4) {
    uint8_t temp = (reg_read8(addr) & ~(15 << offset)) | (data << offset);
    // Debug print
    // printf("%x\n", temp);
    reg_write8(addr, temp);
  } else if (bit_size <= 8) {
    reg_write8(addr, data);
  } else {
    reg_write16(addr, data);
  }
}

// Function that tests (send + check) the baseband debug command
void baseband_debug(uint32_t addr, uint8_t byte_size) {
  /* Debug command:
    Turns on both the RX and TX paths according to the loopback mask and passes the specified number of PDU
    header and data bytes in a loop. For simplicity the return data is stored at <load address + total bytes>,
    rounded to the nearest byte aligned address.
    [ Data = <total bytes> | secondaryInst = <loopback mask> | primaryInst = 15 ]
    [ additionalData = <load address> ]
  */
  // Sending baseband DEBUG instruction
  reg_write32(BASEBAND_ADDITIONAL_DATA, addr);
  
  reg_write32(BASEBAND_INST, BASEBAND_INSTRUCTION(BASEBAND_DEBUG, 0b11, byte_size));
}
