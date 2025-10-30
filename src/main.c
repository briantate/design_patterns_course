#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/pwm.h>

#define PWM0_NODE DT_ALIAS(pwm0)
#define PWM_PERIOD_USEC 20000U  // 20 ms period (50 Hz)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

/* Callback structure */
static struct gpio_callback button_cb_data;

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    gpio_pin_toggle_dt(&led1);
    printk("Button pressed!\n");
}

int main(void)
{
    int ret;

    const struct device *pwm_dev = DEVICE_DT_GET(PWM0_NODE);

    if (!device_is_ready(pwm_dev)) {
        printk("Error: PWM device not ready\n");
        ret = -1;
    }

    uint32_t pulse_channel_0 = PWM_PERIOD_USEC / 4; // 25% duty cycle
    uint32_t pulse_channel_1 = PWM_PERIOD_USEC / 2; // 50% duty cycle

    if (!device_is_ready(led.port)) {
        printk("Error: LED device %s is not ready\n", led.port->name);
        return ret;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: failed to configure LED pin\n", ret);
        return ret;
    }
    if (!device_is_ready(led1.port)) {
        printk("Error: LED device %s is not ready\n", led1.port->name);
        return ret;
    }

    ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: failed to configure LED pin\n", ret);
        return ret;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret < 0) {
        printk("Failed to configure button: %d\n", ret);
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        printk("Failed to configure interrupt: %d\n", ret);
        return ret;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    printk("Button-LED example running on %s\n", CONFIG_BOARD);

    ret = pwm_set(pwm_dev, 0, PWM_PERIOD_USEC, pulse_channel_0, 0);
    printk("pwm_set channel 0 ret=%d\n", ret);
    ret = pwm_set(pwm_dev, 1, PWM_PERIOD_USEC, pulse_channel_1, 0);
    printk("pwm_set channel 1 ret=%d\n", ret);

    while (1) {
        gpio_pin_toggle_dt(&led);
        k_msleep(500); /* toggle every 500 ms */
    }
    return ret;
}
