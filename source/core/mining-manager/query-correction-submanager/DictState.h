/**
 * Copyright: iZENESoft
 * @file DictState.h
 * @brief  Define DictState used for construct dictionary automata
 * @author Jinli Liu
 * @date Sep 15,2009
 * @version v1.0
 */
#ifndef __DICT_STATE_H_
#define __DICT_STATE_H_
#include <vector>
#include <util/ustring/UString.h>
#include <3rdparty/am/rde_hashmap/hash_map.h>
#include <3rdparty/am/rde_hashmap/pair.h>

namespace sf1r
{
typedef uint16_t mychar;
typedef	izenelib::util::UString	UString;

class DictState;
/** @brief	Defination of class transition used for constructing
 *   dictionary automata(DAutomata),one transition consists of label and one target state.
 */
class transition
{

public:
    /// Create new transition
    transition(const mychar l, DictState *t) : label(l), target(t) {}
    /// Get label
    mychar getLabel(void) const
    {
        return label;
    }
    /// Get target state
    DictState *getTarget(void) const
    {
        return target;
    }
    /// Set target
    void setTarget(DictState *t)
    {
        target = t;
    }

    bool operator==(const transition &t) const
    {

        return label == t.label && target == t.target;
    }

    bool operator<(const transition &t) const
    {

        return (label == t.label ? target<t.target : label<t.label);
    }
private:
    ///label used in transition can be seen as weight of edge in graph
    mychar	label;
    ///target state can be seen as end vertex of one edge in graph
    DictState  *target;
};


typedef vector<transition>	trans_vec;



/** @brief	Defination of class DictState used for constructing dictionary automata(DAutomata),
 *  one DictState can be seens as set of transitions
 */
class DictState
{
public:

    /// Create new state with no transitions
    DictState() ;

    /// Clone a state
    DictState(DictState& s);

    /// Delete a state
    ~DictState(void);

    /// True if the state is final
    bool isFinal(void) const;

    /// Make the state final
    void setFinal(void);

    /// Get number of outgoing transitions
    int getOutCount(void) const;

    /// Get number of incoming transitions
    int getInCount(void) const;

    // Get outgoing transitions
    trans_vec& getTransitions(void);

    /// Get outgoing transitions for a constant state
    const trans_vec& getTransitions(void) const;

    /// Increase the counter of ingoing transitions (by 1 by default)
    int hit(const int i=1);

    /// Set state number
    void setNumber(const int n);

    /// Get state number
    int getNumber(void) const;


    /// Traverse a transition
    DictState *next(mychar label);

    /// Find target transition with the given label
    transition *findTransition(mychar label);

    /// Create a transition
    /// Note: target MUST be different
    void setNext(const mychar label, DictState *target);

    ///Get total states
    int getTotalStates(void) const;

protected:

    ///outgoing transitions
    trans_vec	outTrans;
    /// number of incoming transitions
    int			inCount;
    /// accepting state
    bool			final;
    /// state number
    int			stateNo;
    /// number of states in the automaton
    static int		TOTAL_STATES;


};//end of class DictState


}/*end of sf1r*/
#endif /*__D_AUTOMATA_TYPES_H_*/






