#include <linux/module.h> 
#include <linux/init.h>   
#include <linux/fs.h>   
#include <linux/i2c.h> 
#include <asm/uaccess.h>    
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/delay.h>

#define PIFACE_MAJOR	256	//need to be check 
#define PIFACE_ADDR		0x27

#define SEND_CMD		1
#define SEND_DATA		3

#define PIFACE_CS_PIN	8
#define ENABLE			0
#define DISABLE			1

#define DELAY_TIME		1

#define BACK_LIGHT_ON	0x08

struct lcd_data {
	dev_t					devt ;
	struct i2c_client		*i2c_device ;
	struct bin_attribute	bin ;
	struct list_head		device_entry ;
	char					test ;
	unsigned char			addr ;
};

static const struct i2c_device_id lcd_ids[] = {
	{"lcd" , 8 },
	{}
};
MODULE_DEVICE_TABLE(i2c , lcd_ids);

static LIST_HEAD( device_list );

char wokao[]="ABCDEFGHIJKLMNOPQRST";
static char string_that_to_be_send[64];

static struct class *lcd_class ;

static int pcf8574_read(struct lcd_data *lcd)
{	
	return i2c_smbus_read_byte(lcd->i2c_device );
}

static int pcf8574_write(struct lcd_data *lcd , unsigned val )
{
	int status = 0 ; 
	status = i2c_smbus_write_byte(lcd->i2c_device , val );
	return status ;
}

static int lcd1602_wr_cmd(struct lcd_data *lcd , unsigned char command)
{
	unsigned char data ;
	printk("Drv info : %s()\n", __FUNCTION__);
	printk("		   arg = 0x%x\n", command);
	//high 4 bit
	mdelay(DELAY_TIME);
	data = command & 0xf0 ;
	data |= 0x04 ;
	pcf8574_write(lcd ,( data | BACK_LIGHT_ON) );
	mdelay(DELAY_TIME);
	data &= 0xfb ;
	pcf8574_write(lcd ,( data | BACK_LIGHT_ON) );
	mdelay(DELAY_TIME);
	
	//low 4 bit
	data = (command & 0x0f ) << 4;
	data |= 0x04 ;
	pcf8574_write(lcd ,( data | BACK_LIGHT_ON) );
	mdelay(DELAY_TIME);
	data &= 0xfb ;
	pcf8574_write(lcd ,( data | BACK_LIGHT_ON) );
	mdelay(DELAY_TIME);
	return 0 ;
}

static int lcd1602_wr_data(struct lcd_data *lcd , unsigned char command)
{
	unsigned char data ;
	//high 4 bit
	printk("Drv info : %s()\n", __FUNCTION__);
	printk("		   arg = 0x%x\n", command);
	
	mdelay(DELAY_TIME);
	data = ((command&0xf0)|0x01) ;
	data |= 0x04 ;
	pcf8574_write(lcd ,( data | BACK_LIGHT_ON) );
	mdelay(DELAY_TIME);
	data &= 0xfb ;
	pcf8574_write(lcd ,( data | BACK_LIGHT_ON) );
	mdelay(DELAY_TIME);

	//low 4 bit
	mdelay(10);
	data = ((command<<4)|0x01) ;
	data |= 0x04 ;
	pcf8574_write(lcd ,( data | BACK_LIGHT_ON) );
	mdelay(DELAY_TIME);
	data &= 0xfb ;
	pcf8574_write(lcd ,( data | BACK_LIGHT_ON) );
	mdelay(DELAY_TIME);
	return 0 ;
}

static int lcd_open(struct inode *inode , struct file *filp )
{	
	struct lcd_data *lcd ;
	int status = -ENXIO ;

	printk("Drv info : %s()\n", __FUNCTION__);

	list_for_each_entry(lcd , &device_list , device_entry ){
		if(lcd->devt == inode->i_rdev){
			status = 0 ;
			printk("lcd test char = %c \n", lcd->test);
			break ;
		}
	}

	if (status != 0){
		printk("get the lcd structure failured ");
		return status ;	
	}
		
	filp->private_data = lcd ;
	lcd->addr = PIFACE_ADDR ;
#if 1
	//initialize the device
	mdelay(50);
	lcd1602_wr_cmd(lcd , 0x33 );
	mdelay(50);
	lcd1602_wr_cmd(lcd , 0x32 );
	mdelay(50);
	lcd1602_wr_cmd(lcd , 0x28 );
	mdelay(40);
	lcd1602_wr_cmd(lcd , 0x0C );
	mdelay(40);
	lcd1602_wr_cmd(lcd , 0x01 );
#endif
	return status ;
}

