/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/earlysuspend.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include <asm/atomic.h>

#include <mach/msm_rpcrouter.h>
#include <mach/msm_battery.h>

#ifndef MAX8899_CHARGER
#define MAX8899_CHARGER
#endif

#ifdef MAX8899_CHARGER
#include <linux/timer.h>
#include <linux/time.h>
//#include <linux/jiffies.h>
#include "../../arch/arm/mach-msm/proc_comm.h"
#include <linux/delay.h>
#include <linux/wakelock.h>
#endif
#include <linux/slab.h>
#ifndef FUEL_GAUGE_I2C
#define FUEL_GAUGE_I2C

int fuel_gauge_init(void);
void fuel_gauge_exit(void);
unsigned int fg_read_vcell(void);
unsigned int fg_read_soc(void);
unsigned int fg_reset_soc(void);
#endif
void msm_batt_update(void);
static int check_charging_status(int oldChargingState);
void charger_existence_check(void);

//#define __TEMPERATURE_DEBUG_CONTROL__
//#define __COMPENSATE_SAMPLE__

#define __TEMP_BLOCK_EXCEPTION__ //syun

#define BATTERY_RPC_PROG	0x30000089
#define BATTERY_RPC_VERS	0x00010001
#define BATTERY_RPC_CB_PROG	0x31000089
#define BATTERY_RPC_CB_VERS	0x00010001
#define CHG_RPC_PROG		0x3000001a
#define CHG_RPC_VERS		0x00010001
#define BATTERY_REGISTER_PROC                          	2
#define BATTERY_GET_CLIENT_INFO_PROC                   	3
#define BATTERY_MODIFY_CLIENT_PROC                     	4
#define BATTERY_DEREGISTER_CLIENT_PROC			5
#define BATTERY_SERVICE_TABLES_PROC                    	6
#define BATTERY_IS_SERVICING_TABLES_ENABLED_PROC       	7
#define BATTERY_ENABLE_TABLE_SERVICING_PROC            	8
#define BATTERY_DISABLE_TABLE_SERVICING_PROC           	9
#define BATTERY_READ_PROC                              	10
#define BATTERY_MIMIC_LEGACY_VBATT_READ_PROC           	11
#define BATTERY_ENABLE_DISABLE_FILTER_PROC 		14
#define VBATT_FILTER			2
#define BATTERY_CB_TYPE_PROC 		1
#define BATTERY_CB_ID_ALL_ACTIV       	1
#define BATTERY_CB_ID_LOW_VOL		2
#define BATTERY_LOW            	2800
#define BATTERY_HIGH           	4300
#define BATTERY_LOW_CORECTION   	100
#define ONCRPC_CHG_IS_CHARGING_PROC 		2
#define ONCRPC_CHG_IS_CHARGER_VALID_PROC 	3
#define ONCRPC_CHG_IS_BATTERY_VALID_PROC 	4
#define ONCRPC_CHG_UI_EVENT_READ_PROC 		5
#define ONCRPC_CHG_GET_GENERAL_STATUS_PROC 	12
#define ONCRPC_CHARGER_API_VERSIONS_PROC 	0xffffffff
#define CHARGER_API_VERSION  			0x00010003
#define DEFAULT_CHARGER_API_VERSION		0x00010001
#define BATT_RPC_TIMEOUT    10000	/* 10 sec */
#define INVALID_BATT_HANDLE    -1
#define RPC_TYPE_REQ     0
#define RPC_TYPE_REPLY   1
#define RPC_REQ_REPLY_COMMON_HEADER_SIZE   (3 * sizeof(uint32_t))
#define SUSPEND_EVENT		(1UL << 0)
#define RESUME_EVENT		(1UL << 1)
#define CLEANUP_EVENT		(1UL << 2)

#include <linux/gpio.h>
#define GPIO_FG_ALERT	21
#define ADC_DATA_ARR_SIZE				6
#define AVG_CURRENT_ARR_SIZE			20
//#define	CUT_OFF_CURRENT					35			//27 => 190mA    ?????? : 34
#define	CUT_OFF_CURRENT					33			//200mA
static struct hrtimer ChgTimer;

#ifdef MAX8899_CHARGER
#define BATT_CHECK_INTERVAL	( 10 * HZ ) // every 10 sec

// proc comm re-definition
#define SMEM_PROC_COMM_CHARGING_INFO	PCOM_OEM_CHARGING_INFO
#define SMEM_PROC_COMM_GET_ADC			PCOM_OEM_SAMSUNG_GET_ADC

// SMEM_PROC_COMM_CHARGING_INFO data1
enum {
	SMEM_PROC_COMM_CHARGING_ON_OFF = 0x1,
	SMEM_PROC_COMM_CHARGING_READ_STATUS, // status 1byte, event_flag 3byte
	SMEM_PROC_COMM_CHARGING_I2C_READ,
	SMEM_PROC_COMM_CHARGING_I2C_WRITE,
};

// SMEM_PROC_COMM_CHARGING_ON_OFF data2
typedef enum {
	SMEM_PROC_COMM_CHARGING_OFF = 0x0,
	SMEM_PROC_COMM_CHARGING_ON_TA,
	SMEM_PROC_COMM_CHARGING_ON_USB,
}charging_ctrl;

// SMEM_PROC_COMM_GET_ADC data 1
enum {
	SMEM_PROC_COMM_GET_ADC_BATTERY = 0x0,
	SMEM_PROC_COMM_GET_ADC_TEMP,
	SMEM_PROC_COMM_GET_ADC_VF,
	SMEM_PROC_COMM_GET_ADC_ALL,
	SMEM_PROC_COMM_GET_ADC_EAR_ADC,
	SMEM_PROC_COMM_GET_ADC_SSENS, // 5	quattro_jiny46kim
    SMEM_PROC_COMM_GET_ADC_CURRENT,
	SMEM_PROC_COMM_GET_ADC_MAX
};
#endif

#ifdef __TEMP_BLOCK_EXCEPTION__
#define USE_DATA_CALL		(0x1 << 0)
#define USE_MOVIE		(0x1 << 1)
#define USE_MP3			(0x1 << 2)
#define USE_BROWSER		(0x1 << 3)
#define USE_WIMAX		(0x1 << 4)
#define USE_HOTSPOT		(0x1 << 5)
#define USE_CAM			(0x1 << 6)

static struct timer_list use_wimax_timer;
static struct timer_list use_browser_timer;
static struct timer_list use_data_call_timer;

#define USE_MODULE_TIMEOUT	(10*60*1000)

static void use_wimax_timer_func(unsigned long unused);
int chg_temp_check_use_wimax(int onoff);
static void use_browser_timer_func(unsigned long unused);
static int chg_temp_check_use_browser(int onoff);
static void use_data_call_timer_func(unsigned long unused);
static int chg_temp_check_use_data_call(int onoff);
#endif

#define DEBUG  0
#define TRUE 1
#define FALSE 0

#define CHG_DEBUG	0

#if DEBUG
#define DBG(x...) printk(KERN_INFO x)
#else
#define DBG(x...) do {} while (0)
#endif

// #define CHG_RPC_SERVER_AVAILABLE  // charger_rpc_server on the ARM9 is not available

enum {
	BATTERY_REGISTRATION_SUCCESSFUL = 0,
	BATTERY_DEREGISTRATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_MODIFICATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_INTERROGATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_CLIENT_TABLE_FULL = 1,
	BATTERY_REG_PARAMS_WRONG = 2,
	BATTERY_DEREGISTRATION_FAILED = 4,
	BATTERY_MODIFICATION_FAILED = 8,
	BATTERY_INTERROGATION_FAILED = 16,
	/* Client's filter could not be set because perhaps it does not exist */
	BATTERY_SET_FILTER_FAILED         = 32,
	/* Client's could not be found for enabling or disabling the individual
	 * client */
	BATTERY_ENABLE_DISABLE_INDIVIDUAL_CLIENT_FAILED  = 64,
	BATTERY_LAST_ERROR = 128,
};

enum {
	BATTERY_VOLTAGE_UP = 0,
	BATTERY_VOLTAGE_DOWN,
	BATTERY_VOLTAGE_ABOVE_THIS_LEVEL,
	BATTERY_VOLTAGE_BELOW_THIS_LEVEL,
	BATTERY_VOLTAGE_LEVEL,
	BATTERY_ALL_ACTIVITY,
	VBATT_CHG_EVENTS,
	BATTERY_VOLTAGE_UNKNOWN,
};

enum {
	CHG_UI_EVENT_IDLE,	/* Starting point, no charger.  */
	CHG_UI_EVENT_NO_POWER,	/* No/Weak Battery + Weak Charger. */
	CHG_UI_EVENT_VERY_LOW_POWER,	/* No/Weak Battery + Strong Charger. */
	CHG_UI_EVENT_LOW_POWER,	/* Low Battery + Strog Charger.  */
	CHG_UI_EVENT_NORMAL_POWER, /* Enough Power for most applications. */
	CHG_UI_EVENT_DONE,	/* Done charging, batt full.  */
	CHG_UI_EVENT_INVALID,
	CHG_UI_EVENT_MAX32 = 0x7fffffff
};

/*
 * This enum contains defintions of the charger hardware status
 */
enum chg_charger_status_type {
    /* The charger is good      */
    CHARGER_STATUS_GOOD,
    /* The charger is bad       */
    CHARGER_STATUS_BAD,
    /* The charger is weak      */
    CHARGER_STATUS_WEAK,
    /* Invalid charger status.  */
    CHARGER_STATUS_INVALID
};

static char *charger_status[] = {
	"good", "bad", "weak", "invalid"
};

/*
 *This enum contains defintions of the charger hardware type
 */
enum chg_charger_hardware_type {
    /* The charger is removed                 */
    CHARGER_TYPE_NONE,
    /* The charger is a regular wall charger   */
    CHARGER_TYPE_WALL,
    /* The charger is a PC USB                 */
    CHARGER_TYPE_USB_PC,
    /* The charger is a wall USB charger       */
    CHARGER_TYPE_USB_WALL,
    /* The charger is a USB carkit             */
    CHARGER_TYPE_USB_CARKIT,
    /* Invalid charger hardware status.        */
    CHARGER_TYPE_INVALID
};

static char *charger_type[] = {
	"No charger", "wall", "USB PC", "USB wall", "USB car kit",
	"invalid charger"
};

/*
 *  This enum contains defintions of the battery status
 */
enum chg_battery_status_type {
    /* The battery is good        */
    BATTERY_STATUS_GOOD,
    /* The battery is cold/hot    */
    BATTERY_STATUS_BAD_TEMP,
    /* The battery is bad         */
    BATTERY_STATUS_BAD,
    /* Invalid battery status.    */
    BATTERY_STATUS_INVALID
};

static char *battery_status[] = {
	"good ", "bad temperature", "bad", "invalid"
};

/*
 *This enum contains defintions of the battery voltage level
 */
enum chg_battery_level_type {
    /* The battery voltage is dead/very low (less than 3.2V)        */
    BATTERY_LEVEL_DEAD,
    /* The battery voltage is weak/low (between 3.2V and 3.4V)      */
    BATTERY_LEVEL_WEAK,
    /* The battery voltage is good/normal(between 3.4V and 4.2V)  */
    BATTERY_LEVEL_GOOD,
    /* The battery voltage is up to full (close to 4.2V)            */
    BATTERY_LEVEL_FULL,
    /* Invalid battery voltage level.                               */
    BATTERY_LEVEL_INVALID
};

static char *battery_level[] = {
	"dead", "weak", "good", "full", "invalid"
};


/* Generic type definition used to enable/disable charger functions */
enum {
	CHG_CMD_DISABLE,
	CHG_CMD_ENABLE,
	CHG_CMD_INVALID,
	CHG_CMD_MAX32 = 0x7fffffff
};

typedef enum {
        CHARGER_BATTERY = 0,
        CHARGER_USB,
        CHARGER_AC
} charger_type_t;

struct batt_client_registration_req {

	struct rpc_request_hdr hdr;

	/* The voltage at which callback (CB) should be called. */
	u32 desired_batt_voltage;

	/* The direction when the CB should be called. */
	u32 voltage_direction;

	/* The registered callback to be called when voltage and
	 * direction specs are met. */
	u32 batt_cb_id;

	/* The call back data */
	u32 cb_data;
	u32 more_data;
	u32 batt_error;
};

struct batt_client_registration_rep {
	struct rpc_reply_hdr hdr;
	u32 batt_clnt_handle;
};

struct rpc_reply_batt_chg {
	struct rpc_reply_hdr hdr;
	u32 	more_data;

	u32	charger_status;
	u32	charger_type;
	u32	battery_status;
	u32	battery_level;
	u32     battery_voltage;
	u32	battery_temp;
};

static struct rpc_reply_batt_chg rep_batt_chg;

struct battery_comp_check{
	u32 vibrator;
	u32 keypad_backlight;
	u32 lcd_backlight;
	u32 lcd_dimming;
	u32 camera_down;
	u32 camcoder_down;
	u32 amp_down;
	u32 video_down;
	u32 bt_down;
	u32 camera_flash_down;
	u32 batt_low_noty;
	u32 talk_wcdma;
	u32 talk_gsm;
	u32 data_call;
};

struct msm_battery_info {

	u32 voltage_max_design;
	u32 voltage_min_design;
	u32 chg_api_version;
	u32 batt_technology;

	u32 avail_chg_sources;
	u32 current_chg_source;
#ifdef __TEMP_BLOCK_EXCEPTION__
	u32 chg_temp_err_except_check;
#endif
	s32 batt_chg_current;
	u32 batt_status;
	u32 batt_health;
	u32 charger_valid;
	u32 batt_valid;
	u32 batt_capacity;

	s32 charger_status;
	s32 charger_type;
	s32 battery_status;
	s32 battery_level; //Voltage ADC
	s32 battery_voltage; //Voltage
	s32 battery_temp; //Temperature
	s32 battery_Celsius;

#ifdef SOLAR_CHARG
	s32 solar_status;
#endif
    s32 batt_slate_mode;
	s32 battery_pre_level; //Voltage average ADC

	u32(*calculate_capacity) (u32 voltage);

	s32 batt_handle;

	spinlock_t lock;

	struct power_supply *msm_psy_ac;
	struct power_supply *msm_psy_usb;
	struct power_supply *msm_psy_batt;

	struct msm_rpc_endpoint *batt_ep;
	struct msm_rpc_endpoint *chg_ep;

	struct workqueue_struct *msm_batt_wq;

	struct task_struct *cb_thread;

	wait_queue_head_t wait_q;

	struct early_suspend early_suspend;

	atomic_t handle_event;
	atomic_t event_handled;

	u32 type_of_event;
	uint32_t vbatt_rpc_req_xid;

#ifdef MAX8899_CHARGER
	struct timer_list timer;
	s32 battery_vf;
#endif

	unsigned long update_time;
	struct battery_comp_check comp_check;
	u32 batt_comp_delta;
	s32 control_tmp;

};

#define CHARGING_STATE_BATTERY						0x00
#define CHARGING_STATE_ABNORMAL_BATTERY				0x02
#define CHARGING_STATE_TA_FULL_RECHARGING			0x01
#define CHARGING_STATE_USB_FULL_RECHARGING			0x04
#define CHARGING_STATE_USB_CHARGING					0x05
#define CHARGING_STATE_TA_HIGH_TEMPERATURE			0x03
#define CHARGING_STATE_TA_FULL_CHARGED				0x07
#define CHARGING_STATE_TA_CHARGING					0x0F
#define CHARGING_STATE_TA_LOW_TEMPERATURE			0x1F
#define CHARGING_STATE_TA_TIMEOVER_CHARGED			0x3F
#define CHARGING_STATE_INIT_BATTERY					0xFF

