#include <zephyr/kernel.h>             // Funções básicas do Zephyr (ex: k_msleep, k_thread, etc.)
#include <zephyr/device.h>             // API para obter e utilizar dispositivos do sistema
#include <zephyr/drivers/gpio.h>       // API para controle de pinos de entrada/saída (GPIO)

#define INPUT_PORT  "gpio@400ff100"   // Porta E = GPIO_4 no seu .dts
#define INPUT_PIN   20         // PTE20

void main(void)
{
    const struct device *input_dev;
    int ret, val;

    input_dev = device_get_binding(INPUT_PORT);
    if (!input_dev) {
        printk("Erro ao acessar porta %s\n", INPUT_PORT);
        return;
    }

    ret = gpio_pin_configure(input_dev, INPUT_PIN, GPIO_INPUT);
    if (ret != 0) {
        printk("Erro ao configurar pino %d\n", INPUT_PIN);
        return;
    }

    while (1) {
        val = gpio_pin_get(input_dev, INPUT_PIN);
        printk("Valor do PTE20: %d\n", val);
        k_msleep(500);
    }
}