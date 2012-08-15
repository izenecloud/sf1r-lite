#include "AutoFillChildManager.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <util/singleton.h>
#include <idmlib/util/directory_switcher.h>
#include <dirent.h>
#include <stdlib.h>
#include <process/common/XmlConfigParser.h>
#include <util/driver/Controller.h>

using namespace std;
using namespace izenelib::util;
using namespace izenelib::am;
namespace sf1r
{
AutoFillChildManager::AutoFillChildManager()
{
    isUpdating_ = false;
    isUpdating_Wat_ = false;
    isIniting_ = false;
    fromSCD_ = false;

    updatelogdays_ = 1;
    alllogdays_ =  80;
    topN_ =  10;
    updateMin_ = 30;
    updateHour_ = 2;
    QN_ = new QueryNormalize();
}

AutoFillChildManager::~AutoFillChildManager()
{
    closeDB();
    delete QN_;
}

void AutoFillChildManager::SaveItem()
{
    fstream out1;
    if(!ItemVector_.empty())
    {
        out1.open(ItemPath_.c_str(), ios::out);
        if(out1.is_open())
        {
            out1<<ItemVector_.size()<<endl;
            std::vector<ItemType>::iterator it;
            for(it = ItemVector_.begin(); it != ItemVector_.end(); it++)
            {
                out1<<(*it).strItem_<<endl;
            }
            out1.close();
        }
        else
        {
            LOG(ERROR)<<"open saveItem file error"<<endl;
        }
    }
}

void AutoFillChildManager::LoadItem()
{
    ifstream in;
    in.open(ItemPath_.c_str(), ios::in);
    if(in.is_open())
    {
        uint32_t size;
        std::string sizestr;
        getline(in, sizestr);
        size = boost::lexical_cast<uint32_t>(sizestr);
        for(uint32_t k = 0; k < size; k++)
        {
            ItemType item;
            std::string temp;
            getline(in, temp);
            item.strItem_=temp;
            item.offset_ = 0;
            ItemVector_.push_back(item);
        }
        in.close();
    }
    else
    {
        LOG(ERROR)<<"open LoadItem file error"<<endl;
    }
}

void AutoFillChildManager::setCollectionName(const std::string& collectionName)
{
    collectionName_ = collectionName;
}

void AutoFillChildManager::setSource(bool fromSCD)
{
    fromSCD_ = fromSCD;
}

bool AutoFillChildManager::Init(const std::string& fillSupportPath)
{
    AutofillPath_ = fillSupportPath;
    if (!boost::filesystem::is_directory(AutofillPath_))
    {
        boost::filesystem::create_directories(AutofillPath_);
    }

    leveldbPath_ = fillSupportPath + "/leveldb";
    ItemdbPath_ = fillSupportPath + "/itemdb";
    string IDPath = AutofillPath_ + "/id/";
    ItemPath_ = leveldbPath_ + "/Itemlist.list";
    SCDLogPath_=AutofillPath_ +"/SCDLog";
    char* s = new char[512];
    if(!getcwd(s, 512))
    {
        LOG(ERROR)<<"get root dir error"<<endl;
        return false;
    }
    string rootdir(s);
    delete [] s;
    SCDDIC_ = rootdir + "/collection/" + collectionName_ + "/scd/autofill";//TODO

    /*string dictionaryFile = SF1Config::get()->laDictionaryPath_;
    if(dictionaryFile.rfind("/") != dictionaryFile.length()-1)
    {
        dictionaryFile += "/QUERYNORMALIZE";
    }
    else
    {
        dictionaryFile += "QUERYNORMALIZE";
    }*/ //use SF1Config cause the complie wrong of test/"

    QN_->load("../b5m/QUERYNORMALIZE");
    bool leveldbBuild = false;
    try
    {
        if (!boost::filesystem::is_directory(leveldbPath_))
        {
            boost::filesystem::create_directories(leveldbPath_);

            if (!boost::filesystem::is_directory(ItemdbPath_))
            {
                boost::filesystem::create_directories(ItemdbPath_);
            }

            if (!boost::filesystem::is_directory(IDPath))
            {
                boost::filesystem::create_directories(IDPath);
            }

            if (!boost::filesystem::is_directory(SCDDIC_))
            {
                boost::filesystem::create_directories(SCDDIC_);
            }
        }
        else
        {
            if (boost::filesystem::is_directory(IDPath) && boost::filesystem::is_directory(ItemdbPath_) && boost::filesystem::exists(ItemPath_))
            {
                leveldbBuild = true;
            }
        }
    }
    catch (boost::filesystem::filesystem_error& e)
    {
        LOG(ERROR)<<"Path does not exist. Path "<<leveldbPath_;
    }

    idManager_ = new IDManger(IDPath);

    std::string temp = fillSupportPath + "/AutoFill.log";
    out.open(temp.c_str(), ios::out);

    if(!openDB(leveldbPath_, ItemdbPath_))
        return false;

    if(leveldbBuild&&(!fromSCD_))
    {
        InitWhileHaveLeveldb();
    }
    else
    {
        if(!RealInit())
        {
            return false;
        }
    }
    isIniting_ = false;

    return true;
}

bool AutoFillChildManager::InitWhileHaveLeveldb()
{
    std::string IDPath = AutofillPath_ + "/id/";
    LoadItem();
    isUpdating_Wat_ = true;
    buildWat_array(true);
    isUpdating_Wat_ = false;
    boost::function0<void> f =  boost::bind(&AutoFillChildManager::doUpdate_Thread, this);
    boost::thread thrd(f);
    return true;
}

bool AutoFillChildManager::RealInit()
{
    isIniting_ = true;
    std::string IDPath = AutofillPath_+"/id/";
    try
    {
        if (!boost::filesystem::is_directory(IDPath))
        {
            boost::filesystem::create_directories(IDPath);
        }
    }
    catch (boost::filesystem::filesystem_error& e)
    {
        LOG(ERROR)<<"Path does not exist. Path "<<IDPath;
    }

    if(fromSCD_)
    {
        return InitFromSCD();
    }
    else
    {
        return InitFromLog();
    }

    isIniting_ = false;

    return true;
}

void AutoFillChildManager::LoadSCDLog()
{
    ifstream in;
    in.open(SCDLogPath_.c_str(), ios::in);
    if(in.is_open())
    {
        while(!in.eof())
        {
            std::string temp;
            getline(in, temp);
            SCDHaveDone.push_back(temp);
        }
        in.close();
    }
    else
    {
        LOG(ERROR)<<"open LoadSCDLog file error"<<endl;
    }
}

void AutoFillChildManager::SaveSCDLog()
{
    fstream out1;
    if(!SCDHaveDone.empty())
    {
        out1.open(SCDLogPath_.c_str(), ios::out);
        if(out1.is_open())
        {
            for(vector<string>::iterator it =SCDHaveDone.begin(); it != SCDHaveDone.end(); it++)
            {
                out1<<(*it)<<endl;
            }
            out1.close();
        }
        else
        {
            LOG(ERROR)<<"open SaveSCDLog file error"<<endl;
        }
    }
}

bool AutoFillChildManager::InitFromSCD()
{
    if(boost::filesystem::exists(SCDLogPath_))
    {
        LoadSCDLog();
    }
    //updateFromSCD();
    boost::function0<void> f =  boost::bind(&AutoFillChildManager::doUpdate_Thread, this);
    boost::thread thrd(f);
    return true;
}

void AutoFillChildManager::updateFromSCD()
{
    vector<string> files;
    DIR* dp;
    dirent *d;
    dp = opendir(SCDDIC_.c_str());
    if(dp == NULL)
    {
        return;
    }
    else
    {
        while ((d = readdir(dp)) != NULL)
        {
            if (!string(d->d_name).compare(".") || !string(d->d_name).compare(".."))
                continue;
            string filename = string(d->d_name);
            int len = filename.length();
            if(filename[len - 1] == 'D' && filename[len -2] == 'C' && filename[len - 3] == 'S')
            {
                vector<string>::iterator result = find(SCDHaveDone.begin( ), SCDHaveDone.end( ), filename);
                if(result == SCDHaveDone.end())
                {
                    files.push_back(filename);
                    SCDHaveDone.push_back(filename);
                }
            }
        }
        vector<string>::const_iterator it;
        list<ItemValueType> querylist;
        for(it = files.begin(); it != files.end(); it++)
        {
            ifstream in;
            string filename = SCDDIC_ + "/" + *it;
            in.open(filename.c_str(), ios::in);

            if (in.good())
            {
                std::string temp;
                string title = "<Title>";
                while(!in.eof())
                {
                    getline(in, temp);
                    if(temp.substr(0, title.size()) == title)
                    {
                        izenelib::util::UString UStringQuery_(temp.substr(title.size()),izenelib::util::UString::UTF_8);
                        querylist.push_back(boost::make_tuple(0, 0, UStringQuery_));
                    }
                }
                in.close();

            }
            else
            {
                in.close();
                SaveSCDLog();
                return;
            }
        }
        std::list<PropertyLabelType> labelList;
        buildIndex(querylist, labelList);
        SaveSCDLog();
        return;
    }
}

bool AutoFillChildManager::InitFromLog()
{
    boost::posix_time::ptime time_now = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::ptime p = time_now - boost::gregorian::days(alllogdays_);
    std::string time_string = boost::posix_time::to_iso_string(p);
    std::vector<UserQuery> query_records;

    UserQuery::find(
        "query ,max(hit_docs_num) as hit_docs_num, count(*) as count ",
        "collection = '" + collectionName_ + "' and hit_docs_num > 0 AND TimeStamp >= '" + time_string + "'",
        "query",
        "",
        "",
        query_records);
    list<ItemValueType> querylist;
    std::vector<UserQuery>::const_iterator it = query_records.begin();
    for (; it != query_records.end(); ++it)
    {
        izenelib::util::UString UStringQuery_(it->getQuery(),izenelib::util::UString::UTF_8);
        querylist.push_back(boost::make_tuple(it->getCount(), it->getHitDocsNum(), UStringQuery_));
    }
    std::list<PropertyLabelType> labelList;
    buildIndex(querylist, labelList);
    boost::function0<void> f =  boost::bind(&AutoFillChildManager::doUpdate_Thread, this);
    boost::thread thrd(f);
    return true;
}

bool AutoFillChildManager::openDB(string Tablepath, string Itempath)
{
    return dbTable_.open(Tablepath)&&dbItem_.open(Itempath);
}

void AutoFillChildManager::closeDB()
{
    dbTable_.close();
    dbItem_.close();
}

bool AutoFillChildManager::buildDbIndex(const std::list<QueryType>& queryList)
{
    std::list<QueryType>::const_iterator it;
    for(it = queryList.begin(); it != queryList.end(); it++)
    {
        std::vector<izenelib::util::UString> pinyins;
        FREQ_TYPE freq = (*it).freq_;
        uint32_t HitNum = (*it).HitNum_;
        std::string strT=(*it).strQuery_;
        QN_->query_Normalize(strT);
        izenelib::util::UString UStringQuery_(strT,izenelib::util::UString::UTF_8);
        QueryCorrectionSubmanager::getInstance().getRelativeList(UStringQuery_, pinyins);
        std::vector<izenelib::util::UString>::const_iterator itv;

        uint32_t pinyinlen= 0;
        for(itv = pinyins.begin(); itv != pinyins.end(); itv++)
        {
            pinyinlen++;
            if(pinyinlen >= 30)
                break;
            std::string pinyin, value;
            (*itv).convertString(pinyin, izenelib::util::UString::UTF_8);
            buildItemList(pinyin);

            if(!dbTable_.get_item(pinyin, value));
            //return false;
            if(!dbTable_.delete_item(pinyin));
            //return false;
            if(value.length() == 0)
            {
                assert(minToNFreq.length() == TOPN_LEN);
                ValueType newValue;
                std::string value, firstvalue;
                firstvalue = "0000";
                newValue.toValueType(strT, freq, HitNum);
                newValue.toString(value);
                firstvalue.append(value);
                if(!dbTable_.add_item(pinyin, firstvalue));
                //	return false;
            }
            else
            {
                uint32_t len = value.length();
                const char* str = value.data() + TOPN_LEN;
                uint32_t offset = TOPN_LEN;
                while(offset < len)
                {
                    ValueType newValue;
                    newValue.getValueType(str);
                    string strA=newValue.strValue_;
                    string strB = strT;
                    //boost::algorithm::replace_all(strA, " ", "");
                   // boost::algorithm::replace_all(strB, " ", "");
                    //transform(strA.begin(), strA.end(), strA.begin(), ::tolower);
                   // transform(strB.begin(), strB.end(), strB.begin(), ::tolower);
                    if(strA == strB)
                    {
                        FREQ_TYPE freq_new = freq + *(FREQ_TYPE*)(str + *(uint32_t*)str - FREQ_TYPE_LEN  - UINT32_LEN);

                        *(FREQ_TYPE*)(str + *(uint32_t*)str - FREQ_TYPE_LEN  - UINT32_LEN) = freq_new;
                        *(uint32_t*)(str + *(uint32_t*)str - UINT32_LEN) = HitNum;
                        //buildTopNDbTable(value, offset);
                        if(!dbTable_.add_item(pinyin, value));
                        //			return false;
                        break;
                    }
                    offset += *(uint32_t*)str;
                    str += *(uint32_t*)str;
                }
                if(offset == len)
                {
                    ValueType newValue;
                    newValue.toValueType(strT, freq, HitNum);
                    std::string newValueStr;
                    newValue.toString(newValueStr);
                    value.append(newValueStr);
                    //buildTopNDbTable(value, offset);
                    if(!dbTable_.add_item(pinyin, value));
                    //		return false;
                }
            }
        }
    }
    buildItemVector();
    return true;
}

void AutoFillChildManager::buildItemList(std::string key)
{
    ItemType item;
    item.strItem_ = key;
    item.offset_ = 0;
    ItemVector_.push_back(item);
}

void AutoFillChildManager::buildItemVector()
{
    std::sort(ItemVector_.begin(), ItemVector_.end());
    vector<ItemType>::iterator iter = unique(ItemVector_.begin(), ItemVector_.end());
    ItemVector_.erase(iter, ItemVector_.end());
}

void AutoFillChildManager::buildTopNDbTable(std::string &value, const uint32_t offset)//compare with the least one of topN
{
    string  MinFreqstr= value.substr(0, TOPN_LEN);
    string  str0=MinFreqstr;
    uint32_t MinFreq;
    if(MinFreqstr == "0000")
    {
        MinFreq = 0;
    }
    else
    {
        MinFreq = *(uint32_t*)(MinFreqstr.data());
    }

    string  sizestr = value.substr(offset, FREQ_TYPE_LEN);
    uint32_t sizeinoffset = *(uint32_t*)(sizestr.data());
    string  freqstr = value.substr(offset + sizeinoffset - HITNUM_LEN  - UINT32_LEN, FREQ_TYPE_LEN);
    uint32_t freq = *(uint32_t*)(freqstr.data());

    string singlefreq;
    uint32_t size = 0;
    uint32_t ThisPos = TOPN_LEN;
    uint32_t ThisFreq = freq + 1;
    uint32_t ThisOrder = 0;
    if(freq < MinFreq)
    {
    }
    else
    {
        while(ThisPos < offset && freq < ThisFreq)
        {
            ThisOrder++;
            ThisPos = ThisPos+size;
            sizestr = value.substr(ThisPos, FREQ_TYPE_LEN );
            size = *(uint32_t*)(sizestr.data());
            singlefreq = value.substr(ThisPos + size - HITNUM_LEN  - UINT32_LEN, FREQ_TYPE_LEN);
            ThisFreq = *(uint32_t*)(singlefreq.data());
        }
        uint32_t ThisPos2 = ThisPos;

        while( ThisOrder <= topN_-1 && ThisPos2 < value.length())//
        {
            ThisOrder++;
            ThisPos2 = ThisPos2+size;
            sizestr = value.substr( ThisPos2 , 4 );
            size = *(uint32_t*)(sizestr.data());
        }

        string str1,str2,str3,str4;
        if(ThisPos2<value.length())
        {
            str0 = value.substr(ThisPos2 + size - HITNUM_LEN  - UINT32_LEN, FREQ_TYPE_LEN);
        }
        str1 = value.substr(TOPN_LEN, ThisPos - TOPN_LEN);
        str2 = value.substr(ThisPos, offset-ThisPos );
        str3 = value.substr(offset, sizeinoffset );
        str4 = value.substr(offset + sizeinoffset);
        value = str0 + str1 + str3 + str2 + str4;
    }
}

bool AutoFillChildManager::getAutoFillListFromOffset(uint64_t OffsetStart, uint64_t OffsetEnd, std::vector<std::pair<izenelib::util::UString,uint32_t> >& list)
{
    izenelib::util::UString tempUString;
    uint64_t ValueID = 0;
    for(uint64_t i = OffsetStart; i <= OffsetEnd; i++)
    {
        ValueID = wa_.Lookup(i);
        idManager_->getDocNameByDocId(ValueID, tempUString);
        std::string str;
        tempUString.convertString(str, izenelib::util::UString::UTF_8);
        std::size_t pos = str.find("/");
        uint32_t HitNum ;
        string strquery;
        string HitNumStr;
        if(pos == 0)
            continue;
        std::string portStr;
        if(pos == std::string::npos)
        {
            continue;
        }
        else
        {
            HitNumStr = str.substr(0, pos);
            strquery = str.substr(pos+1);
        }
        HitNum=boost::lexical_cast<int>(HitNumStr);
        izenelib::util::UString Ustr(strquery, izenelib::util::UString::UTF_8);
        list.push_back(std::make_pair(Ustr, HitNum));
    }
    return true;
}

bool AutoFillChildManager::getAutoFillListFromWat(const izenelib::util::UString& queryOrgin, std::vector<std::pair<izenelib::util::UString,uint32_t> >& list)
{
    izenelib::util::UString query=izenelib::util::Algorithm<izenelib::util::UString>::trim1(queryOrgin);
    if (query.length() == 0)
        return false;
    izenelib::util::UString tempQuery;
    bool ret = false;
    uint64_t OffsetStart;
    uint64_t OffsetEnd;
    std::string strQuery;
    bool HaveSearched = false;
    query.convertString(strQuery, izenelib::util::UString::UTF_8);

    if (query.isAllChineseChar())
    {
        HaveSearched = getOffset(strQuery, OffsetStart, OffsetEnd);
        if(HaveSearched)
        {
            ret = getAutoFillListFromOffset(OffsetStart, OffsetEnd, list);
        }
    }
    else if (izenelib::util::ustring_tool::processKoreanDecomposerWithCharacterCheck<
             izenelib::util::UString>(query, tempQuery))
    {
        HaveSearched = getOffset(strQuery, OffsetStart, OffsetEnd);
        if(HaveSearched)
        {
            ret = getAutoFillListFromOffset(OffsetStart, OffsetEnd, list);
        }
    }
    else
    {
        if (query.includeChineseChar())
        {
            HaveSearched = getOffset(strQuery, OffsetStart, OffsetEnd);
            if(HaveSearched)
            {
                ret = getAutoFillListFromOffset(OffsetStart, OffsetEnd, list);
            }
            else//Hybrid
            {
                std::vector<izenelib::util::UString> pinyins;
                QueryCorrectionSubmanager::getInstance().getPinyin2(query, pinyins);
                std::vector<std::pair<izenelib::util::UString,uint32_t> > tempList;
                for (uint32_t j = 0; j < pinyins.size(); j++)
                {
                    tempList.clear();
                    pinyins[j].convertString(strQuery, izenelib::util::UString::UTF_8);
                    HaveSearched = getOffset(strQuery, OffsetStart, OffsetEnd);
                    if(HaveSearched)
                    {
                        getAutoFillListFromOffset(OffsetStart, OffsetEnd, tempList);
                        if(!tempList.empty())
                        {
                            list.insert(list.end(), tempList.begin(), tempList.end());
                        }
                    }
                }

                query.filter(list);
                std::vector<std::pair<izenelib::util::UString,uint32_t> >::iterator iter;
                for (iter=list.begin(); iter!=list.end(); )
                {
                    if((*iter).first==query)
                    {
                        iter=list.erase(iter);
                    }
                    else
                        iter++;
                }
                query.KeepOrderDuplicateFilter(list);//不匹配的过滤；
            }
        }
        else
        {
            HaveSearched = getOffset(strQuery, OffsetStart, OffsetEnd);

            if(HaveSearched)
            {
                ret=getAutoFillListFromOffset(OffsetStart, OffsetEnd, list);
            }

        }
    }
    std::vector<std::pair<izenelib::util::UString,uint32_t> >::iterator iter=list.begin();
    if(query.includeChineseChar()&&list.size()<topN_)
    {
        std::vector<izenelib::util::UString> pinyins;
        QueryCorrectionSubmanager::getInstance().getPinyin2(query, pinyins);
        std::vector<std::pair<izenelib::util::UString,uint32_t> > tempList;
        for (uint32_t j = 0; j < pinyins.size(); j++)
        {
            tempList.clear();
            pinyins[j].convertString(strQuery, izenelib::util::UString::UTF_8);
            HaveSearched = getOffset(strQuery, OffsetStart, OffsetEnd);
            if(HaveSearched)
            {
                getAutoFillListFromOffset(OffsetStart, OffsetEnd, tempList);
                if(!tempList.empty())
                {
                    list.insert(list.end(), tempList.begin(), tempList.end());
                }
            }
        }
        query.FuzzyFilter(list);
        query.KeepOrderDuplicateFilter(list);
    }

    if(!list.empty())
    {
        for (iter=list.begin(); iter!=list.end(); )
        {
            if( (*iter).first==query)
            {
                iter=list.erase(iter);
            }
            else
                iter++;
        }
    }
    ret = !list.empty();
    return ret;
}

bool AutoFillChildManager::getAutoFillListFromDbTable(const izenelib::util::UString& queryOrgin, std::vector<std::pair<izenelib::util::UString,uint32_t> >& list)
{
    izenelib::util::UString query=izenelib::util::Algorithm<izenelib::util::UString>::trim1(queryOrgin);
    string value;
    dbTable_.get_item(query, value);
    const char* valuestr= value.data() + TOPN_LEN;
    string sizestr;
    string singlevalue;
    uint32_t size = 0;
    uint32_t count = 0;
    uint32_t NextStartPos = TOPN_LEN;
    uint32_t HitNum = 0;
    bool ret = false;
    while( NextStartPos < value.length() )
    {
        if(count >= topN_)
            break;
        size = *(uint32_t*)valuestr;
        singlevalue = value.substr(NextStartPos + UINT32_LEN, size - UINT32_LEN - HITNUM_LEN- FREQ_TYPE_LEN);
        HitNum = *(uint32_t*)(valuestr + size - FREQ_TYPE_LEN);
        izenelib::util::UString  tempsinglevalue(singlevalue, izenelib::util::UString::UTF_8);
        list.push_back(std::make_pair(tempsinglevalue, HitNum));
        NextStartPos = NextStartPos + size;
        valuestr += size;
        count++;
        ret = true;
    }
    return ret;
}

bool AutoFillChildManager::getAutoFillList(const izenelib::util::UString& query, std::vector<std::pair<izenelib::util::UString,uint32_t> >& list)
{
    std::string strquery;
    query.convertString(strquery, izenelib::util::UString::UTF_8);
    QN_->query_Normalize(strquery);
    izenelib::util::UString  queryLow(strquery, izenelib::util::UString::UTF_8);

    if(!isIniting_)
    {
        if( isUpdating_ && isUpdating_Wat_)
            return getAutoFillListFromDbTable(queryLow , list);
        else
            return getAutoFillListFromWat(queryLow, list);
    }
    else
    {
        if( isUpdating_Wat_)
        {
            return getAutoFillListFromDbTable(queryLow , list);
        }
    }
    return true;
}

void AutoFillChildManager::buildWat_array(bool _fromleveldb)
{
    vector<uint64_t> A;
    std::vector<ItemType>::iterator it;
    std::string itemValue, value, IDstring;
    uint64_t ID = 0;
    uint32_t offsettmp = 0;
    if(!_fromleveldb)
        dbItem_.clear();
    for(it = ItemVector_.begin(); it != ItemVector_.end(); it++)
    {
        itemValue = (*it).strItem_;
        dbTable_.get_item(itemValue, value);
        const char* str = value.data() + TOPN_LEN;
        uint32_t len = value.length();
        uint32_t count = 0;
        for(uint32_t offset = TOPN_LEN; offset < len;)
        {
            if(count >= topN_)
                break;
            ValueType newValue;
            newValue.getValueType(str);
            newValue.getHitString(IDstring);
            izenelib::util::UString tempValue(IDstring, izenelib::util::UString::UTF_8);
            idManager_->getDocIdByDocName(tempValue, ID);

            A.push_back(ID);
            count++;
            offset += *(uint32_t*)str;
            str += *(uint32_t*)str;
        }
        (*it).offset_ = offsettmp;
        offsettmp += count;
        if(!_fromleveldb)
        {
            string offsetstring = boost::lexical_cast<string>((*it).offset_) + "/" + boost::lexical_cast<string>(count);
            //dbItem_.delete_item(itemValue);
            dbItem_.add_item(itemValue, offsetstring);
        }
    }
    wa_.Init(A);
    SaveItem();
    vector<ItemType>().swap(ItemVector_);
}

bool AutoFillChildManager::getOffset(const std::string& query, uint64_t& OffsetStart, uint64_t& OffsetEnd)//
{
    OffsetStart = 1;
    OffsetEnd = 0;
    string offsetstr = "";
    dbItem_.get_item(query, offsetstr);
    if(offsetstr.length() > 0)
    {
        std::size_t pos = offsetstr.find("/");
        string offsetbegin = offsetstr.substr(0, pos);
        string offsetcount = offsetstr.substr(pos+1);
        try
        {
            OffsetStart = boost::lexical_cast<uint32_t>(offsetbegin);
            OffsetEnd = boost::lexical_cast<uint32_t>(offsetcount) + OffsetStart - 1;
        }
        catch(boost::bad_lexical_cast& e)
        {
        }
    }
    return true;
}

void AutoFillChildManager::updateAutoFill(uint32_t days)
{
    isUpdating_ = true;
    if(!fromSCD_)
    {
        updateFromLog(days);
    }
    else
    {
        updateFromSCD();
    }
    isUpdating_Wat_ = false;
    isUpdating_ = false;
}

void AutoFillChildManager::updateFromLog(uint32_t days)
{

    boost::posix_time::ptime time_now = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::ptime p = time_now - boost::gregorian::days(days);
    std::string time_string = boost::posix_time::to_iso_string(p);
    std::vector<UserQuery> query_records;

    UserQuery::find(
        "query ,max(hit_docs_num) as hit_docs_num, count(*) as count ",
        "collection = '" + collectionName_ + "' and hit_docs_num > 0 AND TimeStamp >= '" + time_string + "'",
        "query",
        "",
        "",
        query_records);
    list<QueryType> querylist;
    std::vector<UserQuery>::const_iterator it = query_records.begin();
    for ( ; it != query_records.end(); ++it)
    {
        QueryType TempQuery;
        TempQuery.strQuery_ = it->getQuery();
        TempQuery.freq_ = it->getCount();
        TempQuery.HitNum_ = it->getHitDocsNum();
        querylist.push_back(TempQuery );
    }
    buildDbIndex(querylist);
    isUpdating_Wat_ = true;
    LoadItem();
    wa_.Clear();
    buildWat_array(false);//xxx build new Itemdb_ when build new

}

void AutoFillChildManager::doUpdate_Thread()
{
    uint32_t x,h,m;
    bool inited = false;
    while(1)
    {
        x=time(NULL);
        h=(x/3600%24+8)%24;
        m=x%3600/60;
        if(h == updateHour_ && m >= updateMin_ && m < (updateMin_ + 15) && inited == true)
        {
            updateAutoFill(updatelogdays_);
            out<<"finish one update at:"<<h<<":"<<m<<endl;
            sleep(900);
        }
        else
        {
            inited = true;
            sleep(120);
        }
    }
}

bool AutoFillChildManager::buildIndex(const std::list<ItemValueType>& queryList, const std::list<PropertyLabelType>& labelList)
{
    QueryType Query;
    std::list<QueryType> queryLists;
    std::list<ItemValueType>::const_iterator it;
    uint32_t count = 0;
    for(it = queryList.begin(); it != queryList.end(); it++)
    {
        count++;
        std::string str;
        (*it).get<2>().convertString(str, izenelib::util::UString::UTF_8);
        Query.strQuery_ = str;
        Query.freq_ = (FREQ_TYPE)(*it).get<0>();
        Query.HitNum_ = (*it).get<1>();
        queryLists.push_back(Query);
    }

    buildDbIndex(queryLists);
    isUpdating_Wat_ = true;
    buildWat_array(false);
    isUpdating_Wat_ = false;
    return true;
}
};