#define VBUSAOKB_MASK			(0x1 << 7)
#define DONE_MASK				(0x1 << 6)
#define VCHGINOK_MASK			(0x1 << 5)
#define VBUSOK_MASK			(0x1 << 4)
#define CHGON_MASK				(0x1 << 3)
#define FCHG_MASK				(0x1 << 2)
#define PQL_MASK				(0x1 << 1)
#define CHGROUTOV_MASK		(0x1 << 0)

// quattro_jiny46kim
#define CHG_OFF_LOW_TEMP 		132 //131
#define CHG_RESTART_LOW_TEMP 	137 // 135 //136
#define CHG_OFF_HIGH_TEMP 		203 //200
#define CHG_RESTART_HIGH_TEMP 	193
#define CHG_EXCEPT_HIGH_TEMP	228 //215


#define CHG_OFF_LEVEL 146
#define CHG_RESTART_LEVEL 139

#define CHARGING_TEMPERATURE_NORMAL     		0
#define CHARGING_TEMPERATURE_HIGH	      		1
#define CHARGING_TEMPERATURE_LOW    			2

#define MAX_BATTERY_ADC_COUNT_VALUE			5

#define AVERAGE_SAMPLE_NUMBER			3

#define CHG_CONNECT			(0x1 << 3)
#define FULL_CHG_MASK			(0x1 << 2)
#define TIMER_MASK				(0x1 << 1)
#define RECHG_START_MASK		(0x1 << 0)

static int uCount=1;
static int dwTotal=0;

static int gChg_connect = 0;
static int gFullChgFlag = 0;
static int gChgTimerFlag = 0;

int fg_gFull_chg = 0;
int fg_alert = 0;
static int gTimer = 0;
static int gRechg_start = 0;
static int gTimeover_start = 0;
static int gAvg_Current[20] = {0};
static int bChg_Flag = 0;
static int gChg_Cnt = 0;

extern unsigned char hw_version;

//init Value
static int tempInterrupt=CHARGING_TEMPERATURE_NORMAL;
static int dwChargingState=CHARGING_STATE_INIT_BATTERY;

//average cnt
static int AverageSample=0;
static unsigned int cache_time = 1000;
static int bSetTimer = 0;

unsigned int on_call = 0;

//inital check
//static int msm_batt_init_done = FALSE;

//syun
static int fuelgauge_val[100];
static int fuel_gauge_index = 0;

static void msm_batt_wait_for_batt_chg_event(struct work_struct *work);
void batt_registertimer(struct timer_list* ptimer, unsigned long timeover );

static DECLARE_WORK(msm_batt_cb_work, msm_batt_wait_for_batt_chg_event);

static int msm_batt_cleanup(void);

static struct msm_battery_info msm_batt_info = {
	.batt_handle = INVALID_BATT_HANDLE,
	.charger_status = -1,
	.charger_type = -1,
	.battery_status = -1,
	.battery_level = -1,
	.battery_voltage = -1,
	.battery_temp = -1,
        .batt_slate_mode = 0,
};

static enum power_supply_property msm_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *msm_power_supplied_to[] = {
	"battery",
};

static ssize_t battery_show_property(struct device *dev,
                                          struct device_attribute *attr,
                                          char *buf);

static ssize_t battery_store_property(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t size);

#define SEC_BATTERY_ATTR(_name)                                                 \
{                                                                               \
        .attr = { .name = #_name, .mode = 0664, .owner = THIS_MODULE },      \
        .show = battery_show_property,                                      \
        .store = battery_store_property,                                                          \
}

#define SEC_BATTERY_ATTR_SLATE(_name)                                                 \
{                                                                               \
        .attr = { .name = #_name, .mode = 0660, .owner = THIS_MODULE },      \
        .show = battery_show_property,                                      \
        .store = battery_store_property,                                                          \
}

static struct device_attribute battery_attrs[] = {
	SEC_BATTERY_ATTR(vibrator),
	SEC_BATTERY_ATTR(keypad_backlight),
	SEC_BATTERY_ATTR(lcd_backlight),
	SEC_BATTERY_ATTR(lcd_dimming),
	SEC_BATTERY_ATTR(camera_down),
	SEC_BATTERY_ATTR(camcoder_down),
	SEC_BATTERY_ATTR(amp_down),
	SEC_BATTERY_ATTR(video_down),
	SEC_BATTERY_ATTR(bt_down),
	SEC_BATTERY_ATTR(camera_flash_down),
	SEC_BATTERY_ATTR(batt_low_noty),
	SEC_BATTERY_ATTR(talk_wcdma),
	SEC_BATTERY_ATTR(talk_gsm),
	SEC_BATTERY_ATTR(data_call),
	SEC_BATTERY_ATTR(control_tmp),
#ifdef FUEL_GAUGE_I2C
    SEC_BATTERY_ATTR(fg_soc),
    SEC_BATTERY_ATTR(reset_soc),
#endif /* FUEL_GAUGE_I2C */
#ifdef __TEMP_BLOCK_EXCEPTION__
	SEC_BATTERY_ATTR(chg_temp_err_except_check),
#endif
    SEC_BATTERY_ATTR(batt_chg_current),
    SEC_BATTERY_ATTR_SLATE(batt_slate_mode),
};

enum {
	VIBRATOR = 0,
	KEYPAD_BACKLIGHT,
	LCD_BACKLIGHT,
	LCD_DIMMING,
	CAMERA_DOWN,
	CAMCODER_DOWN,
	AMP_DOWN,
	VIDEO_DOWN,
	BT_DOWN,
	CAMERA_FLASH_DOWN,
	BATT_LOW_NOTY,
	TALK_WCDMA,
	TALK_GSM,
	DATA_CALL,
	CONTROL_TMP,
#ifdef FUEL_GAUGE_I2C
    BATT_FG_SOC,
    BATT_RESET_SOC,
#endif /* FUEL_GAUGE_I2C */
#ifdef __TEMP_BLOCK_EXCEPTION__
	BATT_CHG_TEMP_ERR_EXCEPTION,
#endif
    BATT_CHG_CURRENT,
    BATT_SLATE_MODE,
};

static ssize_t battery_show_property(struct device *dev,
                                         struct device_attribute *attr,
                                         char *buf)
{
        int i = 0;
        const ptrdiff_t off = attr - battery_attrs;

        /* rpc lock is used to prevent two threads from calling
         * into the get info rpc at the same time
         */

        //mutex_lock(&msm_batt_info.rpc_lock);
        spin_lock(&msm_batt_info.lock);
        /* check cache time to decide if we need to update */
        if (msm_batt_info.update_time &&
            time_before(jiffies, msm_batt_info.update_time +
                                msecs_to_jiffies(cache_time)))
                goto dont_need_update;

        //if (get_batt_info(&msm_batt_info.rep) < 0) {
        //       printk(KERN_ERR "%s: rpc failed!!!\n", __FUNCTION__);
        //} else {
                msm_batt_info.update_time = jiffies;
        //}

dont_need_update:
        //mutex_unlock(&msm_batt_info.rpc_lock);
        spin_unlock(&msm_batt_info.lock);

        //mutex_lock(&msm_batt_info.lock);
        spin_lock(&msm_batt_info.lock);
        switch (off) {
        case VIBRATOR:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               msm_batt_info.comp_check.vibrator);
                break;
        case KEYPAD_BACKLIGHT:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               msm_batt_info.comp_check.keypad_backlight);
                break;
        case LCD_BACKLIGHT:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               msm_batt_info.comp_check.lcd_backlight);
                break;
        case LCD_DIMMING:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               msm_batt_info.comp_check.lcd_dimming);
                break;
        case CAMERA_DOWN:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               msm_batt_info.comp_check.camera_down);
                break;
        case CAMCODER_DOWN:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               msm_batt_info.comp_check.camcoder_down);
                break;
        case AMP_DOWN:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               msm_batt_info.comp_check.amp_down);
                break;
        case VIDEO_DOWN:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               msm_batt_info.comp_check.video_down);
                break;
        case BT_DOWN:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               msm_batt_info.comp_check.bt_down);
                break;
        case CAMERA_FLASH_DOWN:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               msm_batt_info.comp_check.camera_flash_down);
                break;
        case BATT_LOW_NOTY:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               msm_batt_info.comp_check.batt_low_noty);
                break;
        case TALK_WCDMA:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               msm_batt_info.comp_check.talk_wcdma);
                break;
        case TALK_GSM:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               msm_batt_info.comp_check.talk_gsm);
                break;
		case DATA_CALL:
				i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
							   msm_batt_info.comp_check.data_call);
				break;
        case CONTROL_TMP:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               msm_batt_info.control_tmp);
                break;
#ifdef FUEL_GAUGE_I2C
        case BATT_FG_SOC:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                              msm_batt_info.batt_capacity);
                break;
#endif /* FUEL_GAUGE_I2C */
#ifdef __TEMP_BLOCK_EXCEPTION__
		case BATT_CHG_TEMP_ERR_EXCEPTION:
				i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
							  msm_batt_info.chg_temp_err_except_check);
				break;
#endif
		case BATT_CHG_CURRENT:
				i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
							  msm_batt_info.batt_chg_current);
				break;
		case BATT_SLATE_MODE:
				/*FOR SLATE TEST
				Don't charge by USB*/
				i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",msm_batt_info.batt_slate_mode);
				break;


        default:
                i = -EINVAL;
        }
        //mutex_unlock(&msm_batt_info.lock);
        spin_unlock(&msm_batt_info.lock);

        return i;
}

static ssize_t battery_store_property(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t ret = -EINVAL;
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	//size_t count = after - buf;
	const ptrdiff_t offset = attr - battery_attrs;
	int x;

	//mutex_lock(&msm_batt_info.lock);
	spin_lock(&msm_batt_info.lock);

       switch (offset) {

        case VIBRATOR:
		msm_batt_info.comp_check.vibrator = state;
                break;
        case KEYPAD_BACKLIGHT:
		msm_batt_info.comp_check.keypad_backlight= state;
                break;
        case LCD_BACKLIGHT:
		msm_batt_info.comp_check.lcd_backlight= state;
                break;
        case LCD_DIMMING:
		msm_batt_info.comp_check.lcd_dimming= state;
                break;
        case CAMERA_DOWN:
		msm_batt_info.comp_check.camera_down= state;
                break;
        case CAMCODER_DOWN:
		msm_batt_info.comp_check.camcoder_down= state;
                break;
        case AMP_DOWN:
		msm_batt_info.comp_check.amp_down= state;
                break;
        case VIDEO_DOWN:
		msm_batt_info.comp_check.video_down= state;
                break;
        case BT_DOWN:
		msm_batt_info.comp_check.bt_down= state;
                break;
        case CAMERA_FLASH_DOWN:
		msm_batt_info.comp_check.camera_flash_down= state;
                break;
        case BATT_LOW_NOTY:
		msm_batt_info.comp_check.batt_low_noty= state;
                break;
        case TALK_WCDMA:
		msm_batt_info.comp_check.talk_wcdma= state;
                break;
        case TALK_GSM:
		msm_batt_info.comp_check.talk_gsm= state;
                break;
		case DATA_CALL:
		msm_batt_info.comp_check.data_call= state;
//			chg_temp_check_use_data_call(state);
			break;
        case CONTROL_TMP:
		msm_batt_info.control_tmp = state;
                break;
#ifdef FUEL_GAUGE_I2C
        case BATT_RESET_SOC:
                fg_reset_soc();
                if ((msm_batt_info.batt_capacity = fg_read_soc()) < 0) {
					msm_batt_info.batt_capacity = fg_read_soc();
				}
                ret = 0;
                break;
#endif /* FUEL_GAUGE_I2C */
#ifdef __TEMP_BLOCK_EXCEPTION__
		case BATT_CHG_TEMP_ERR_EXCEPTION:
			msm_batt_info.chg_temp_err_except_check = state;
			break;
#endif
		case BATT_CHG_CURRENT:
			msm_batt_info.batt_chg_current = state;
			break;
		case BATT_SLATE_MODE:
		    if (sscanf(buf, "%d\n", &x) == 1) {
			    msm_batt_info.batt_slate_mode = x;
				charger_existence_check();
			    check_charging_status(dwChargingState);
	            msm_batt_update();
		        }
		    break;

        default:
                ret = -EINVAL;
        }
        //mutex_unlock(&msm_batt_info.lock);
        spin_unlock(&msm_batt_info.lock);

	on_call = msm_batt_info.comp_check.talk_wcdma | msm_batt_info.comp_check.talk_gsm;

	return ret;

}

static int battery_create_attrs(struct device * dev)
{
        int i, rc;

        for (i = 0; i < ARRAY_SIZE(battery_attrs); i++) {
                rc = device_create_file(dev, &battery_attrs[i]);
                if (rc)
                        goto attrs_failed;
        }
        goto succeed;

attrs_failed:
        while (i--)
                device_remove_file(dev, &battery_attrs[i]);
succeed:
        return rc;
}

#ifdef __COMPENSATE_SAMPLE__
int battery_compensation_val(int vbatt_adc)
{
	int compensation_val = 0, comp_dev = 0;

	if(msm_batt_info.charger_type == CHARGER_TYPE_NONE) // battery compensation when no charger attached
	{
		if(msm_batt_info.comp_check.vibrator){
			compensation_val += 30;
			comp_dev |= 0x0001;
		}
		if(msm_batt_info.comp_check.keypad_backlight){
			compensation_val += 0;
			comp_dev |= 0x0002;
		}
		if(msm_batt_info.comp_check.lcd_backlight){
			compensation_val += 30;
			comp_dev |= 0x0004;
		}
		if(msm_batt_info.comp_check.lcd_dimming){
			compensation_val += 20;
			comp_dev |= 0x0008;
		}
		if(msm_batt_info.comp_check.camera_down){
			compensation_val += 100;
			comp_dev |= 0x0010;
		}
		if(msm_batt_info.comp_check.camcoder_down){
			compensation_val += 20;
			comp_dev |= 0x0020;
		}
		if(msm_batt_info.comp_check.amp_down){
			compensation_val += 10;
			comp_dev |= 0x0040;
		}
		if(msm_batt_info.comp_check.video_down){
			compensation_val += 40;
			comp_dev |= 0x0080;
		}
		if(msm_batt_info.comp_check.bt_down){
			compensation_val += 0;
			comp_dev |= 0x0100;
		}
		if(msm_batt_info.comp_check.camera_flash_down){
			compensation_val += 30;
			comp_dev |= 0x0200;
		}
	}
/*
	else if((msm_batt_info.charger_type == CHARGER_TYPE_USB_PC) || //USB Charging// when USB attached level measured higer
		(msm_batt_info.charger_type == CHARGER_TYPE_USB_WALL) ||
		(msm_batt_info.charger_type == CHARGER_TYPE_USB_CARKIT))
	{
		//if(vbatt_adc <= 910)
		//	compensation_val -= 250;
		//else if(vbatt_adc <= 1132)
		//	compensation_val -= 187;
		//else if(vbatt_adc <= 1442)
			compensation_val -= 170;
	}
	else if(msm_batt_info.charger_type == CHARGER_TYPE_WALL) // when TA attached level measured higer
	{
		//if(vbatt_adc <= 910)
		//	compensation_val -= 275;
		//else if(vbatt_adc <= 1132)
		//	compensation_val -= 212;
		//else if(vbatt_adc <= 1442)
			compensation_val -= 170;
	}
*/

	//Temp for test
	if(msm_batt_info.comp_check.batt_low_noty)
		printk("[Battery] Got low batt noty from RIL : 0x%d\n", msm_batt_info.comp_check.batt_low_noty);


	printk("[Battery] %s on using devs %x : %d\n", __func__, comp_dev, compensation_val);

	return compensation_val;
}
#endif

static int msm_power_get_property(struct power_supply *psy,
				  enum power_supply_property psp,
				  union power_supply_propval *val)
{
	charger_type_t charger;
	charger = msm_batt_info.current_chg_source;

	//printk(KERN_INFO "%s(): [psp = %d]  [psy->type = %d]\n"
	//		, __func__, psp, psy->type);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:

