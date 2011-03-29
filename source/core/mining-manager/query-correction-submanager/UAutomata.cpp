/**
 * @file UAutomata.cpp
 * @brief  source file of class UAutomata, it implements leveinshtain universal automata with  q-errors.(edit distance is q)
 * @author Jinli Liu
 * @date Sep 15,2009
 * @version v2.0
 * @detail 
 * -Log
 *  --2009.11.05 rename the functions
 */
#include "UAutomata.h"
namespace sf1r{
    
    UAutomata::UAutomata(int q):ed(q){}  
     
    //check if one position can be deleted(if it is subsumed by another position) 
    //in order to make the deterministic automata
    bool UAutomata::lessThanSubsume(const Position& p1, const Position& p2){
	
	    int m;
	    if (p1.type != USUAL || p2.Y <= p1.Y )
		    return false;
	    if (p2.type == TRANSPOSTION)
		    m = p2.X + 1 - p1.X;
	    else
		    m = p2.X - p1.X;
	    if (m<0)
		    m = -m;
	
	    int y_diff = (p2.Y - p1.Y);
	    return m <= y_diff;
    }

    //Check whether the length of the word b1 b2 ...bk is suitable to define the transition function findNextState()
    bool UAutomata::lengthCoversAllPositions( int k, const STATE& st)
    {
	    Position pos;
	    if (st[0].para == I)
	    {
		    Position temp(I, USUAL, 0, 0);
		    STATE tempSt;
		    tempSt.push_back(temp);
		    if (st == tempSt)
			    return k >= st[0].X+ed;
		    else
		    {
			    for (unsigned int i=0; i<st.size(); i++)
				{
				    if ( k < (2*ed+st[i].X-st[i].Y+1) )
					    return false;
			    }
		    }
	    }
	    else
	    {
		    if (k < ed)
		    {
			    Position temp( M, USUAL, 0, ed-k);
			    pos = temp;
		    }
		    else
		    {
			    Position temp( M, USUAL, ed-k, 0);
			    pos = temp;
		    }
		    for (unsigned int i=0; i<st.size(); i++)
	        {
	        	if ( !(st[i]==pos) && lessThanSubsume(pos, st[i])==false)
	        		return false;
	        }
	    }
	    return true;
    }
    

    //Convert the non-final states into final state
    STATE  UAutomata::convertToFinalState( const STATE& st, int k )
    {
	    STATE m;
	    int size=st.size();
	    for (int i=0; i<size; i++)
		{
		    if (st[i].para == I) //non-final state
		    {
			    Position temp(M, st[i].type, st[i].X+ed+1-k, st[i].Y);
			    m.push_back(temp);
		    }
		    else
		    {
			    Position temp(I, st[i].type, st[i].X-ed-1+k, st[i].Y);
			    m.push_back(temp);
		    }
	    }
	    return m;
    }
    

    //Find the right most position(element) of one state
    Position UAutomata::findRightMostPosition( const STATE &st)
    {
	    Position rm;
	    for (int i=0; i<(int)st.size(); i++)
		{
		    if (st[i].type == USUAL)
			{
			    rm = st[i];
		    }
	    }
	    for ( unsigned int j=0;j<st.size();j++)
	    {
		    bool isUsual = st[j].type==USUAL;
		    bool isLarger = (st[j].X-st[j].Y)>(rm.X-rm.Y);
		    if (isUsual && isLarger)
			{
			    rm = st[j];
		    }
	    }
	    
	    return rm;
    }
    
    //Check if one position is good candidate to be inclued by one state
    bool UAutomata::isGoodCandidate( const Position &pos, int k )
    {
	    if (pos.para == I)
		    return ( k <= 2*ed+1 && 
		             pos.Y <= (pos.X+2*ed+1-k) );
		
	    return pos.Y > (pos.X+ed);
    }
    
    //Compute  the deterministic automaton preparing for find elementary state in Delta_E()
    Points UAutomata::findDeterimineState( const Point &point, const std::string& h)
    {
	    int x = point.X;
	    int y = point.Y;
	    int posOfFirst;
	    if (h.length() == 0)
	    {	
		    Points points;
		    if (y < ed)
		    {
			    Point point(USUAL, x, y+1);	
			    points.push_back(point);
			    return points;
		    }
		    return points;//null
	    }
	    if (h[0] == '1')
	    {
		    Point point(USUAL, x+1, y);
		    Points points;
		    points.push_back(point);
		    return points;
	    }
	    posOfFirst = 0;
	    unsigned int j;
	    for (j = 1; j< h.length(); j++)
	    {
		     if ( h[j] == '1' ) 
		    {
			    posOfFirst= j;
			    break;
		    }
		
	    }
	
	    if ( posOfFirst == 0 )
	    {
		    Points points;
		    if (y < ed)
		    {   				
			    Point point1(USUAL, x, y+1);
			    Point point2(USUAL, x+1, y+1);	
			    points.push_back(point1);
			    points.push_back(point2);	
			    return points;
		    }
		    else
			    return points;	
	    }
	    else
	    {
		    Points points;	
		    Point point1(USUAL, x, y+1);
		    Point point2(USUAL, x+1, y+1);	
		    Point point3(USUAL, x+j+1, y+j);
		    points.push_back(point1);
		    points.push_back(point2);	
		    points.push_back(point3);
		    return points;	
	    }	
    }


