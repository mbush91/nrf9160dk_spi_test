// https://github.com/too1/ncs-spi-master-slave-example

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_slave_main, LOG_LEVEL_INF);

#define SLEEP_TIME_MS 1000
#define LED0_NODE DT_ALIAS(led0)
#define THE_SPI_SLAVE DT_NODELABEL(the_spi_slave)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static const struct device *spi_slave_dev;
static struct k_poll_signal spi_slave_done_sig = K_POLL_SIGNAL_INITIALIZER(spi_slave_done_sig);
static const struct spi_config spi_slave_cfg = {
	.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
				 SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_OP_MODE_SLAVE,
	.frequency = 4000000,
	.slave = 0,
};
static uint8_t slave_tx_buffer[2];
static uint8_t slave_rx_buffer[2];

int spi_slave_init(void) {
    spi_slave_dev = DEVICE_DT_GET(THE_SPI_SLAVE);
	if(!device_is_ready(spi_slave_dev)) {
		LOG_ERR("SPI slave device not ready!\n");
        return -EIO;
	}

    return 0;
}

static int spi_slave_write_test_msg(void)
{
	static uint8_t counter = 0;


	const struct spi_buf s_tx_buf = {
		.buf = slave_tx_buffer,
		.len = sizeof(slave_tx_buffer)
	};
	const struct spi_buf_set s_tx = {
		.buffers = &s_tx_buf,
		.count = 1
	};

	struct spi_buf s_rx_buf = {
		.buf = slave_rx_buffer,
		.len = sizeof(slave_rx_buffer),
	};
	const struct spi_buf_set s_rx = {
		.buffers = &s_rx_buf,
		.count = 1
	};

	// Update the TX buffer with a rolling counter
	slave_tx_buffer[1] = counter++;
	LOG_INF("SPI SLAVE TX: 0x%.2x, 0x%.2x\n", slave_tx_buffer[0], slave_tx_buffer[1]);

	// Reset signal
	k_poll_signal_reset(&spi_slave_done_sig);
	
	// Start transaction
	int error = spi_transceive_signal(spi_slave_dev, &spi_slave_cfg, &s_tx, &s_rx, &spi_slave_done_sig);
	if(error != 0){
		LOG_ERR("SPI slave transceive error: %i\n", error);
		return error;
	}
	return 0;
}

static int spi_slave_check_for_message(void)
{
	int signaled, result;
	k_poll_signal_check(&spi_slave_done_sig, &signaled, &result);
	if(signaled != 0){
		return 0;
	}
	else return -1;
}

int main(void)
{
    int err;

    LOG_INF("Hello from slave");

    err = spi_slave_init();

    spi_slave_write_test_msg();


    while(1) {
        LOG_INF("I'm Alive");
		err = gpio_pin_toggle_dt(&led);
		if (err < 0) {
			return 0;
		}
		k_msleep(SLEEP_TIME_MS);

		if(spi_slave_check_for_message() == 0){
			// Print the last received data
			LOG_INF("SPI SLAVE RX: 0x%.2x, 0x%.2x\n", slave_rx_buffer[0], slave_rx_buffer[1]);
			
			// Prepare the next SPI slave transaction
			spi_slave_write_test_msg();
		}
    }

    return 0;
}

