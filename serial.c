// SPDX-License-Identifier: GPL-2.0
#include "linux/printk.h"
#include <linux/completion.h>
#include <linux/dma-mapping.h> /* to access dma resource map api's*/
#include <linux/atomic.h>
#include <linux/fs.h> /* to access file operations struct*/
#include <linux/dmaengine.h>
#include <linux/spinlock.h>	/* to access spinlock locking mechanism objetcs */
#include <linux/spinlock_types.h> /* to access spinlock locking mechanism objetcs */
#include <linux/wait.h> /* to access to wait on queue functions */
#include <linux/interrupt.h> /* to access irq related definitions */
#include <linux/pm_runtime.h> /* to access power management related functions */
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/serial_reg.h> /* to access uart peripheral registers' definitions */
#include <linux/io.h> /* to access read/write functions from/to mmio region */
#include <linux/clk.h>

/** @brief ioctl command to reset counter to zero of how many bytes written */
#define IOCTL_CMD_SERIAL_RESET_COUNTER 0
/** @brief ioctl command to get counter of how many bytes written */
#define IOCTL_CMD_SERIAL_GET_COUNTER 1

/**
 * Register and Bit values to enable DMA on UART peripheral
 */
#define OMAP_UART_SCR_DMAMODE_CTL3 0x7
#define OMAP_UART_SCR_TX_TRIG_GRANU1 BIT(6)

/** @brief The size of UART Receive Buffer (bytes were read will be stored in this buffer) */
#define SERIAL_BUFSIZE 16

/** @brief Serial device structure that describes peripherals */
struct serial_dev {
	/** @brief memory mapped i/o registers base address */
	void __iomem *regs;
	
	/** @brief serial driver allocates misc device for itself */
	struct miscdevice miscdev;
	
	/** @brief counter of sending bytes (number of characters to be written to terminal) */
	atomic_t counter;
	
	/** @brief UART Receive Buffer (bytes were read will be stored in this buffer). 
	it is filled within interrupt handler. */
	char rx_buf[SERIAL_BUFSIZE];
	
	/** @brief used for dma operations */
	char tx_buf[SERIAL_BUFSIZE];
	
	/** @brief The cursor of read buffer where last index that we read. used for implementing circular buffer */
	unsigned int buf_rd;
	
	/** @brief The cursor of read buffer where last index that we write. used for implementing circular buffer */
	unsigned int buf_wr;
	
	/** @brief it is used in read function until data becomes available on `rx_buf` */
	wait_queue_head_t wait;
	
	/** @brief Driver spinlock object to prevent concurrent access to shared resources */
	spinlock_t lock;
	
	/** @brief resource pointer to show register physical address of devices */
	struct resource *res;

	/** @brief device structure comes from platform device. it is used for DMA address mappings */
	struct device *dev;

	/** @brief the dma channel used for serial tx operations. this channel is requested dma engine framework */
	struct dma_chan *txchan;

	/** @brief dma fifo buffer address used by dma apis. it is filled address that is mapped */
	dma_addr_t fifo_dma_addr;

	/** @brief The flag to show whether there is ongoing dma operation or not. if it is, wait until it is completed */
	bool txongoing;
	// struct completion txcomp;
};

/* -------- wrappers functions to access registers -------- */
/*
in general, serial peripheral register addresses incremented by 1 but accesses as a word length
therefore, next register address is incremented by 4 bytes.
*/
static u32 reg_read(struct serial_dev *serial, unsigned int reg)
{
	return readl(serial->regs + (reg * 4));
}

static void reg_write(struct serial_dev *serial, u32 val, unsigned int reg)
{
	writel(val, serial->regs + (reg * 4));
}

