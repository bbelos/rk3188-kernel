/*
*	file name:	a3907.c
*	author: 	ouyangyafeng
* 	date:	2012-6-20
*/
#include <linux/i2c.h>
#include <linux/log2.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/videodev2.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/circ_buf.h>
#include <linux/miscdevice.h>
#include <media/v4l2-common.h>
#include <media/v4l2-chip-ident.h>
#include <media/soc_camera.h>
//#include <mach/rk30_camera.h>
#include <plat/rk_camera.h>

#include "a3907.h"

static int debug;
module_param(debug, int, S_IRUGO|S_IWUSR);

#define dprintk(level, fmt, arg...) do {			\
	if (debug >= level) 					\
	printk(KERN_WARNING fmt , ## arg); } while (0)

#define MOTOR_TR(format, ...) printk(KERN_ERR format, ## __VA_ARGS__)
#define MOTOR_DG(format, ...) dprintk(1, format, ## __VA_ARGS__)

#define _CONS(a,b) a##b
#define CONS(a,b) _CONS(a,b)

#define __STR(x) #x
#define _STR(x) __STR(x)
#define STR(x) _STR(x)

/* Motor write register continues by preempt_disable/preempt_enable for current process not be scheduled */
#define CONFIG_MOTOR_I2C_NOSCHED   0
#define CONFIG_MOTOR_I2C_RDWRCHK   0

#define CONFIG_MOTOR_I2C_SPEED     200000       /* Hz */



/* motor driver Configuration */
#define MOTOR_NAME RK29_CAM_VCM_A3907
#define MOTOR_ID 0x3907

#define MOTOR_NAME_STRING(a) STR(CONS(MOTOR_NAME, a))
#define MOTOR_NAME_VARFUN(a) CONS(MOTOR_NAME, a)

//motor time mode
#define CONFIG_A3907_TIME_MODE  4
#define FIRST_STEP_DELAY_TIME (781) //delay time (ns)

struct motor_delaytime
{
	int first_delay_time;
	int second_delay_time;
};



struct motor
{
	struct device dev;
	struct i2c_client *client;

	struct motor_parameter* init_param;	
  	struct motor_parameter param;
	struct motor_setting setting;
	struct motor_delaytime delaytime;
    
	#if CONFIG_MOTOR_I2C_NOSCHED
	atomic_t tasklock_cnt;
	#endif

	int use_count;
	
	int (*command)(struct i2c_client *client, unsigned int cmd, void* arg);//sensor control vcm command
};


#if 0
static unsigned short normal_i2c[] = { 
	0x18 >> 1,  
	0x1a >> 1,
	0x1c >> 1,
	0x1d >> 1,        
	I2C_CLIENT_END  
};  
#endif
//I2C_CLIENT_INSMOD;

static int delay_time_table[] = {//	XXX (ns)
	0,
	6250,
	12500,
	25000,
	50000,
	100000,
	200000,
	0
};
#if 0
static int time_mode_table[] = {0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
};
#endif

static struct motor_parameter init_param =
{
	.mode = CONFIG_A3907_TIME_MODE, //time mode
	.present_code = CURRENT_INIT_VAL, //current code init value;
	.current_step =BIG_CURRENT_STEP,//current step init value;
};

static struct motor* to_motor(const struct i2c_client *i2c_client)
{
 	return container_of(i2c_get_clientdata(i2c_client), struct motor, dev);
}

static int motor_set_param_currentStep(struct motor *motor, int current_step)
{
	if(!motor)
		return -1;
	
	if(current_step<0 || current_step>MAX_CURRENT){
		MOTOR_TR("%s..%s..%d wrong current_step=%d \n", MOTOR_NAME_STRING(), __FUNCTION__, __LINE__, current_step);
		return -1;
	}
	MOTOR_DG("%s..%s..%d current_step=%d \n", MOTOR_NAME_STRING(), __FUNCTION__, __LINE__, current_step);
	motor->param.current_step = current_step;
	return 0;
	
}

static int motor_set_param_timemode(struct motor *motor, int mode)
{
	if(!motor)
		return -1;
		
	if(mode<0 || mode>15){
		return -1;
	}else{
		if(mode>=0 && mode <=7){
			motor->param.mode = mode;
			motor->delaytime.first_delay_time = delay_time_table[mode];
			motor->delaytime.second_delay_time = 0;
			motor->setting.lsb &= 0xf0;
			motor->setting.lsb |= mode;
		}else{
			motor->param.mode = mode;
			if(mode ==7 || mode == 15){
				motor->delaytime.first_delay_time = 0;
			}else{
				motor->delaytime.first_delay_time = FIRST_STEP_DELAY_TIME;
			}
			motor->delaytime.second_delay_time = delay_time_table[mode - 7];
			motor->setting.lsb &= 0xf0;
			motor->setting.lsb |= mode;			
		}
	}
	
	MOTOR_TR("\n%s..%s..msb=%u, lsb=%u, first_delaytime=%d, sencond_delaytime=%d\n",__FUNCTION__,MOTOR_NAME_STRING(),motor->setting.msb, motor->setting.lsb, motor->delaytime.first_delay_time, motor->delaytime.second_delay_time);
	return 0;
}

static int motor_set_param_current_code(struct motor *motor, int next_code)
{
	int low = 0;
	int high = 0;

	if(next_code > MAX_CURRENT)
		next_code = MAX_CURRENT;
	
	if(next_code < MIN_CURRENT)
		next_code = MIN_CURRENT;
	
	low = next_code & 0x0f;
	high = (next_code & 0x3f0)>> 4;
	
	motor->setting.lsb &= 0x0f;
	motor->setting.lsb |= low << 4;
	motor->setting.msb &= 0xc0;
	motor->setting.msb |= high;
	motor->param.present_code = next_code;

	MOTOR_DG("\n%s..%s..msb=%u, lsb=%u, present_code=%d \n",__FUNCTION__,MOTOR_NAME_STRING(), motor->setting.msb, motor->setting.lsb, motor->param.present_code);
	return 0;	
}

static int motor_set_param_sleep(struct motor *motor)
{
	motor->setting.msb |= (1<<7);	
	MOTOR_TR("\n%s..%s..msb=%u \n",__FUNCTION__,MOTOR_NAME_STRING(), motor->setting.msb);
	return 0;

}

static int motor_set_param_work(struct motor *motor)
{
	motor->setting.msb &= ~(1<<7);
	MOTOR_TR("\n%s..%s...msb=%u \n",__FUNCTION__,MOTOR_NAME_STRING(),motor->setting.msb);
	return 0;
}


#if 0
static int motor_task_lock(struct motor *motor, int lock)
{
#if CONFIG_MOTOR_I2C_NOSCHED
	int cnt = 3;

	if (lock) {
		if (atomic_3read(&motor->tasklock_cnt) == 0) {
			while ((atomic_read(&motor->client->adapter->bus_lock.count) < 1) && (cnt>0)) {
				MOTOR_TR("\n %s will obtain i2c in atomic, but i2c bus is locked! Wait...\n",MOTOR_NAME_STRING());
				msleep(35);
				cnt--;
			}
			if ((atomic_read(&motor->client->adapter->bus_lock.count) < 1) && (cnt<=0)) {
				MOTOR_TR("\n %s obtain i2c fail in atomic!!\n",MOTOR_NAME_STRING());
				goto motor_task_lock_err;
			}
			preempt_disable();
		}

		atomic_add(1, &motor->tasklock_cnt);
	} else {
		if (atomic_read(&motor->tasklock_cnt) > 0) {
			atomic_sub(1, &motor->tasklock_cnt);

			if (atomic_read(&motor->tasklock_cnt) == 0)
				preempt_enable();
		}
	}
	return 0;
motor_task_lock_err:
	return -1;   
#else
    return 0;
#endif

}
#endif


static int motor_i2c_write(struct i2c_client *client, unsigned char msb, unsigned char lsb)
{
    int err,cnt;
    unsigned char buf[2];
    struct i2c_msg msg[1];

    buf[0] = msb;
    buf[1] = lsb;

    msg->addr = client->addr;
    msg->flags = client->flags;
    msg->buf = buf;
    msg->len = sizeof(buf);
    msg->scl_rate = CONFIG_MOTOR_I2C_SPEED;         /* ddl@rock-chips.com : 100kHz */
    msg->read_type = 0;               /* fpga i2c:0==I2C_NORMAL : direct use number not enum for don't want include spi_fpga.h */

    cnt = 3;
    err = -EAGAIN;

    while ((cnt-- > 0) && (err < 0))  /* ddl@rock-chips.com :  Transfer again if transent is failed   */
    {                      
        err = i2c_transfer(client->adapter, msg, 1);

        if (err >= 0) {
	        MOTOR_DG("\n %s write a3907(msb:0x%x, lsb:0x%x) sucesess\n", MOTOR_NAME_STRING(), msb, lsb);
            err = 0;
            break;
        } else {
            MOTOR_TR("\n %s write a3907(msb:0x%x, lsb:0x%x) failed, try to write again!\n", MOTOR_NAME_STRING(), msb, lsb);
            udelay(10);
        }
    }

    return err;
}

static int motor_i2c_read(struct i2c_client *client, unsigned char *msb, unsigned char *lsb)
{
    int err,cnt;
    u8 buf[2];
    struct i2c_msg msg[1];

    msg[0].addr = client->addr;
    msg[0].flags = client->flags|I2C_M_RD;
    msg[0].buf = buf;
    msg[0].len = 2;
    msg[0].scl_rate = CONFIG_MOTOR_I2C_SPEED;                       /* ddl@rock-chips.com : 100kHz */
    msg[0].read_type = 2;                             /* fpga i2c:0==I2C_NO_STOP : direct use number not enum for don't want include spi_fpga.h */

    cnt = 3;
    err = -EAGAIN;
    
    while ((cnt-- > 0) && (err < 0)) 
    {                       
        err = i2c_transfer(client->adapter, msg, 1);

        if (err >= 0) {
            *msb = buf[0];
            *lsb = buf[1];
	    	MOTOR_DG("\n %s..%s..%d read a3907(msb:0x%x lsb:0x%x) sucesess! \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__, *msb, *lsb);
          	err = 0;
		   	break;
        } else {
            MOTOR_TR("\n %s..%s..%d read a3907(msb:0x%x lsb:0x%x) failed, try to read again! \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__, *msb, *lsb);
            udelay(10);
        }

    }

    return err;
}

static int vcm_init(struct motor *motor)
{
	int ret = 0;
	
	motor_set_param_timemode(motor, motor->init_param->mode);	
	motor_set_param_current_code(motor, motor->init_param->present_code);
	motor_set_param_currentStep(motor, motor->init_param->current_step);
	motor_set_param_work(motor);

	ret = motor_i2c_write(motor->client,motor->setting.msb,motor->setting.lsb);
	if(!ret){
        MOTOR_DG("\n%s..%s..%d  init success ret = %d\n",__FUNCTION__,__FILE__,__LINE__,ret);	
	    return 0;
	}else{
	    MOTOR_TR("\n%s..%s..%d  init failed ret = %d\n",__FUNCTION__,__FILE__,__LINE__,ret);
        return -1;
	}
}

static int vcm_get_motor_param(struct motor *motor, struct motor_parameter* param)
{
	int ret;
	unsigned char msb=0, lsb=0;
	int tmp1 = 0;
	int tmp2 = 0;
	
	ret = motor_i2c_read(motor->client, &msb, &lsb);
	if(ret == 0)
	{
		param->current_step = motor->param.current_step;
		param->mode = lsb&0x0f;
		tmp1 = (lsb&0xf0)>>4;
		tmp2 = (msb&0x3f)<<4;
		//param->present_code = tmp1 | tmp2;
		param->present_code = motor->param.present_code;

		MOTOR_DG("\n %s..%s..%d motor_read sucesess tmp=%d, current_code=%d lsb=%d msb=%d\n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__, (tmp1|tmp2), param->present_code, lsb, msb);
		
		return 0;
	}
	
	MOTOR_TR("\n %s..%s..%d  motor_read fail \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);
	return -1;
}

static int vcm_set_motor_param(struct motor *motor, struct motor_parameter* param)
{
	int ret = 0;
	int old_time_mode = motor->param.mode;
	int old_current_code = motor->param.present_code;
	int old_current_setp = motor->param.current_step;
	
	motor_set_param_timemode(motor, param->mode);
	motor_set_param_current_code(motor, param->present_code);
	motor_set_param_currentStep(motor, param->current_step);
	
	ret = motor_i2c_write(motor->client, motor->setting.msb, motor->setting.lsb);
	if(!ret){
		MOTOR_DG("\n %s..%s..%d motor_write sucesess \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);
		return 0;
	}else{
	    MOTOR_TR("\n %s..%s..%d motor_write fail \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);
        motor_set_param_timemode(motor, old_time_mode);
	    motor_set_param_current_code(motor, old_current_code);
	    motor_set_param_currentStep(motor, old_current_setp);
        return -1;
    }
}

static int vcm_lens_near(struct motor *motor)
{
	int res = 0;
	int present_code = motor->param.present_code ;
	int next_code = present_code - motor->param.current_step;
	
	if(next_code > MAX_CURRENT)
		next_code = MAX_CURRENT;
	
	if(next_code < MIN_CURRENT)
		next_code = MIN_CURRENT;
		
	MOTOR_DG("%s..%s..%d, next_code(%d)\n", MOTOR_NAME_STRING(), __FUNCTION__, __LINE__, next_code);
	
	motor_set_param_current_code(motor, next_code);
	
	res = motor_i2c_write(motor->client, motor->setting.msb, motor->setting.lsb);
	if(!res){
		MOTOR_DG("\n %s vcm_lens_near motor_write sucesess \n",MOTOR_NAME_STRING());
		return 0;
	}else{
	    motor_set_param_current_code(motor, present_code);
	    MOTOR_TR("\n %s vcm_lens_near motor_write fail \n",MOTOR_NAME_STRING());
	    return -1;
	}
}

static int vcm_lens_far(struct motor *motor)
{
	int res = 0;
	int present_code = motor->param.present_code;
	int next_code = present_code + motor->param.current_step;
	
	if(next_code > MAX_CURRENT)
		next_code = MAX_CURRENT;
	
	if(next_code < MIN_CURRENT)
		next_code = MIN_CURRENT;
	
	motor_set_param_current_code(motor, next_code);

	res = motor_i2c_write(motor->client, motor->setting.msb, motor->setting.lsb);
	if(!res){
		MOTOR_DG("\n %s vcm_lens_far motor_write sucesess \n",MOTOR_NAME_STRING());
		return 0;
	}else{
	    motor_set_param_current_code(motor, present_code);
	    MOTOR_TR("\n %s vcm_lens_fat motor_write fail \n",MOTOR_NAME_STRING());
	    return -1;
	}

}

static int vcm_lens_furthest(struct motor *motor)
{
	int res = 0;
	int present_code = motor->param.present_code;
	
	motor_set_param_current_code(motor, MAX_CURRENT);
	
	res = motor_i2c_write(motor->client, motor->setting.msb, motor->setting.lsb);
	if(!res){
		MOTOR_DG("\n %s..%s..%d  motor_write sucesess \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);
		return 0;
	}else{
	    motor_set_param_current_code(motor, present_code);
	    MOTOR_TR("\n %s..%s..%d  motor_write fail \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);
	    return -1;
	}
}

static int vcm_lens_nearest(struct motor *motor)
{
	int res = 0;
	int present_code = motor->param.present_code;
	
	motor_set_param_current_code(motor, MIN_CURRENT);
	
	res = motor_i2c_write(motor->client, motor->setting.msb, motor->setting.lsb);
	if(!res){
		MOTOR_DG("\n %s..%s..%d motor_write sucesess \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);
		return 0;
	}else{
	    motor_set_param_current_code(motor, present_code);
	    MOTOR_TR("\n %s..%s..%d motor_write fail \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);
        return -1;
	}
}

static int vcm_set_currentStep(struct motor *motor, int currentStep)
{
	int ret = 0;
	
	ret = motor_set_param_currentStep(motor, currentStep);

	return ret;
}

static int vcm_set_timeMode(struct motor *motor, int time_mode)
{
	int ret = 0;
	int old_timemode = motor->param.mode;
	
	motor_set_param_timemode(motor, time_mode);
	
	ret = motor_i2c_write(motor->client, motor->setting.msb, motor->setting.lsb);
	if(!ret){
		MOTOR_DG("\n %s..%s..%d motor_write sucesess \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);
		return 0;
	}else{
	    motor_set_param_timemode(motor, old_timemode);
	    MOTOR_TR("\n %s..%s..%d motor_write fail \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);  
	    return -1;
	}
}

static int vcm_set_current_code(struct motor *motor, int i_current)
{
	int ret = 0;
	int old_current = motor->param.present_code;

	if(i_current > MAX_CURRENT)
		i_current = MAX_CURRENT;
	
	if(i_current < MIN_CURRENT)
		i_current = MIN_CURRENT;

	ret = motor_set_param_current_code(motor, i_current);
	
	ret = motor_i2c_write(motor->client, motor->setting.msb, motor->setting.lsb);
	if(!ret){
		MOTOR_DG("\n %s..%s..%d motor_write sucesess code(%d) \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__,i_current);
		return 0;
	}else{
	    ret = motor_set_param_current_code(motor, old_current);
	    MOTOR_TR("\n %s..%s..%d motor_write fail code(%d)\n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__,i_current);
	    return -1;
	}
}




static int vcm_set_sleep(struct motor *motor)
{
	int ret = 0;
	
	motor_set_param_sleep(motor);
	
	ret = motor_i2c_write(motor->client, motor->setting.msb, motor->setting.lsb);
	if(!ret){
		MOTOR_DG("\n %s..%s..%d motor_write sleep sucesess \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);
		return 0;
	}else{
	    motor_set_param_work(motor);
	    MOTOR_TR("\n %s..%s..%d  motor_write  sleep fail \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);
	    return -1;
	}
}

static int vcm_set_work(struct motor *motor)
{
	int ret = 0;

	motor_set_param_work(motor);
	
	ret = motor_i2c_write(motor->client, motor->setting.msb, motor->setting.lsb);
	if(!ret){
		MOTOR_DG("\n %s..%s..%d motor_write sucesess \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);
		return 0;
	}else{
	    motor_set_param_sleep(motor);
	    MOTOR_TR("\n %s..%s..%d  motor_write fail \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);
	    return -1;
	}
}



static int vcm_get_current_code(struct motor *motor, int *present_code)
{
    int ret = 0;
    unsigned char msb, lsb;
    int current_code;
    
    ret = motor_i2c_read(motor->client, &msb, &lsb);
    if(!ret){
        current_code =  ((((msb>>2)|0x7f)<<4) | ((lsb>>4)&0x0f));
        *present_code = current_code;
        MOTOR_DG("\n %s..%s..%d motor_read sucesess \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);
        return 0;
    }else{
        MOTOR_DG("\n %s..%s..%d motor_read fail \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);
        return -1;
    }
		
}

static int vcm_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	int res = 0;
	struct motor *motor = to_motor(client);
	struct motor_parameter *param = NULL;
	int temp=0;
	int *ptr_current_code;

	if(!motor){
		MOTOR_TR("\n %s..%s..%d a3907 motor is NULL pointor  cmd=0x%x \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__, cmd);
		return -1;
	}
	
	switch(cmd)
	{
		case StepFocus_Near_Tag:
			res = vcm_lens_near(motor);
			break;
		case StepFocus_Far_Tag:
			res = vcm_lens_far(motor);
			break;
		case StepFocus_Furthest_Tag:
			res = vcm_lens_furthest(motor);
			break;
		case StepFocus_Nearest_Tag:
			res = vcm_lens_nearest(motor);
			break;
		case StepFocus_Init_Tag:
			res = vcm_init(motor);
			break;
		case StepFocus_Sleep_Tag:
			res = vcm_set_sleep(motor);
			break;
		case StepFocus_Work_Tag:
			res = vcm_set_work(motor);
			break;
		case StepFocus_Write_Tag:
			param = (struct motor_parameter*)arg;
			res = vcm_set_motor_param(motor, param);
			break;
		case StepFocus_Read_Tag:
			param = (struct motor_parameter*)arg;
			res = vcm_get_motor_param(motor, param);
			break;
		case StepFocus_Get_Current_Tag:
			ptr_current_code = (int*)arg;
			res = vcm_get_current_code(motor, ptr_current_code);
			break;
		case StepFocus_Set_Current_Tag:
			temp = *((int*)arg);
			res = vcm_set_current_code(motor, temp);
			break;
		case StepFocus_Set_CurrentStep_Tag:
			temp = *((int*)arg);
			res = vcm_set_currentStep(motor, temp);
			break;
		case StepFocus_Set_TimeMode_Tag:
			temp = *((int*)arg);
			res = vcm_set_timeMode(motor, temp);	
			break;
		default:
			MOTOR_TR("\n %s..%s..%d a3907 unknow cmd=%d \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__, cmd);
			break;
	}
	
	return res;
		
}

static int vcm_probe(struct i2c_client *client, const struct i2c_device_id *device_id)
{
	int ret = 0;
	struct motor *motor;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
        dev_warn(&adapter->dev,"I2C-Adapter doesn't support I2C_FUNC_I2C\n");
        return -EIO;
  	}
    
  	motor = kzalloc(sizeof(struct motor), GFP_KERNEL);
 	if (!motor)
      	return -ENOMEM;
  
  	motor->dev = client->dev;
  	motor->client = client;
  	motor->init_param = &init_param; 
	motor->command = vcm_command;
	i2c_set_clientdata(client, &motor->dev);

	#if 1
    ret = vcm_init(motor);
  	if(ret){
  		MOTOR_TR("\n %s..%s..%d motor_init_for_work failed! \n",MOTOR_NAME_STRING(), __FUNCTION__, __LINE__);
  		return -1;
  	}
  	#endif

	MOTOR_DG("\n%s..%s..%d  ret = %x adapter=%d\n", MOTOR_NAME_STRING(), __FUNCTION__, __LINE__, ret, adapter->nr);
  	return 0;
}

static int vcm_remove(struct i2c_client *client)
{
  	struct motor *motor = to_motor(client);
	 
   	i2c_set_clientdata(client, NULL);
   	client->driver = NULL;
   	kfree(motor);
	motor = NULL;
    
   	return 0;	 
}

static const struct i2c_device_id vcm_motor_id[] = {
	{MOTOR_NAME_STRING(), 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, vcm_motor_id);

static struct i2c_driver vcm_i2c_driver = {
	.driver = {
		.name = MOTOR_NAME_STRING(),
	},
	.probe		= vcm_probe,
	.remove		= vcm_remove,
	.command 	= vcm_command,
	.id_table	= vcm_motor_id,
};

static int __init motor_mod_init(void)
{	
	int ret = 0;

   	ret = i2c_add_driver(&vcm_i2c_driver);
	MOTOR_DG("\n%s..%s..%d i2c_add_driver ret=%d \n", MOTOR_NAME_STRING(),__FUNCTION__,  __LINE__, ret);
	
	return ret;
}

static void __exit motor_mod_exit(void)
{    	
	i2c_del_driver(&vcm_i2c_driver);
	MOTOR_DG("\n%s..%s..%d del driver and chrdev \n",MOTOR_NAME_STRING(),__FUNCTION__,  __LINE__);
}

module_init(motor_mod_init);
module_exit(motor_mod_exit);

MODULE_DESCRIPTION(MOTOR_NAME_STRING(Camera VCM driver));
MODULE_AUTHOR("oyyf <kernel@rock-chips>");
MODULE_LICENSE("GPL");
