#include <cassert>
#include <stdio.h>
#include <algorithm>
#include <math.h>
#include "CMU418intrin.h"
#include "logger.h"
using namespace std;


void absSerial(float* values, float* output, int N) {
    for (int i=0; i<N; i++) {
	float x = values[i];
	if (x < 0) {
	    output[i] = -x;
	} else {
	    output[i] = x;
	}
    }
}

// implementation of absolute value using 15418 instrinsics
void absVector(float* values, float* output, int N) {
    __cmu418_vec_float x;
    __cmu418_vec_float result;
    __cmu418_vec_float zero = _cmu418_vset_float(0.f);
    __cmu418_mask maskAll, maskIsNegative, maskIsNotNegative;

    //  Note: Take a careful look at this loop indexing.  This example
    //  code is not guaranteed to work when (N % VECTOR_WIDTH) != 0.
    //  Why is that the case?
    for (int i=0; i<N; i+=VECTOR_WIDTH) {

	// All ones
	maskAll = _cmu418_init_ones();

	// All zeros
	maskIsNegative = _cmu418_init_ones(0);

	// Load vector of values from contiguous memory addresses
	_cmu418_vload_float(x, values+i, maskAll);               // x = values[i];

	// Set mask according to predicate
	_cmu418_vlt_float(maskIsNegative, x, zero, maskAll);     // if (x < 0) {

	// Execute instruction using mask ("if" clause)
	_cmu418_vsub_float(result, zero, x, maskIsNegative);      //   output[i] = -x;

	// Inverse maskIsNegative to generate "else" mask
	maskIsNotNegative = _cmu418_mask_not(maskIsNegative);     // } else {

	// Execute instruction ("else" clause)
	_cmu418_vload_float(result, values+i, maskIsNotNegative); //   output[i] = x; }

	// Write results back to memory
	_cmu418_vstore_float(output+i, result, maskAll);
    }
}

// Accepts an array of values and an array of exponents
// For each element, compute values[i]^exponents[i] and clamp value to
// 4.18.  Store result in outputs.
// Uses iterative squaring, so that total iterations is proportional
// to the log_2 of the exponent
void clampedExpSerial(float* values, int* exponents, float* output, int N) {
    for (int i=0; i<N; i++) {
	float x = values[i];
	float result = 1.f;
	int y = exponents[i];
	float xpower = x;
	while (y > 0) {
	    if (y & 0x1) {
			result *= xpower;
		}
	    xpower = xpower * xpower;
	    y >>= 1;
	}
	if (result > 4.18f) {
	    result = 4.18f;
	}
	output[i] = result;
    }
}

void clampedExpVector(float* values, int* exponents, float* output, int N) {
    // Implement your vectorized version of clampedExpSerial here
    //  ...

    int limit = N - (N % VECTOR_WIDTH);

    __cmu418_vec_float x; // values[i]
    __cmu418_vec_int y; // exponent[i]
    __cmu418_vec_float result; // output[i]
    __cmu418_vec_int zero = _cmu418_vset_int(0.f);
    __cmu418_vec_int one = _cmu418_vset_int(1);
    __cmu418_vec_int two = _cmu418_vset_int(2);
    __cmu418_vec_int bd = _cmu418_vset_int(1); // for checking if exponent is odd
    __cmu418_vec_float ths = _cmu418_vset_float(4.18f);
    __cmu418_mask maskAll, maskIsGreaterThanThreshold, maskExpIsOdd, maskGreaterThanZero;
    float threshold = 4.18f;

    for (int i = 0; i < limit; i+=VECTOR_WIDTH)  {
    // mask all 
    maskAll = _cmu418_init_ones(); 
    // mask for greater than the threshold , ie 4.18
    maskIsGreaterThanThreshold = _cmu418_init_ones(0);
    // mask for odd exponents
    maskExpIsOdd = _cmu418_init_ones(0);
    // mask for y > 0 
    maskGreaterThanZero = _cmu418_init_ones();

    // load values and exponents into vector registers
    _cmu418_vload_float(x, values+i, maskAll);
    _cmu418_vload_int(y, exponents+i, maskAll);
    // set results to 1.f
    _cmu418_vset_float(result, 1.f, maskAll);
    
    // while y > 0 branch
    while (_cmu418_cntbits(maskGreaterThanZero) > 0) {
        // check if exponent is odd
        _cmu418_vbitand_int(bd, y, one, maskAll);
        // set mask for odd exponents
        _cmu418_veq_int(maskExpIsOdd, bd, one, maskAll);
        // if exponent is odd, multiply result by x
        _cmu418_vmult_float(result, result, x, maskExpIsOdd);
        // square x
        _cmu418_vmult_float(x, x, x, maskGreaterThanZero);
        // divide y by 2
        _cmu418_vdiv_int(y, y, two, maskGreaterThanZero);
        // update mask for y > 0
        _cmu418_vgt_int(maskGreaterThanZero, y, zero, maskAll);
    }
    // if greater than threshold, set result to threshold
    _cmu418_vgt_float(maskIsGreaterThanThreshold, result, ths, maskAll);
    _cmu418_vset_float(result, threshold, maskIsGreaterThanThreshold);
    // write back to memeory
    _cmu418_vstore_float(output+i, result, maskAll);
  }

  // cleanup remaining elements
  for (int i = limit; i < N; i++){
    clampedExpSerial(values+i, exponents+i, output+i, 1);
  }
}


float arraySumSerial(float* values, int N) {
    float sum = 0;
    for (int i=0; i<N; i++) {
	sum += values[i];
    }

    return sum;
}

// Assume N % VECTOR_WIDTH == 0
// Assume VECTOR_WIDTH is a power of 2
float arraySumVector(float* values, int N) {
    // Implement your vectorized version here
    //  ...
    __cmu418_vec_float x;
    float sum = 0.f;


    for (int i = 0; i < N; i+=VECTOR_WIDTH) {
        __cmu418_mask maskAll = _cmu418_init_ones();
        _cmu418_vload_float(x, values+i, maskAll);
        // horizontal add
        for (int j = VECTOR_WIDTH; j > 1; j /= 2) {
            __cmu418_vec_float temp;
            _cmu418_hadd_float(temp, x);
            _cmu418_interleave_float(x, temp);
        }
        sum += x.value[0];
  }

	return sum;
}
