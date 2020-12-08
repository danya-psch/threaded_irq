#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Danyil Peschanskyi");
MODULE_VERSION("0.1");

#define GPIO_NUMBER(port, bit) (32 * (port) + (bit))

#define LED_SD  GPIO_NUMBER(1, 22)
#define LED_MMC GPIO_NUMBER(1, 24)
#define BUTTON  GPIO_NUMBER(2, 8)

#define EXEC_THREAD_TIME 100L

static bool simulate_busy = false;
module_param(simulate_busy, bool, 0644);

static struct dentry *dir = 0;

static int led_gpio = -1;
static int button_gpio = -1;

static unsigned int counter = 0;

static int led_gpio_init(int gpio)
{
	int rc;

	rc = gpio_direction_output(gpio, 0);
	if (rc)
		return rc;

	led_gpio = gpio;
	return 0;
}

static int button_gpio_init(int gpio)
{
	int rc;

	rc = gpio_request(gpio, "Onboard user button");
	if (rc)
		goto err_register;

	rc = gpio_direction_input(gpio);
	if (rc)
		goto err_input;

	button_gpio = gpio;
	pr_info("Init GPIO%d OK\n", button_gpio);
	return 0;

err_input:
	gpio_free(gpio);
err_register:
	return rc;
}

static void button_gpio_deinit(void)
{
	if (button_gpio >= 0) {
		gpio_free(button_gpio);
		pr_info("Deinit GPIO%d\n", button_gpio);
	}
}

static irqreturn_t button_handler(int irq, void *dev_id)
{
	unsigned int *cntr = (unsigned int *)dev_id;
	++(*cntr);
	if (led_gpio) {
		gpio_set_value(led_gpio, 0);
		led_gpio = (led_gpio == LED_SD) ? LED_MMC : LED_SD;
		gpio_set_value(led_gpio, 1);
	}
	return IRQ_WAKE_THREAD;
}

static irqreturn_t button_thread(int irq, void *dev_id)
{
	unsigned int cntr = *(unsigned int *)dev_id;
	if (simulate_busy)
		msleep(EXEC_THREAD_TIME);
	pr_info("counter: %d\n", cntr);
	return IRQ_HANDLED;
}

static int __init threaded_irq_init(void)
{
	struct dentry *cntr;
	int rc, gpio, button_state, button_irq;

	rc = button_gpio_init(BUTTON);
	if (rc) {
		pr_err("Can't set GPIO%d for button\n", BUTTON);
		goto err_button;
	}

	dir = debugfs_create_dir(KBUILD_MODNAME, NULL);
	if (!dir) {
		pr_err("Failed to create dir\n");
		goto err_dir;
	}
	cntr = debugfs_create_u32("counter", 0444, dir, &counter);
	if (!cntr) {
		pr_err("Failed to create counter\n");
		goto err_counter;
	}

	button_irq = gpio_to_irq(button_gpio);
	rc = request_threaded_irq(button_irq, button_handler, button_thread,
	                          IRQF_TRIGGER_FALLING, "test", &counter);

	if (rc) {
		pr_err("Can't set threaded irq\n");
		goto err_irq;
	}

	button_state = gpio_get_value(button_gpio);

	gpio = button_state ? LED_MMC : LED_SD;

	rc = led_gpio_init(gpio);
	if (rc) {
		pr_err("Can't set GPIO%d for output\n", gpio);
		goto err_led;
	}

	gpio_set_value(led_gpio, 1);
	pr_info("LED at GPIO%d ON\n", led_gpio);

	return 0;

err_led:
	button_gpio_deinit();
err_irq:
	free_irq(button_irq, &counter);
err_counter:
err_dir:
err_button:
	return rc;
}

static void __exit threaded_irq_exit(void)
{
	if (led_gpio >= 0) {
		gpio_set_value(led_gpio, 0);
		pr_info("LED at GPIO%d OFF\n", led_gpio);
	}
	button_gpio_deinit();
	debugfs_remove_recursive(dir);
}

module_init(threaded_irq_init);
module_exit(threaded_irq_exit);
