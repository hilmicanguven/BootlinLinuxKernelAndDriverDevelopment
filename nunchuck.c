// SPDX-License-Identifier: GPL-2.0

#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>


static int nunchuk_probe(struct i2c_client * client)
{
    pr_info("Function: %s - Line: %d \n", __func__, __LINE__);
    return 0;
}

/**
 * @brief The clean up function for driver
 * 
 * @note the function should never return error even something goes wrong during cleanup
 * 
 */
static void nunchuk_remove(struct i2c_client * client)
{
    
}

static const struct of_device_id nunchuk_of_match[] = {
    { .compatible = "nintendo,nunchuk", },
    { /* sentinel last element to understand the end of list */},
};

/**
 *
 */
static struct i2c_driver nunchuk_i2c_driver = {

    .probe = nunchuk_probe,
    .remove = nunchuk_remove,
    .driver = {
        .of_match_table = nunchuk_of_match,
        .name = "Nunchuk",
    },
}

/**
 * @brief The function that register our module into kernel.
 * It also add init and exit functions.
 */
module_i2c_driver(nunchuk_i2c_driver);

MODULE_LICENSE("GPL");