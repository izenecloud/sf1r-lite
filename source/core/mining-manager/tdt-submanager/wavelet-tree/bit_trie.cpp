#include "bit_trie.hpp"
#include <algorithm>
#include <math.h>

using namespace std;

namespace sf1r
{
BitNode::BitNode(int grade)
    : grade_(grade)
    , ZNext_(NULL)
    , ONext_(NULL)
{

}

BitNode::~BitNode()
{
    delete ZNext_;
    delete ONext_;
}

BitTrie::BitTrie(int alphbatNum)
    : height_(ceil((log(double(alphbatNum+1))/log(2.0))))
{
    Root_=new BitNode(height_);
}
BitTrie::~BitTrie()
{
    delete Root_;
}
void BitTrie::insert(int val)
{
    //cout<<"insert"<<val<<endl;
    BitNode* temp=Root_;
    for(int i=0; i<=height_-1; i++)
    {
        if(val&(1<<i))
        {
            if(!temp->ONext_)
            {
                BitNode* newtemp=new BitNode(i);
                temp->ONext_=newtemp;
                temp=newtemp;
            }
            else
            {
                temp=temp->ONext_;
            }
        }
        else
        {
            if(!temp->ZNext_)
            {
                BitNode* newtemp=new BitNode(i);
                temp->ZNext_=newtemp;
                temp=newtemp;
            }
            else
            {
                temp=temp->ZNext_;
            }
        }

    }
}
bool BitTrie::exist(int val)
{
    //cout<<"insert"<<val<<endl;
    int value=0;
    BitNode* temp=Root_;
    //cout<<"height_"<<height_<<endl;
    for(int i=0; i<=height_-1; i++)
    {
        if(val&(1<<i))
        {
            if(temp->ONext_)
            {
                temp=temp->ONext_;
                value=(value<<1)+1;
            }
            else
            {
                // cout<<"value"<<value<<endl;
                return false;
            }

        }
        else
        {
            if(temp->ZNext_)
            {
                temp=temp->ZNext_;
                value=(value<<1);
            }
            else
            {
                //cout<<"value"<<value<<endl;
                return false;

            }
        }

    }
    //cout<<"value"<<value<<endl;
    return true;
}

};

