/*!
 \file      AutoFillSubManager.h
 \author    Hongliang.Zhao&&Qiang.Wang
 \brief     AutoFillSubManager will get the search item list which match the prefix of the keyword user typed.
 \dat	    2012-07-26
*/
#include "AutoFillChildManager.h"

namespace sf1r
{

class AutoFillSubManager: public boost::noncopyable
{   
    typedef boost::tuple<uint32_t, uint32_t, izenelib::util::UString> ItemValueType;
    typedef std::pair<uint32_t, izenelib::util::UString> PropertyLabelType;

    AutoFillChildManager Child1,Child2;
public:
    AutoFillSubManager()
    {
       Child2.setSource(true);
    }

    ~AutoFillSubManager()
    {    	

    }
     
    bool Init(const std::string& fillSupportPath, const std::string& collectionName, const string& cronExpression)
    {    
         bool ret1 = Child1.Init(fillSupportPath + "/child1", collectionName, cronExpression);
         bool ret2 = Child2.Init(fillSupportPath + "/child2", collectionName, cronExpression);
         return ret1&&ret2;
    }

    bool buildIndex(const std::list<ItemValueType>& queryList, const std::list<PropertyLabelType>& labelList)//empty, because it will auto update each 24hours,don't need rebuild for all  
    {   
         return true;
    }
    bool getAutoFillList(const izenelib::util::UString& query, std::vector<std::pair<izenelib::util::UString,uint32_t> >& list)//自动填充
    {    
         std::vector<std::pair<izenelib::util::UString,uint32_t> > list2;
         bool ret1 = Child1.getAutoFillList(query,list);
         bool ret2 = Child2.getAutoFillList(query,list);
         return ret1&&ret2;
    }

};


}