static int lcd_release(struct inode *inode , struct file *filp )
{
	struct lcd_data *lcd ;
	lcd = filp->private_data ;
	printk("Drv info : %s()\n", __FUNCTION__);
	if ( lcd == NULL )
		return -ENODEV ;
	filp->private_data = NULL ;
	return 0 ;
}

static long lcd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int status=1 ;
	struct lcd_data *lcd ;
	lcd = filp->private_data ;
//	printk("lcd test char = %c \n", lcd->test);
//	printk("Drv info : %s()\n", __FUNCTION__);
//	printk("     cmd = %d \n" , cmd);
//	printk("     arg = %ld \n" , arg);

	switch (cmd){
		case	SEND_CMD :
			lcd1602_wr_cmd(lcd , arg);
			break;
		case	SEND_DATA :
			lcd1602_wr_data(lcd , arg);
			break;
		default :
			printk("%s() cmd is not supprt at this moment\n" , __FUNCTION__ );
			break;
	}
	
//	status = __put_user( data , (unsigned char __user *)arg);
	return status ;
}

static ssize_t lcd_write(struct file *filp ,const char __user *buf , size_t count , loff_t *f_ops )
{
	int err ;
	char data[2];
	struct lcd_data *lcd ;
	lcd = filp->private_data ;
	printk("Drv info : %s()\n", __FUNCTION__);
	err = copy_from_user(data+*f_ops , buf ,count );
	if(err )
		return -EFAULT ;
	
	return 0 ;
}

static ssize_t lcd_read( struct file *filp , char __user *buf , size_t count , loff_t *f_ops )
{
	int err ;
	char d[2];
	struct lcd_data *lcd ;
	lcd = filp->private_data ;
	printk("Drv info : %s()\n", __FUNCTION__);

	err = copy_to_user(buf, d  , count );
	printk("%s()-buffer : %s \n",__FUNCTION__ , buf);
	if (err)
		return -EFAULT ;
//	err = pcf8574_read(lcd);

	return err ;
}

/**********************************************************************/
static ssize_t lcd_bin_write(struct file *file , struct kobject *kobj ,
								struct bin_attribute *attr ,
								char *buf , loff_t off , size_t cnt )
{
	struct device *dev ;
	struct lcd_data *lcd ;
	unsigned reg ;
	dev = container_of(kobj, struct device , kobj);
	lcd = dev_get_drvdata(dev);
	lcd->addr = PIFACE_ADDR ;
	reg = buf[0] ;
	string_that_to_be_send[0]=buf[0];
	printk("write_register value is %d \n", string_that_to_be_send[0]);
	return cnt ;
}

static ssize_t lcd_bin_read(struct file *file , struct kobject *kobj ,
						       struct bin_attribute *attr ,
							   char *buf , loff_t off , size_t cnt )
{	
	struct device *dev ;
	struct lcd_data *lcd ;
	int str_length ;
	int reg ;
	dev = container_of(kobj, struct device , kobj);
	lcd = dev_get_drvdata(dev);
	lcd->addr = PIFACE_ADDR ;
	if( off >= lcd->bin.size)
		return 0 ;
	if((off+cnt)>lcd->bin.size)
		cnt = lcd->bin.size-off ;

	reg = string_that_to_be_send[0];
	printk("read register value is %d \n", reg );
//	data = mcp23s17_read(lcd , (reg-48) ) ; 
//	printk("read register(chip) is - 0x%x \n", data );
//	data = 68 ;
//	str_length = snprintf(buf , cnt , "%s/n" , string_that_to_be_send);
//	str_length = snprintf(buf , cnt , "%d" , data);
	printk("buf is %s \n", buf);
	printk("cnt is %d \n", cnt);
//	printk("length is %d \n", str_length);
	return str_length ;
}

/**********************************************************************/

