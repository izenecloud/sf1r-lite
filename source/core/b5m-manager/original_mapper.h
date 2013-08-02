#ifndef SF1R_B5MMANAGER_ORIGINALMAPPER_H_
#define SF1R_B5MMANAGER_ORIGINALMAPPER_H_
#include "b5m_helper.h"
#include "b5m_types.h"

namespace sf1r {


    class OriginalMapper {
    public:
        //static std::string GetVersion(const std::string& path)
        //{
        //}
        typedef std::pair<std::string, std::string> Key;
        typedef std::string Value;
        typedef boost::unordered_map<Key, Value> Mapper;
        OriginalMapper(): is_open_(false)
        {
        }
        bool IsOpen() const
        {
            return is_open_;
        }
        bool Open(const std::string& path)
        {
            std::string file = path+"/data";
            std::string line;
            std::ifstream ifs(file.c_str());
            while(getline(ifs, line))
            {
                boost::algorithm::trim(line);
                Key key;
                Value value;
                if(!Parse_(line, key, value)) continue;
                mapper_[key] = value;
            }

            is_open_ = true;
            return true;
        }

        bool Diff(const std::string& new_path)
        {
            Mapper new_mapper;
            std::string file = new_path+"/data";
            std::string line;
            std::ifstream ifs(file.c_str());
            while(getline(ifs, line))
            {
                boost::algorithm::trim(line);
                Key key;
                Value value;
                if(!Parse_(line, key, value)) continue;
                bool diff = false;
                Mapper::const_iterator it = mapper_.find(key);
                if(it==mapper_.end())
                {
                    diff = true;
                }
                else
                {
                    if(it->second!=value)
                    {
                        diff = true;
                    }
                }
                if(diff)
                {
                    new_mapper[key] = value;
                }
            }
            ifs.close();
            mapper_.swap(new_mapper);
            return true;
        }

        bool Map(const std::string& source, const std::string& original, std::string& category) const
        {
            std::pair<std::string, std::string> key(source, original);
            Mapper::const_iterator it = mapper_.find(key);
            if(it==mapper_.end()) return false;
            else
            {
                category = it->second;
                return true;
            }
        }

        std::size_t Size() const
        {
            return mapper_.size();
        }

    private:
        static bool Parse_(const std::string& input, Key& key, Value& value)
        {
            std::vector<std::string> vec;
            boost::algorithm::split(vec, input, boost::algorithm::is_any_of(":"));
            if(vec.size()!=2) return false;
            value = vec[1];
            std::string key_str = vec[0];
            vec.clear();
            boost::algorithm::split(vec, key_str, boost::algorithm::is_any_of(","));
            if(vec.size()!=2) return false;
            key.first = vec[0];
            key.second = vec[1];
            return true;
        }
    private:
        bool is_open_;
        Mapper mapper_;

    };

}

#endif


