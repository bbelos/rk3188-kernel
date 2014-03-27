
#define VERSION_ERROR_STRINTG       "Current driver doesn't support the requested firmware"

enum {
    OP_STATION_MODE = 0,
    OP_AP_MODE,
    OP_P2P,

    OP_MAX
};

struct nmc_version {
    char     driver_ver[10];        // wifi driver version
    char     op_mode;               // wifi mode
    char     fw_ver[10];            // wifi firmware version
    u32    fw_size;                 // wifi firmware size
};

static struct nmc_version nmc_ver_list[] = 
{
        { "9.3.0", STATION_MODE, "0", 143808},
        { "9.3.0", AP_MODE, "0", 111416},
        { "9.3.0", GO_MODE|CLIENT_MODE, "0", 141944},

        { "9.3.0", STATION_MODE, "1", 143816},
        { "9.3.0", AP_MODE, "1", 111420},
        { "9.3.0", GO_MODE|CLIENT_MODE, "1", 141864},

        { "9.3.1", STATION_MODE, "0", 143808},
        { "9.3.1", AP_MODE, "0", 111416},
        { "9.3.1", GO_MODE|CLIENT_MODE, "0", 141944},
		
        { "9.3.1", STATION_MODE, "1", 143816},
        { "9.3.1", AP_MODE, "1", 111420},
        { "9.3.1", GO_MODE|CLIENT_MODE, "1", 141864},

        { "9.3.2", STATION_MODE, "0", 143832},
        { "9.3.2", AP_MODE, "0", 111448},
        { "9.3.2", GO_MODE|CLIENT_MODE, "0", 141876},
		
        { "9.3.3", STATION_MODE, "0", 144012},
        { "9.3.3", AP_MODE, "0", 112432},
        { "9.3.3", GO_MODE|CLIENT_MODE, "0", 142084},
};


int check_firmware_version(char iftype, char* driver_version, u32 given_firmware_size)
{
    int ret = -1;
    int i = 0;
    int cnt = 0;

    cnt = sizeof(nmc_ver_list)/sizeof(struct nmc_version);

    if ( ( iftype == 0 ) || ( driver_version == NULL ) || ( given_firmware_size == 0 ) )
    {
        printk("[NMI] invaild arguments\n");
        return -1;
    }
    
	printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    printk("[NMI] iftype = %d, driver_version = %s, given_firmware_size = %d, nmc_ver_list count = %d\n", iftype, driver_version, given_firmware_size, cnt);

    switch (iftype)
    {
        case STATION_MODE:

            for ( i = OP_STATION_MODE ; i < cnt ; i += OP_MAX )
            {
                if ( strcmp(nmc_ver_list[i].driver_ver, driver_version) == 0 )
                {
                    if ( nmc_ver_list[i].fw_size == given_firmware_size )
                    {
                        printk("[NMI][STATION_MODE] : firmware version level = %s for %s driver\n",nmc_ver_list[i].fw_ver, nmc_ver_list[i].driver_ver);

                        return ret = 0;
                    }
                }
            }

            printk("[NMI][OP_STATION_MODE] : %s\n",VERSION_ERROR_STRINTG);

            ret = -1;

            break;

        case AP_MODE:

            for ( i = OP_AP_MODE ; i < cnt ; i += OP_MAX )
            {
                if ( strcmp(nmc_ver_list[i].driver_ver, driver_version) == 0 )
                {
                    if ( nmc_ver_list[i].fw_size == given_firmware_size )
                    {
                        printk("[NMI][OP_AP_MODE] :firmware version level = %s for %s driver\n",nmc_ver_list[i].fw_ver, nmc_ver_list[i].driver_ver);

                        return ret = 0;
                    }
                }
            }

            printk("[NMI][OP_AP_MODE] : %s\n",VERSION_ERROR_STRINTG);

            ret = -1;

            break;

        case GO_MODE:
        case CLIENT_MODE:

            for ( i = OP_P2P ; i < cnt ; i += OP_MAX )
            {
                if ( strcmp(nmc_ver_list[i].driver_ver, driver_version) == 0 )
                {
                    if ( nmc_ver_list[i].fw_size == given_firmware_size )
                    {
                        printk("[NMI][OP_P2P] : firmware version level = %s for %s driver\n",nmc_ver_list[i].fw_ver, nmc_ver_list[i].driver_ver);

                        return ret = 0;
                    }
                }
            }

            printk("[NMI][OP_P2P] : %s\n",VERSION_ERROR_STRINTG);

            ret = -1;

            break;

    
        default:
            printk("[NMI][UNKNOWN Mode] : %s\n",VERSION_ERROR_STRINTG);

			ret = -1;
            break;


    }

    return ret;
}
