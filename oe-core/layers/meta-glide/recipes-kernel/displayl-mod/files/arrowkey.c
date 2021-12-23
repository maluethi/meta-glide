/*
 * This is all inspired / copy and pasted from:
 *      https://github.com/JamesGKent/rotary_volume/blob/master/module/rotary_volume.c
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/device.h>

MODULE_AUTHOR("Andreas Lüthi");
MODULE_DESCRIPTION("rotary encoder as arrow key module");
MODULE_ALIAS("rotary-arrowkey");
MODULE_LICENSE("GPL");

static struct input_dev *button_dev;
static bool enter_key_pressed = false;
static bool esc_key_pressed = false;

static void send_key(int key) {
    input_report_key(button_dev, key, 1);
    input_sync(button_dev);
    input_report_key(button_dev, key, 0);
    input_sync(button_dev);
}

extern void power_reset_hard_power_off(bool reset);

static void rotary_event(struct input_handle *handle, unsigned int type, unsigned int code, int value) {

    //printk(KERN_DEBUG pr_fmt("Event. Dev: %s, Type: %d, Code: %d, Value: %d\n"), dev_name(&handle->dev->dev), type, code, value);
    if (type == EV_REL) {
        if (code == REL_X) {
            if (enter_key_pressed) {
                send_key((value > 0) ? KEY_DOWN : KEY_UP);
                power_reset_hard_power_off(true);
            } else {
                send_key((value > 0) ? KEY_RIGHT : KEY_LEFT);
            }
        } else if (code == REL_Y) {
            if (esc_key_pressed) {
                send_key((value > 0) ? KEY_BRIGHTNESSUP : KEY_BRIGHTNESSDOWN);
            } else {
                send_key((value > 0) ? KEY_DOWN : KEY_UP);
            }
        }
    } else if (type == EV_KEY) {
        if (code == KEY_ENTER) {
            enter_key_pressed = (value == 1);
            power_reset_hard_power_off(false);
        } else if (code == KEY_ESC){
            esc_key_pressed = (value == 1);
        }
    }
}

static int rotary_connect(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id) {
    struct input_handle *handle;
    int error;

    handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
    if (!handle)
        return -ENOMEM;

    handle->dev = dev;
    handle->handler = handler;
    handle->name = "gpio_arrowkey";

    error = input_register_handle(handle);
    if (error)
        goto err_free_handle;

    error = input_open_device(handle);
    if (error)
        goto err_unregister_handle;

    printk(KERN_DEBUG
    pr_fmt("Connected device: %s (%s at %s)\n"),
            dev_name(&dev->dev),
            dev->name ?: "unknown",
            dev->phys ?: "unknown");

    return 0;

    err_unregister_handle:
    input_unregister_handle(handle);
    err_free_handle:
    kfree(handle);
    return error;
}

bool startsWith(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
            lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

static bool rotary_match(struct input_handler *handler, struct input_dev *dev) {
    return startsWith("rotary-encoder", dev->name) || startsWith("gpio-keys", dev->name);
}

static void rotary_disconnect(struct input_handle *handle) {
    printk(KERN_DEBUG
    pr_fmt("Disconnected device: %s\n"), dev_name(&handle->dev->dev));
    input_close_device(handle);
    input_unregister_handle(handle);
    kfree(handle);
}

static const struct input_device_id rotary_ids[] = {
        {.driver_info = 1},    /* Matches all devices */
        {},                    /* Terminating zero entry */
};

MODULE_DEVICE_TABLE(input, rotary_ids
);

static struct input_handler rotary_handler = {
        .event =    rotary_event,
        .match =    rotary_match,
        .connect =    rotary_connect,
        .disconnect =    rotary_disconnect,
        .name =        "rotary_arrowkey",
        .id_table =    rotary_ids,
};

static int __init

button_init(void) {
    int error;

    button_dev = input_allocate_device();
    if (!button_dev) {
        printk(KERN_ERR
        pr_fmt("Not enough memory\n"));
        error = -ENOMEM;
        return error;
    }

    button_dev->name = "Rotary Encoder Arrow Key";
    button_dev->evbit[0] = BIT_MASK(EV_KEY);// | BIT_MASK(EV_REP);
    set_bit(KEY_LEFT, button_dev->keybit);
    set_bit(KEY_RIGHT, button_dev->keybit);
    set_bit(KEY_UP, button_dev->keybit);
    set_bit(KEY_DOWN, button_dev->keybit);
    set_bit(KEY_BRIGHTNESSUP, button_dev->keybit);
    set_bit(KEY_BRIGHTNESSDOWN, button_dev->keybit);

    error = input_register_device(button_dev);
    if (error) {
        printk(KERN_ERR
        pr_fmt("Failed to register device\n"));
        goto err_free_dev;
    }
    return 0;
    err_free_dev:
    input_free_device(button_dev);
    return error;
}

static int __init

rotary_arrowkey_init(void) {
    int error = button_init();
    if (error == 0) {
        if (input_register_handler(&rotary_handler) == 0) {
            printk(KERN_INFO
            pr_fmt("loaded.\n"));
            return 0;
        } else {
            input_unregister_device(button_dev);
            input_free_device(button_dev);
        }
    }
    return error;
}

static void __exit

rotary_arrowkey_exit(void) {
    input_unregister_device(button_dev);
    input_free_device(button_dev);
    input_unregister_handler(&rotary_handler);
}

module_init(rotary_arrowkey_init);
module_exit(rotary_arrowkey_exit);
