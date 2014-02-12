/*
 * tchip_devices.c
 *
 *  Created on: 2012-2-17
 *      Author: zhansb
 */

struct tchip_device {
	char		name[20];
	unsigned short	active;
};

#define TCSI_GET_GROUP_INDEX(pos, group) ((pos) - TCSI_##group##_PRESTART - 1)

#define TCSI_GET_CODEC_INDEX(pos) (TCSI_GET_GROUP_INDEX(pos, CODEC))
#define TCSI_GET_WIFI_INDEX(pos) (TCSI_GET_GROUP_INDEX(pos, WIFI))
#define TCSI_GET_HDMI_INDEX(pos) (TCSI_GET_GROUP_INDEX(pos, HDMI))
#define TCSI_GET_MODEM_INDEX(pos) (TCSI_GET_GROUP_INDEX(pos, MODEM))

#define GET_CUR_DEVICE(list)	get_cur_device(list, (sizeof(list)/sizeof(list[0])))
#define GET_DEVICE_LIST(Info, list)	get_device_list(Info, list, (sizeof(list)/sizeof(list[0])))

/*
 * 	board support list
 */
static const struct tchip_device tchip_boards[] =
{
#if defined(CONFIG_TCHIP_MACH_DEFAULT)
	{.name = "TCHIP",.active = 1},
#elif defined(CONFIG_TCHIP_MACH_TR976Q)
    {.name = "TR976Q",.active = 1},
#elif defined(CONFIG_TCHIP_MACH_TR976D)
    {.name = "TR976D",.active = 1},
#elif defined(CONFIG_TCHIP_MACH_TD7BP11)
    {.name = "TD7BP11",.active = 1},
#elif defined(CONFIG_TCHIP_MACH_TR830D)
    {.name = "TR830D",.active = 1},
#elif defined(CONFIG_TCHIP_MACH_TR736D)
#if defined(CONFIG_TCHIP_MACH_TR736D_V00)
   #if defined(CONFIG_TCHIP_MACH_TR736_HD)
    {.name = "TR736DV00_HD",.active = 1},
   #else
    {.name = "TR736DV00",.active = 1},
   #endif
#elif defined(CONFIG_TCHIP_MACH_TR736D_V01)
   #if defined(CONFIG_TCHIP_MACH_TR736_HD)
    {.name = "TR736DV01_HD",.active = 1},
   #else
    {.name = "TR736DV01",.active = 1},
   #endif
#endif
#elif defined(CONFIG_TCHIP_MACH_TR737)
 #if defined(CONFIG_TCHIP_MACH_TR737_V00)
   #if defined(CONFIG_TCHIP_MACH_TR737_HD)
    {.name = "TR737V00_HD",.active = 1},
   #else
    {.name = "TR737V00",.active = 1},
   #endif
 #elif defined(CONFIG_TCHIP_MACH_TR737_V02)
   #if defined(CONFIG_TCHIP_MACH_TR737_HD)
    {.name = "TR737V02_HD",.active = 1},
   #else
    {.name = "TR737V02",.active = 1},
   #endif 
 #elif defined(CONFIG_TCHIP_MACH_TR737_V10)
   #if defined(CONFIG_TCHIP_MACH_TR737_HD)
    {.name = "TR737V10_HD",.active = 1},
   #else
    {.name = "TR737V10",.active = 1},
   #endif
 #endif
#elif defined(CONFIG_TCHIP_MACH_TR1020)
    #if defined(CONFIG_TCHIP_MACH_TR920D)
      {.name = "TR920D",.active = 1},
    #elif defined(CONFIG_TCHIP_MACH_TR920HD)
      {.name = "TR920HD",.active = 1},
    #elif defined(CONFIG_TCHIP_MACH_TR101D)
      {.name = "TR101D",.active = 1},
    #else
      {.name = "TR1020",.active = 1},
    #endif
#elif defined(CONFIG_TCHIP_MACH_MD7009)
    {.name = "MD7009",.active = 1},
#elif defined(CONFIG_TCHIP_MACH_TR785_V20)
        { .name = "TR785V21", .active = 1 },
#elif defined(CONFIG_TCHIP_MACH_TR785)
        { .name = "TR785", .active = 1 },
#elif defined(CONFIG_TCHIP_MACH_TR1088)
        { .name = "TR1088", .active = 1 },
#elif defined(CONFIG_TCHIP_MACH_TR7088TN)
        { .name = "TR7088TN", .active = 1 },
#elif defined(CONFIG_TCHIP_MACH_TR7088)
        { .name = "TR7088", .active = 1 },
#elif defined(CONFIG_TCHIP_MACH_TR7078)
        { .name = "TR7078", .active = 1 },
#elif defined(CONFIG_TCHIP_MACH_TR7028)
	{ .name = "TR7028", .active = 1 },
#elif defined(CONFIG_TCHIP_MACH_TRQ7_LJ)
	{ .name = "TRQ7_LJ", .active = 1 },
#elif defined(CONFIG_TCHIP_MACH_TRQ7_AR)
	{ .name = "TRQ7_AR", .active = 1 },
#endif
};

