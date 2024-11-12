#ifndef DMA_CTRL_H
#define	DMA_CTRL_H

uint32_t icb_tx_addr;
uint32_t icb_tx_len;
uint32_t icb_rx_addr;
uint32_t icb_rx_len;
#define s6_BASEADDR         0xC0000000u
#define s7_BASEADDR         0xE0000000u

void ICB_Tx(uint32_t TxAddr, uint32_t Length);
void ICB_Rx(uint32_t RxAddr, uint32_t Length);

#endif
