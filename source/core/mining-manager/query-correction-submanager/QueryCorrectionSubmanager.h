/**
 * @brief  Defination of QueryCorrectionSubmanager. If user inputs a wrong query, it can be used to get correct candidates and select the best one.
 * @author  Jinglei Zhao & Jinli Liu
 * @date Sep 15,2009
 * @version v3.0
 * ================
 * log:
 * 2009.02.23 Peisheng Wang
 *   --Using log-manager to enhance queryCorrection Manager
 *   --Integrate Chinese query correcion
 *  
 *  
 */
#ifndef _QUERY_CORRECTION_SUBMANAGER_H_
#define _QUERY_CORRECTION_SUBMANAGER_H_
#include <log-manager/LogManager.h>
#include <query-manager/QueryManager.h>
#include "ChineseQueryCorrection.h"
#include "EkQueryCorrection.h"
#include <boost/noncopyable.hpp>
#include <sdb/SequentialDB.h>

namespace sf1r {

class _QueryCorrectionSubmanagerParam
{
public:
    _QueryCorrectionSubmanagerParam()
    {
    }
    std::string path_;
    std::string workingPath_;
    bool enableEK_;
    bool enableChn_;

};

class QueryCorrectionSubmanagerParam
{
public:
    static _QueryCorrectionSubmanagerParam param_;
    
    static _QueryCorrectionSubmanagerParam& get()
    {
        return param_;
    }
    
    static void set(const std::string& path, const std::string& workingPath, bool enableEK, bool enableChn)
    {
        param_.path_ = path;
        param_.workingPath_ = workingPath;
        param_.enableEK_ = enableEK;
        param_.enableChn_ = enableChn;
    }
};


class QueryCorrectionSubmanager : public boost::noncopyable {
	static const int DEFAULT_MAX_EDITDISTANCE_ = 2;
private:
	QueryCorrectionSubmanager(const string& path, const std::string& workingPath, bool enableEK, bool enableChn, int ed=DEFAULT_MAX_EDITDISTANCE_);
public:
    
    static QueryCorrectionSubmanager& getInstance();
    
	~QueryCorrectionSubmanager();
	
	/**
	 * @brief The main interface to refine a user query,the query can be set of tokens
	 * @param queryUString         the inputed query of the user.
	 * @param refinedQueryUString  the corrected query, if no correction is done, this string is returned empty.
	 * @return true if success false if failed.
	 */

    bool getRefinedQuery(const UString& queryUString,
            UString& refinedQueryUString);

    bool getRefinedQuery(const std::string& collectionName, const UString& queryUString,
            UString& refinedQueryUString);
	
	bool getPinyin(const izenelib::util::UString& hanzis,
	        std::vector<izenelib::util::UString>& pinyin);
	
	bool pinyinSegment(const string& str,
		vector<string>& result) ;

	bool isPinyin(const izenelib::util::UString& str);
    
    void updateCogramAndDict(const std::list<std::pair<izenelib::util::UString, uint32_t> >& recentQueryList);
    
    void updateCogramAndDict(const std::string& collectionName, const std::list<std::pair<izenelib::util::UString, uint32_t> >& recentQueryList);

protected:
	
    /**
     * @brief Initialize some member variables  
     * @return true if success false if failed.
     */
    bool initialize();

	bool stopHere(const izenelib::util::UString& query);	

	bool inDict_(const izenelib::util::UString& ustr);
	
private:
    std::string path_;
    std::string workingPath_;
    bool enableEK_;
    bool enableChn_;

	/**
	 *  max edit distance used to canididae generation
	 */
	bool activate_;

	boost::mutex logMutex_;
	
	//Chinese CorrectionManager mgr_;
	ChineseQueryCorrection cmgr_;

	//English or Korean Query CorrectionManager
    EkQueryCorrection ekmgr_;
    
};

}
#endif/*_QUERY_CORRECTION_SUBMANAGER_H_*/