static void serial_write_one_char(struct serial_dev *serial, char c) 
{
	unsigned long flags;

	spin_lock_irqsave(&serial->lock, flags);

	// wait until transmit buffer is empty and ready to send one character
	while (!(reg_read(serial, UART_LSR) & UART_LSR_THRE))
	{
		// ensure that cpu does not optimize the loop (it may optimize and may remove while loop)	
		// in ARM, this only calls `barrier()` or `nop` instructions
		cpu_relax();
	}

	// put the character into transmit register of uart
	reg_write(serial, c, UART_TX);
	
	spin_unlock_irqrestore(&serial->lock, flags);

	/**
	on some processors, even `counter++` may not be atomic.
	and this counter is resetted in somewhere else in the driver.
	therefore, it is better to use atomic operations to increment its value instead of `serial->counter++` .
	 */
	atomic_inc(&serial->counter);
}

/**
 * @brief Read function for serial device. The data will be read 'rx_buf' of serial device.
 if there is no data inside of it, then sleep until data becomes available
 * @param file
 * @param buf , buffer coming from user space (specified by __user)
 * @param sz
 * @param oofs
 * @return _EOPNOTSUPP, the function is not supported yet. 
 */
static ssize_t serial_read(struct file *file, char __user *buf, size_t sz, loff_t *offs)
{
	struct miscdevice *miscdev_ptr = file->private_data;
	struct serial_dev *serial = container_of(miscdev_ptr, struct serial_dev, miscdev);

	unsigned long flags;
	char c;

	/* wait until condition (buffer not empty) occurs. it is interruptible 
	*/
	wait_event_interruptible(serial->wait, serial->buf_rd != serial->buf_wr);

	/*
	used spinlock to protect register access and buffer operations.it used irqsave and restore because
	
	If we use holds a normal spin_lock() and 
	an interrupt (in this case, irq is triggered whenever uart peripheral receives a byte) triggered on the same local CPU and 
	the IRQ handler tries to take the same lock
	!! deadlock occurs, as a result of that:
    	process context holds the lock
    	IRQ handler spins forever waiting for it
		process context can’t run to release it (it is preempted by the interrupt and never return)

	to prevent this, we must disable local interrupts while holding the spinlock.
	that's what _irqsave and _restore are used for

	note: flags variable hold interrupt state before disabled.
	 */
	spin_lock_irqsave(&serial->lock, flags);

	c = serial->rx_buf[serial->buf_rd++];

	if (SERIAL_BUFSIZE == serial->buf_rd)
	{
		serial->buf_rd = 0;
	}

	spin_unlock_irqrestore(&serial->lock, flags);

	// copy character into user buffer
	put_user(c, buf);

	return 1;
}

/** @brief function to copy user data to the serial port, writing characters one by one */
static ssize_t serial_write_pio(struct file *file, const char __user *buf, size_t sz, loff_t *offs) 
{
	/* 
	The first thing to do is to retrieve the serial_dev structure from the miscdevice structure itself, accessible
	through the private_data field of the open file structure (file).
	At the time we registered our misc device, we didn’t keep any pointer to the serial_dev structure. However,
	as the struct miscdevice structure is accessible through file->private_data, and is a member of the
	serial_dev structure, we can use a magic macro to compute the address of the parent structure:
	*/
	struct miscdevice *miscdev_ptr = file->private_data;
	/**
	find parent structure address which embeds miscdev_ptr address whose field name is miscdev */
	struct serial_dev *serial = container_of(miscdev_ptr, struct serial_dev, miscdev);

	int i, ret;
	char c;

	for (i = 0; i < sz; i++) 
	{
		/* buffer is user space buffer, therefore we should first map into kernel space.
		it is forbidden to directly use it. use one of possible copy function between spaces.
		*/
		ret = get_user(c, &buf[i]);
		if (ret) 
			return -EFAULT;

		serial_write_one_char(serial, c);

		if (c == '\n')
			serial_write_one_char(serial, '\r');
	}

	return sz;
}

// static void serial_tx_done(void *param)
// {
// 	struct serial_dev *serial = param;
// 	complete(&serial->txcomp);
// }

