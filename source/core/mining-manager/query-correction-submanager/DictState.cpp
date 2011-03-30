/**
 * Copyright: iZENESoft
 * @file DictState.cpp
 * @brief  source file of class DictState,it is the element of dictionary automata
 * @author Jinli Liu
 * @date Sep 15,2009
 * @version v1.0
 */

#include "DictState.h"
using namespace std;
typedef uint16_t mychar;

namespace sf1r
{
/// Create new state with no transitions
DictState::DictState(void) : inCount(0), final(false)
{
    TOTAL_STATES++;
    stateNo = -1;
}

/// Clone a state
DictState::DictState(DictState &s): outTrans(s.outTrans), inCount(0),final(s.final)
{
    trans_vec::iterator t = outTrans.begin();
    for (; t != outTrans.end(); t++)
    {
        t->getTarget()->hit();
    }
    TOTAL_STATES++;
    stateNo = -1;
}

/// Delete a state
DictState::~DictState(void)
{
    trans_vec::iterator t = outTrans.begin();
    for (; t != outTrans.end(); t++)
    {
        if (t->getTarget()->hit(-1) == 0)
            delete t->getTarget();
        --TOTAL_STATES;
    }
}

/// True if the state is final
bool DictState::isFinal(void) const
{
    return final;
}

/// Make the state final
void DictState::setFinal(void)
{
    final = true;
}

/// Get number of outgoing transitions
int DictState::getOutCount(void) const
{
    return outTrans.size();
}

/// Get number of incoming transitions
int DictState::getInCount(void) const
{
    return inCount;
}

// Get outgoing transitions
trans_vec& DictState::getTransitions(void)
{
    return outTrans;
}

/// Get outgoing transitions for a constant state
const trans_vec& DictState::getTransitions(void) const
{
    return outTrans;
}

/// Increase the counter of ingoing transitions (by 1 by default)
int DictState::hit(int i)
{
    return (inCount += i);
}

/// Set state number
void DictState::setNumber(const int n)
{
    stateNo = n;
}

/// Get state number
int DictState::getNumber(void) const
{
    return stateNo;
}


/// Traverse a transition
DictState* DictState::next(mychar label)
{
    trans_vec::iterator p = outTrans.begin();
    for (; p != outTrans.end(); p++)
    {
        if (p->getLabel() == label)
            return p->getTarget();
    }
    return NULL;
}

/// Find transition with the given label
transition* DictState::findTransition(mychar label)
{
    trans_vec::iterator p = outTrans.begin();
    for (;p != outTrans.end(); p++)
    {
        if (p->getLabel() == label)
            return &(*p);
    }
    return NULL;
}

/// Create a transition
/// Note: target MUST be different
void DictState::setNext(const mychar label, DictState *target)
{
    trans_vec::iterator p = outTrans.begin();

    for (; p!=outTrans.end() && p->getLabel()<label; p++);

    if ( p!=outTrans.end() && p->getLabel()==label)
    {
        bool del = false;
        DictState *s = p->getTarget();
        if ( s->hit(-1) == 0)
            del = true;
        p->setTarget(target);
        if (del)
            delete s;
    }
    else
        outTrans.insert(p, transition(label, target));

    target->hit();
}

int DictState::getTotalStates(void) const
{
    return TOTAL_STATES;
}



}/*end of sf1r*/







