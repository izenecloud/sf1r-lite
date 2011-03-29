/**
 * @file ViterbiNode.h
 * @brief Defination of  viterbiNode used for viterbi algorithm 
 * @author Jinglei Zhao
 * @date July 15,2009
 * @version v1.0
 */

#ifndef __VITERBI_NODE_H_
#define __VITERBI_NODE_H_


namespace sf1r{
    typedef struct
    {
        int x;
        int y;
    } node;

    class ViterbiNode
    {
    public:
	    float prob;
	    std::vector<unsigned int> v_path;
	    float v_prob;

    };
}/*end of sf1r*/


#endif /* __VITERBI_NODE_H_ */
