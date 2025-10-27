#include <stdint.h>

#include <zephyr/drivers/emul.h>

void hv_shield_v2_emul_set_reg(const struct emul* target,
                               uint8_t reg_addr,
                               const uint8_t val);

uint8_t hv_shield_v2_emul_get_reg(const struct emul* target, uint8_t reg_addr);

uint16_t hv_shield_v2_emul_get_u16_reg(const struct emul* target,
                                       uint8_t reg_addr);
