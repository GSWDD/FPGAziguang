#include "system.h"
#include "icb_ctrl.h"

void ICB_Tx(uint32_t TxAddr, uint32_t Length)
{
    uint32_t icb_tx_addr = TxAddr;
    uint32_t icb_tx_len = Length;

    for (int i = 0; i < icb_tx_len; i += 4){
        SYS_RWMEM_W(s7_BASEADDR) = *(uint32_t *)icb_tx_addr;
        icb_tx_addr += 4;
    }
}

void ICB_Rx(uint32_t RxAddr, uint32_t Length)
{
    uint32_t icb_rx_addr = RxAddr;
    uint32_t icb_rx_len = Length;

    for (int i = 0; i < icb_rx_len; i += 4){
        *(uint32_t *)icb_rx_addr = SYS_RWMEM_W(s7_BASEADDR);
        icb_rx_addr += 4;
    }
}
