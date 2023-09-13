#include <asm/delay.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#define MIN_ABS_X       0
#define MAX_ABS_X       1920
#define MIN_ABS_Y       0
#define MAX_ABS_Y       1080

#define MAX_CONTACTS    5

#define DEVICE_NAME "virtual_touchscreen"

static struct input_dev *ts_dev;

static int __init virtual_ts_init(void)
{
    int err;
    ts_dev = input_allocate_device();
    if (!ts_dev)
        return -ENOMEM;

    ts_dev->id.bustype = BUS_I2C;
    ts_dev->id.vendor = 0x0001;
    ts_dev->id.product = 0x0001;
    ts_dev->id.version = 0x0001;
    ts_dev->evbit[0] = BIT_MASK(EV_ABS) | BIT_MASK(EV_KEY);
    ts_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

    input_set_abs_params(ts_dev, ABS_X, MIN_ABS_X, MAX_ABS_X, 0, 0);
    input_set_abs_params(ts_dev, ABS_Y, MIN_ABS_Y, MAX_ABS_Y, 0, 0);
    ts_dev->name = "Virtual touchscreen";
    ts_dev->phys = "virtual_ts/input0";


    input_mt_init_slots(ts_dev, MAX_CONTACTS, INPUT_MT_DIRECT);
    input_set_abs_params(ts_dev, ABS_MT_POSITION_X, MIN_ABS_X, MAX_ABS_X, 0, 0);
    input_set_abs_params(ts_dev, ABS_MT_POSITION_Y, MIN_ABS_Y, MAX_ABS_Y, 0, 0);

    err = input_register_device(ts_dev);
    if (err)
        goto fail1;

    return 0;

 fail1:
    input_free_device(ts_dev);
    return err;
}


static void __exit virtual_ts_exit(void)
{
    input_unregister_device(ts_dev);
    return;
}

module_init(virtual_ts_init);
module_exit(virtual_ts_exit);

MODULE_AUTHOR("Amlogic, Inc");
MODULE_DESCRIPTION("Virtual touchscreen driver");
MODULE_LICENSE("GPL");