		if (psy->type == POWER_SUPPLY_TYPE_MAINS) {
			val->intval = (charger ==  CHARGER_AC ? 1 : 0);
			//printk(KERN_INFO "%s(): power supply = %s online = %d\n"
			//		, __func__, psy->name, val->intval);
		}

		else if (psy->type == POWER_SUPPLY_TYPE_USB) {
			if(msm_batt_info.batt_slate_mode){
			    charger = CHARGER_BATTERY;
	        }
			val->intval = (charger ==  CHARGER_USB ? 1 : 0);
			//printk(KERN_INFO "%s(): power supply = %s online = %d\n"
			//		, __func__, psy->name, val->intval);
		}
                else val->intval = 0;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static struct power_supply msm_psy_ac = {
	.name = "ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.supplied_to = msm_power_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
	.properties = msm_power_props,
	.num_properties = ARRAY_SIZE(msm_power_props),
	.get_property = msm_power_get_property,
};

static struct power_supply msm_psy_usb = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.supplied_to = msm_power_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
	.properties = msm_power_props,
	.num_properties = ARRAY_SIZE(msm_power_props),
	.get_property = msm_power_get_property,
};

static enum power_supply_property msm_batt_power_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_BATT_TEMP,
	POWER_SUPPLY_PROP_BATT_TEMP_ADC,
	POWER_SUPPLY_PROP_BATT_VOL,
	POWER_SUPPLY_PROP_BATT_VOL_ADC,
	POWER_SUPPLY_PROP_BATT_VF_ADC,
	POWER_SUPPLY_PROP_BATT_VOL_ADC_AVER,
	POWER_SUPPLY_PROP_BATT_TEMP_ADC_AVER,
	POWER_SUPPLY_PROP_BATT_VOL_AVER,
	POWER_SUPPLY_PROP_BATT_TEMP_AVER,
    POWER_SUPPLY_PROP_BATT_CHG_CURRENT,
};

static void msm_batt_update_psy_status_v0(void);
static void msm_batt_update_psy_status_v1(void);

static void msm_batt_external_power_changed(struct power_supply *psy)
{
//	printk(KERN_INFO "%s() : external power supply changed for %s\n",
//			__func__, psy->name);
	power_supply_changed(psy);
}

static int msm_batt_power_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	switch (psp) {

	case POWER_SUPPLY_PROP_STATUS:
		val->intval = msm_batt_info.batt_status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = msm_batt_info.batt_health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = msm_batt_info.batt_valid;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = msm_batt_info.batt_technology;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = msm_batt_info.voltage_max_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = msm_batt_info.voltage_min_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = msm_batt_info.battery_voltage;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = msm_batt_info.batt_capacity;
		break;
	case POWER_SUPPLY_PROP_BATT_TEMP:
		val->intval = msm_batt_info.battery_Celsius;
		break;
	case POWER_SUPPLY_PROP_BATT_TEMP_ADC:
		val->intval = msm_batt_info.battery_temp;
		break;
	case POWER_SUPPLY_PROP_BATT_VOL:
		val->intval = msm_batt_info.battery_voltage;
		break;
	case POWER_SUPPLY_PROP_BATT_VOL_ADC:
		val->intval = msm_batt_info.battery_pre_level; //battery_pre_level
		break;
	case POWER_SUPPLY_PROP_BATT_VF_ADC:
		val->intval = msm_batt_info.battery_vf;
		break;

	case POWER_SUPPLY_PROP_BATT_VOL_ADC_AVER:
		val->intval = msm_batt_info.battery_level;
		break;
	case POWER_SUPPLY_PROP_BATT_TEMP_ADC_AVER:
		val->intval = msm_batt_info.battery_temp;
		break;
	case POWER_SUPPLY_PROP_BATT_VOL_AVER:
		val->intval = msm_batt_info.battery_voltage;
		break;
	case POWER_SUPPLY_PROP_BATT_TEMP_AVER:
		val->intval = dwChargingState;
		break;
	case POWER_SUPPLY_PROP_BATT_CHG_CURRENT:
		val->intval = msm_batt_info.batt_chg_current;
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static struct power_supply msm_psy_batt = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = msm_batt_power_props,
	.num_properties = ARRAY_SIZE(msm_batt_power_props),
	.get_property = msm_batt_power_get_property,
	.external_power_changed = msm_batt_external_power_changed,
};

#ifdef MAX8899_CHARGER
static struct wake_lock vbus_wake_lock;
extern int fsa9280_i2c_write(unsigned char u_addr, unsigned char u_data);
extern int fsa9280_i2c_read(unsigned char u_addr, unsigned char *pu_data);
extern void Is_Interrupt(void);

#define VBUS_5V		0x02

#define FSA_INT1 	0x03
#define FSA_INT2 	0x04
#define FSA_DTYPE1 	0x0A
#define FSA_DTYPE2 	0x0B
#define HIDDEN_REG	0x1D

#define BATT_LOW_VOLT		3350
#define BATT_LEVEL1_VOLT	3600
#define BATT_LEVEL2_VOLT	3640
#define BATT_LEVEL3_VOLT	3700
#define BATT_LEVEL4_VOLT	3760
#define BATT_LEVEL5_VOLT	3860
#define BATT_LEVEL6_VOLT	3990
#define BATT_FULL_VOLT		4200

#define BATT_RECHR_VOLT		4110 //4121		//?????? : 4.12v ??????(4121)

//syun
#define BATT_LOW_ADC		2100 //2158
#define BATT_LEVEL1_ADC		2541 //2589
#define BATT_LEVEL2_ADC		2600 //2657
#define BATT_LEVEL3_ADC		2707 //2760
#define BATT_LEVEL4_ADC		2817 //2863
#define BATT_LEVEL5_ADC		2990 //3035
#define BATT_LEVEL6_ADC		3260
#define BATT_FULL_ADC		3400 //3620

#define BATT_RECHR_ADC		3515

//#define BATT_FULL_ADC		3440
//#define BATT_FULL_VOLT		4150
//#define BATT_LOW_ADC		2140

//#define BATT_VOLT_RANGE	(BATT_FULL_VOLT - BATT_LOW_VOLT)
//#define BATT_ADC_RANGE		(BATT_FULL_ADC - BATT_LOW_ADC)

void msm_batt_check_event(struct work_struct *work);
int battery_restart(void);

static DECLARE_WORK(msm_batt_work, msm_batt_check_event);
void msm_fsa9280_check_event(struct work_struct *work);

static DECLARE_WORK(msm_fsa9280_work, msm_fsa9280_check_event);

void batt_timeover(unsigned long arg )
{
	queue_work(msm_batt_info.msm_batt_wq, &msm_batt_work);
 	batt_registertimer( &msm_batt_info.timer, BATT_CHECK_INTERVAL );
}

void batt_registertimer(struct timer_list* ptimer, unsigned long timeover )
{
	//printk(KERN_ERR "[Battery] %s \n", __func__);

	if(bSetTimer) del_timer( ptimer );
	init_timer( ptimer );
	ptimer->expires = get_jiffies_64() + timeover;
	ptimer->data = (long) NULL;
	ptimer->function = batt_timeover;
	add_timer( ptimer );
	bSetTimer =1;

}

void batt_deregistertimer(struct timer_list* ptimer)
{
	//printk(KERN_ERR "[Battery] %s Before\n", __func__);
	if(bSetTimer){
	//printk(KERN_ERR "[Battery] %s After\n", __func__);
	del_timer( ptimer );
	bSetTimer =0;
	}
}
static enum hrtimer_restart chg_timer_func(struct hrtimer *timer)
{
	gFullChgFlag = 1;
	return HRTIMER_NORESTART;
}




int get_batt_adc(int adc_data_type)
{
	int res = 0;
	int data1 = adc_data_type;
	int data2 = 0;

	res = msm_proc_comm(SMEM_PROC_COMM_GET_ADC, &data1, &data2);
	if(res < 0)
	{
		printk(KERN_ERR "[Battery] %s :%d fail %d\n", __func__, data1, res);
		return res;
	}

	return (data1<<16)|data2;
}

int calculate_batt_level(int batt_volt)
{
	int scaled_level = 0;

	if(batt_volt >= BATT_FULL_VOLT) //99%
	{
		scaled_level = 99;
	}
	else if(batt_volt >=  BATT_LEVEL6_VOLT) //99% ~ 90%
	{
		scaled_level = ((batt_volt -BATT_LEVEL6_VOLT)*9)/(BATT_FULL_VOLT-BATT_LEVEL6_VOLT);
 		scaled_level = scaled_level+90;
	}
	else if(batt_volt >= BATT_LEVEL5_VOLT) //90% ~ 70%
	{
		scaled_level = ((batt_volt -BATT_LEVEL5_VOLT)*20)/(BATT_LEVEL6_VOLT-BATT_LEVEL5_VOLT);
 		scaled_level = scaled_level+70;
	}
	else if(batt_volt >= BATT_LEVEL4_VOLT) //70% ~ 50%
	{
		scaled_level = ((batt_volt -BATT_LEVEL4_VOLT)*20)/(BATT_LEVEL5_VOLT-BATT_LEVEL4_VOLT);
 		scaled_level = scaled_level+50;
	}
	else if(batt_volt >= BATT_LEVEL3_VOLT) //50% ~ 30%
	{
		scaled_level = ((batt_volt -BATT_LEVEL3_VOLT)*20)/(BATT_LEVEL4_VOLT-BATT_LEVEL3_VOLT);
 		scaled_level = scaled_level+30;
	}
	else if(batt_volt >= BATT_LEVEL2_VOLT) //30% ~ 15%
	{
		scaled_level = ((batt_volt -BATT_LEVEL2_VOLT)*15)/(BATT_LEVEL3_VOLT-BATT_LEVEL2_VOLT);
 		scaled_level = scaled_level+15;
	}
	else if(batt_volt >= BATT_LEVEL1_VOLT) //15% ~ 5%
	{
		scaled_level = ((batt_volt -BATT_LEVEL1_VOLT)*10)/(BATT_LEVEL2_VOLT-BATT_LEVEL1_VOLT);
 		scaled_level = scaled_level+5;
	}
	else if(batt_volt > BATT_LOW_VOLT) //5% ~ 1%
	{
		scaled_level = ((batt_volt -BATT_LOW_VOLT)*4)/(BATT_LEVEL1_VOLT-BATT_LOW_VOLT);
 		scaled_level = scaled_level+1;
	}
	else
	{
		if( msm_batt_info.charger_type == CHARGER_TYPE_NONE ) scaled_level = 0;
		else scaled_level = 1;
	}

	printk("[Battery] %s : batt_volt %d, scaled_level %d\n", __func__, batt_volt, scaled_level);

	return scaled_level;
}

int calculate_batt_voltage(int vbatt_adc)
{
	int batt_volt = 0;
/*
	int temp =0;

	if(vbatt_adc < BATT_LOW_ADC)
		return BATT_LOW_VOLT;

	temp = vbatt_adc - BATT_LOW_ADC;
	temp *= BATT_VOLT_RANGE;
	temp /= BATT_ADC_RANGE;

	batt_volt = BATT_LOW_VOLT + temp;
*/
	if(vbatt_adc >= BATT_FULL_ADC)
	{
		batt_volt = BATT_FULL_VOLT;
	}
	else if(vbatt_adc >=  BATT_LEVEL6_ADC) //4.200v ~ 3.990v
	{
		batt_volt = ((vbatt_adc -BATT_LEVEL6_ADC)*(BATT_FULL_VOLT-BATT_LEVEL6_VOLT))/(BATT_FULL_ADC-BATT_LEVEL6_ADC);
 		batt_volt = batt_volt+BATT_LEVEL6_VOLT;
	}
	else if(vbatt_adc >=  BATT_LEVEL5_ADC) //3.990v ~ 3.860v
	{
		batt_volt = ((vbatt_adc -BATT_LEVEL5_ADC)*(BATT_LEVEL6_VOLT-BATT_LEVEL5_VOLT))/(BATT_LEVEL6_ADC-BATT_LEVEL5_ADC);
 		batt_volt = batt_volt+BATT_LEVEL5_VOLT;
	}
	else if(vbatt_adc >=  BATT_LEVEL4_ADC) //3.860v ~ 3.760v
	{
		batt_volt = ((vbatt_adc -BATT_LEVEL4_ADC)*(BATT_LEVEL5_VOLT-BATT_LEVEL4_VOLT))/(BATT_LEVEL5_ADC-BATT_LEVEL4_ADC);
 		batt_volt = batt_volt+BATT_LEVEL4_VOLT;
	}
	else if(vbatt_adc >=  BATT_LEVEL3_ADC) //3.760v ~ 3.700v
	{
		batt_volt = ((vbatt_adc -BATT_LEVEL3_ADC)*(BATT_LEVEL4_VOLT-BATT_LEVEL3_VOLT))/(BATT_LEVEL4_ADC-BATT_LEVEL3_ADC);
 		batt_volt = batt_volt+BATT_LEVEL3_VOLT;
	}
	else if(vbatt_adc >=  BATT_LEVEL2_ADC) //3.700v ~ 3.640v
	{
		batt_volt = ((vbatt_adc -BATT_LEVEL2_ADC)*(BATT_LEVEL3_VOLT-BATT_LEVEL2_VOLT))/(BATT_LEVEL3_ADC-BATT_LEVEL2_ADC);
 		batt_volt = batt_volt+BATT_LEVEL2_VOLT;
	}
	else if(vbatt_adc >=  BATT_LEVEL1_ADC) //3.640v ~ 3.600v
	{
		batt_volt = ((vbatt_adc -BATT_LEVEL1_ADC)*(BATT_LEVEL2_VOLT-BATT_LEVEL1_VOLT))/(BATT_LEVEL2_ADC-BATT_LEVEL1_ADC);
 		batt_volt = batt_volt+BATT_LEVEL1_VOLT;
	}
	else if(vbatt_adc >  BATT_LOW_ADC) //3.600v ~ 3.350v
	{
		batt_volt = ((vbatt_adc -BATT_LOW_ADC)*(BATT_LEVEL1_VOLT-BATT_LOW_VOLT))/(BATT_LEVEL1_ADC-BATT_LOW_ADC);
 		batt_volt = batt_volt+BATT_LOW_VOLT;
	}
	else batt_volt = BATT_LOW_VOLT;

	printk("[Battery] %s : vbatt_adc %d & batt_volt %d\n", __func__, vbatt_adc, batt_volt);

	return batt_volt;
}


static int charging_control(charging_ctrl on)
{
	int res = 0;
	int data1 = SMEM_PROC_COMM_CHARGING_ON_OFF;
	int data2 = on;

	//printk(KERN_ERR "\n[Battery] %s (%d)\n\n", __func__, data2);
	res = msm_proc_comm(SMEM_PROC_COMM_CHARGING_INFO, &data1, &data2);
	if(res < 0)
	{
		printk(KERN_ERR "[Battery] %s :SMEM_PROC_COMM_CHARGING_INFO %d fail %d\n", __func__, data1, res);
		return res;
	}

	return 0;
}

void msm_batt_update(void)
{
	power_supply_changed(&msm_psy_ac);
	power_supply_changed(&msm_psy_usb);
	power_supply_changed(&msm_psy_batt);
}