/*
 * 	touch support list
 */
static const struct tchip_device tchip_touchs[] =
{
#ifdef	CONFIG_GOODIX_CAPACITIVE_SIGNAL_IC
	{.name = "GT801",.active = 1},
#elif defined(CONFIG_TOUCHSCREEN_CT36X)
	{.name = "CT365",.active = 1},
#endif
#ifdef  CONFIG_TOUCHSCREEN_GSLX680
	{.name = "GSLX680",.active = 1},
#endif
#ifdef  CONFIG_TOUCHSCREEN_GSLX680_NOID
	{.name = "GSL1680e",.active = 1},
#endif
#ifdef  CONFIG_TOUCHSCREEN_ICN83XX
	{.name = "ICN83XX",.active = 1},
#endif
};

/*
 *	pmu support list
 */
 
static const struct tchip_device tchip_pmus[] =
{
#ifdef	CONFIG_REGULATOR_ACT8846
	{.name = "ACT8846",.active = 1},
#endif
};

/*
 *  gsensor support list
 */
static const struct tchip_device tchip_misc[] =
{
#if defined(CONFIG_SENSORS_STK8312) && defined(CONFIG_GS_MC3230) && defined(CONFIG_GS_MMA7660)
    {.name = "GS2",.active = 1},
#endif
#if defined(CONFIG_SENSORS_STK8312)
    {.name = "STK8312",.active = 1},
#endif
#if defined(CONFIG_GS_MMA8452)
    {.name = "MMA8452",.active = 1},
#endif
};

/*
 * 	encrypt support list
 */
static const struct tchip_device tchip_encrypts[] =
{

#if defined (CONFIG_ENCRYPTION_DEVICE) || defined (CONFIG_ENCRYPTION_DEVICE_MODULE)
    #if (CONFIG_ATXX_DEVICE_TYPE==18)	
	{.name = "AT18",.active = 1},
    #elif (CONFIG_ATXX_DEVICE_TYPE==28)
	{.name = "AT28",.active = 1},
    #elif (CONFIG_ATXX_DEVICE_TYPE==38)
	{.name = "AT38",.active = 1},
    #endif
#endif
};

/*
 * 	tp support list
 */
static const struct tchip_device tchip_tps[] =
{
#ifdef	CONFIG_TCHIP_TP_01
	{.name = "TP01",.active = 1},
#elif defined(CONFIG_TCHIP_TP_02)
	{.name = "TP02",.active = 1},
#endif
};

/*
 * 	modem support list
 */
static const struct tchip_device tchip_modems[] =
{
#ifdef	CONFIG_MODEM_ROCKCHIP_DEMO
	[TCSI_GET_MODEM_INDEX(TCSI_MODEM_OTHERS)] =
	{
		.name = "MODEM",
#ifdef	CONFIG_TCHIP_MACH_MODEM_OTHERS
		.active = 1,
#endif
	},
#endif
#ifdef	CONFIG_TDM330
	[TCSI_GET_MODEM_INDEX(TCSI_MODEM_TDM330)] =
	{
		.name = "TDM330",
#ifdef	CONFIG_TCHIP_MACH_MODEM_TDM330
		.active = 1,
#endif
	},
#endif
};

/*
 * 	hdmi support list
 */
