#ifndef SF1R_B5MMANAGER_PIDDICTIONARY_H
#define SF1R_B5MMANAGER_PIDDICTIONARY_H 
#include <boost/unordered_set.hpp>
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string/trim.hpp>

namespace sf1r {

    class PidDictionary{

    public:
        typedef boost::unordered_set<std::string> Container;
        PidDictionary()
        {
        }
        void Add(const std::string& pid_candidate)
        {
            pid_set_.insert(pid_candidate);
        }

        std::size_t Size() const
        {
            return pid_set_.size();
        }

        bool Exist(const std::string& pid) const
        {
            Container::const_iterator it = pid_set_.find(pid);
            return it!=pid_set_.end();
        }

        void Erase(const std::string& id)
        {
            pid_set_.erase(id);
        }

        bool Save(const std::string& file)
        {
            std::ofstream ofs(file.c_str());
            if(ofs.fail()) return false;
            for(Container::const_iterator it = pid_set_.begin(); it!=pid_set_.end(); ++it)
            {
                ofs<<*it<<std::endl;
            }
            ofs.close();
            return true;
        }

        bool Load(const std::string& file)
        {
            std::ifstream ifs(file.c_str());
            if(ifs.fail()) return false;
            std::string pid;
            while(getline(ifs, pid))
            {
                boost::algorithm::trim(pid);
                if(pid.empty()) continue;
                pid_set_.insert(pid);
            }
            ifs.close();
            return true;
        }


    private:
        Container pid_set_;
    };

}

#endif