//int get_charger_type(void)
void get_charger_type(void)
{
	unsigned char dtype1 = 0, int1 = 0;
	unsigned char dtype2 = 0, int2 = 0;
	unsigned char vbus_valid = 0;

	int charger_type = CHARGER_TYPE_NONE;

	//Lnt removal : Should not read interrupt, They are clear on read, effecting the disconnect functionality of cables in fsa9280.c

	//mdelay(100);
	fsa9280_i2c_read(FSA_INT1, &int1);
	//mdelay(10);
	//fsa9280_i2c_read(FSA_INT2, &int2);
	udelay(250);	//mdelay(3);
	fsa9280_i2c_read(FSA_DTYPE1, &dtype1);
	udelay(250);	//mdelay(3);
	fsa9280_i2c_read(FSA_DTYPE2, &dtype2);
	udelay(250);	//mdelay(3);
	fsa9280_i2c_read(HIDDEN_REG, &vbus_valid);

	printk(KERN_ERR "[Battery] %s : devicetype1(0x%x)\n", __func__,  dtype1);
	printk(KERN_ERR "[Battery] %s : devicetype2(0x%x)\n", __func__,  dtype2);
	//printk(KERN_ERR "[Battery] %s : int1(0x%x) dtype1(0x%x)\n", __func__, int1, dtype1);
	//printk(KERN_ERR "[Battery] %s : int2(0x%x) dtype2(0x%x)\n", __func__, int2, dtype2);

	if((vbus_valid & VBUS_5V) && (dtype1 == 0x0))
	{
		dtype1 = 0x40;
	}

	if(dtype1==0x00)
	{
		charger_type = CHARGER_TYPE_NONE;
	}
	else if (dtype1==0x04)
	{
		charger_type = CHARGER_TYPE_USB_PC;
	}
	else if (dtype1==0x40)
	{
		charger_type = CHARGER_TYPE_WALL;
	}
	else if (dtype1==0x10)  // old charger (ver IV)
	{
		charger_type = CHARGER_TYPE_WALL;
	}
	else
	{
		charger_type = CHARGER_TYPE_NONE;
	}
	/*
	//Lnt addition for checking jigUSB off bit 2'nd bit in 255K for checking, similar to victory
	if (dtype2 == 0x02)
	{
		charger_type = CHARGER_TYPE_USB_PC;
	}

	//Lnt  End
	*/
	printk(KERN_ERR "[Battery] %s charger_type is (%d)\n", __func__, charger_type);

	msm_batt_info.charger_type = charger_type;

	//mutex_lock(&msm_batt_info.mlock);

	switch(msm_batt_info.charger_type)
	{
		case CHARGER_TYPE_NONE:
			msm_batt_info.current_chg_source = CHARGER_BATTERY;
			break;
		case CHARGER_TYPE_USB_PC:
			if(msm_batt_info.batt_slate_mode == 1){
			msm_batt_info.current_chg_source = CHARGER_BATTERY;
			msm_batt_info.charger_type = CHARGER_TYPE_NONE;
			break;
			}else if(msm_batt_info.batt_slate_mode == 0){
			msm_batt_info.current_chg_source = CHARGER_USB;
			break;
			}
		case CHARGER_TYPE_WALL:
			msm_batt_info.current_chg_source = CHARGER_AC;
			break;
	        default:
                	printk(KERN_ERR "%s: Not supported cable status received!\n",
                                __FUNCTION__);
	}

	//mutex_unlock(&msm_batt_info.mlock);

        if (charger_type == CHARGER_TYPE_WALL || charger_type == CHARGER_TYPE_USB_PC) {
		wake_lock(&vbus_wake_lock);
		printk("[Battery] %s : wake_lock!!! !!!!\n", __func__);
        } else {
                /* give userspace some time to see the uevent and update
                 * LED state or whatnot...
                 */
		wake_lock_timeout(&vbus_wake_lock, HZ );
		printk("[Battery] %s : wake_lock_timeout!!! !!!!\n", __func__);
        }

	/* if the power source changes, all power supplies may change state */
	msm_batt_update();
}

void charger_existence_check(void)
{
	unsigned char dtype1 = 0;
//	unsigned char dtype2 = 0;
	unsigned char vbus_valid = 0;

	int charger_type = CHARGER_TYPE_NONE;

	udelay(250);	//mdelay(10);
	fsa9280_i2c_read(FSA_DTYPE1, &dtype1);
//	mdelay(10);
//	fsa9280_i2c_read(FSA_DTYPE2, &dtype2);
	udelay(250);	//mdelay(10);
	fsa9280_i2c_read(HIDDEN_REG, &vbus_valid);

    if((vbus_valid & VBUS_5V) && (dtype1 == 0x0))
    {
    	dtype1 = 0x40;
	}

	if(dtype1==0x00)
	{
		charger_type = CHARGER_TYPE_NONE;
	}
	else if (dtype1==0x04)
	{
		charger_type = CHARGER_TYPE_USB_PC;
	}
	else if (dtype1==0x40)
	{
		charger_type = CHARGER_TYPE_WALL;
	}
	else if (dtype1==0x10)  // old charger (ver IV)
	{
		charger_type = CHARGER_TYPE_WALL;
	}
	else
	{
		charger_type = CHARGER_TYPE_NONE;
	}

    if((charger_type == CHARGER_TYPE_USB_PC)&&(msm_batt_info.batt_slate_mode ==1))
		charger_type = CHARGER_TYPE_NONE;
	msm_batt_info.charger_type = charger_type;
}

#ifdef __TEMP_BLOCK_EXCEPTION__
int chg_temp_err_except()
{
  if((msm_batt_info.chg_temp_err_except_check != 0x0) ||
     (msm_batt_info.comp_check.talk_gsm != 0x0) /* Call */ ||
     (msm_batt_info.comp_check.data_call != 0x0)
    )
	{
		printk("chg_temp_err_except_check = %d talk_gsm = %d data_call = %d\n", msm_batt_info.chg_temp_err_except_check, msm_batt_info.comp_check.talk_gsm, msm_batt_info.comp_check.data_call);
    return TRUE;
	}
  else
	{
		printk("chg_temp_err_except_check = %d talk_gsm = %d data_call = %d\n", msm_batt_info.chg_temp_err_except_check, msm_batt_info.comp_check.talk_gsm, msm_batt_info.comp_check.data_call);
    return FALSE;
}
}
#endif

int CheckTemperature(int avg_temp, int oldTempInterrupt)
{
	int newTempInterrupt;

	if( oldTempInterrupt == CHARGING_TEMPERATURE_NORMAL )         // temperate is normal
 	{
		if( avg_temp >= CHG_OFF_HIGH_TEMP )
		{
#ifdef __TEMP_BLOCK_EXCEPTION__
			if( chg_temp_err_except() )
			{
				if( avg_temp >= CHG_EXCEPT_HIGH_TEMP )
				{
					newTempInterrupt = CHARGING_TEMPERATURE_HIGH;
					//printk("[Battery] %s :  Temperate is so HIGH => charging is Stopped!! \n", __func__);
				}
				else
				{
					newTempInterrupt = CHARGING_TEMPERATURE_NORMAL;
					//printk("[Battery] %s :  Temperate is normal!! \n", __func__);
				}
			}
			else
#endif
			{
		 		newTempInterrupt = CHARGING_TEMPERATURE_HIGH;
				//printk("[Battery] %s :  Temperate is so HIGH => charging is Stopped!! \n", __func__);
			}
	    }
		else if( avg_temp <= CHG_OFF_LOW_TEMP )
		{
		 	newTempInterrupt = CHARGING_TEMPERATURE_LOW;
			//printk("[Battery] %s :  Temperate is so LOW => charging is Stopped!!!! \n", __func__);
	    }
		else
		{
			newTempInterrupt = CHARGING_TEMPERATURE_NORMAL;
			//printk("[Battery] %s :  Temperate is normal!! \n", __func__);
		}
	}
	else
	{
		if(oldTempInterrupt == CHARGING_TEMPERATURE_HIGH)         // temperature is HIGH
		{
			if(avg_temp <= CHG_RESTART_HIGH_TEMP)
		 	{
				newTempInterrupt = CHARGING_TEMPERATURE_NORMAL;
				//printk("[Battery] %s :  Temperate is normal(from HIGH) so charging is Restored!!!! \n", __func__);
			}
			else
			{
			 	newTempInterrupt = CHARGING_TEMPERATURE_HIGH;
				//printk("[Battery] %s : Temperate is still HIGH!! \n", __func__);
	       	}
		}
		else													// temperature is LOW
		{
			if(avg_temp >= CHG_RESTART_LOW_TEMP)
		 	{
				newTempInterrupt = CHARGING_TEMPERATURE_NORMAL;
				//printk("[Battery] %s :  Temperate is normal(from LOW) so charging is Restored!!!! \n", __func__);
		    }
			else
			{
				newTempInterrupt = CHARGING_TEMPERATURE_LOW;
				//printk("[Battery] %s :   Temperate is still LOW!!! \n", __func__);
			}
		}
	}
	return newTempInterrupt;

}


int IsRechargingValue(int fg_voltage)
{
	if(fg_voltage <= BATT_RECHR_VOLT)	return 1;
	else	return 0;
}
/*
static int avg_current_ex(int avg)
{
	int total = 0;
	int i = 0;

	if(!bChg_Flag)
	{
		gAvg_Current[gChg_Cnt] = avg;
		for( i = 0 ; i <= gChg_Cnt ; i++ )
			total += gAvg_Current[i];

		gChg_Cnt++;
		total /= gChg_Cnt;

	if( gChg_Cnt >= AVG_CURRENT_ARR_SIZE )	bChg_Flag = 1;
	}
	else
	{
		gChg_Cnt %= AVG_CURRENT_ARR_SIZE;
		gAvg_Current[gChg_Cnt] = avg;
		for( i = 0 ; i < AVG_CURRENT_ARR_SIZE ; i++ )
			total += gAvg_Current[i];
		total /= AVG_CURRENT_ARR_SIZE;
		gChg_Cnt++;
	}

	return total;
}
*/
static void FullChgCutoff(void)
{
	if( msm_batt_info.battery_voltage >= 4000 && ( msm_batt_info.batt_chg_current <= CUT_OFF_CURRENT ) )
	{
		if( !gChgTimerFlag )
		{
			hrtimer_start(&ChgTimer, ktime_set(20, 0), HRTIMER_MODE_REL);
			gChgTimerFlag = 1;
		}
		else return;
	}
	else
	{
		if( gChgTimerFlag )
		{
			hrtimer_try_to_cancel(&ChgTimer);
			gChgTimerFlag = 0;
		}
		else return;
	}
}


int get_charging_status(void)
{
	int res = 0;
	int data1 = SMEM_PROC_COMM_CHARGING_READ_STATUS;
	int data2 = 0;
	char event_flag = 0;

	res = msm_proc_comm(SMEM_PROC_COMM_CHARGING_INFO, &data1, &data2);
	if(res < 0)
	{
		printk(KERN_ERR "[Battery] %s :SMEM_PROC_COMM_CHARGING_INFO : %d fail %d\n", __func__, data1, res);
		return res;
	}
	event_flag = (data2 & 0xF00) >> 8;
	printk("[Battery] %s : event_flag =%x \n", __func__, event_flag);

	if(event_flag & CHG_CONNECT )	gChg_connect = 1;
	else gChg_connect = 0;

	if(event_flag & TIMER_MASK ) gTimer = 1;
	else gTimer = 0;

	return 1;
}

int IsFullCharged(int avg_level)
{
	FullChgCutoff();

	fg_gFull_chg = gFullChgFlag;

	if( gFullChgFlag || gTimer )
	{
		//printk("[Battery] %s : Full chg !!! !!!!\n", __func__);
		return 1;
	}
	else return 0;
}