static ssize_t serial_write_dma(struct file *file, const char __user *buf, size_t sz, loff_t *offs) 
{
	struct miscdevice *miscdev_ptr = file->private_data;
	struct serial_dev *serial = container_of(miscdev_ptr, struct serial_dev, miscdev);
	
	// flags for spinlock irq save and restore
	unsigned long flags;
	dma_addr_t dma_addr;
	dma_cookie_t cookie;
	// length of the transfer
	size_t len;
	int ret = 0;
	struct dma_async_tx_descriptor *desc;

	/* Prevent concurrent Tx */
	spin_lock_irqsave(&serial->lock, flags);
	
	if (serial->txongoing) {
		spin_unlock_irqrestore(&serial->lock, flags);
		return -EBUSY;
	}

	serial->txongoing = true;
	spin_unlock_irqrestore(&serial->lock, flags);

	// transfer len is either buffer length or user given sz 
	len = min_t(size_t, SERIAL_BUFSIZE, sz);

	// get user space buffer into driver buffer
	ret = copy_from_user(serial->tx_buf, buf, len);
	if (ret)
		goto err;

	// there is a hw bug (specific to ti beaglebone/play SoCs) only for the first byte. therefore we need to send it manually
	dma_addr = dma_map_single(serial->dev, serial->tx_buf, len, DMA_TO_DEVICE);
	if (dma_mapping_error(serial->dev, dma_addr))
		goto err;

	desc = dmaengine_prep_slave_single(serial->txchan, dma_addr, len, DMA_MEM_TO_DEV, DMA_PREP_INTERRUPT | DMA_CTRL_ACK);

	if (!desc) {
		dma_unmap_single(serial->dev, dma_addr, len, DMA_TO_DEVICE);
		goto err;
	}

	desc->callback = serial_tx_done;
	desc->callback_param = serial;
	
	cookie = dmaengine_submit(desc);

	ret = dma_submit_error(cookie);
	if (ret)
		goto err;

	dma_async_issue_pending(serial->txchan);

	wait_for_completion(&serial->txcomp);

	dma_unmap_single(serial->dev, dma_addr, len, DMA_TO_DEVICE);

	spin_lock_irqsave(&serial->lock, flags);
	serial->txongoing = false;
	spin_unlock_irqrestore(&serial->lock, flags);

	return len;
err:
	spin_lock_irqsave(&serial->lock, flags);
	serial->txongoing = false;
	spin_unlock_irqrestore(&serial->lock, flags);
	return ret;
}

