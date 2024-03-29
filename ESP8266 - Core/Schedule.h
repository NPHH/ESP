/*
 Schedule.h - Header file for scheduled functions.
 Copyright (c) 2020 esp8266/Arduino
 
 This file is part of the esp8266 core for Arduino environment.
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef ESP_SCHEDULE_H
#define ESP_SCHEDULE_H

#include <functional>
#include <stdint.h>

#define SCHEDULED_FN_MAX_COUNT 32

// The purpose of scheduled functions is to trigger, from SYS stack (like in
// an interrupt or a system event), registration of user code to be executed
// in user stack (called CONT stack) without the common restrictions from
// system context.  Details are below.

// The purpose of recurrent scheduled function is to independently execute
// user code in CONT stack on a regular basis.
// It has been introduced with ethernet service in mind, it can also be used
// for all libraries in the need of a regular `libdaemon_handlestuff()`.
// It allows these services to be run even from a user loop not going back
// to `loop()`.
// Ticker + scheduled function offer the same service but recurrent
// scheduled function happen more often: every yield() (vs every loop()),
// and time resolution is Microsecond (vs millisecond). Details are below.

// compute_scheduled_recurrent_grain() is used by delay() to give a chance to
// all recurrent functions to run per their timing requirement.

uint32_t compute_scheduled_recurrent_grain ();

// scheduled functions called once:
//
// * internal queue is FIFO.
// * Add the given lambda to a fifo list of lambdas, which is run when
//   `loop` function returns.
// * Use lambdas to pass arguments to a function, or call a class/static
//   member function.
// * Please ensure variables or instances used from inside lambda will exist
//   when lambda is later called.
// * There is no mechanism for cancelling scheduled functions.
// * `yield` can be called from inside lambdas.
// * Returns false if the number of scheduled functions exceeds
//   SCHEDULED_FN_MAX_COUNT (or memory shortage).
// * Run the lambda only once next time.
// * A scheduled function can schedule a function.

bool schedule_function (const std::function<void(void)>& fn);

// Run all scheduled functions.
// Use this function if your are not using `loop`,
// or `loop` does not return on a regular basis.

void run_scheduled_functions();

// recurrent scheduled function:
//
// * Internal queue is a FIFO.
// * Run the lambda periodically about every <repeat_us> Microseconds until
//   it returns false.
// * Note that it may be more than <repeat_us> Microseconds between calls if
//   `yield` is not called frequently, and therefore should not be used for
//   timing critical operations.
// * Please ensure variables or instances used from inside lambda will exist
//   when lambda is later called.
// * There is no mechanism for externally cancelling recurrent scheduled
//   functions.  However a user function returning false will cancel itself.
// * Long running operations or yield() or delay() are not allowed in the
//   recurrent function.
// * If alarm is used, anytime during scheduling when it returns true,
//   any remaining delay from repeat_us is disregarded, and fn is executed.

bool schedule_recurrent_function_us(const std::function<bool(void)>& fn,
    uint32_t repeat_us, const std::function<bool(void)>& alarm = nullptr);

// Test recurrence and run recurrent scheduled functions.
// (internally called at every `yield()` and `loop()`)

void run_scheduled_recurrent_functions();

#endif // ESP_SCHEDULE_H