static const struct tchip_device tchip_hdmis[] =
{
#if	defined(CONFIG_HDMI_RK610)
	[TCSI_GET_HDMI_INDEX(TCSI_HDMI_RK610)] =
	{
		.name = "RK610HDMI",
#if defined(CONFIG_TCHIP_MACH_HDMI_RK610)
		.active = 1,
#endif
	},
#endif
#if	defined(CONFIG_HDMI_RK616)
	[TCSI_GET_HDMI_INDEX(TCSI_HDMI_RK616)] =
	{
		.name = "RK616HDMI",
#if defined(CONFIG_TCHIP_MACH_HDMI_RK616)
		.active = 0,
#endif
	},
#endif
#if	defined(CONFIG_ANX7150) || defined(CONFIG_ANX7150_NEW)
	[TCSI_GET_HDMI_INDEX(TCSI_HDMI_ANX7150)] =
	{
		.name = "ANX7150",
#ifdef	CONFIG_TCHIP_MACH_ANX7150
		.active = 1,
#endif
	},
#endif
#if	defined(CONFIG_CAT6611) || defined(CONFIG_CAT6611_NEW)
	[TCSI_GET_HDMI_INDEX(TCSI_HDMI_CAT6611)] =
	{
		.name = "CAT6611",
#if defined(CONFIG_TCHIP_MACH_CAT6611)
		.active = 1,
#endif
	},
#endif
#if	defined(CONFIG_HDMI_RK30)
	[TCSI_GET_HDMI_INDEX(TCSI_HDMI_RK30)] =
	{
		.name = "RK30HDMI",
#if defined(CONFIG_TCHIP_MACH_HDMI_RK30)
		.active = 1,
#endif
	},
#endif
};

/*
 * 	wifi support list
 */
static const struct tchip_device tchip_wifis[] =
{
#ifdef	CONFIG_MT5931_MT6622
	[TCSI_GET_WIFI_INDEX(TCSI_WIFI_MT5931_MT6622)] =
	{
		.name = "CDTK25931",
#ifdef	CONFIG_TCHIP_MACH_MT5931_MT6622
		.active = 1,
#endif
	},
#endif
#ifdef	CONFIG_AR6003
	[TCSI_GET_WIFI_INDEX(TCSI_WIFI_AR6003)] =
	{
		.name = "AR6302",
#ifdef	CONFIG_TCHIP_MACH_AR6003
		.active = 1,
#endif
	},
#endif
#ifdef	CONFIG_BCM4329
	[TCSI_GET_WIFI_INDEX(TCSI_WIFI_BCM4329)] =
	{
		.name = "B23",
#ifdef	CONFIG_TCHIP_MACH_BCM4329
		.active = 1,
#endif
	},
#endif
#ifdef	CONFIG_MV8686
	[TCSI_GET_WIFI_INDEX(TCSI_WIFI_MV8686)] =
	{
		.name = "MV8686",
#ifdef	CONFIG_TCHIP_MACH_MV8686
		.active = 1,
#endif
	},
#endif
#ifdef	CONFIG_RTL8188EU
	[TCSI_GET_WIFI_INDEX(TCSI_WIFI_RTL8188)] =
	{
		.name = "RTL8188EU",
		.active = 1,
	},
#endif
#ifdef	CONFIG_RTL8192CU
	[TCSI_GET_WIFI_INDEX(TCSI_WIFI_RTL8192)] =
	{
		.name = "RTL8188",
#ifdef	CONFIG_TCHIP_MACH_RTL8192
		.active = 1,
#endif
	},
#endif
#ifdef	CONFIG_RK903
	[TCSI_GET_WIFI_INDEX(TCSI_WIFI_RK903)] =
	{
		.name = "RK903",
		.active = 1,
	},
#endif
#ifdef	CONFIG_ESP8089
	[TCSI_GET_WIFI_INDEX(TCSI_WIFI_ESP8089)] =
	{
		.name = "ESP8089",
#ifdef	CONFIG_TCHIP_MACH_ESP8089
		.active = 1,
#endif
	},
#endif
#ifdef	CONFIG_MT7601
	[TCSI_GET_WIFI_INDEX(TCSI_WIFI_MT7601)] =
	{
		.name = "MT7601",
#ifdef	CONFIG_TCHIP_MACH_MT7601
		.active = 1,
#endif
	},
#endif
#ifdef	CONFIG_MT7601_MT6622
	[TCSI_GET_WIFI_INDEX(TCSI_WIFI_MT7601_MT6622)] =
	{
		.name = "J27601",
#ifdef	CONFIG_TCHIP_MACH_MT7601_MT6622
		.active = 1,
#endif
	},
#endif
};

/*
 * 	codec support list
 */
