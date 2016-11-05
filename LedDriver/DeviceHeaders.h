#include <linux/ioctl.h>
#define COUNTER_VALUE 1
#define STATES 2
#define LENGTH 3
#define SUCCESS 0
#define IOCTL_GET_LED_STATE _IOR(MAJOR_NUM, 1, int)
#define IOCTL_SET_LED_STATE _IOR(MAJOR_NUM, 2, int)
#define DEVICE_NAME "ledModule"
#define CLASS_NAME "led"
#define MAJOR_NUM 240
