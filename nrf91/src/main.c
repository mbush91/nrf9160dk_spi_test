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

static const struct device *const spi_slave_dev = DEVICE_DT_GET(THE_SPI_SLAVE);
static const struct spi_config spi_slave_cfg = {
	.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
				 SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_OP_MODE_SLAVE,
	.frequency = 125000,
    .slave = 1,
};
static uint8_t slave_tx_buffer[2];
static uint8_t slave_rx_buffer[2];

int spi_slave_init(void) {

	if(!device_is_ready(spi_slave_dev)) {
		LOG_ERR("SPI slave device not ready!\n");
        return -EINVAL;
	}

    return 0;
}

SYS_INIT(spi_slave_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

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


int main(void)
{
    int err;
    static uint8_t counter = 0;

    LOG_INF("Hello from slave");

    if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (err < 0) {
		return 0;
	}

    while(1) {
        gpio_pin_toggle_dt(&led);

        slave_tx_buffer[1] = counter++;

        err = spi_read(spi_slave_dev, &spi_slave_cfg,&s_rx);

        if(err >= 0) {
            LOG_INF("SPI Ret Code: %d", err);
            LOG_HEXDUMP_INF(slave_rx_buffer,sizeof(slave_rx_buffer),"Data: ");
            spi_write(spi_slave_dev, &spi_slave_cfg,&s_rx);
        } else {
            LOG_ERR("SPI ERR: %d", err);
        }
    }

    return 0;
}

