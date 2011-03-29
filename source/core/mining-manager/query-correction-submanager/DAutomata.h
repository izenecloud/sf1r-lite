/**
 * @file DAutomata.h
 * @brief  Definition of class DAutomata, it is used to construct dictionary automata according 
 * to given dictionary,currently we use support two types of language:english and korean
 * @author Jinli Liu
 * @date Create Sep 15,2009
 * @date Update Nov 24,2009
 * @version v2.0
 */
#ifndef __D_AUTOMATA_H_
#define __D_AUTOMATA_H_
#include "DictState.h"
#include <am/3rdparty/rde_hash.h>
#include <sdb/SequentialDB.h>
 namespace sf1r{
 
 typedef rde::hash_map<mychar,int> rde_map;
 typedef izenelib::am::rde_hash<izenelib::util::UString, char> rde_hash;
    class DAutomata{

    public:

        DAutomata(){};
        
        
        virtual ~DAutomata(){}
         
        /** @brief	Builds a trie from dictionary, note dictionary only with one word in each line.
         *  @param dictFileName  the path of  dictionary
         *  @param phonemes the set of phoneme,which is the smallest unit of one character.
         *  @param alphabet it contains pairs made of phoneme and phoneme's hash number. eg: <a,1>
         *  @return	The initial state of the automaton.
         */
        DictState *buildTrie( string dictFileName, 
                              vector<mychar>& phonemes, 
                              rde_map& alphabet, 
                              rde_hash& lexicon_);   
        
        DictState *buildTrie(izenelib::am::sdb_btree<izenelib::util::UString>& dictSDB, 
                                   vector<mychar>& phonemes, 
                                   rde_map& alphabet, 
                                   rde_hash& lexicon_);  
       
        /**@brief  Prints words in an automaton.
         * @param  s    initial state.
         */
        void printDAutomata(DictState *s, UString &ustr);
    };

 }/*end of sf1r*/
 #endif