static int check_charging_status(int oldChargingState)
{
	spin_lock(&msm_batt_info.lock);

	if(msm_batt_info.charger_type == CHARGER_TYPE_NONE)
	{
		dwChargingState = CHARGING_STATE_BATTERY;
//		printk("[Battery] %s :  Not Charging \n", __func__);
		gRechg_start = 0;
		gTimeover_start = 0;
		gFullChgFlag = 0;
	}
	else
	{
		if(hw_version == 0 && msm_batt_info.battery_vf > 70)  //syun
		{
			dwChargingState=CHARGING_STATE_ABNORMAL_BATTERY;
//			printk("[Battery] %s msm_batt_info.battery_vf= %d  :  Abnormal battery \n", __func__, msm_batt_info.battery_vf);
		}
		else if( ((hw_version == 1)||(hw_version == 2)||(hw_version == 3)) && ( msm_batt_info.battery_vf < 0 || msm_batt_info.battery_vf > 8 )  )  //vf_adc : 3
		{
			dwChargingState=CHARGING_STATE_ABNORMAL_BATTERY;
//			printk("[Battery] %s msm_batt_info.battery_vf= %d  :  Abnormal battery \n", __func__, msm_batt_info.battery_vf);
		}
		else if( hw_version >= 4 && ( msm_batt_info.battery_vf < 3 || msm_batt_info.battery_vf > 13 )  )   //vf_adc : 8
		{
			dwChargingState=CHARGING_STATE_ABNORMAL_BATTERY;
//			printk("[Battery] %s msm_batt_info.battery_vf= %d  :  Abnormal battery \n", __func__, msm_batt_info.battery_vf);
		}
		else
		{
			tempInterrupt = CheckTemperature(msm_batt_info.battery_temp, tempInterrupt );
#if CHG_DEBUG
//			printk("[Battery] %s :	msm_batt_info.battery_temp[%d]\n", __func__ ,msm_batt_info.battery_temp);  //syun
#endif

			if(tempInterrupt)
			{
				if(tempInterrupt == CHARGING_TEMPERATURE_HIGH ) dwChargingState=CHARGING_STATE_TA_HIGH_TEMPERATURE;
				else dwChargingState=CHARGING_STATE_TA_LOW_TEMPERATURE;

				gRechg_start = 0;
				gFullChgFlag = 0;

				hrtimer_try_to_cancel(&ChgTimer);
//				printk("[Battery] %s :  TA temperature interrupt \n", __func__);
			}
			else //Temperature Normal!!!
			{
				if(IsFullCharged( msm_batt_info.battery_voltage)) //Full charged!!
				{
					if(!gTimer)
					{
						if(IsRechargingValue(msm_batt_info.battery_voltage))
						{
								//////////////////////////////////////////////////////////////////////
								gRechg_start = 1;
								//printk("[Battery] %s :  full Re-charging start \n", __func__);
								if(msm_batt_info.charger_type == CHARGER_TYPE_WALL)
								{
									dwChargingState=CHARGING_STATE_TA_FULL_RECHARGING;
									//printk("[Battery] %s :  TA Full-Recharging \n", __func__);
								}
								else
								{
									dwChargingState=CHARGING_STATE_USB_FULL_RECHARGING;
									//printk("[Battery] %s :  USB Full-Recharging \n", __func__);
								}
								//////////////////////////////////////////////////////////////////////
						}
						else
						{
							//printk("[Battery] %s :  TA full charged \n", __func__);
							 dwChargingState=CHARGING_STATE_TA_FULL_CHARGED;
						}
					}
					else
					{
						//printk("[Battery] %s :  Time Over Stop charging!!! \n", __func__);
						dwChargingState=CHARGING_STATE_TA_TIMEOVER_CHARGED;
						gTimeover_start = 1;
						gRechg_start = 0;
					}
						msm_batt_info.batt_capacity = 100;
				}
				else //charging
				{
					if(gRechg_start)  //In case of recharging after full-chg
					{
						//////////////////////////////////////////////////////////////////////
						gRechg_start = 1;
						//printk("[Battery] %s :  full Re-charging start \n", __func__);
						if(msm_batt_info.charger_type == CHARGER_TYPE_WALL)
						{
							dwChargingState=CHARGING_STATE_TA_FULL_RECHARGING;
							//printk("[Battery] %s :  TA Full-Recharging \n", __func__);
						}
						else
						{
							dwChargingState=CHARGING_STATE_USB_FULL_RECHARGING;
							//printk("[Battery] %s :  USB Full-Recharging \n", __func__);
						}
						//////////////////////////////////////////////////////////////////////
						msm_batt_info.batt_capacity = 100;
					}
					else //In case of Normal charging
					{
						//Normal case
						if(!gTimeover_start)
						{
							if(msm_batt_info.charger_type == CHARGER_TYPE_WALL)
							{
							dwChargingState=CHARGING_STATE_TA_CHARGING;
							//printk("[Battery] %s :  TA charging \n", __func__);
							}
							else
							{
								dwChargingState=CHARGING_STATE_USB_CHARGING;
								//printk("[Battery] %s :  USB charging \n", __func__);
							}
						}
						//Timer over re-charging start check
						else
						{
							if(IsRechargingValue(msm_batt_info.battery_voltage))
							{
    							gRechg_start = 1;
								if(msm_batt_info.charger_type == CHARGER_TYPE_WALL)
								{
								dwChargingState=CHARGING_STATE_TA_CHARGING;
								//printk("[Battery] %s : Timer over TA charging \n", __func__);
								}
								else
								{
									dwChargingState=CHARGING_STATE_USB_CHARGING;
									//printk("[Battery] %s : Timer over  USB charging \n", __func__);
								}
							}
                            else
							{
								dwChargingState=CHARGING_STATE_TA_TIMEOVER_CHARGED;
								gTimeover_start = 1;
								gRechg_start = 0;
							}
							msm_batt_info.batt_capacity = 100;
						}

					}
				}
			}
		}
	}

#ifdef __TEMPERATURE_DEBUG_CONTROL__
	switch(msm_batt_info.control_tmp)
	{
		case 0:
			break;
		case 1:
			dwChargingState = CHARGING_STATE_TA_HIGH_TEMPERATURE;
			break;
		case 2:
			dwChargingState = CHARGING_STATE_TA_LOW_TEMPERATURE;
			break;
	}
#endif

	//printk("[Battery] Currnet Charging state [%x, %x]  Batt Level [%d] Percent [%d]\n", dwChargingState, msm_batt_info.battery_Celsius, msm_batt_info.battery_level, msm_batt_info.batt_capacity);

	charger_existence_check();

	if(oldChargingState!= dwChargingState)
	{
#if CHG_DEBUG
//		printk("[Battery] Charging state is changed [%x]=>[%x]   Batt Level [%d]   Percent [%d]\n", oldChargingState, dwChargingState, msm_batt_info.battery_level, msm_batt_info.batt_capacity);
#endif
		switch(dwChargingState)
			{
			case CHARGING_STATE_BATTERY:
				msm_batt_info.batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
				msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
				msm_batt_info.charger_status = CHARGER_STATUS_GOOD;
				charging_control(SMEM_PROC_COMM_CHARGING_OFF);
				break;

			case CHARGING_STATE_ABNORMAL_BATTERY:
				msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
				msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_DEAD;
				msm_batt_info.charger_status = CHARGER_STATUS_INVALID;
				charging_control(SMEM_PROC_COMM_CHARGING_OFF);
				break;

			case CHARGING_STATE_USB_CHARGING:
				if(msm_batt_info.batt_slate_mode == 0){
				    msm_batt_info.batt_status = POWER_SUPPLY_STATUS_CHARGING;
				    msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
				    msm_batt_info.charger_status = CHARGER_STATUS_GOOD;
				    charging_control(SMEM_PROC_COMM_CHARGING_ON_USB);
				}else if(msm_batt_info.batt_slate_mode == 1){
				    msm_batt_info.batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
				    msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
				    msm_batt_info.charger_status = CHARGER_STATUS_GOOD;
				    charging_control(SMEM_PROC_COMM_CHARGING_OFF);
				}
				break;

			case CHARGING_STATE_TA_HIGH_TEMPERATURE:
				msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
				msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_OVERHEAT;
				msm_batt_info.charger_status = CHARGER_STATUS_GOOD;
				charging_control(SMEM_PROC_COMM_CHARGING_OFF);
				break;

			case CHARGING_STATE_TA_LOW_TEMPERATURE:
				msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
				msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_COLD;
				msm_batt_info.charger_status = CHARGER_STATUS_GOOD;
				charging_control(SMEM_PROC_COMM_CHARGING_OFF);
				break;

			case CHARGING_STATE_TA_FULL_CHARGED:
				msm_batt_info.batt_capacity = 100;
				msm_batt_info.batt_status = POWER_SUPPLY_STATUS_FULL;
				msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
				msm_batt_info.charger_status = CHARGER_STATUS_GOOD;
				charging_control(SMEM_PROC_COMM_CHARGING_OFF);
				break;

			case CHARGING_STATE_TA_FULL_RECHARGING:
				msm_batt_info.batt_capacity = 100;
				msm_batt_info.batt_status = POWER_SUPPLY_STATUS_FULL;
				msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
				msm_batt_info.charger_status = CHARGER_STATUS_GOOD;
				charging_control(SMEM_PROC_COMM_CHARGING_ON_TA);
				break;

			case CHARGING_STATE_TA_CHARGING:
				msm_batt_info.batt_status = POWER_SUPPLY_STATUS_CHARGING;
				msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
				msm_batt_info.charger_status = CHARGER_STATUS_GOOD;
				charging_control(SMEM_PROC_COMM_CHARGING_ON_TA);
				break;

			case CHARGING_STATE_TA_TIMEOVER_CHARGED:
				msm_batt_info.batt_status = POWER_SUPPLY_STATUS_FULL;
				msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
				msm_batt_info.charger_status = CHARGER_STATUS_GOOD;
				charging_control(SMEM_PROC_COMM_CHARGING_OFF);
				break;

			case CHARGING_STATE_USB_FULL_RECHARGING:
				msm_batt_info.batt_capacity = 100;
				msm_batt_info.batt_status = POWER_SUPPLY_STATUS_FULL;
				msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
				msm_batt_info.charger_status = CHARGER_STATUS_GOOD;
				charging_control(SMEM_PROC_COMM_CHARGING_ON_USB);
				break;

			}

		switch(dwChargingState)
		{
			case CHARGING_STATE_USB_CHARGING:
				if(msm_batt_info.batt_slate_mode == 1){
				break;
				}
			case CHARGING_STATE_TA_CHARGING:
				gFullChgFlag = 0;
				break;


			case CHARGING_STATE_USB_FULL_RECHARGING:
			case CHARGING_STATE_TA_FULL_RECHARGING:
				gFullChgFlag = 0;
			case CHARGING_STATE_TA_TIMEOVER_CHARGED:
			case CHARGING_STATE_TA_FULL_CHARGED:

				break;

			case CHARGING_STATE_TA_HIGH_TEMPERATURE:
			case CHARGING_STATE_TA_LOW_TEMPERATURE:

				break;

			case CHARGING_STATE_BATTERY: // NOTHING
			case CHARGING_STATE_ABNORMAL_BATTERY:
			default:

				break;
		}
	}

	if(gTimeover_start)
	{
		msm_batt_info.batt_status = POWER_SUPPLY_STATUS_FULL;
		msm_batt_info.batt_capacity = 100;
		//printk("[Battery] %s : Timer over charging 100\n", __func__);
	}

	spin_unlock(&msm_batt_info.lock);

	return 0;
}

//Calculate Average Sample

int GetAverageSample(int modified_sample){

	int dwTotal_temp=0;

	//printk("[Battery] %s : Raw Vol ADC Sample = %d \n", __func__, modified_sample);

	if(uCount <= AVERAGE_SAMPLE_NUMBER)
	{
		dwTotal += modified_sample;
		AverageSample= dwTotal/uCount;
		//printk("[Battery] %s :Battery: ++uCount =%d \n", __func__, uCount);
		//printk("[Battery] %s :Battery: Total_voltage =%d \n", __func__, dwTotal);
		//printk("[Battery] %s :Battery: AverageSample =%d \n", __func__, AverageSample);
		uCount +=1;
	}
	else
	{
		dwTotal_temp = dwTotal - AverageSample;
		dwTotal = dwTotal_temp + modified_sample;
		//printk("[Battery] %s :Battery: ++dwTotal =%d \n", __func__, dwTotal);
		AverageSample= dwTotal/AVERAGE_SAMPLE_NUMBER;

		//printk("[Battery] %s :Battery: ++dwTotal =%d \n", __func__, dwTotal);
		//printk("[Battery] %s :Battery: --dwTotal =%d \n", __func__, uCount);
		//printk("[Battery] %s :Battery: Total_voltage =%d \n", __func__, dwTotal);
		//printk("[Battery] %s :Battery: AverageSample =%d \n", __func__, AverageSample);
	}
	//printk("[Battery] %s :Battery{CalculateAverage()}: AverageSample = %d \n", __func__, AverageSample);
	return AverageSample;

}

static int get_batt_info(void)
{
	int Alldata = 0;
#ifdef FUEL_GAUGE_I2C
	int	fg_soc = 0;
	int fg_vcell = 0;
	unsigned long flags;

/* Compensation Algorithm code : for simulating QSTRT pin functionality to reset the chip : START */

/* Below value is taken from Vinsq */
#define FG_COMPER_COND_VOLTAGE 3550

	spin_lock_irqsave(&msm_batt_info.lock, flags);

	if ((fg_soc = fg_read_soc()) < 0) {
		fg_soc = msm_batt_info.batt_capacity;
	}else
		msm_batt_info.batt_capacity = fg_soc;

        if ((fg_vcell = fg_read_vcell()) < 0) {
		fg_vcell = msm_batt_info.battery_voltage;
	}else
		msm_batt_info.battery_voltage = fg_vcell;
	spin_unlock_irqrestore(&msm_batt_info.lock, flags);

	if (fg_vcell >= FG_COMPER_COND_VOLTAGE) {
		if (fg_soc == 0) {
			printk(" **************** LNT DEBUG : Before Resetting ********************  \n");
			fg_reset_soc();
			fg_soc = fg_read_soc();
			}
 	}

/* Compensation Algorithm code : for simulating QSTRT pin functionality to reset the chip : END */

#endif

	spin_lock(&msm_batt_info.lock);

	if(msm_batt_info.charger_type != CHARGER_TYPE_NONE)
	{
		Alldata = get_batt_adc(SMEM_PROC_COMM_GET_ADC_CURRENT);
		msm_batt_info.batt_chg_current = (Alldata>>16) & 0xFFFF;
	}
	else
	{
		msm_batt_info.batt_chg_current = 0;
	}

	Alldata = get_batt_adc(SMEM_PROC_COMM_GET_ADC_ALL);

	msm_batt_info.battery_level	=(Alldata>>16) & 0xFFFF;
	msm_batt_info.battery_temp	= Alldata & 0xFFFF;

	//Temp in celsius
	msm_batt_info.battery_Celsius = (((msm_batt_info.battery_temp-141)*140)/13)-120;  // temp  = (5/6)(120 - adc),   10.0 => 100

	Alldata = get_batt_adc(SMEM_PROC_COMM_GET_ADC_VF);
	msm_batt_info.battery_vf = (Alldata>>16) & 0xFFFF;
#if CHG_DEBUG
	printk("[%s] \n", __func__);

    printk("battery_level = %d \n", msm_batt_info.battery_level);
    printk("battery_temp = %d battery_Celsius = %d\n", msm_batt_info.battery_temp, msm_batt_info.battery_Celsius);
	printk("chg_current = %d \n", msm_batt_info.batt_chg_current);
    printk("battery_voltage = %d batt_capacity = %d\n", msm_batt_info.battery_voltage, msm_batt_info.batt_capacity);
#endif
	//Temp For Service Mode App
	msm_batt_info.battery_pre_level = msm_batt_info.battery_level;

#ifdef __COMPENSATE_SAMPLE__
	msm_batt_info.batt_comp_delta = battery_compensation_val(msm_batt_info.battery_level);
	printk("[Battery] %s : batt_comp_delta ADC Value =%d \n", __func__, msm_batt_info.batt_comp_delta);
	msm_batt_info.battery_level += msm_batt_info.batt_comp_delta;
#endif

#ifndef FUEL_GAUGE_I2C
 	//Calculate Voltage & Capacity from Battery Average Voltage ADC
	msm_batt_info.battery_voltage = calculate_batt_voltage(msm_batt_info.battery_level);
	msm_batt_info.batt_capacity = calculate_batt_level(msm_batt_info.battery_voltage);
#else
	msm_batt_info.battery_voltage = fg_vcell;
	msm_batt_info.batt_capacity = fg_soc;
#endif

	msm_batt_info.charger_status = CHARGER_STATUS_GOOD; // temp
	spin_unlock(&msm_batt_info.lock);
/*
	printk("[Battery] %s : charger_status = %s, charger_type = %s,"
		" batt_volt = %d, batt_temp = %d,\n", __func__,
		charger_status[msm_batt_info.charger_status],
		charger_type[msm_batt_info.charger_type],
		msm_batt_info.battery_voltage,
		msm_batt_info.battery_temp);
*/
	return 0;
}

static int init_skip = 0;
void msm_batt_check_event(struct work_struct *work)
{
	if(!init_skip)
	{
		printk(KERN_ERR "[Battery] %s (init_skip) \n",__func__);
		init_skip = 1;
		return;
	}

	get_batt_info();
	if(fg_alert)	Is_Interrupt();
	check_charging_status(dwChargingState);
	msm_batt_update();
	//arging_control(SMEM_PROC_COMM_CHARGING_ON_TA);
}

void msm_fsa9280_check_event(struct work_struct *work)
{
	unsigned char control = 0;
	int ret=0;
	unsigned char int1 = 0;

	fsa9280_i2c_read(FSA_INT1, &int1); // clear intterupt pin

	ret=fsa9280_i2c_read(0x02, &control);
	
	printk(KERN_ERR "[Battery] %s , control reg 0x02 : 0x%x \n", __func__, control);

	if(control == 0x1f)
	{
		/* fsa9280 init sequence */
		fsa9280_i2c_write(0x02, 0x1E); // FSA9480 initilaization
		fsa9280_i2c_read(0x02, &control); // FSA9480 initilaization check
		printk("[Battery]: Initial control reg 0x02 : 0x%x\n", control);

		/* delay 2 ms */
		mdelay(2);
		printk(KERN_INFO "[Battery]: fsa9280 sensor init sequence done\n");
	}
}

static int msm_batt_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	//printk(KERN_ERR "[Battery] %s\n", __func__);
	batt_deregistertimer(&msm_batt_info.timer);
	return 0;
}

static int msm_batt_resume(struct platform_device *pdev)
{
	printk(KERN_ERR "[Battery] %s\n", __func__);

	queue_work(msm_batt_info.msm_batt_wq, &msm_batt_work);

	batt_registertimer(&msm_batt_info.timer, BATT_CHECK_INTERVAL);
	return 0;
}

int battery_restart(void)
{
	//printk(KERN_ERR "[Battery] %s\n", __func__);
	queue_work(msm_batt_info.msm_batt_wq, &msm_fsa9280_work);
	
	batt_deregistertimer(&msm_batt_info.timer);

	queue_work(msm_batt_info.msm_batt_wq, &msm_batt_work);

	batt_registertimer(&msm_batt_info.timer, BATT_CHECK_INTERVAL);
	
	return 0;
}
#endif

static int msm_batt_get_batt_chg_status_v1(void)
{
	int rc ;
	struct rpc_req_batt_chg {
		struct rpc_request_hdr hdr;
		u32 more_data;
	} req_batt_chg;

	req_batt_chg.more_data = cpu_to_be32(1);

	memset(&rep_batt_chg, 0, sizeof(rep_batt_chg));

	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_GET_GENERAL_STATUS_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_batt_chg, sizeof(rep_batt_chg),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		printk(KERN_ERR
		       "%s: msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_GET_GENERAL_STATUS_PROC, rc);
		return rc;
	} else if (be32_to_cpu(rep_batt_chg.more_data)) {

		rep_batt_chg.charger_status =
			be32_to_cpu(rep_batt_chg.charger_status);

		rep_batt_chg.charger_type =
			be32_to_cpu(rep_batt_chg.charger_type);

		rep_batt_chg.battery_status =
			be32_to_cpu(rep_batt_chg.battery_status);

		rep_batt_chg.battery_level =
			be32_to_cpu(rep_batt_chg.battery_level);

		rep_batt_chg.battery_voltage =
			be32_to_cpu(rep_batt_chg.battery_voltage);

		rep_batt_chg.battery_temp =
			be32_to_cpu(rep_batt_chg.battery_temp);

		printk(KERN_INFO "charger_status = %s, charger_type = %s,"
				" batt_status = %s, batt_level = %s,"
				" batt_volt = %u, batt_temp = %u,\n",
				charger_status[rep_batt_chg.charger_status],
				charger_type[rep_batt_chg.charger_type],
				battery_status[rep_batt_chg.battery_status],
				battery_level[rep_batt_chg.battery_level],
				rep_batt_chg.battery_voltage,
				rep_batt_chg.battery_temp);

	} else {
		printk(KERN_INFO "%s():No more data in batt_chg rpc reply\n",
				__func__);
		return -EIO;
	}

	return 0;
}

