
#define _CRT_SECURE_NO_DEPRECATE

#include "NMI_OsWrapper/include/NMI_OSWrapper.h"

#ifdef CONFIG_NMI_MATH_OPERATIONS_FEATURE

#include "math.h"


/**
*  @brief 		Returns the arc cosine of x in radians.
*  @details 	Returns the arc cosine of x in radians.
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		The value x must be within the range of -1 to +1 (inclusive).
				The returned value is in the range of 0 to pi (inclusive).
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_acos(NMI_Double x)
{
	return acos(x);
}

/**
*  @brief 		Returns the arc sine of x in radians.
*  @details 	Returns the arc sine of x in radians.
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		The value x must be within the range of -1 to +1 (inclusive).
				The returned value is in the range of 0 to pi (inclusive).
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_asin(NMI_Double x)
{
	return asin(x);
}


/**
*  @brief 		Returns the arc tangent of x in radians.
*  @details 	Returns the arc tangent of x in radians.
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		The value of x has no range. 
				The returned value is in the range of -p/2 to +p/2 (inclusive).
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_atan(NMI_Double x)
{
	return atan(x);
}

/**
*  @brief 		Returns the arc tangent in radians
*  @details 	Returns the arc tangent in radians of y/x based on the
				signs of both values to determine the correct quadrant.
*  @param[in] 	NMI_Double x
*  @param[in] 	NMI_Double y
*  @return 		NMI_Double 
*  @note 		Both y and x cannot be zero.
				The returned value is in the range of -p/2 to +p/2 (inclusive).
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_atan2(NMI_Double y, NMI_Double x)
{
	return atan2(y,x);
}


/**
*  @brief 		Returns the cosine of a radian angle x.
*  @details 	Returns the cosine of a radian angle x.
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		The value of x has no range.
				The returned value is in the range of -1 to +1 (inclusive).
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_cos(NMI_Double x)
{
	return cos(x);
}


/**
*  @brief 		Returns the hyperbolic cosine of x.
*  @details 	Returns the hyperbolic cosine of x.
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		There is no range limit on the argument or return value.
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_cosh(NMI_Double x)
{
	return cosh(x);
}


/**
*  @brief 		Returns the sine of a radian angle x.
*  @details 	Returns the sine of a radian angle x.
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		The value of x has no range.
				The returned value is in the range of -1 to +1 (inclusive).
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_sin(NMI_Double x)
{
	return sin(x);
}


/**
*  @brief 		Returns the hyperbolic sine of x.
*  @details 	Returns the hyperbolic sine of x.
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		There is no range limit on the argument or return value.
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_sinh(NMI_Double x)
{
	return sinh(x);
}

/**
*  @brief 		Returns the tangent of a radian angle x.
*  @details 	Returns the tangent of a radian angle x.
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		There is no range limit on the argument or return value.
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_tan(NMI_Double x)
{
	return tan(x);
}

/**
*  @brief 		Returns the hyperbolic tangent of x.
*  @details 	Returns the hyperbolic tangent of x.
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		The value of x has no range.
				The returned value is in the range of -1 to +1 (inclusive).
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_tanh(NMI_Double x)
{
	return tanh(x);
}


/**
*  @brief 		Returns the value of e raised to the xth power.
*  @details 	Returns the value of e raised to the xth power.
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		There is no range limit on the argument or return value.
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_exp(NMI_Double x)
{
	return exp(x);
}

/**
*  @brief 		The returned value is the mantissa and the integer pointed to by exponent is the exponent.
*  @details 	The returned value is the mantissa and the integer pointed to by exponent is the exponent.
				The resultant value is x=mantissa * 2^exponent.
*  @param[in] 	NMI_Double x
*  @param[out] 	NMI_Sint32* exponent
*  @return 		NMI_Double 
*  @note 		The mantissa is in the range of .5 (inclusive) to 1 (exclusive).
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_frexp(NMI_Double x, NMI_Sint32* exponent)
{
	return frexp(x,exponent);
}



/**
*  @brief 		Returns x multiplied by 2 raised to the power of exponent.
*  @details 	Returns x multiplied by 2 raised to the power of exponent.
				x*2^exponent
*  @param[in] 	NMI_Double x
*  @param[out] 	NMI_Sint32 exponent
*  @return 		NMI_Double 
*  @note 		There is no range limit on the argument or return value.
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_ldexp(NMI_Double x, NMI_Sint32 exponent)
{
	return ldexp(x,exponent);
}


/**
*  @brief 		Returns the natural logarithm (base-e logarithm) of x.
*  @details 	Returns the natural logarithm (base-e logarithm) of x.
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		There is no range limit on the argument or return value.
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_log(NMI_Double x)
{
	return log(x);
}



/**
*  @brief 		Returns the common logarithm (base-10 logarithm) of x.
*  @details 	Returns the common logarithm (base-10 logarithm) of x.
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		There is no range limit on the argument or return value.
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_log10(NMI_Double x)
{
	return log10(x);
}


/**
*  @brief 		Breaks the floating-point number x into integer and fraction components.
*  @details 	Breaks the floating-point number x into integer and fraction components.
				The returned value is the fraction component (part after the decimal),
				and sets integer to the integer component.

*  @param[in] 	NMI_Double x
*  @param[out] 	NMI_Double* dInteger
*  @return 		NMI_Double 
*  @note 		There is no range limit on the argument or return value.
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_modf(NMI_Double x,NMI_Double* dInteger)
{
	return modf(x,dInteger);
}



/**
*  @brief 		Returns x raised to the power of y.
*  @details 	Returns x raised to the power of y.
*  @param[in] 	NMI_Double x
*  @param[in] 	NMI_Double y
*  @return 		NMI_Double 
*  @note 		x cannot be negative if y is a fractional value.
				x cannot be zero if y is less than or equal to zero.
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_pow(NMI_Double x,NMI_Double y)
{
	return pow(x,y);
}



/**
*  @brief 		Returns the square root of x.
*  @details 	Returns the square root of x.
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		The argument cannot be negative. The returned value is always positive.
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_sqrt(NMI_Double x)
{
	return sqrt(x);
}

/**
*  @brief 		Returns the smallest integer value greater than or equal to x.
*  @details 	Returns the smallest integer value greater than or equal to x.
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		There is no range limit on the argument or return value.
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_ceil(NMI_Double x)
{
	return ceil(x);
}


/**
*  @brief 		Returns the absolute value of x
*  @details 	Returns the absolute value of x 
				(a negative value becomes positive, positive value is unchanged).
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		There is no range limit on the argument.
				The return value is always positive.
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_fabs(NMI_Double x)
{
	return fabs(x);
}

/**
*  @brief 		Returns the largest integer value less than or equal to x.
*  @details 	Returns the largest integer value less than or equal to x.
*  @param[in] 	NMI_Double x
*  @return 		NMI_Double 
*  @note 		There is no range limit on the argument or return value.
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_floor(NMI_Double x)
{
	return floor(x);
}


/**
*  @brief 		Returns the remainder of x divided by y.
*  @details 	Returns the remainder of x divided by y.
*  @param[in] 	NMI_Double x
*  @param[in] 	NMI_Double y
*  @return 		NMI_Double 
*  @note 		There is no range limit on the return value. 
				If y is zero, then either a range error will occur or the function 
				will return zero (implementation-defined).
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Double NMI_fmod(NMI_Double x,NMI_Double y)
{
	return fmod(x,y);
}


/**
*  @brief 		
*  @details 	Returns the absolute value of x. 
				Note that in two's compliment that the most maximum number cannot be 
				represented as a positive number. The result in this case is undefined.
				The absolute value is returned.
*  @param[in] 	NMI_Sint32 x
*  @return 		NMI_Sint32 
*  @note 		
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Sint32 NMI_abs(NMI_Sint32 x)
{
	return abs(x);
}




/**
*  @brief 		
*  @details 	Returns a pseudo-random number in the range of 0 to RAND_MAX.
*  @return 		NMI_Sint32 
*  @note 		
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
NMI_Sint32 NMI_rand()
{
	return rand();
}


/**
*  @brief 		
*  @details 	This function seeds the random number generator used by the function rand.
				Seeding srand with the same seed will cause rand to return the same sequence
				of pseudo-random numbers.
				If srand is not called, NMI_rand acts as if NMI_srand(1) has been called.

*  @param[in] 	NMI_Uint32 seed
*  @note 		No value is returned.
*  @author		remil
*  @date		2 Nov 2010
*  @version		1.0
*/
void NMI_srand(NMI_Uint32 seed)
{
	srand(seed);
}



#endif