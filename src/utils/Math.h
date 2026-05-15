#pragma once
#ifndef Math_h__
#define Math_h__
#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */

class Math
{
public:

	static double randomD() {
		return static_cast<double>(rand()) / RAND_MAX;
    }
	
	static float randomF() {
		return static_cast<float>(rand()) / RAND_MAX;
    }
	
	static int randomI() {
		return rand();
    }
protected:
	
private:
};

#endif // Math_h__