static const struct tchip_device tchip_codecs[] =
{
#ifdef CONFIG_SND_RK29_SOC_ES8323
    [TCSI_GET_CODEC_INDEX(TCSI_CODEC_ES8323)] =
    {
        .name = "ES8323",
#ifdef  CONFIG_TCHIP_MACH_SND_RK29_SOC_ES8323
        .active = 1,
#endif
    },
#endif
#ifdef	CONFIG_SND_RK29_SOC_RK610
	[TCSI_GET_CODEC_INDEX(TCSI_CODEC_RK610)] =
	{
		.name = "RK610CODEC",
#ifdef	CONFIG_TCHIP_MACH_SND_RK29_SOC_RK610
		.active = 1,
#endif
	},
#endif
#ifdef	CONFIG_SND_RK_SOC_RK616
	[TCSI_GET_CODEC_INDEX(TCSI_CODEC_RK616)] =
	{
		.name = "RK616CODEC",
#ifdef	CONFIG_TCHIP_MACH_SND_RK_SOC_RK616
		.active = 0,
#endif
	},
#endif	
#ifdef	CONFIG_SND_RK29_SOC_WM8988
	[TCSI_GET_CODEC_INDEX(TCSI_CODEC_WM8988)] =
	{
		.name = "WM8988",
#ifdef	CONFIG_TCHIP_MACH_SND_RK29_SOC_WM8988
		.active = 1,
#endif
	},
#endif

#ifdef	CONFIG_SND_RK29_SOC_WM8900
	[TCSI_GET_CODEC_INDEX(TCSI_CODEC_WM8900)] =
	{
		.name = "WM8900",
#ifdef	CONFIG_TCHIP_MACH_SND_RK29_SOC_WM8900
		.active = 1,
#endif
	},
#endif

#ifdef	CONFIG_SND_RK29_SOC_RT5621
	[TCSI_GET_CODEC_INDEX(TCSI_CODEC_RT5621)] =
	{
		.name = "RT5621",
#ifdef	CONFIG_TCHIP_MACH_SND_RK29_SOC_RT5621
		.active = 1,
#endif
	},
#endif

#ifdef	CONFIG_SND_RK29_SOC_WM8994
	[TCSI_GET_CODEC_INDEX(TCSI_CODEC_WM8994)] =
	{
		.name = "WM8994",
#ifdef	CONFIG_TCHIP_MACH_SND_RK29_SOC_WM8994
		.active = 1,
#endif
	},
#endif

#ifdef	CONFIG_SND_RK29_SOC_RT5631
	[TCSI_GET_CODEC_INDEX(TCSI_CODEC_RT5631)] =
	{
		.name = "RT5631",
#ifdef	CONFIG_TCHIP_MACH_SND_RK29_SOC_RT5631
		.active = 1,
#endif
	},
#endif
};



struct tchip_device *get_cur_device(struct tchip_device *devices, int size)
{
	int i;

	for(i = 0; i < size; devices++,i++)
	{
		if(devices->active)
		{
			return devices;
		}
	}

	return 0;
}

static char * strupper(char * dst, char *src)
{
	char * start = dst;

	while(*src!='\0')
		*dst++ = toupper(*src++);
	*dst = '\0';

	return start;
}

static void add2versionex(char *version, struct tchip_device *dev, char *prefix)
{
	char str[20];

	strcpy(str, prefix);

	strupper(&str[strlen(prefix)], dev->name);
	strcat(version, str);
}

static void add2version(char *version, struct tchip_device *dev)
{
	add2versionex(version,dev,"_");
}

struct tchip_device *set_all_active_device_version(struct tchip_device *devices, int size, char *version)
{
	int i;

	for(i = 0; i < size; devices++,i++)
	{
		if(devices->active)
		{
			add2version(version, devices);
		}
	}
}

#if 0
void cur_sensor_init(void)
{
	int i;

	cur_sensor[SENSOR_BACK] = cur_sensor[SENSOR_FRONT] = &no_sensor;

	for(i = 0; i < (TCSI_GET_SENSOR_INDEX(TCSI_CAMERA_END)); i++)
	{
		if(sensors[i].active)
		{
			if(TCSI_SENSOR_POS_BACK == sensors[i].pos)
				cur_sensor[SENSOR_BACK] = &sensors[i];
			else if(TCSI_SENSOR_POS_FRONT == sensors[i].pos)
				cur_sensor[SENSOR_FRONT] = &sensors[i];
		}
	}
}
#endif

int get_device_list(char * Info, struct tchip_device *devices, int size)
{
	int i;

	for (i = 0; i < size; devices++, i++)
	{
		if (devices->active)
			Info[i] = 1;

		strncpy(&Info[10 + i * 9], devices->name, 8);

		//printf("-->%9s:%d\n", &Info[10 + i * 9], Info[i]);
	}

	return i;
}