/** @brief The io control function to provide drivers specific commands */
static long serial_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct miscdevice *miscdev_ptr = file->private_data;
	struct serial_dev *serial = container_of(miscdev_ptr, struct serial_dev, miscdev);

	int ret;

	switch(cmd) 
	{
	case IOCTL_CMD_SERIAL_RESET_COUNTER:
		atomic_set(&serial->counter, 0);
		break;
	
	case IOCTL_CMD_SERIAL_GET_COUNTER:
		// copy data to user space
		ret = put_user(atomic_read(&serial->counter), (unsigned int __user *) arg);
		if (ret)
			return -EFAULT;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/** @brief The interrupt handler that triggered when uart controller receives a message 
 *	@param `irq` IRQ number
 *	@param `dev_id` input parameter specified when creating irq handler
 *	@return IRQ_HANDLED, irq handled successfully
 *
 * @note in most cases, we need to acknowledge the interrupt controller that we get the interrupt and operate it
 * otherwise, it continuously generates same interrupt. this acknowledgement is hardware specific. for our board, it 
 * is simply enough to read RX register of uart peripheral.
*/
static irqreturn_t serial_interrupt(int irq, void *dev_id) 
{
	struct serial_dev *serial = dev_id;
	char c;

	/*
	used spinlock to protect register access and buffer operations.
	
	this handler can not be interrupted by itself on the same local CPU
	therefore IRQ handlers do not need to disable interrupts again for self-protection.
	but still need to be locked to prevent other cpu cores
	unnecessary to use _irqlock/restore in this case

	 */
	spin_lock(&serial->lock);

	c = reg_read(serial, UART_RX);

	serial->rx_buf[serial->buf_wr++] = c;
	
	if (SERIAL_BUFSIZE == serial->buf_wr)
	{
		serial->buf_wr = 0;
	}

	if (serial->buf_wr == serial->buf_rd) 
	{
		serial->buf_rd++;
		if (SERIAL_BUFSIZE == serial->buf_rd)
		{
			serial->buf_rd = 0;
		}
	}

	spin_unlock(&serial->lock);

	wake_up(&serial->wait);

	return IRQ_HANDLED;
}

/**
 * @brief File operations for our serial device. this structure should have function pointers
 for file operations and their prototypes are defined in <linux/fs.h>

 @note pio stands for polling io
 */
static const struct file_operations serial_fops_pio = {
	/** open function (default implemented) to this module is incremented counter of users number for this module
	otherwise, there is situation where kernel crashes when we rmmod while device is opened */
	.owner = THIS_MODULE,
	.read = serial_read,
	.write = serial_write_pio,
	.unlocked_ioctl = serial_ioctl,
};

/** @brief File operation with DMA support for our driver. */
static const struct file_operations serial_fops_dma = {
	.owner = THIS_MODULE,
	.read = serial_read,
	.write = serial_write_dma,
	.unlocked_ioctl = serial_ioctl,
};


static int serial_dma_setup(struct serial_dev *serial)
{
	/** structure that will describe transfer */
	struct dma_slave_config txconf = {};
	int ret;

	/*
	dma channel requested by dma engine. 
	*/
	serial->txchan = dma_request_chan(serial->dev, "tx");
	
	if (IS_ERR(serial->txchan)) 
	{
		// error pointer (may be special pointer encoded inside of linux, do not know!) returns in case of error
		ret = PTR_ERR(serial->txchan);
		serial->txchan = NULL;
	}

	/**
	 * do mapping of whole register range into the device's dma address
	 */
	serial->fifo_dma_addr = dma_map_resource(serial->dev, serial->res->start + UART_TX * 4, 4, DMA_TO_DEVICE, 0);

	/* check if an error occur. if occured, goto error branch */
	if (dma_mapping_error(serial->dev, serial->fifo_dma_addr))
		goto out_chan;

	// from memory to device (peripheral)
	txconf.direction = DMA_MEM_TO_DEV;
	// byte-by-byte transferred. one byte at a time because uart tx register has 1 byte width.
	txconf.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	txconf.dst_addr = serial->fifo_dma_addr;

	// configure channel and transfer descriptor
	ret = dmaengine_slave_config(serial->txchan, &txconf);
	if (ret)
		goto out_unmap;

	// Enable UART peripheral to support DMA mode
	reg_write(serial, OMAP_UART_SCR_DMAMODE_CTL3 | OMAP_UART_SCR_TX_TRIG_GRANU1, UART_OMAP_SCR);

	return 0;

out_unmap:
	dma_unmap_resource(serial->dev, serial->res->start + UART_TX * 4, 4, DMA_TO_DEVICE, 0); 
	
out_chan:
	// branch jumped when channel is allocated but resource can not be mapped. release the channel
	dma_release_channel(serial->txchan);
	return -ENODEV;
}

/** @brief dma cleanup function called when driver module is removed */
static void serial_dma_cleanup(struct serial_dev *serial)
{
	dmaengine_terminate_sync(serial->txchan);
	dma_release_channel(serial->txchan);
	// undo mapping registers' addresses of serial driver dma address pointer
	dma_unmap_resource(serial->dev, serial->res->start + UART_TX * 4, 4, DMA_TO_DEVICE, 0); 
}

static int serial_probe(struct platform_device *pdev)
{
	struct serial_dev *serial;
	int ret;
	int irq;
	// struct clk *clk;
	// unsigned int baud_divisor, uartclk;

	/* serial device has a lifetime as long as platform device exists */
	serial = devm_kzalloc(&pdev->dev, sizeof(*serial), GFP_KERNEL);
	if (!serial)
		return -ENOMEM;

	/* pdev corresponds to device that should be found at device tree file
	and second parameter zero means first <reg> property found at the same
	device tree file. found this reg property and maps the serial->regs.
	it allocates resources for registers memory range, creates pointers for that  */
	serial->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(serial->regs)) 
		return PTR_ERR(serial->regs);

	/**
	request irq number, acquire it from the dtsi file. 0 means first irq number in that file 
	alternatively, we can get these irqs by their names which are also defined in "interrupt-names property" 
	*/
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
	{
		// returns negative if an error happened
		return irq;
	}

	/**
	associate(match) interrupt handler `serial_interrupt` and irq number `irq` 
	last parameter argument to interrupt handler which holds all required driver-specific information */
	ret = devm_request_irq(&pdev->dev, irq, serial_interrupt, 0, "serial-rx", serial);
	if (ret)
		return ret;

	/** initialize waiting queue in the driver. 
	 */
	init_waitqueue_head(&serial->wait);

	/** spinlock object is initialized */
	spin_lock_init(&serial->lock);

	
	serial->dev = &pdev->dev;
	
	// init_completion(&serial->txcomp);

	/*
	they are about power management setup
	*/
	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);

	// /* Configure the baud rate to 115200 */
	
	u32 uart_clk;
	ret = of_property_read_u32(pdev->dev.of_node, "clock frequency", &uart_clk);
	if(ret)
	{
		dev_error("Error ");
		return ret;
	}
	// clk = devm_clk_get(&pdev->dev, NULL);
	// if (IS_ERR(clk)) {
	// 	ret = PTR_ERR(clk);
	// 	pm_runtime_disable(&pdev->dev);
	// 	return ret;
	// }

	// uartclk = clk_get_rate(clk);
	baud_divisor = uartclk / 16 / 115200;

	reg_write(serial, 0x07, UART_OMAP_MDR1);
	reg_write(serial, 0x00, UART_LCR);
	reg_write(serial, UART_LCR_DLAB, UART_LCR);
	reg_write(serial, baud_divisor & 0xff, UART_DLL);
	reg_write(serial, (baud_divisor >> 8) & 0xff, UART_DLM);
	reg_write(serial, UART_LCR_WLEN8, UART_LCR);
	reg_write(serial, 0x00, UART_OMAP_MDR1);

	/*
	Up to this point, register access may not be locked because interrupts are
	not enabled yet */

	/* Clear UART internal FIFOs */
	reg_write(serial, UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT, UART_FCR);
	/** enable the interrupts for receiving */
	reg_write(serial, UART_IER_RDI, UART_IER);

	/**
	associates platform device with our serial driver, set driver data */
	platform_set_drvdata(pdev, serial);

	/**
	used to get resource of platform (resource points registers physical address)
	Memory Type of resource is selected with `IORESOURCE_MEM`, other option is interrupt resource named IRQ number
	0 means the first resource described in dtsi file.
	 */
	serial->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!serial->res)
	{
		return -EINVAL;
	}

	serial->miscdev.minor = MISC_DYNAMIC_MINOR;
	/* 
	unique name is satisfied by adding physical address of first register.
	get resource pointer and append the name
	*/
	serial->miscdev.name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "serial-%llx", serial->res->start);
	
	serial->miscdev.parent = &pdev->dev;

	/*
	if dma setup worked, used operation with dma support.
	otherwise, use polling ones without dma
	*/
	ret = serial_dma_setup(serial);
	if (ret) 
		serial->miscdev.fops = &serial_fops_pio;
	else
		serial->miscdev.fops = &serial_fops_dma;

	return misc_register(&serial->miscdev);
}

static int serial_remove(struct platform_device *pdev)
{
	struct serial_dev *serial = platform_get_drvdata(pdev);

	// if (serial->txchan) 
	// 	serial_dma_cleanup(serial);

	misc_deregister(&serial->miscdev);

	pm_runtime_disable(&pdev->dev);

	return 0;
}

/** @brief The of (open firmware) device id table that fetches from dtsi file */
static const struct of_device_id serial_of_match[] = {
	{ .compatible = "bootlin,serial", },
	{ /* sentinel */},
};

static struct platform_driver serial_driver = {
	.driver = {
		.name = "serial",
		.owner = THIS_MODULE,
		.of_match_table = serial_of_match,
	},
	.probe = serial_probe,
	.remove = serial_remove,
};

module_platform_driver(serial_driver);

MODULE_LICENSE("GPL");