    //Compute the characteristic vetor determined by Posistion pos and symbol b
    string UAutomata::computeCharVector(const Position &pos, const std::string &b ) 
    {
	    int len;
	    string str;
	    if (pos.para == I)
	    {
		    int temp =(int)b.length()-ed-pos.X;
		    if ((ed-pos.Y+1) < temp)
		    {
			   
			    len = ed-pos.Y+1;
		    }
		    else
		    {
			    len = temp;			 
		    }
		
		    if (len>0)
			    return b.substr(ed+pos.X, len);
		    else 
			    return str;
			
	    }
	    else
	    {
		    if (ed-pos.Y+1 < -pos.X)
			    len = ed-pos.Y+1;
		    else
			    len = -pos.X;			    
		    return b.substr(b.length()+pos.X, len);
	    }
    }

    //Find elementary state preparing for further refinement in findNextState()
    STATE UAutomata::findElementaryState(const Position &q, const std::string &b )
    {	   	    
	    string chars = computeCharVector(q, b);
	    Point temp(q.type, q.X, q.Y);
	    Points deltaED = findDeterimineState(temp, chars);
	    STATE st;
	    if (deltaED.empty())
		    return st;
	    if (q.para == I)
	    {
		     Points::iterator it;
		     for (it=deltaED.begin(); it!=deltaED.end(); it++)
		     {
		     	Position temp(I, it->type, it->X-1, it->Y);
			    st.push_back(temp);
		     } 
	    }
	    else
	    {
		     Points::iterator it;
		     for (it=deltaED.begin(); it!=deltaED.end(); it++)
		     {
		     	Position temp(M, it->type, it->X, it->Y);
			    st.push_back(temp);
		     } 
	    }
	    return st;
    }


    //Find next state(nextSt) to be stored in automaton from given state st and symbol b
    STATE UAutomata::findNextState( const STATE& st, const string &b)
    {
	   
	    STATE nextSt;
	    STATE deltaE;
	  
	    for (int j=0; j<(int)st.size(); j++)
	    {
		    deltaE=findElementaryState(st[j], b);
	       
		    for (int k=0; k<(int)deltaE.size(); k++)		    			
			{
			    nextSt.push_back(deltaE[k]);
		    }
			    
		    int size = deltaE.size();
		    if (size != 0)
		    {
			    for (int i=0; i<size; i++)//p in delta_E
			    {
				    
				    for (vector<Position>::iterator it=nextSt.begin(); it!=nextSt.end();)//p in nextSt
				    {
	                    bool isSubsumed = lessThanSubsume( deltaE[i], *it );
	                    bool isLarger = deltaE[i].Y>ed && it->Y>ed ;
					    if (isSubsumed || isLarger)
					    {
						    it = nextSt.erase(it);
						    	
					    }
					    else
					    {
						    if (*it == deltaE[i] || !isSubsumed)						    
							    it++;						    
					    }//end else
				    }//end for 
				
			    }//end for 
		    }//end if 
		
	    }//end for
	    
	    if (nextSt.size()!=0)
	    {	
		    if ( isGoodCandidate( findRightMostPosition(nextSt), b.length()) ) 		    
			    nextSt = convertToFinalState(nextSt, b.length());
	    }
	    return nextSt;
    }


    //Generate the combination made of {0,1}*
    void UAutomata::initCombination(vector<string>& combination, 
                                    string& candidate, 
                                    unsigned int position)
    {
	    if (position == candidate.size())
	    {
		    combination.push_back(candidate);
		    return;
	    }
	    // visit current position, and turn to next position
	    candidate[position] = '0';
	    initCombination(combination, candidate, position+1);
	    candidate[position] = '1';
	    initCombination(combination, candidate, position+1);
	    
    }

    //The main public interface in UAutomata to construct universal automata. The process can be seen as BFS.
    //Automaton stores the transition(source stateNo + label) and target stateNo. 
    void UAutomata::buildAutomaton(vector<STATE>& states,
                                   rde::hash_map<rde_tran,int>& autoMaton)
    {	
	    string temp;
	    for (int i=1; i<=2*ed+2; i++)
	    {
		    temp.resize (i, '0');
		    initCombination(combination,  temp,  0);
	    }
	    STATE st, nextSt;
	    Position pos(I, USUAL, 0, 0);
	    st.push_back(pos);
	    queue<STATE> myqueue;
	    myqueue.push(st);//Initialize myqueue
	    int i = 0;
	    stateMap.insert(pair<STATE,int>(st,  i++));
	    states.push_back(st);
	    while (!myqueue.empty())
	    {
		    st = myqueue.front();
		    myqueue.pop();
		    int stNo = stateMap.find(st)->second;	
		    //tranverse every comination of {0,1}*
		    vector<string>::iterator it=combination.begin();		    
		    for ( ; it!=combination.end(); it++)
		    {			    
			    if (lengthCoversAllPositions(it->length(), st))
			    {	
				    nextSt = findNextState(st, *it );//find nextSt
				    if ( nextSt.size() != 0)
				    {
				        rde_tran transition(stNo, *it);
					    if (stateMap.find(nextSt) == stateMap.end())//can't find, nextSt is new STATE
					    {						
						    stateMap.insert(pair<STATE,int>(nextSt,  i));
						    states.push_back(nextSt);
						    myqueue.push(nextSt);				
						    autoMaton.insert(rde::pair<rde_tran, int>(transition, i));	
						    i++;						
					    }
					    else 
					    {					
						    int nextStNo = stateMap.find(nextSt)->second;						
						    autoMaton.insert(rde::pair<rde_tran, int>(transition, nextStNo));							
					    }					    
				    }							
			    }//end if							
		    }//end	for	
	    }//end	while
    }//end
}/*end of sf1r*/
/*int main()
{	

  
    //#ifdef PRINT
	int n=2;
	vector<STATE> stateVector;
	rde::hash_map<rde_tran, int> autoTest;
	UAutomata uAutomata;
	uAutomata.Build_Automaton(n,stateVector,autoTest);
   //#endif  
	return 0;
	
}*/

