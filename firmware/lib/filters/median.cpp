#include "median.h"

//-----------------------------------------------------------------------------
// Median filter
//
// https://zbotic.in/how-to-fix-noisy-sensor-readings-filtering-techniques-for-arduino/#median-filter
//-----------------------------------------------------------------------------
//
uint32_t median_filter(uint32_t* buf, size_t bufSize) {

    // Simple insertion sort
    for (size_t i = 1; i < bufSize; i++) {
        unsigned  key = buf[i];
        size_t j   = i - 1;
        while (j >= 0 && buf[j] > key) {
            buf[j + 1] = buf[j];
            j--;
        }
        buf[j + 1] = key;
    }

    // Return the middle element
    return buf[bufSize / 2];
}
