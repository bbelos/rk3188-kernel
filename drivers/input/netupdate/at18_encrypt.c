#include <linux/random.h>
#include <linux/kernel.h>
#define AT08                0x19
#define AT18                0x19
#define AT28                0x49
#define AT38                0x28

void atx8_encrypt(uint8_t *buf, int type)
{
    int i=0;
    get_random_bytes(&buf[1], 1);
    get_random_bytes(&buf[2], 1);
    buf[3] = (buf[1] + buf[2]) ^ buf[1];


    switch (type)
    {
	//AT08
        case 8:
        {
	     //buf[3] = (buf[1] + buf[2]) ^ buf[1];         
	     
	     while((buf[3] & AT08) == AT08) 
	     {
	     	buf[1]++;
	     	buf[3] = (buf[1] + buf[2]) ^ buf[1];
	     }
	     buf[3] = (buf[3] & AT08) ; 
        }
            break;
    
        case 18:
        {
             if (AT18 == (buf[3] & AT18))
            {
                buf[1]++;
                buf[3] = (buf[1] + buf[2]) ^ buf[1];
            }
            //buf[3] &= AT18;
        }
            break;
        case 28:
        {
            if (AT28 == (buf[3] & AT28))
            {
                buf[2]++;
                buf[3] = (buf[1] + buf[2]) ^ buf[1];
            }
            buf[3] |= AT28;
        }
            break;
        case 38:
        default:
        {
            if (AT38 == (buf[3] & AT38))
            {
                buf[2]++;
                buf[3] = (buf[1] + buf[2]) ^ buf[1];
            }
            buf[3] |= AT38;
        }
            break;
    }
}


void atx8_encrypt_ex(uint8_t *buf, int type)
{
    buf[3] = (buf[1] + buf[2]) ^ buf[1];

    switch (type)
    {
        case 18:
        {
             if (AT18 == (buf[3] & AT18))
            {
                buf[1]++;
                buf[3] = (buf[1] + buf[2]) ^ buf[1];
            }
            //buf[3] &= AT18;
        }
            break;
        case 28:
        {
            if (AT28 == (buf[3] & AT28))
            {
                buf[2]++;
                buf[3] = (buf[1] + buf[2]) ^ buf[1];
            }
            buf[3] |= AT28;
        }
            break;
        case 38:
        default:
        {
            if (AT38 == (buf[3] & AT38))
            {
                buf[2]++;
                buf[3] = (buf[1] + buf[2]) ^ buf[1];
            }
            buf[3] |= AT38;
        }
            break;
    }
}
