/**
 * Copyright: iZENESoft
 * @file UAutomataTypes.h
 * @brief  Define serveral structs preparing for constructing universal leveinstain automata
 * @author Jinli Liu
 * @date Sep 15, 2009
 * @version v1.0
 * @detail
 * -Log
 *   --2009.11.05 revise the relationship between Point and Position
 *
 */
#ifndef U_AUTOMATA_TYPES_H_
#define U_AUTOMATA_TYPES_H_
#include <map>
#include <algorithm>
#include <iterator>
#include <util/hashFunction.h>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/hash_map.hpp>
#include <fstream>
#include <queue>
#include <stack>
#include <boost/timer.hpp>
#include<boost/tokenizer.hpp>
#include <3rdparty/am/rde_hashmap/hash_map.h>
#include <3rdparty/am/rde_hashmap/pair.h>
#include <bitset>

    using  namespace   __gnu_cxx;

    /**
     * @brief it is used to check  status of state, I means non-final state, M means final state.
     */
    enum Parameter{I, M};
    
    
    /**
     * @brief it is used to used to define the type of automaton,TRANSPOSTION means automata extended with transpostion,SPLIT means extended with merge and split.we only focus on USUAL type.
     */
    enum Type{USUAL, TRANSPOSTION, SPLIT};
    
    

    /**
     * @brief Define struct POINT used in automata such as {Type,X,Y}
     */
    class Point{
	public:
	    
	    Point(){}
	    Point(Type t, int x, int y):type(t),  X(x),  Y(y){}	
	
    public:
        Type type;
	    int X;
	    int Y;
    };
    typedef vector<Point> Points;
    
    
    
    /**
     * @brief Define struct Position used in automata such as {Parameter,Type,X,Y}
     */
    class Position:public Point{
	public:
	   
	    Position(){}
	    Position(Parameter p, Type t, int x, int y): Point(t, x, y), para(p){}	
	   
        bool operator == (const Position& other)const
        {
           return (para == other.para) && 
                  (type == other.type) &&
                  (X == other.X) &&
                  (Y == other.Y);
        }
        Position& operator = ( const Position& other)
        {
           if(this!=&other)
		    {
		       this->para =other.para;
		       this->type = other.type;
		       this->X = other.X;
		       this->Y = other.Y;
           }
           return *this;
        }
        bool isFinal()
        {
            return para == M ? true :false;
            
        }
   public:
       Parameter para;
    };
    
    
    
    /**
     * @brief one STATE is set of positions,such as {{I,USUAL,1,2},{M,TRANSPOSTION,0,3}}. 
     */
    typedef vector<Position> STATE;
    /**
     * @brief this is MEMCPY_SERILIZATION of STATE
     */
    MAKE_MEMCPY_SERIALIZATION( STATE );
    
    
    
    
    
    /**
     * @brief Define transition in universal automata,will be used in rde::hash_map
     */
    class rde_tran{
    public:   
       
        rde_tran(){}
        rde_tran(int i, const string& str):stateNo(i), label(str){}
        bool operator == (const rde_tran& other)const
        {
           return (stateNo == other.stateNo) && 
                  (label== other.label);
        }
    public:
         /**
         * @brief current stateNo,can be seen as begin vetex of an edge in graph
         */
        int stateNo; 
        /**
         * @brief  corresponding label(symbol),can be seen as weight of edge in graph
         */
        string label;
    };
    
    
    
    //need serialization for further used in rde::hash_map
    namespace rde
    {
        #include <util/hashFunction.h>
        /**
         * @brief Define hash function for user defined struct rde_tran
         */
        template<> inline
        hash_value_t extract_int_key_value<rde_tran>(const rde_tran& t)
        {
           hash_value_t aaaa = t.stateNo+ izenelib::util::izene_hashing( t.label);
           return aaaa;
        }
   }
  
#endif/*U_AUTOMATA_TYPES_H_*/