//read 
static ssize_t show_lcd(struct device *dev, struct device_attribute *attr,char *buf)
{
	int err ;
	char *dataa = "fuck!this is test\n"; 
	err = snprintf(buf ,PAGE_SIZE , "%s", dataa );
	return err;
}

//write
static ssize_t store_lcd(struct device *dev, struct device_attribute *attr,const char *buf, size_t count )
{
	printk("input buf = %s \n" , buf );
//	length = simple_strtol(buf, NULL, 0);
	return count;
}

static DEVICE_ATTR( lcd_attr , S_IWUGO | S_IRUGO , show_lcd , store_lcd);

static const struct file_operations lcd_ops = {
	.open = lcd_open ,
	.release = lcd_release ,
	.write = lcd_write ,
	.read = lcd_read ,
	.unlocked_ioctl = lcd_ioctl ,
};

static int lcd_probe (struct i2c_client *i2cdev , const struct i2c_device_id *id)
{
	struct lcd_data *lcd = NULL;
	unsigned long minor ;
	int err = 0 ;

	printk("Drv info : %s()\n", __FUNCTION__);
	lcd = kzalloc(sizeof(*lcd), GFP_KERNEL);
	if(!lcd)
		return -ENOMEM ;

	lcd->i2c_device = i2cdev ;
	INIT_LIST_HEAD(&lcd->device_entry);

	minor = 0 ;
	lcd->devt = MKDEV(PIFACE_MAJOR , minor);
	device_create(lcd_class , &i2cdev->dev,lcd->devt , 
				  lcd , "lcd1602");
	printk("device number is %d \n", lcd->devt);

	lcd->test = 'X';
	list_add( &lcd->device_entry , &device_list );
/**********************************************************************/
	sysfs_bin_attr_init(&lcd->bin);
	lcd->bin.attr.name = "sys_lcd";
	lcd->bin.attr.mode = S_IRUGO | S_IWUGO ;
	lcd->bin.read = lcd_bin_read ;
	lcd->bin.write = lcd_bin_write ;
	lcd->bin.size = 16 ; //need to be update

	err = sysfs_create_bin_file(&i2cdev->dev.kobj , &lcd->bin);
	if (err)
			return err ;

	err = device_create_file( &i2cdev->dev , &dev_attr_lcd_attr );
	if (err)
		return err ;

/**********************************************************************/
	i2c_set_clientdata( i2cdev , lcd);
	printk("Drv info : %s()\n", __FUNCTION__);
	return 0 ;
}

static int lcd_remove (struct i2c_client *i2cdev)
{
	struct lcd_data *lcd ;
	lcd = i2c_get_clientdata(i2cdev);
	device_remove_file( &i2cdev->dev , &dev_attr_lcd_attr );
	sysfs_remove_bin_file(&i2cdev->dev.kobj , &lcd->bin);
	lcd->i2c_device = NULL ;
	i2c_set_clientdata(i2cdev , NULL);
	device_destroy(lcd_class , lcd->devt);
	kfree(lcd);
	return 0 ;
}

static struct i2c_driver lcd_driver = {
	.driver = {
		.name = "lcd",
		.owner = THIS_MODULE ,	
	},
	.probe = lcd_probe ,
	.remove = lcd_remove ,
	.id_table = lcd_ids ,
};

static int __init lcd_init(void)
{	int status ;
	status = register_chrdev(PIFACE_MAJOR , "lcd" , &lcd_ops);
	if (status < 0 )
		return status ;

	lcd_class = class_create(THIS_MODULE , "lcd-class");
	if (IS_ERR(lcd_class)){
		unregister_chrdev(PIFACE_MAJOR , lcd_driver.driver.name);
	}

	printk("hello , Kernel-%s()\n", __FUNCTION__);
	return i2c_add_driver(&lcd_driver);
	return 0 ;
}

static void __exit lcd_exit(void)
{
	printk("Goodbye , Kernel-%s()\n", __FUNCTION__);
	i2c_del_driver(&lcd_driver);
	class_destroy(lcd_class);
	unregister_chrdev(PIFACE_MAJOR , lcd_driver.driver.name);
}

module_init(lcd_init);
module_exit(lcd_exit);

MODULE_AUTHOR("hubuyu");
MODULE_LICENSE("GPL");
MODULE_ALIAS("i2c:lcd");
MODULE_DESCRIPTION("This is SPI device Piface");
