/*
 * File:   Timer.hpp
 * Author: Paolo D'Apice
 *
 * Created on September 14, 2012, 2:52 PM
 */

#ifndef TIMER_HPP
#define TIMER_HPP

#include <ctime>

/// @brief Simple timer class
class Timer {
    clock_t ticks;
public:
    /// Start measuring time.
    void tic() {
        ticks = clock();
    }

    /// Stop measuring time.
    void toc() {
        ticks = clock() - ticks;
    }

    /// Get elapsed time in seconds.
    float seconds() const {
        return static_cast<float>(ticks)/CLOCKS_PER_SEC;
    }
};

#endif /* TIMER_HPP */
