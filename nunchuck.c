// SPDX-License-Identifier: GPL-2.0

/**

@brief 

Most of the magic numbers come from nunchuk device datasheet.

 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
/* *
to provide delay functionfor consecutive i2c data send operations.
this header is generic and contains architecture specific implementation.
*/
#include <linux/delay.h>



static int nunchuk_read_registers(struct i2c_client* client)
{
    const int msg_size = 6;
    char command = 0x0;
    char buff[msg_size];
    int ret;
    /**
    wake me up somewhere between 10ms and 20ms.
    it relaxes the timer subsystem by using same timer for multiple events */
    usleep_range(10000, 20000); 

    ret = i2c_master_send(client, &command, 1);

    if(ret != 1)
    {
        return -EINVAL;
    }

    usleep_range(10000, 20000); 
    
    ret = i2c_master_recv(client, &buff, msg_size);

    if(ret != msg_size)
    {
        return -EINVAL;
    }

    /**
    last two bits of last byte of the message corresponds to buttons status */
    pr_info("Button status: 0x%02x \n", buff[5] & 0x3);

    return 0;
}

/** @brief The function initializes the i2c device
*/
static int nunchuk_probe(struct i2c_client * client)
{
    const int msg_size = 2;
    char buff[msg_size];
    int ret;

    pr_info("Function: %s - Line: %d \n", __func__, __LINE__);
    
    buff[0] = 0xf0;
    buff[1] = 0x55;

    ret = i2c_master_send(client, buff, msg_size);
    
    if(ret != msg_size)
    {
        return -EINVAL;
    }

    udelay(1000);

    buff[0] = 0xfb;
    buff[1] = 0x00;

    ret = i2c_master_send(client, buff, msg_size);
    
    if(ret != msg_size)
    {
        return -EINVAL;
    }
    
    /**
    datasheet/instructions says two read operation is required */
    ret = nunchuk_read_registers(client);
    if(ret != 0)
    {
        return -EINVAL;
    }

    return nunchuk_read_registers(client);
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