static void msm_batt_update_psy_status_v1(void)
{
	msm_batt_get_batt_chg_status_v1();

	if (msm_batt_info.charger_status == rep_batt_chg.charger_status &&
		msm_batt_info.charger_type == rep_batt_chg.charger_type &&
		msm_batt_info.battery_status ==  rep_batt_chg.battery_status &&
		msm_batt_info.battery_level ==  rep_batt_chg.battery_level &&
		msm_batt_info.battery_voltage  == rep_batt_chg.battery_voltage
		&& msm_batt_info.battery_temp ==  rep_batt_chg.battery_temp) {

		printk(KERN_NOTICE "%s() : Got unnecessary event from Modem "
			"PMIC VBATT driver. Nothing changed in Battery or "
			"charger status\n", __func__);
		return;
	}

	msm_batt_info.battery_voltage  	= 	rep_batt_chg.battery_voltage;
	msm_batt_info.battery_temp 	=	rep_batt_chg.battery_temp;


	if (rep_batt_chg.battery_status != BATTERY_STATUS_INVALID) {

		msm_batt_info.batt_valid = 1;

		if (msm_batt_info.battery_voltage >
		    msm_batt_info.voltage_max_design) {

			msm_batt_info.batt_health =
			    POWER_SUPPLY_HEALTH_OVERVOLTAGE;

			msm_batt_info.batt_status =
				POWER_SUPPLY_STATUS_NOT_CHARGING;

		} else if (msm_batt_info.battery_voltage
			 < msm_batt_info.voltage_min_design) {

			msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_DEAD;
			msm_batt_info.batt_status =
				POWER_SUPPLY_STATUS_UNKNOWN;

		} else if (rep_batt_chg.battery_status == BATTERY_STATUS_BAD) {

			msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_DEAD;
			msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;

		} else if (rep_batt_chg.battery_status ==
				BATTERY_STATUS_BAD_TEMP) {

			msm_batt_info.batt_health =
				POWER_SUPPLY_HEALTH_OVERHEAT;

			if (rep_batt_chg.charger_status == CHARGER_STATUS_BAD
				|| rep_batt_chg.charger_status ==
				CHARGER_STATUS_INVALID)
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_UNKNOWN;
			else
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_NOT_CHARGING;

		} else if ((rep_batt_chg.charger_status == CHARGER_STATUS_GOOD
			|| rep_batt_chg.charger_status == CHARGER_STATUS_WEAK)
			&& (rep_batt_chg.battery_status ==
				BATTERY_STATUS_GOOD)) {

			msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;

			if (rep_batt_chg.battery_level == BATTERY_LEVEL_FULL)
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_FULL;
			else
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_CHARGING;

		} else if ((rep_batt_chg.charger_status == CHARGER_STATUS_BAD
				|| rep_batt_chg.charger_status ==
				CHARGER_STATUS_INVALID) &&
				(rep_batt_chg.battery_status ==
					BATTERY_STATUS_GOOD)) {

			msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
			msm_batt_info.batt_status =
				POWER_SUPPLY_STATUS_DISCHARGING;
		}

#ifndef FUEL_GAUGE_I2C
		msm_batt_info.batt_capacity =
			msm_batt_info.calculate_capacity(
					msm_batt_info.battery_voltage);
#endif

	} else {
		msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_UNKNOWN;
		msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;
		msm_batt_info.batt_capacity = 0;
	}


	if (msm_batt_info.charger_type != rep_batt_chg.charger_type) {

		msm_batt_info.charger_type = rep_batt_chg.charger_type ;

		if (msm_batt_info.charger_type == CHARGER_TYPE_WALL) {

			msm_batt_info.current_chg_source &= ~USB_CHG;
			msm_batt_info.current_chg_source |= AC_CHG;

			printk(KERN_INFO "%s() : charger_type = WALL\n",
					__func__);

			power_supply_changed(&msm_psy_ac);

		} else if (msm_batt_info.charger_type ==
				CHARGER_TYPE_USB_WALL ||
				msm_batt_info.charger_type ==
				CHARGER_TYPE_USB_PC) {

			msm_batt_info.current_chg_source &= ~AC_CHG;
			msm_batt_info.current_chg_source |= USB_CHG;

			printk(KERN_INFO "%s() : charger_type = %s\n",
			__func__,
			charger_type[msm_batt_info.charger_type]);

			power_supply_changed(&msm_psy_usb);

		} else {

			printk(KERN_INFO "%s() : charger_type = %s\n",
			__func__,
			charger_type[msm_batt_info.charger_type]);

			msm_batt_info.batt_status =
				POWER_SUPPLY_STATUS_DISCHARGING;

			if (msm_batt_info.current_chg_source & AC_CHG) {

				msm_batt_info.current_chg_source &= ~AC_CHG;

				printk(KERN_INFO "%s() : AC WALL charger"
					" removed\n", __func__);

				power_supply_changed(&msm_psy_ac);

			} else if (msm_batt_info.current_chg_source & USB_CHG) {

				msm_batt_info.current_chg_source &= ~USB_CHG;
				printk(KERN_INFO "%s() : USB charger removed\n",
						__func__);

				power_supply_changed(&msm_psy_usb);
			} else
				power_supply_changed(&msm_psy_batt);
		}
	} else
		power_supply_changed(&msm_psy_batt);

	msm_batt_info.charger_status 	= 	rep_batt_chg.charger_status;
	msm_batt_info.charger_type 	= 	rep_batt_chg.charger_type;
	msm_batt_info.battery_status 	=  	rep_batt_chg.battery_status;
	msm_batt_info.battery_level 	=  	rep_batt_chg.battery_level;
	msm_batt_info.battery_voltage  	= 	rep_batt_chg.battery_voltage;
	msm_batt_info.battery_temp 	=	rep_batt_chg.battery_temp;
}


