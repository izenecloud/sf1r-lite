#ifndef SF1R_B5MMANAGER_BRANDDB_H_
#define SF1R_B5MMANAGER_BRANDDB_H_

#include <string>
#include <vector>
#include <product-manager/product_price.h>
#include "b5m_types.h"
#include "b5m_helper.h"
#include <types.h>
#include <am/succinct/fujimap/fujimap.hpp>
#include <am/tc/BTree.h>
#include <am/tc/Hash.h>
#include <am/leveldb/Table.h>
#include <ir/id_manager/IDManager.h>
#include <glog/logging.h>
#include <boost/unordered_map.hpp>
#include <am/sequence_file/ssfr.h>

namespace sf1r {


    class BrandDb
    {
    public:
        typedef uint128_t IdType;
        typedef uint32_t BidType;
        typedef PropertyValue::PropertyValueStrType StringType;
        typedef StringType::value_type CharType;
        typedef std::vector<BidType> BidList;
        typedef std::map<std::string, BidList> SourceType;
        typedef boost::unordered_map<std::string, double> BidScore;
        typedef izenelib::util::UString UString;

        //typedef izenelib::ir::idmanager::_IDManager<StringType, BidType,
               //izenelib::util::NullLock,
               //izenelib::ir::idmanager::EmptyWildcardQueryHandler<StringType, BidType>,
               //izenelib::ir::idmanager::UniqueIDGenerator<StringType, BidType>,
               //izenelib::ir::idmanager::HDBIDStorage<StringType, BidType> >  IdManager;
        typedef izenelib::ir::idmanager::_IDManager<UString, UString, BidType,
                   izenelib::util::NullLock,
                   izenelib::ir::idmanager::EmptyWildcardQueryHandler<UString, BidType>,
                   izenelib::ir::idmanager::UniqueIDGenerator<UString, BidType>,
                   izenelib::ir::idmanager::HDBIDStorage<UString, BidType>,
                   izenelib::ir::idmanager::UniqueIDGenerator<UString, uint64_t>,
                   izenelib::ir::idmanager::EmptyIDStorage<UString, uint64_t> > IdManager;


        typedef izenelib::am::succinct::fujimap::Fujimap<IdType, BidType> DbType;
        BrandDb(const std::string& path)
        : path_(path), db_path_(path+"/db"), id_path_(path+"/id"), source_path_(path+"/source")
          , tmp_path_(path+"/tmp")
        , db_(NULL), id_manager_(NULL), is_open_(false)
        {
        }

        ~BrandDb()
        {
            if(db_!=NULL)
            {
                delete db_;
            }
            if(id_manager_!=NULL)
            {
                delete id_manager_;
            }
        }

        bool is_open() const
        {
            return is_open_;
        }

        bool open()
        {
            if(is_open_) return true;
            boost::filesystem::create_directories(path_);
            if(boost::filesystem::exists(tmp_path_))
            {
                boost::filesystem::remove_all(tmp_path_);
            }
            db_ = new DbType(tmp_path_.c_str());
            db_->initFP(32);
            db_->initTmpN(30000000);
            if(boost::filesystem::exists(db_path_))
            {
                db_->load(db_path_.c_str());
            }
            boost::filesystem::create_directories(id_path_);
            id_manager_ = new IdManager(id_path_+"/id");
            izenelib::am::ssf::Util<>::Load(source_path_, source_);
            is_open_ = true;
            return true;
        }

        BidType set(const IdType& pid, const StringType& brand)
        {
            BidType bid;
            UString u = propstr_to_ustr(brand);
            id_manager_->getTermIdByTermString(u, bid);
            db_->setInteger(pid, bid);
            return bid;
        }

        void set_source(const StringType& brand, const BidType& bid)
        {
            std::vector<std::string> text_list;
            B5MHelper::SplitAttributeValue(propstr_to_str(brand), text_list);
            for(uint32_t i=0;i<text_list.size();i++)
            {
                source_[text_list[i]].push_back(bid);
            }
        }

        bool get(const IdType& pid, StringType& brand)
        {
            BidType bid = db_->getInteger(pid);
            if(bid==(BidType)izenelib::am::succinct::fujimap::NOTFOUND)
            {
                return false;
            }
            UString u;
            if(id_manager_->getTermStringByTermId(bid, u))
            {
                brand = ustr_to_propstr(u);
                return true;
            }
            return false;
        }

        bool get_source(const StringType& input, StringType& output)
        {
            std::string sinput = propstr_to_str(input);
            uint32_t begin = 0;
            BidScore bid_score;
            while(begin<sinput.length())
            {
                std::string found;
                uint32_t length = 1;
                while(true)
                {
                    if(begin+length>sinput.length()) break;
                    std::string sub = sinput.substr(begin, length);

                    SourceType::const_iterator it = source_.lower_bound(sub);
                    if(it==source_.end())
                    {
                        break;
                    }
                    if(sub==it->first)
                    {
                        found = sub;
                    }
                    else if(it->first.find(sub)==0)
                    {
                    }
                    else
                    {
                        break;
                    }
                    length++;
                }
                if(found.length()>0)
                {
                    //BidType bid;
                    //id_manager_->getTermIdByTermString(StringType(found, StringType::UTF_8), bid);
                    BidScore::iterator it = bid_score.find(found);
                    if(it==bid_score.end())
                    {
                        bid_score.insert(std::make_pair(found, 1.0));
                    }
                    else
                    {
                        it->second+=1.0;
                    }
                }
                begin+=length;
            }
            if(bid_score.empty()) return false;
            std::vector<std::pair<double, std::string> > score_vector;
            score_vector.reserve(bid_score.size());
            for(BidScore::const_iterator it = bid_score.begin();it!=bid_score.end();it++)
            {
                const std::string& brand = it->first;
                double score = it->second;
                std::vector<std::string> str_list;
                B5MHelper::SplitAttributeValue(brand, str_list);
                double count = str_list.size();
                score/=std::sqrt(count);
                score_vector.push_back(std::make_pair(score, brand));
            }
            std::sort(score_vector.begin(), score_vector.end());
            output = str_to_propstr(score_vector.back().second);
            if(output.length()<=1)
            {
                output.clear();
                return false;
            }
            return true;

        }

        bool flush()
        {
            LOG(INFO)<<"try flush bdb.."<<std::endl;
            if(db_->build()==-1)
            {
                LOG(ERROR)<<"fujimap build error"<<std::endl;
                return false;
            }
            db_->save(db_path_.c_str());
            id_manager_->flush();
            izenelib::am::ssf::Util<>::Save(source_path_, source_);
            return true;
        }

    private:

    private:

        std::string path_;
        std::string db_path_;
        std::string id_path_;
        std::string source_path_;
        std::string tmp_path_;
        DbType* db_;
        IdManager* id_manager_;
        SourceType source_;
        bool is_open_;
    };

}

#endif

