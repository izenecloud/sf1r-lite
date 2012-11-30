#include "GroupMiningTask.h"
#include "DateStrParser.h"
#include <mining-manager/util/split_ustr.h>
#include <mining-manager/util/FSUtil.hpp>
#include <mining-manager/MiningException.hpp>
#include <iostream>
#include <cassert>

using namespace sf1r::faceted;
	namespace
	{
	const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;
	}

/*template<class PropVauleType>
	GroupMiningTask<PropVauleType>::GroupMiningTask(
			DocumentManager& documentManager
			, const GroupConfigMap& groupConfigMap
			, PropVauleType& propValueTable)
		:documentManager_(documentManager)
		, groupConfigMap_(groupConfigMap)
		, propValueTable_(propValueTable)
		, dateStrParser_(*DateStrParser::get())
	{

	}

template<class PropVauleType>
	GroupMiningTask<PropVauleType>::~GroupMiningTask()
	{

	}
template<class PropVauleType>
	docid_t GroupMiningTask<PropVauleType>::getLastDocId()
	{
		return propValueTable_.docIdNum();
	}

template<class PropVauleType>
	bool GroupMiningTask<PropVauleType>::buildDocment(docid_t docID, Document& doc)
	{
		izenelib::util::UString propValue;
        doc.getProperty(propValueTable_.propName(), propValue);
        buildDoc_(docID, propValue, propValueTable_);//return
        return true;
	}
template<class PropVauleType>
	void GroupMiningTask<PropVauleType>::preProcess()
	{
		const docid_t endDocId = documentManager_.getMaxDocId();
		docid_t startDocId = propValueTable_.docIdNum();
		if (startDocId > endDocId)
            return;
        if (isRebuildProp_(propValueTable_.propName()))
       	{
            LOG(INFO) << "**clear old group data to rebuild property: " << propValueTable_.propName() <<endl;
   	    }
        cout<<propValueTable_.propName()<<", ";
        propValueTable_.reserveDocIdNum(endDocId + 1);
	}
template<class PropVauleType>
	void GroupMiningTask<PropVauleType>::postProcess()
	{
		if (!propValueTable_.flush())
       	{
            LOG(ERROR) << "propTable.flush() failed, property name: " << propValueTable_.propName();
        }
	}
template<class PropVauleType>
    void GroupMiningTask<PropVauleType>::buildDoc_(
        docid_t docId,
        const izenelib::util::UString propValue,
        DateGroupTable& dateTable)
	{
	    DateGroupTable::DateSet dateSet;
	    
	    {
	        std::vector<vector<izenelib::util::UString> > groupPaths;
	        split_group_path(propValue, groupPaths);

	        DateGroupTable::date_t dateValue;
	        std::string dateStr;
	        std::string errorMsg;

	        for (std::vector<vector<izenelib::util::UString> >::const_iterator pathIt = groupPaths.begin();
	            pathIt != groupPaths.end(); ++pathIt)
	        {
	            if (pathIt->empty())
	                continue;

	            pathIt->front().convertString(dateStr, ENCODING_TYPE);
	            if (dateStrParser_.scdStrToDate(dateStr, dateValue, errorMsg))
	            {
	                dateSet.insert(dateValue);
	            }
	            else
	            {
	                LOG(WARNING) << errorMsg;
	            }
	        }
	    }

	    try
	    {
	        dateTable.appendDateSet(dateSet);
	    }
	    catch(MiningException& e)
	    {
	        LOG(ERROR) << "exception: " << e.what()
	                   << ", doc id: " << docId;
	    }
	}
template<class PropVauleType>
	void GroupMiningTask<PropVauleType>::buildDoc_(
	    docid_t docId,
	    const izenelib::util::UString propValue,
	    PropValueTable& pvTable)
	{ 
	    std::vector<PropValueTable::pvid_t> propIdList;

	    {
	        std::vector<vector<izenelib::util::UString> > groupPaths;
	        split_group_path(propValue, groupPaths);

	        try
	        {
	            for (std::vector<vector<izenelib::util::UString> >::const_iterator pathIt = groupPaths.begin();
	                pathIt != groupPaths.end(); ++pathIt)
	            {
	                PropValueTable::pvid_t pvId = pvTable.insertPropValueId(*pathIt);
	                propIdList.push_back(pvId);
	            }
	        }
	        catch (MiningException& e)
	        {
	            LOG(ERROR) << "exception: " << e.what()
	                       << ", doc id: " << docId;
	        }
	    }

	    try
	    {
	        pvTable.appendPropIdList(propIdList);
	    }
	    catch (MiningException& e)
	    {
	        LOG(ERROR) << "exception: " << e.what()
	                   << ", doc id: " << docId;
	    }
	}
template<class PropVauleType>
	bool GroupMiningTask<PropVauleType>::isRebuildProp_(const std::string& propName) const
	{
	    GroupConfigMap::const_iterator it = groupConfigMap_.find(propName);

	    if (it != groupConfigMap_.end())
	        return it->second.isRebuild();

	    return false;
	}*/
