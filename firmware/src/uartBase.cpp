/**
 * uartBase.cpp
 *
 * Author: David Antaki
 * Date: 9/24/2022
 */

#include "uartBase.h"
uartBase::uartBase(const struct device &_uartDev) : uartDev(_uartDev) {}
uartBase::~uartBase() {}

bool uartBase::initialize(){
    bool success = true;

	uart_irq_callback_user_data_set(&uartDev, uartBase::uart_fifo_callback, this);

	// Enable RX interrupt
	uart_irq_rx_enable(&uartDev);
    return success;
}

// Just a wrapper for the time being
int uartBase::poll_read(unsigned char &p_char){
    return uart_poll_in(&uartDev, &p_char);
}

void uartBase::uart_fifo_callback(const struct device *dev, void *uartInstance)
{
    uartBase* pThis(static_cast<uartBase*>(uartInstance));
	static int tx_data_idx;

	/* Verify uart_irq_update() */
	if (!uart_irq_update(dev)) {
		printk("retval should always be 1\n");
		return;
	}

	/* Verify uart_irq_tx_ready() */
	/* Note that TX IRQ may be disabled, but uart_irq_tx_ready() may
	 * still return true when ISR is called for another UART interrupt,
	 * hence additional check for i < DATA_SIZE.
	 */
	if (uart_irq_tx_ready(dev) && tx_data_idx < txFifoSize) {
		/* We arrive here by "tx ready" interrupt, so should always
		 * be able to put at least one byte into a FIFO. If not,
		 * well, we'll fail test.
		 */
		if (uart_fifo_fill(dev,
				   (uint8_t *)&pThis->txFifo[tx_data_idx++], 1) > 0) {
		}

		if (tx_data_idx == txFifoSize) {
			/* If we transmitted everything, stop IRQ stream,
			 * otherwise main app might never run.
			 */
			uart_irq_tx_disable(dev);
		}
	}

	/* Verify uart_irq_rx_ready() */
	if (uart_irq_rx_ready(dev)) {
		/* Verify uart_fifo_read() */
		uart_fifo_read(dev, pThis->rxFifo, rxFifoSize);
		// printk("%c", recvData);
	}
    uint8_t test = 0;
}