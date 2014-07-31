/*
*	file name: a3907.h
*	author: ouyang yafeng
*	date: 2012-6-20
*/
//ouyang
#ifndef _A3907_H_
#define _A3907_H_

//motor max min current
#define MAX_CURRENT 1000
#define MIN_CURRENT 1
#define BIG_CURRENT_STEP 100
#define CURRENT_INIT_VAL 1
#define SMALL_CURRENT_STEP 20 

#define CURRENT_STEP1 20
#define CURRENT_STEP2 50
#define CURRENT_STEP3 100


struct motor_setting
{
	unsigned char msb;
	unsigned char lsb;
};


struct motor_parameter
{
	int mode ;
	int present_code;
	int current_step;
};


#endif
