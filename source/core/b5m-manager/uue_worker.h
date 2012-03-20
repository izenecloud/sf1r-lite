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
        }

        void Run()
        {
            for(uint32_t i=0;i<uue_list_.size();i++)
            {
                processor_->Process(uue_list_[i]);
            }
            processor_->Finish();
        }

        void BatchRun()
        {
            std::vector<BuueItem> buue_list;
            GetBuueList_(buue_list);
            for(uint32_t i=0;i<buue_list.size();i++)
            {
                processor_->Process(buue_list[i]);
            }
            processor_->Finish();
        }

    private:
        void GetBuueList_(std::vector<BuueItem>& buue_list)
        {
            {
                std::sort(uue_list_.begin(), uue_list_.end(), CompareUueFrom());
                izenelib::util::VectorJoiner joiner(&uue_list_, CompareUueFrom());
                std::vector<UueItem> vec;
                while(joiner.Next(vec))
                {
                    BuueItem buue;
                    buue.type = BUUE_REMOVE;
                    buue.uuid = vec[0].from_to.from;
                    if(buue.uuid.empty()) continue;
                    for(uint32_t i=0;i<vec.size();i++)
                    {
                        buue.docid_list.push_back(vec[i].docid);
                    }
                    buue_list.push_back(buue);
                }
            }
            {
                std::sort(uue_list_.begin(), uue_list_.end(), CompareUueTo());
                izenelib::util::VectorJoiner joiner(&uue_list_, CompareUueTo());
                std::vector<UueItem> vec;
                while(joiner.Next(vec))
                {
                    BuueItem buue;
                    buue.type = BUUE_APPEND;
                    buue.uuid = vec[0].from_to.to;
                    if(buue.uuid.empty()) continue;
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

