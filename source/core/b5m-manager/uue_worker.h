#ifndef SF1R_B5MMANAGER_UUEWORKER_H_
#define SF1R_B5MMANAGER_UUEWORKER_H_

#include <string>
#include <vector>
#include "b5m_types.h"
#include "b5m_helper.h"
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>
#include <util/vector_joiner.h>

namespace sf1r {

    template<class Processor>
    class UueWorker {
    public:
        UueWorker(Processor* processor)
        :processor_(processor)
        {
        }

        ~UueWorker()
        {
        }

        void Load(const std::string& path)
        {
            LOG(INFO)<<"loading uue"<<std::endl;
            std::ifstream ifs(path.c_str());
            std::string line;
            while( getline(ifs,line))
            {
                boost::algorithm::trim(line);
                std::vector<std::string> vec;
                boost::algorithm::split(vec, line, boost::is_any_of(","));
                if(vec.size()<3) continue;
                UueItem uue;
                uue.docid = vec[0];
                uue.from_to.from = vec[1];
                uue.from_to.to = vec[2];
                uue_list_.push_back(uue);
            }
            ifs.close();
            LOG(INFO)<<"loading uue finished"<<std::endl;
        }

        bool Run()
        {
            for(uint32_t i=0;i<uue_list_.size();i++)
            {
                if(i%100000==0)
                {
                    LOG(INFO)<<"processing "<<i<<" uue item"<<std::endl;
                }
                processor_->Process(uue_list_[i]);
            }
            uue_list_.clear();
            processor_->Finish();
            return true;
        }

        bool BatchRun()
        {
            std::vector<BuueItem> buue_list;
            LOG(INFO)<<"getting buue list"<<std::endl;
            GetBuueList_(buue_list);
            LOG(INFO)<<"buue list got"<<std::endl;
            uue_list_.clear();
            for(uint32_t i=0;i<buue_list.size();i++)
            {
                if(i%100000==0)
                {
                    LOG(INFO)<<"processing "<<i<<" buue item"<<std::endl;
                }
                processor_->Process(buue_list[i]);
            }
            buue_list.clear();
            processor_->Finish();
            return true;
        }

    private:
        void GetBuueList_(std::vector<BuueItem>& buue_list)
        {
            {
                std::sort(uue_list_.begin(), uue_list_.end(), CompareUueFrom());
                izenelib::util::VectorJoiner<UueItem, CompareUueFrom> joiner(&uue_list_);
                std::vector<UueItem> vec;
                while(joiner.Next(vec))
                {
                    BuueItem buue;
                    buue.type = BUUE_REMOVE;
                    buue.pid = vec[0].from_to.from;
                    if(buue.pid.empty()) continue;
                    for(uint32_t i=0;i<vec.size();i++)
                    {
                        buue.docid_list.push_back(vec[i].docid);
                    }
                    buue_list.push_back(buue);
                }
            }
            {
                std::sort(uue_list_.begin(), uue_list_.end(), CompareUueTo());
                izenelib::util::VectorJoiner<UueItem, CompareUueTo> joiner(&uue_list_);
                std::vector<UueItem> vec;
                while(joiner.Next(vec))
                {
                    BuueItem buue;
                    buue.type = BUUE_APPEND;
                    buue.pid = vec[0].from_to.to;
                    if(buue.pid.empty()) continue;
                    for(uint32_t i=0;i<vec.size();i++)
                    {
                        buue.docid_list.push_back(vec[i].docid);
                    }
                    buue_list.push_back(buue);
                }
            }
        }

    private:
        Processor* processor_;
        std::vector<UueItem> uue_list_;
    };

}

#endif

