/*!
 \file      AutoFillSubManager.h
 \author    Hongliang.Zhao&&Qiang.Wang
 \brief     AutoFillSubManager will get the search item list which match the prefix of the keyword user typed.
 \dat	    2012-07-26
*/
#include "AutoFillChildManager.h"
#include <boost/scoped_ptr.hpp>

namespace sf1r
{

class AutoFillSubManager: public boost::noncopyable
{   
    typedef boost::tuple<uint32_t, uint32_t, izenelib::util::UString> ItemValueType;
    typedef std::pair<uint32_t, izenelib::util::UString> PropertyLabelType;

    boost::scoped_ptr<AutoFillChildManager> Child1_, Child2_;
public:
    AutoFillSubManager()
    {
        Child1_.reset(new AutoFillChildManager(false));
        Child2_.reset(new AutoFillChildManager(true));
    }

    ~AutoFillSubManager()
    {
    }
     
    bool Init(const CollectionPath& collectionPath, const std::string& collectionName, const string& cronExpression)
    {    
         bool ret1 = Child1_->Init(collectionPath, collectionName, cronExpression, "child1");
         bool ret2 = Child2_->Init(collectionPath, collectionName, cronExpression, "child2" );
         return ret1&&ret2;
    }

    bool buildIndex(const std::list<ItemValueType>& queryList, const std::list<PropertyLabelType>& labelList)//empty, because it will auto update each 24hours,don't need rebuild for all  
    {   
         return true;
    }
    bool getAutoFillList(const izenelib::util::UString& query, std::vector<std::pair<izenelib::util::UString,uint32_t> >& list)//自动填充
    {    
         bool ret1 = Child1_->getAutoFillList(query,list);
         bool ret2 = Child2_->getAutoFillList(query,list);
         return ret1&&ret2;
    }

};


}
