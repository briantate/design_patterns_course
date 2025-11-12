#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/pwm.h>

#define STEP PWM_USEC(100)

enum direction {
	DOWN,
	UP,
};

static const struct pwm_dt_spec servo_left = PWM_DT_SPEC_GET(DT_NODELABEL(servo_left));
static const struct pwm_dt_spec servo_right = PWM_DT_SPEC_GET(DT_NODELABEL(servo_right));
static const uint32_t min_pulse = DT_PROP(DT_NODELABEL(servo_left), min_pulse);
static const uint32_t max_pulse = DT_PROP(DT_NODELABEL(servo_left), max_pulse);

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
    int ret = 0;
    uint32_t pulse_width_left = min_pulse;
    uint32_t pulse_width_right = max_pulse;
	enum direction dir = UP;

	printk("Servomotor control\n");

	if (!pwm_is_ready_dt(&servo_left)) {
		printk("Error: PWM device %s is not ready\n",
		       servo_left.dev->name);
		return 0;
	}
	if (!pwm_is_ready_dt(&servo_right)) {
		printk("Error: PWM device %s is not ready\n",
		       servo_right.dev->name);
		return 0;
	}

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

    while (1) {
        gpio_pin_toggle_dt(&led);
        ret = pwm_set_pulse_dt(&servo_left, pulse_width_left);

        if (ret < 0) {
			printk("Error %d: failed to set pulse width left\n", ret);
			return 0;
		}

        ret = pwm_set_pulse_dt(&servo_right, pulse_width_right);
		if (ret < 0) {
			printk("Error %d: failed to set pulse width right\n", ret);
			return 0;
		}

		if (dir == DOWN) {
			if (pulse_width_left <= min_pulse) {
				dir = UP;
				pulse_width_left = min_pulse;
			} else {
				pulse_width_left -= STEP;
			}

            pulse_width_right += STEP;

			if (pulse_width_right >= max_pulse) {
				dir = DOWN;
				pulse_width_right = max_pulse;
			}
		} else {
			pulse_width_left += STEP;

			if (pulse_width_left >= max_pulse) {
				dir = DOWN;
				pulse_width_left = max_pulse;
			}

            if (pulse_width_right <= min_pulse) {
				dir = UP;
				pulse_width_right = min_pulse;
			} else {
				pulse_width_right -= STEP;
			}
		}
        k_msleep(500); /* toggle every 500 ms */
    }
    return ret;
}
