/**@file   MinuteEditDistance.cpp.cpp
 * @brief  source file of minute edit distance
 * @author Park
 * @date  2008-08-21
 * @details
 * - Log
 */
#include <string.h>

#include "MinuteEditDistance.h"
namespace sf1r{
    unsigned int MinuteEditDistance::getDistance(
		    const char* s1, const char* s2)
    {
	    const unsigned int WIDTH = strlen(s1) + 1;
	    const unsigned int HEIGHT = strlen(s2) + 1;

	    unsigned int eArray[HEIGHT][WIDTH];
	    unsigned int i; // index var. for the row
	    unsigned int j; // index var. for the columns

	    // Initialization
	    eArray[0][0] = 0;

	    for (i = 1; i < HEIGHT; i++)
		    eArray[i][0] = i;

	    for (j = 1; j < WIDTH; j++)
		    eArray[0][j] = j;

	    for (i = 1; i < HEIGHT; i++)
	    {
		    for (j = 1; j < WIDTH; j++)
		    {
			    eArray[i][j] = min
				    (eArray[i - 1][j - 1] + (s1[j - 1] == s2[i - 1] ? 0 : 1),
				    min(eArray[i - 1][j] + 1, eArray[i][j - 1] + 1));
		    }
	    }

	    return eArray[HEIGHT - 1][WIDTH - 1];
    }

    unsigned int MinuteEditDistance::min(unsigned int v1, unsigned int v2)
    {
	    if (v1 > v2)
		    return v2;
	
	    return v1;
    }
}/*end of sf1r*/
