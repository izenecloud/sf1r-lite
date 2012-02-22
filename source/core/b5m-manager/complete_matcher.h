#ifndef SF1R_B5MMANAGER_COMPLETEMATCHER_H
#define SF1R_B5MMANAGER_COMPLETEMATCHER_H 
#include <boost/unordered_map.hpp>
#include <util/ustring/UString.h>

namespace sf1r {

    class CompleteMatcher{
        typedef izenelib::util::UString UString;

        struct ValueType
        {
            std::string soid;
            UString title;
            uint32_t n;
            
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & soid & title & n;
            }
            
            bool operator<(const ValueType& other) const
            {
                if(title.length()<other.title.length()) return true;
                else if(title.length()==other.title.length())
                {
                    return n<other.n;
                }
                return false;
            }
        };
    public:
        CompleteMatcher();
        bool Index(const std::string& scd_file, const std::string& knowledge_dir);

    private:
        void ClearKnowledge_(const std::string& knowledge_dir);
    };

}

#endif

