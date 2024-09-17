#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_master_main, LOG_LEVEL_INF);

#define SLEEP_TIME_MS   1000

#define THE_SPI_MASTER DT_NODELABEL(the_spi_master)
#define THE_SPI_MASTER_CS_DT_SPEC SPI_CS_GPIOS_DT_SPEC_GET(DT_NODELABEL(reg_the_spi_master))

const struct device *spi_dev;
static struct spi_config spi_cfg = {
	.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA,
	.frequency = 125000,
	.slave = 0,
	.cs = {.gpio = THE_SPI_MASTER_CS_DT_SPEC, .delay = 0},
};


static void spi_init(void)
{
	spi_dev = DEVICE_DT_GET(THE_SPI_MASTER);
	if(!device_is_ready(spi_dev)) {
		LOG_ERR("SPI master device not ready!\n");
	}
	struct gpio_dt_spec spim_cs_gpio = THE_SPI_MASTER_CS_DT_SPEC;
	if(!device_is_ready(spim_cs_gpio.port)){
		LOG_ERR("SPI master chip select device not ready!\n");
	}
}

static uint8_t tx_buffer[2];
static uint8_t rx_buffer[2];
const struct spi_buf tx_buf = {
    .buf = tx_buffer,
    .len = sizeof(tx_buffer)
};
const struct spi_buf_set tx = {
    .buffers = &tx_buf,
    .count = 1
};

struct spi_buf rx_buf = {
    .buf = rx_buffer,
    .len = sizeof(rx_buffer),
};
const struct spi_buf_set rx = {
    .buffers = &rx_buf,
    .count = 1
};

void spi_master_cb(const struct device *dev, int result, void *data)
{
    LOG_INF("SPI CB Result: %d",result);
    LOG_HEXDUMP_INF(rx_buffer,sizeof(rx_buffer),"Data: ");
}

static int spi_write_test_msg(void)
{
	static uint8_t counter = 0;

	// Update the TX buffer with a rolling counter
	tx_buffer[0] = counter++;
	LOG_INF("SPI TX: 0x%.2x, 0x%.2x\n", tx_buffer[0], tx_buffer[1]);
	
	// Start transaction
	int error = spi_transceive_cb(spi_dev, &spi_cfg, &tx, &rx, spi_master_cb, NULL);
	if(error != 0){
		LOG_ERR("SPI transceive error: %i\n", error);
		return error;
	}

	return 0;
}

int main(void)
{

    spi_init();

    while (1) {
		spi_write_test_msg();

		k_msleep(SLEEP_TIME_MS);

        LOG_INF("Bump");
	}

    return 0;
}
