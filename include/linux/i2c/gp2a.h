#ifndef __GP2A_H__
#define __GP2A_H__

#define I2C_M_WR 0 /* for i2c */
#define I2c_M_RD 1 /* for i2c */


//#define DELAY_PRX 1
#define DEBUG 1

#if defined(CONFIG_MACH_RANT3)
extern unsigned char hw_version;
#define GPIO_PS_VOUT 		((hw_version>=2)?(94):(91))
#elif defined(CONFIG_MACH_ROOKIE) || defined(CONFIG_MACH_ESCAPE) || defined(CONFIG_MACH_GIO)
extern unsigned char hw_version; /* YUNG@DISYS */
#define GPIO_PS_VOUT 		41
#else
#define GPIO_PS_VOUT 91
#endif
#define IRQ_GP2A_INT MSM_GPIO_TO_INT(GPIO_PS_VOUT)

#if defined(CONFIG_MACH_ROOKIE) || defined(CONFIG_MACH_ESCAPE) || defined(CONFIG_MACH_GIO)
#define GP2A_SENSOR_SDA 109
#define GP2A_SENSOR_SCL 110
#endif

#define I2C_DF_NOTIFY			0x01 /* for i2c */

//ADDSEL is LOW
#define GP2A_ADDR		0x88 /* slave addr for i2c */

#define REGS_PROX 	    0x0 // Read  Only
#define REGS_GAIN    	0x1 // Write Only
#define REGS_HYS		0x2 // Write Only
#define REGS_CYCLE		0x3 // Write Only
#define REGS_OPMOD		0x4 // Write Only
#define REGS_CON	0x6 // Write Only

/* sensor type */
#define LIGHT           0
#define PROXIMITY		1
#define ALL				2

/* power control */
#define ON              1
#define OFF				0

/* IOCTL for proximity sensor */
#define SHARP_GP2AP_IOC_MAGIC   'C'                                 
#define SHARP_GP2AP_OPEN    _IO(SHARP_GP2AP_IOC_MAGIC,1)            
#define SHARP_GP2AP_CLOSE   _IO(SHARP_GP2AP_IOC_MAGIC,2)      

/* input device for proximity sensor */
#define USE_INPUT_DEVICE 	0  /* 0 : No Use  ,  1: Use  */

#define USE_INTERRUPT		1
#define INT_CLEAR    1 /* 0 = by polling operation, 1 = by interrupt operation */
#define LIGHT_PERIOD 1 /* per sec */
#define ADC_CHANNEL  9 /* index for s5pC110 9?? channel adc */ 

/*for light sensor */
#define STATE_NUM			3   /* number of states */
#define LIGHT_LOW_LEVEL     1    /* brightness of lcd */
#define LIGHT_MID_LEVEL 	 11    
#define LIGHT_HIGH_LEVEL    23

#define ADC_BUFFER_NUM	6


#define GUARDBAND_BOTTOM_ADC    700
#define GUARDBAND_TOP_ADC    800




/*
 * STATE0 : 30 lux below
 * STATE1 : 31~ 3000 lux
 * STATE2 : 3000 lux over
 */




#define ADC_CUT_HIGH 1100            /* boundary line between STATE_0 and STATE_1 */
#define ADC_CUT_LOW  220            /* boundary line between STATE_1 and STATE_2 */
#define ADC_CUT_GAP  50            /* in order to prevent chattering condition */

#define LIGHT_FOR_16LEVEL 


/* 16 level for premium model*/
typedef enum t_light_state
{
	LIGHT_DIM   = 0,
	LIGHT_LEVEL1   = 1,
	LIGHT_LEVEL2   = 2,
	LIGHT_LEVEL3   = 3,
	LIGHT_LEVEL4   = 4,
	LIGHT_LEVEL5   = 5,
	LIGHT_LEVEL6   = 6,
	LIGHT_LEVEL7   = 7,
	LIGHT_LEVEL8   = 8,
	LIGHT_LEVEL9   = 9,
	LIGHT_LEVEL10   = 10,
	LIGHT_LEVEL11   = 11,
	LIGHT_LEVEL12   = 12,
	LIGHT_LEVEL13   = 13,	
	LIGHT_LEVEL14   = 14,		
	LIGHT_LEVEL15   = 15,	
	LIGHT_LEVEL16   = 16,	
	LIGHT_INIT  = 17,	
}state_type;

static u8 gp2a_original_image[8] =
{
	0x00,  	// REGS_PROX
	0x08,  	// REGS_GAIN
	0x40,  	// REGS_HYS
	0x04,  	// REGS_CYCLE
	0x03,   // REGS_OPMOD
};

/* for state transition */
struct _light_state {
	state_type type;
	int adc_bottom_limit;
	int adc_top_limit;
	int brightness;

};

/* driver data */
struct gp2a_data {
	struct input_dev *input_dev;
	struct delayed_work work_prox;  /* for proximity sensor */
	struct work_struct work_light; /* for light_sensor     */
	int             irq;
    struct hrtimer timer;
	struct timer_list light_init_timer;

};


//struct workqueue_struct *gp2a_wq;

/* prototype */
extern short gp2a_get_proximity_value(void);
extern short gp2a_get_light_sensor_value(void);		// iamaj 
extern bool gp2a_get_lightsensor_status(void);
int opt_i2c_read(u8 reg, u8 *val, unsigned int len );
int opt_i2c_write( u8 reg, u8 *val );
static int proximity_open(struct inode *ip, struct file *fp);
static int proximity_release(struct inode *ip, struct file *fp);
static long proximity_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int lightsensor_get_adcvalue(void);

#endif
