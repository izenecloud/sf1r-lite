#ifndef PROCESS_NEW_TRINITY_COMMON_ON_EXIT_H
#define PROCESS_NEW_TRINITY_COMMON_ON_EXIT_H
/**
 * @file process/new-trinity/common/OnSignal.h
 * @author Ian Yang
 * @date Created <2009-10-29 11:36:37>
 * @date Updated <2010-07-05 15:00:14>
 * @brief Add hook to be run on exit
 *
 * The hooks are added in a global variable. The registering is not
 * thread-safe. The best practice is only include this file in the source file
 * containing main function, and register all hooks in main function before
 * starting any thread.
 */

#include <boost/function.hpp>

namespace sf1r
{

/**
 * @brief Setup default strategy.
 */
void setupDefaultSignalHandlers();

void waitSignalThread();
/**
 * Handler accepts one argument which indicates the system signal number. 0
 * indicates the process exits normally.
 */
typedef boost::function1<void, int> OnSignalHook;

/**
 * @brief add one hook to be run on exit
 * @param hook functor accept one \c int argument.
 *
 * \c hook must guarantee that it is reentrant.
 */
void addExitHook(const OnSignalHook& hook);

/**
 * @brief register signal number
 *
 * Use 0 to register hooks using stdlib atexit.
 *
 * For system like windows, behavior of registering singal other than 0 is
 * undefined.
 */
void registerExitSignal(int signal);

} // namespace sf1v5

#endif // PROCESS_NEW_TRINITY_COMMON_ON_EXIT_H