static int msm_batt_get_batt_chg_status_v0(u32 *batt_charging,
					u32 *charger_valid,
					u32 *chg_batt_event)
{
	struct rpc_request_hdr req_batt_chg;

	struct rpc_reply_batt_volt {
		struct rpc_reply_hdr hdr;
		u32 voltage;
	} rep_volt;

#ifdef CHG_RPC_SERVER_AVAILABLE
	struct rpc_reply_chg_reply {
		struct rpc_reply_hdr hdr;
		u32 chg_batt_data;
	} rep_chg;
#endif
	int rc;
	*batt_charging = 0;
	*chg_batt_event = CHG_UI_EVENT_INVALID;
	*charger_valid = 0;

	rc = msm_rpc_call_reply(msm_batt_info.batt_ep,
				BATTERY_READ_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_volt, sizeof(rep_volt),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		printk(KERN_ERR
		       "%s: msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, BATTERY_READ_PROC, rc);

		return rc;
	}
	msm_batt_info.battery_voltage = be32_to_cpu(rep_volt.voltage);

#ifdef CHG_RPC_SERVER_AVAILABLE
	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_IS_CHARGING_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_chg, sizeof(rep_chg),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		printk(KERN_ERR
		       "%s: msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_IS_CHARGING_PROC, rc);
		return rc;
	}
	*batt_charging = be32_to_cpu(rep_chg.chg_batt_data);

	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_IS_CHARGER_VALID_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_chg, sizeof(rep_chg),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		printk(KERN_ERR
		       "%s: msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_IS_CHARGER_VALID_PROC, rc);
		return rc;
	}
	*charger_valid = be32_to_cpu(rep_chg.chg_batt_data);

	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_IS_BATTERY_VALID_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_chg, sizeof(rep_chg),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		printk(KERN_ERR
		       "%s: msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_IS_BATTERY_VALID_PROC, rc);
		return rc;
	}
	msm_batt_info.batt_valid = be32_to_cpu(rep_chg.chg_batt_data);

	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_UI_EVENT_READ_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_chg, sizeof(rep_chg),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		printk(KERN_ERR
		       "%s: msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_UI_EVENT_READ_PROC, rc);
		return rc;
	}
	*chg_batt_event = be32_to_cpu(rep_chg.chg_batt_data);

#endif
	return 0;
}

static void msm_batt_update_psy_status_v0(void)
{
	u32 batt_charging = 0;
	u32 chg_batt_event = CHG_UI_EVENT_INVALID;
	u32 charger_valid = 0;

	msm_batt_get_batt_chg_status_v0(&batt_charging, &charger_valid,
				     &chg_batt_event);

	printk(KERN_INFO "batt_charging = %u  batt_valid = %u "
			" batt_volt = %u\n charger_valid = %u "
			" chg_batt_event = %u\n",
			batt_charging, msm_batt_info.batt_valid,
			msm_batt_info.battery_voltage,
			charger_valid, chg_batt_event);

	printk(KERN_INFO "Previous charger valid status = %u"
			"  current charger valid status = %u\n",
			msm_batt_info.charger_valid, charger_valid);

	if (msm_batt_info.charger_valid != charger_valid) {

		msm_batt_info.charger_valid = charger_valid;
		if (msm_batt_info.charger_valid)
			msm_batt_info.current_chg_source |= USB_CHG;
		else
			msm_batt_info.current_chg_source &= ~USB_CHG;
		power_supply_changed(&msm_psy_usb);
	}

	msm_batt_info.batt_valid = 1;

	if (msm_batt_info.batt_valid) {

		if (msm_batt_info.battery_voltage >
		    msm_batt_info.voltage_max_design)
			msm_batt_info.batt_health =
			    POWER_SUPPLY_HEALTH_OVERVOLTAGE;

		else if (msm_batt_info.battery_voltage
			 < msm_batt_info.voltage_min_design)
			msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_DEAD;
		else
			msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;

		if (batt_charging && msm_batt_info.charger_valid)
			msm_batt_info.batt_status =
			    POWER_SUPPLY_STATUS_CHARGING;
		else if (!batt_charging)
			msm_batt_info.batt_status =
			    POWER_SUPPLY_STATUS_DISCHARGING;

		if (chg_batt_event == CHG_UI_EVENT_DONE)
			msm_batt_info.batt_status = POWER_SUPPLY_STATUS_FULL;

#ifndef FUEL_GAUGE_I2C
		msm_batt_info.batt_capacity =
		    msm_batt_info.calculate_capacity(
				    msm_batt_info.battery_voltage);
#endif

	} else {
		msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_UNKNOWN;
		msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;
		msm_batt_info.batt_capacity = 0;
	}

	power_supply_changed(&msm_psy_batt);
}

static int msm_batt_register(u32 desired_batt_voltage,
			     u32 voltage_direction, u32 batt_cb_id, u32 cb_data)
{
	struct batt_client_registration_req req;
	struct batt_client_registration_rep rep;
	int rc;

	req.desired_batt_voltage = cpu_to_be32(desired_batt_voltage);
	req.voltage_direction = cpu_to_be32(voltage_direction);
	req.batt_cb_id = cpu_to_be32(batt_cb_id);
	req.cb_data = cpu_to_be32(cb_data);
	req.more_data = cpu_to_be32(1);
	req.batt_error = cpu_to_be32(0);

	rc = msm_rpc_call_reply(msm_batt_info.batt_ep,
				BATTERY_REGISTER_PROC, &req,
				sizeof(req), &rep, sizeof(rep),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		printk(KERN_ERR
		       "%s: msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, BATTERY_REGISTER_PROC, rc);
		return rc;
	} else {
		rc = be32_to_cpu(rep.batt_clnt_handle);
		printk(KERN_INFO "batt_clnt_handle = %d\n", rc);
		return rc;
	}
}

static int msm_batt_modify_client(u32 client_handle, u32 desired_batt_voltage,
	     u32 voltage_direction, u32 batt_cb_id, u32 cb_data)
{
	int rc;

	struct batt_modify_client_req {
		struct rpc_request_hdr hdr;

		u32 client_handle;

		/* The voltage at which callback (CB) should be called. */
		u32 desired_batt_voltage;

		/* The direction when the CB should be called. */
		u32 voltage_direction;

		/* The registered callback to be called when voltage and
		 * direction specs are met. */
		u32 batt_cb_id;

		/* The call back data */
		u32 cb_data;
	} req;

	req.client_handle = cpu_to_be32(client_handle);
	req.desired_batt_voltage = cpu_to_be32(desired_batt_voltage);
	req.voltage_direction = cpu_to_be32(voltage_direction);
	req.batt_cb_id = cpu_to_be32(batt_cb_id);
	req.cb_data = cpu_to_be32(cb_data);

	msm_rpc_setup_req(&req.hdr, BATTERY_RPC_PROG, BATTERY_RPC_VERS,
			 BATTERY_MODIFY_CLIENT_PROC);

	msm_batt_info.vbatt_rpc_req_xid = req.hdr.xid;

	rc = msm_rpc_write(msm_batt_info.batt_ep, &req, sizeof(req));

	if (rc < 0) {
		printk(KERN_ERR
		       "%s(): msm_rpc_write failed.  proc = 0x%08x rc = %d\n",
		       __func__, BATTERY_MODIFY_CLIENT_PROC, rc);
		return rc;
	}

	return 0;
}

static int msm_batt_deregister(u32 handle)
{
	int rc;
	struct batt_client_deregister_req {
		struct rpc_request_hdr req;
		s32 handle;
	} batt_deregister_rpc_req;

	struct batt_client_deregister_reply {
		struct rpc_reply_hdr hdr;
		u32 batt_error;
	} batt_deregister_rpc_reply;

	batt_deregister_rpc_req.handle = cpu_to_be32(handle);
	batt_deregister_rpc_reply.batt_error = cpu_to_be32(BATTERY_LAST_ERROR);

	rc = msm_rpc_call_reply(msm_batt_info.batt_ep,
				BATTERY_DEREGISTER_CLIENT_PROC,
				&batt_deregister_rpc_req,
				sizeof(batt_deregister_rpc_req),
				&batt_deregister_rpc_reply,
				sizeof(batt_deregister_rpc_reply),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		printk(KERN_ERR
		       "%s: msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, BATTERY_DEREGISTER_CLIENT_PROC, rc);
		return rc;
	}

	if (be32_to_cpu(batt_deregister_rpc_reply.batt_error) !=
			BATTERY_DEREGISTRATION_SUCCESSFUL) {

		printk(KERN_ERR "%s: vBatt deregistration Failed "
		       "  proce_num = %d,"
		       " batt_clnt_handle = %d\n",
		       __func__, BATTERY_DEREGISTER_CLIENT_PROC, handle);
		return -EIO;
	}
	return 0;
}

static int  msm_batt_handle_suspend(void)
{
	int rc;

	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {

		rc = msm_batt_modify_client(msm_batt_info.batt_handle,
				BATTERY_LOW, BATTERY_VOLTAGE_BELOW_THIS_LEVEL,
				BATTERY_CB_ID_LOW_VOL, BATTERY_LOW);

		if (rc < 0) {
			printk(KERN_ERR
			       "%s(): failed to modify client for registering"
			       " call back when  voltage goes below %u\n",
			       __func__, BATTERY_LOW);

			return rc;
		}
	}

	return 0;
}

static int  msm_batt_handle_resume(void)
{
	int rc;

	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {

		rc = msm_batt_modify_client(msm_batt_info.batt_handle,
				BATTERY_LOW, BATTERY_ALL_ACTIVITY,
			       BATTERY_CB_ID_ALL_ACTIV, BATTERY_ALL_ACTIVITY);
		if (rc < 0) {
			printk(KERN_ERR
			       "%s(): failed to modify client for registering"
			       " call back for ALL activity \n", __func__);
			return rc;
		}
	}
	return 0;
}


static int  msm_batt_handle_event(void)
{
	int rc;

	if (!atomic_read(&msm_batt_info.handle_event)) {

		printk(KERN_ERR "%s(): batt call back thread while in "
			"msm_rpc_read got signal. Signal is not from "
			"early suspend or  from late resume or from Clean up "
			"thread.\n", __func__);
		return 0;
	}

	printk(KERN_INFO "%s(): batt call back thread while in msm_rpc_read "
			"got signal\n", __func__);

	if (msm_batt_info.type_of_event & SUSPEND_EVENT) {

		printk(KERN_INFO "%s(): Handle Suspend event. event = %08x\n",
				__func__, msm_batt_info.type_of_event);

		rc = msm_batt_handle_suspend();

		return rc;

	} else if (msm_batt_info.type_of_event & RESUME_EVENT) {

		printk(KERN_INFO "%s(): Handle Resume event. event = %08x\n",
				__func__, msm_batt_info.type_of_event);

		rc = msm_batt_handle_resume();

		return rc;

	} else if (msm_batt_info.type_of_event & CLEANUP_EVENT) {

		printk(KERN_INFO "%s(): Cleanup event occured. event = %08x\n",
				__func__, msm_batt_info.type_of_event);

		return 0;

	} else  {

		printk(KERN_ERR "%s(): Unknown event occured. event = %08x\n",
				__func__, msm_batt_info.type_of_event);
		return 0;
	}
}


static void msm_batt_handle_vbatt_rpc_reply(struct rpc_reply_hdr *reply)
{

	struct rpc_reply_vbatt_modify_client {
		struct rpc_reply_hdr hdr;
		u32 modify_client_result;
	} *rep_vbatt_modify_client;

	u32 modify_client_result;

	if (msm_batt_info.type_of_event & SUSPEND_EVENT) {
		printk(KERN_INFO "%s(): Suspend event. Got RPC REPLY for vbatt"
			" modify client RPC req. \n", __func__);
	} else if (msm_batt_info.type_of_event & RESUME_EVENT) {
		printk(KERN_INFO "%s(): Resume event. Got RPC REPLY for vbatt"
			" modify client RPC req. \n", __func__);
	}

	/* If an earlier call timed out, we could get the (no longer wanted)
	 * reply for it. Ignore replies that  we don't expect.
	 */
	if (reply->xid != msm_batt_info.vbatt_rpc_req_xid) {

		printk(KERN_ERR "%s(): returned RPC REPLY XID is not"
				" equall to VBATT RPC REQ XID \n", __func__);
		kfree(reply);

		return;
	}
	if (reply->reply_stat != RPCMSG_REPLYSTAT_ACCEPTED) {

		printk(KERN_ERR "%s(): reply_stat != "
			" RPCMSG_REPLYSTAT_ACCEPTED \n", __func__);
		kfree(reply);

		return;
	}

	if (reply->data.acc_hdr.accept_stat != RPC_ACCEPTSTAT_SUCCESS) {

		printk(KERN_ERR "%s(): reply->data.acc_hdr.accept_stat "
				" != RPCMSG_REPLYSTAT_ACCEPTED \n", __func__);
		kfree(reply);

		return;
	}

	rep_vbatt_modify_client =
		(struct rpc_reply_vbatt_modify_client *) reply;

	modify_client_result =
		be32_to_cpu(rep_vbatt_modify_client->modify_client_result);

	if (modify_client_result != BATTERY_MODIFICATION_SUCCESSFUL) {

		printk(KERN_ERR "%s() :  modify client failed."
			"modify_client_result  = %u\n", __func__,
			modify_client_result);
	} else {
		printk(KERN_INFO "%s() : modify client successful.\n",
				__func__);
	}

	kfree(reply);
}

static void msm_batt_wake_up_waiting_thread(u32 event)
{
	msm_batt_info.type_of_event &= ~event;

	atomic_set(&msm_batt_info.handle_event, 0);
	atomic_set(&msm_batt_info.event_handled, 1);
	wake_up(&msm_batt_info.wait_q);
}


static void msm_batt_wait_for_batt_chg_event(struct work_struct *work)
{
	void *rpc_packet;
	struct rpc_request_hdr *req;
	int rpc_packet_type;
	struct rpc_reply_hdr rpc_reply;
	int len;
	unsigned long flags;
	int rc;

	spin_lock_irqsave(&msm_batt_info.lock, flags);
	msm_batt_info.cb_thread = current;
	spin_unlock_irqrestore(&msm_batt_info.lock, flags);

	printk(KERN_INFO "%s: Batt RPC call back thread started.\n", __func__);

	allow_signal(SIGCONT);

	printk(KERN_INFO "%s: First time Update Batt status without waiting for"
			" call back event from modem .\n", __func__);

	if (msm_batt_info.chg_api_version >= CHARGER_API_VERSION)
		msm_batt_update_psy_status_v1();
	else
		msm_batt_update_psy_status_v0();

	while (1) {

		rpc_packet = NULL;

		len = msm_rpc_read(msm_batt_info.batt_ep, &rpc_packet, -1, -1);

		if (len == -ERESTARTSYS) {

			flush_signals(current);

			rc = msm_batt_handle_event();

			if (msm_batt_info.type_of_event & CLEANUP_EVENT) {

				msm_batt_wake_up_waiting_thread(CLEANUP_EVENT);
				break;

			} else if (msm_batt_info.type_of_event &
					(SUSPEND_EVENT | RESUME_EVENT)) {

				if (rc < 0) {
					/*Could not sent VBATT modify rpc req */
					msm_batt_wake_up_waiting_thread(
						SUSPEND_EVENT | RESUME_EVENT);
				}
				/* Wait for RPC reply  for vbatt modify
				 * client RPC call. Then wake up suspend and
				 * resume threads.
				 */
				continue;
			}
		}

		printk(KERN_INFO "%s: Got some packet from modem Vbatt server\n"
				, __func__);

		if (len < 0) {
			printk(KERN_ERR "%s: msm_rpc_read failed while "
			       "waiting for call back packet. rc = %d\n",
			       __func__, len);
			continue;
		}

		if (len < RPC_REQ_REPLY_COMMON_HEADER_SIZE) {
			printk(KERN_ERR "%s: The pkt is neither req nor reply."
			       " len of pkt = %d\n", __func__, len);
			if (!rpc_packet)
				kfree(rpc_packet);
			continue;
		}

		req = (struct rpc_request_hdr *)rpc_packet;

		rpc_packet_type = be32_to_cpu(req->type);

		if (rpc_packet_type == RPC_TYPE_REPLY) {

			if (msm_batt_info.type_of_event &
				(SUSPEND_EVENT | RESUME_EVENT)) {

				msm_batt_handle_vbatt_rpc_reply(rpc_packet);

				msm_batt_wake_up_waiting_thread(
						SUSPEND_EVENT | RESUME_EVENT);

			} else {
				printk(KERN_ERR "%s: Should not get rpc reply"
					" Type_of_packet = %d\n", __func__,
					rpc_packet_type);
				kfree(rpc_packet);
			}
			continue;
		}
		if (rpc_packet_type != RPC_TYPE_REQ) {
			printk(KERN_ERR "%s: Type_of_packet is neither req or"
			       " reply. Type_of_packet = %d\n",
			       __func__, rpc_packet_type);
			kfree(rpc_packet);
			continue;
		}

		req->type = be32_to_cpu(req->type);
		req->xid = be32_to_cpu(req->xid);
		req->rpc_vers = be32_to_cpu(req->rpc_vers);

		if (req->rpc_vers != 2) {
			printk(KERN_ERR "%s: incorrect rpc version = %d\n",
			       __func__, req->rpc_vers);
			kfree(rpc_packet);
			continue;
		}

		req->prog = be32_to_cpu(req->prog);
		if (req->prog != BATTERY_RPC_CB_PROG) {
			printk(KERN_ERR "%s: Invalid Prog number for rpc call"
			       " back req. prog number = %d\n",
			       __func__, req->prog);
			kfree(rpc_packet);
			continue;
		}

		req->procedure = be32_to_cpu(req->procedure);

		if (req->procedure != BATTERY_CB_TYPE_PROC) {
			printk(KERN_ERR "%s: Invalid procedure num  rpc call"
			       " back req. req->procedure = %d\n",
			       __func__, req->procedure);
			kfree(rpc_packet);
			continue;
		}

		rpc_reply.xid = cpu_to_be32(req->xid);
		rpc_reply.type = cpu_to_be32(RPC_TYPE_REPLY);
		rpc_reply.reply_stat = cpu_to_be32(RPCMSG_REPLYSTAT_ACCEPTED);
		rpc_reply.data.acc_hdr.accept_stat =
		    cpu_to_be32(RPC_ACCEPTSTAT_SUCCESS);
		rpc_reply.data.acc_hdr.verf_flavor = 0;
		rpc_reply.data.acc_hdr.verf_length = 0;

		len = msm_rpc_write(msm_batt_info.batt_ep,
				    &rpc_reply, sizeof(rpc_reply));
		if (len < 0)
			printk(KERN_ERR "%s: could not send rpc reply for"
			       " call back from  batt server."
			       " reply  write response %d\n", __func__, len);

		kfree(rpc_packet);

		printk(KERN_INFO "%s: Update Batt status.\n", __func__);

		if (msm_batt_info.chg_api_version >= CHARGER_API_VERSION)
			msm_batt_update_psy_status_v1();
		else
			msm_batt_update_psy_status_v0();
	}

	printk(KERN_INFO "%s: Batt RPC call back thread stopped.\n", __func__);
}

static int msm_batt_send_event(u32 type_of_event)
{
	int rc;
	unsigned long flags;

	rc = 0;

	spin_lock_irqsave(&msm_batt_info.lock, flags);


	if (type_of_event & SUSPEND_EVENT)
		printk(KERN_INFO "%s() : Suspend event ocurred."
				"events = %08x\n", __func__, type_of_event);
	else if (type_of_event & RESUME_EVENT)
		printk(KERN_INFO "%s() : Resume event ocurred."
				"events = %08x\n", __func__, type_of_event);
	else if (type_of_event & CLEANUP_EVENT)
		printk(KERN_INFO "%s() : Cleanup event ocurred."
				"events = %08x\n", __func__, type_of_event);
	else {
		printk(KERN_ERR "%s() : Unknown event ocurred."
				"events = %08x\n", __func__, type_of_event);

		spin_unlock_irqrestore(&msm_batt_info.lock, flags);
		return -EIO;
	}

	msm_batt_info.type_of_event |=  type_of_event;

	if (msm_batt_info.cb_thread) {
		atomic_set(&msm_batt_info.handle_event, 1);
		send_sig(SIGCONT, msm_batt_info.cb_thread, 0);
		spin_unlock_irqrestore(&msm_batt_info.lock, flags);

		rc = wait_event_interruptible(msm_batt_info.wait_q,
			atomic_read(&msm_batt_info.event_handled));

		if (rc == -ERESTARTSYS) {

			printk(KERN_ERR "%s(): Suspend/Resume/cleanup thread "
				"got a signal while waiting for batt call back"
				" thread to finish\n", __func__);

		} else if (rc < 0) {

			printk(KERN_ERR "%s(): Suspend/Resume/cleanup thread "
				"wait returned error while waiting for batt "
				"call back thread to finish. rc = %d\n",
				__func__, rc);
		} else
			printk(KERN_INFO "%s(): Suspend/Resume/cleanup thread "
				"wait returned rc = %d\n", __func__, rc);

		atomic_set(&msm_batt_info.event_handled, 0);
	} else {
		printk(KERN_INFO "%s(): Battery call Back thread not Started.",
				__func__);

		atomic_set(&msm_batt_info.handle_event, 1);
		spin_unlock_irqrestore(&msm_batt_info.lock, flags);
	}

	return rc;
}

static void msm_batt_start_cb_thread(void)
{
	atomic_set(&msm_batt_info.handle_event, 0);
	atomic_set(&msm_batt_info.event_handled, 0);
#ifdef MAX8899_CHARGER
	return;
#else
	queue_work(msm_batt_info.msm_batt_wq, &msm_batt_cb_work);
#endif
}

static void msm_batt_early_suspend(struct early_suspend *h);

static int msm_batt_cleanup(void)
{
	int rc = 0;
	int rc_local;

	if (msm_batt_info.msm_batt_wq) {
		msm_batt_send_event(CLEANUP_EVENT);
		destroy_workqueue(msm_batt_info.msm_batt_wq);
	}

	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {

		rc = msm_batt_deregister(msm_batt_info.batt_handle);
		if (rc < 0)
			printk(KERN_ERR
			       "%s: msm_batt_deregister failed rc=%d\n",
			       __func__, rc);
	}
	msm_batt_info.batt_handle = INVALID_BATT_HANDLE;

	if (msm_batt_info.msm_psy_ac)
		power_supply_unregister(msm_batt_info.msm_psy_ac);

	if (msm_batt_info.msm_psy_usb)
		power_supply_unregister(msm_batt_info.msm_psy_usb);
	if (msm_batt_info.msm_psy_batt)
		power_supply_unregister(msm_batt_info.msm_psy_batt);

	if (msm_batt_info.batt_ep) {
		rc_local = msm_rpc_close(msm_batt_info.batt_ep);
		if (rc_local < 0) {
			printk(KERN_ERR
			       "%s: msm_rpc_close failed for batt_ep rc=%d\n",
			       __func__, rc_local);
			if (!rc)
				rc = rc_local;
		}
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	if (msm_batt_info.early_suspend.suspend == msm_batt_early_suspend)
		unregister_early_suspend(&msm_batt_info.early_suspend);
#endif
	return rc;
}

static u32 msm_batt_capacity(u32 current_voltage)
{
	u32 low_voltage = msm_batt_info.voltage_min_design;
	u32 high_voltage = msm_batt_info.voltage_max_design;

	return (current_voltage - low_voltage) * 100
	    / (high_voltage - low_voltage);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
void msm_batt_early_suspend(struct early_suspend *h)
{
	int rc;

	printk(KERN_INFO "%s(): going to early suspend\n", __func__);

	rc = msm_batt_send_event(SUSPEND_EVENT);

	printk(KERN_INFO "%s(): Handled early suspend event."
	       " rc = %d\n", __func__, rc);
}

void msm_batt_late_resume(struct early_suspend *h)
{
	int rc;

	printk(KERN_INFO "%s(): going to resume\n", __func__);

	rc = msm_batt_send_event(RESUME_EVENT);

	printk(KERN_INFO "%s(): Handled Late resume event."
	       " rc = %d\n", __func__, rc);
}
#endif


static int msm_batt_enable_filter(u32 vbatt_filter)
{
	int rc;
	struct rpc_req_vbatt_filter {
		struct rpc_request_hdr hdr;
		u32 batt_handle;
		u32 enable_filter;
		u32 vbatt_filter;
	} req_vbatt_filter;

	struct rpc_rep_vbatt_filter {
		struct rpc_reply_hdr hdr;
		u32 filter_enable_result;
	} rep_vbatt_filter;

	req_vbatt_filter.batt_handle = cpu_to_be32(msm_batt_info.batt_handle);
	req_vbatt_filter.enable_filter = cpu_to_be32(1);
	req_vbatt_filter.vbatt_filter = cpu_to_be32(vbatt_filter);

	rc = msm_rpc_call_reply(msm_batt_info.batt_ep,
				BATTERY_ENABLE_DISABLE_FILTER_PROC,
				&req_vbatt_filter, sizeof(req_vbatt_filter),
				&rep_vbatt_filter, sizeof(rep_vbatt_filter),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		printk(KERN_ERR
			"%s: msm_rpc_call_reply failed! proc = %d rc = %d\n",
			__func__, BATTERY_ENABLE_DISABLE_FILTER_PROC, rc);
		return rc;
	} else {
		rc =  be32_to_cpu(rep_vbatt_filter.filter_enable_result);

		if (rc != BATTERY_DEREGISTRATION_SUCCESSFUL) {
			printk(KERN_ERR "%s: vbatt Filter enable failed."
				" proc = %d  filter enable result = %d"
				" filter number = %d\n", __func__,
				BATTERY_ENABLE_DISABLE_FILTER_PROC, rc,
				vbatt_filter);
			return -EIO;
		}

		printk(KERN_INFO "%s: vbatt Filter enabled successfully.\n",
				__func__);
		return 0;
	}
}

static irqreturn_t fg_alert_irq_handler(int irq, void *dev_id)
{
	fg_alert = 1;
	return IRQ_HANDLED;
}

// hsil
int batt_init_done = 0;
EXPORT_SYMBOL(batt_init_done);

static int __devinit msm_batt_probe(struct platform_device *pdev)
{
	int ret;
	int rc;
	struct msm_psy_batt_pdata *pdata = pdev->dev.platform_data;
	printk(KERN_ERR "[Battery] %s \n", __func__);
	msm_batt_info.batt_slate_mode = 0;

	unsigned int fg_alret_irq;
	gpio_tlmm_config(GPIO_CFG(GPIO_FG_ALERT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	fg_alret_irq = gpio_to_irq(GPIO_FG_ALERT);
	if(fg_alret_irq < 0)
		printk(KERN_ERR "GPIO_FG_ALRET_DET irq error\n");

	ret = request_irq(fg_alret_irq,  fg_alert_irq_handler, IRQF_TRIGGER_FALLING, "fg_alert_detect", NULL);

	if (pdev->id != -1) {
		dev_err(&pdev->dev,
			"%s: MSM chipsets Can only support one"
			" battery ", __func__);
		return -EINVAL;
	}

	if (pdata->avail_chg_sources & AC_CHG) {
		rc = power_supply_register(&pdev->dev, &msm_psy_ac);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"%s: power_supply_register failed"
				" rc = %d\n", __func__, rc);
			msm_batt_cleanup();
			return rc;
		}
		msm_batt_info.msm_psy_ac = &msm_psy_ac;
		msm_batt_info.avail_chg_sources |= AC_CHG;
	}

	if (pdata->avail_chg_sources & USB_CHG) {
		rc = power_supply_register(&pdev->dev, &msm_psy_usb);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"%s: power_supply_register failed"
				" rc = %d\n", __func__, rc);
			msm_batt_cleanup();
			return rc;
		}
		msm_batt_info.msm_psy_usb = &msm_psy_usb;
		msm_batt_info.avail_chg_sources |= USB_CHG;
	}

	if (!msm_batt_info.msm_psy_ac && !msm_batt_info.msm_psy_usb) {

		dev_err(&pdev->dev,
			"%s: No external Power supply(AC or USB)"
			"is avilable\n", __func__);
		msm_batt_cleanup();
		return -ENODEV;
	}

	msm_batt_info.voltage_max_design = pdata->voltage_max_design;
	msm_batt_info.voltage_min_design = pdata->voltage_min_design;
	msm_batt_info.batt_technology = pdata->batt_technology;
	msm_batt_info.calculate_capacity = pdata->calculate_capacity;

	if (!msm_batt_info.voltage_min_design)
		msm_batt_info.voltage_min_design = BATTERY_LOW;
	if (!msm_batt_info.voltage_max_design)
		msm_batt_info.voltage_max_design = BATTERY_HIGH;

	if (msm_batt_info.batt_technology == POWER_SUPPLY_TECHNOLOGY_UNKNOWN)
		msm_batt_info.batt_technology = POWER_SUPPLY_TECHNOLOGY_LION;

	if (!msm_batt_info.calculate_capacity)
		msm_batt_info.calculate_capacity = msm_batt_capacity; // ignore 1st value of battery capacity

	rc = power_supply_register(&pdev->dev, &msm_psy_batt);
	if (rc < 0) {
		dev_err(&pdev->dev, "%s: power_supply_register failed"
			" rc=%d\n", __func__, rc);
		msm_batt_cleanup();
		return rc;
	}
	msm_batt_info.msm_psy_batt = &msm_psy_batt;

	rc = msm_batt_register(BATTERY_LOW, BATTERY_ALL_ACTIVITY,
			       BATTERY_CB_ID_ALL_ACTIV, BATTERY_ALL_ACTIVITY);
	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_register failed rc=%d\n", __func__, rc);
		msm_batt_cleanup();
		return rc;
	}
	msm_batt_info.batt_handle = rc;
	msm_batt_info.update_time = jiffies;

        /* create sec detail attributes */
        battery_create_attrs(msm_psy_batt.dev);

	rc =  msm_batt_enable_filter(VBATT_FILTER);

#ifdef CONFIG_HAS_EARLYSUSPEND
	msm_batt_info.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	msm_batt_info.early_suspend.suspend = msm_batt_early_suspend;
	msm_batt_info.early_suspend.resume = msm_batt_late_resume;
	register_early_suspend(&msm_batt_info.early_suspend);
#endif
	msm_batt_start_cb_thread();

#ifdef MAX8899_CHARGER
	msm_batt_info.batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
	msm_batt_info.batt_valid = 1;

	batt_registertimer( &msm_batt_info.timer, BATT_CHECK_INTERVAL );
#endif

#ifdef __TEMP_BLOCK_EXCEPTION__
setup_timer(&use_wimax_timer, use_wimax_timer_func, 0);
setup_timer(&use_data_call_timer, use_data_call_timer_func, 0);
setup_timer(&use_browser_timer, use_browser_timer_func, 0);
#endif

#ifdef FUEL_GAUGE_I2C
	fuel_gauge_init();
#endif
	//Charging Timer init
	hrtimer_init(&ChgTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ChgTimer.function = chg_timer_func;

	batt_init_done = 1; 	// hsil
	return 0;
}


int msm_batt_get_charger_api_version(void)
{
	int rc ;
	struct rpc_reply_hdr *reply;

	struct rpc_req_chg_api_ver {
		struct rpc_request_hdr hdr;
		u32 more_data;
	} req_chg_api_ver;

	struct rpc_rep_chg_api_ver {
		struct rpc_reply_hdr hdr;
		u32 num_of_chg_api_versions;
		u32 *chg_api_versions;
	};

	u32 num_of_versions;

	struct rpc_rep_chg_api_ver *rep_chg_api_ver;


	req_chg_api_ver.more_data = cpu_to_be32(1);

	msm_rpc_setup_req(&req_chg_api_ver.hdr, CHG_RPC_PROG, CHG_RPC_VERS,
			 ONCRPC_CHARGER_API_VERSIONS_PROC);

	rc = msm_rpc_write(msm_batt_info.chg_ep, &req_chg_api_ver,
			sizeof(req_chg_api_ver));
	if (rc < 0) {
		printk(KERN_ERR
		       "%s(): msm_rpc_write failed.  proc = 0x%08x rc = %d\n",
		       __func__, ONCRPC_CHARGER_API_VERSIONS_PROC, rc);
		return rc;
	}

	for (;;) {
		rc = msm_rpc_read(msm_batt_info.chg_ep, (void *) &reply, -1,
				BATT_RPC_TIMEOUT);
		if (rc < 0)
			return rc;
		if (rc < RPC_REQ_REPLY_COMMON_HEADER_SIZE) {
			printk(KERN_ERR "%s(): msm_rpc_read failed. read"
					" returned invalid packet which is"
					" neither rpc req nor rpc reply. "
					"legnth of packet = %d\n",
					__func__, rc);

			rc = -EIO;
			break;
		}
		/* we should not get RPC REQ or call packets -- ignore them */
		if (reply->type == RPC_TYPE_REQ) {

			printk(KERN_ERR "%s(): returned RPC REQ packets while"
				" waiting for RPC REPLY replay read \n",
				__func__);
			kfree(reply);
			continue;
		}

		/* If an earlier call timed out, we could get the (no
		 * longer wanted) reply for it.	 Ignore replies that
		 * we don't expect
		 */
		if (reply->xid != req_chg_api_ver.hdr.xid) {

			printk(KERN_ERR "%s(): returned RPC REPLY XID is not"
					" equall to RPC REQ XID \n", __func__);
			kfree(reply);
			continue;
		}
		if (reply->reply_stat != RPCMSG_REPLYSTAT_ACCEPTED) {
			rc = -EPERM;
			break;
		}
		if (reply->data.acc_hdr.accept_stat !=
				RPC_ACCEPTSTAT_SUCCESS) {
			rc = -EINVAL;
			break;
		}

		rep_chg_api_ver = (struct rpc_rep_chg_api_ver *)reply;

		num_of_versions =
			be32_to_cpu(rep_chg_api_ver->num_of_chg_api_versions);

		rep_chg_api_ver->chg_api_versions =  (u32 *)
			((u8 *) reply + sizeof(struct rpc_reply_hdr) +
			sizeof(rep_chg_api_ver->num_of_chg_api_versions));

		rc = be32_to_cpu(
			rep_chg_api_ver->chg_api_versions[num_of_versions - 1]);

		printk(KERN_INFO "%s(): num_of_chg_api_versions = %u"
			"  The chg api version = 0x%08x\n", __func__,
			num_of_versions, rc);
		break;
	}
	kfree(reply);
	return rc;
}


static struct platform_driver msm_batt_driver;
static int __devinit msm_batt_init_rpc(void)
{
	int rc;

	spin_lock_init(&msm_batt_info.lock);

	msm_batt_info.msm_batt_wq =
	    create_singlethread_workqueue("msm_battery");

	if (!msm_batt_info.msm_batt_wq) {
		printk(KERN_ERR "%s: create workque failed \n", __func__);
		return -ENOMEM;
	}

	msm_batt_info.batt_ep =
	    msm_rpc_connect_compatible(BATTERY_RPC_PROG, BATTERY_RPC_VERS, 0);

	if (msm_batt_info.batt_ep == NULL) {
		return -ENODEV;
	} else if (IS_ERR(msm_batt_info.batt_ep)) {
		int rc = PTR_ERR(msm_batt_info.batt_ep);
		printk(KERN_ERR
		       "%s: rpc connect failed for BATTERY_RPC_PROG."
		       " rc = %d\n ", __func__, rc);
		msm_batt_info.batt_ep = NULL;
		return rc;
	}

	rc = platform_driver_register(&msm_batt_driver);

	if (rc < 0) {
		printk(KERN_ERR "%s: platform_driver_register failed for "
			"batt driver. rc = %d\n", __func__, rc);
		return rc;
	}

	init_waitqueue_head(&msm_batt_info.wait_q);

	return 0;
}

static int __devexit msm_batt_remove(struct platform_device *pdev)
{
	int rc;
	rc = msm_batt_cleanup();

	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_cleanup  failed rc=%d\n", __func__, rc);
		return rc;
	}

#ifdef FUEL_GAUGE_I2C
	fuel_gauge_exit();
#endif

	return 0;
}

static struct platform_driver msm_batt_driver = {
	.probe = msm_batt_probe,
	.suspend = msm_batt_suspend,
	.resume = msm_batt_resume,
	.remove = __devexit_p(msm_batt_remove),
	.driver = {
		   .name = "msm-battery",
		   .owner = THIS_MODULE,
		   },
};

static int __init msm_batt_init(void)
{
	int rc;
//	int temp_vf;

	printk("[Battery] %s\n", __func__);

	wake_lock_init(&vbus_wake_lock, WAKE_LOCK_SUSPEND, "vbus_present");
	rc = msm_batt_init_rpc();

	if (rc < 0) {
		printk(KERN_ERR "%s: msm_batt_init_rpc Failed  rc=%d\n",
		       __func__, rc);
		msm_batt_cleanup();
		return rc;
	}

	//Check VF value in once.
//	temp_vf = get_batt_adc(SMEM_PROC_COMM_GET_ADC_VF);
//	msm_batt_info.battery_vf = (temp_vf>>16) & 0xFFFF;

	//check Charger connect status during init!!!!
	get_charger_type();
	get_charging_status();
	get_batt_info();
	check_charging_status(dwChargingState);
	msm_batt_update();

	return 0;
}

static void __exit msm_batt_exit(void)
{
	platform_driver_unregister(&msm_batt_driver);
}

#ifdef __TEMP_BLOCK_EXCEPTION__
static void use_wimax_timer_func(unsigned long unused)
{
	msm_batt_info.chg_temp_err_except_check = msm_batt_info.chg_temp_err_except_check & (~USE_WIMAX);
	printk("%s: OFF (0x%x) \n", __func__, msm_batt_info.chg_temp_err_except_check);
}

int chg_temp_check_use_wimax(int onoff)
{
	if (onoff)
	{
		del_timer_sync(&use_wimax_timer);
		msm_batt_info.chg_temp_err_except_check = msm_batt_info.chg_temp_err_except_check | USE_WIMAX;
		printk("%s: ON (0x%x) \n", __func__, msm_batt_info.chg_temp_err_except_check);
	}
	else
	{
		mod_timer(&use_wimax_timer, jiffies + msecs_to_jiffies(USE_MODULE_TIMEOUT));
	}

	return msm_batt_info.chg_temp_err_except_check;
}
EXPORT_SYMBOL(chg_temp_check_use_wimax);

static void use_browser_timer_func(unsigned long unused)
{
	msm_batt_info.chg_temp_err_except_check = msm_batt_info.chg_temp_err_except_check & (~USE_BROWSER);
	printk("%s: OFF (0x%x) \n", __func__, msm_batt_info.chg_temp_err_except_check);
}

static int chg_temp_check_use_browser(int onoff)
{
	if (onoff)
	{
		del_timer_sync(&use_browser_timer);
		msm_batt_info.chg_temp_err_except_check = msm_batt_info.chg_temp_err_except_check | USE_BROWSER;
		printk("%s: ON (0x%x) \n", __func__, msm_batt_info.chg_temp_err_except_check);
	}
	else
	{
		mod_timer(&use_browser_timer, jiffies + msecs_to_jiffies(USE_MODULE_TIMEOUT));
	}

	return msm_batt_info.chg_temp_err_except_check;
}

static int data_call_off_request = 0;

static void use_data_call_timer_func(unsigned long unused)
{
	msm_batt_info.comp_check.data_call = msm_batt_info.comp_check.data_call & (~USE_DATA_CALL);
	printk("%s: OFF (0x%x) \n", __func__, msm_batt_info.comp_check.data_call);
}

static int chg_temp_check_use_data_call(int onoff)
{
	if (onoff)
	{
		data_call_off_request = 0;
		del_timer_sync(&use_data_call_timer);
		msm_batt_info.comp_check.data_call = msm_batt_info.comp_check.data_call | USE_DATA_CALL;
		printk("%s: ON (0x%x) \n", __func__, msm_batt_info.comp_check.data_call);
	}
	else
	{
		if (data_call_off_request == 0)
		{
			data_call_off_request = 1;
			mod_timer(&use_data_call_timer, jiffies + msecs_to_jiffies(USE_MODULE_TIMEOUT));
			printk("%s: OFF waiting (0x%x) \n", __func__, msm_batt_info.comp_check.data_call);
		}
	}

	return msm_batt_info.comp_check.data_call;
}
#endif

module_init(msm_batt_init);
module_exit(msm_batt_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Kiran Kandi, Qualcomm Innovation Center, Inc.");
MODULE_DESCRIPTION("Battery driver for Qualcomm MSM chipsets.");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:msm_battery");
