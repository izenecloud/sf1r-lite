/* 
 * File:   Timer.hpp
 * Author: Paolo D'Apice
 *
 * Created on September 14, 2012, 2:52 PM
 */

#ifndef TIMER_HPP
#define	TIMER_HPP

#include <ctime>

class Timer {
    clock_t ticks;
public:
    void tic() {
        ticks = clock();
    }

    void toc() {
        ticks = clock() - ticks;
    }

    float seconds() const {
        return static_cast<float>(ticks)/CLOCKS_PER_SEC;
    } 
};


#endif	/* TIMER_HPP */

