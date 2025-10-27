#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

static const struct device* hv_shield = DEVICE_DT_GET(DT_NODELABEL(hv_shield0));
static const struct emul* hv_shield_emul =
    EMUL_DT_GET(DT_NODELABEL(hv_shield0));
