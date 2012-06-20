#ifndef SF1R_B5MMANAGER_B5MHELPER_H_
#define SF1R_B5MMANAGER_B5MHELPER_H_
#include <string>
#include <vector>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <common/ScdParser.h>
#include <common/Utilities.h>
#include <types.h>

namespace sf1r {


    class CategoryStringMatcher
    {
    public:
        CategoryStringMatcher(bool reverse = false):reverse_(reverse)
        {
        }
        void AddCategoryRegex(const std::string& r)
        {
            boost::regex reg(r);
            regex_list_.push_back(reg);
        }
        bool Match(const std::string& scategory) const
        {
            for(uint32_t i=0;i<regex_list_.size();i++)
            {
                if(boost::regex_match(scategory, regex_list_[i]))
                {
                    return !reverse_;
                }

            }
            return reverse_;
        }
    private:
        bool reverse_;
        std::vector<boost::regex> regex_list_;
    };

    class B5MHelper {
    public:

        static uint128_t StringToUint128(const std::string& str)
        {
            unsigned long long high = 0, low = 0;
            sscanf(str.c_str(), "%016llx%016llx", &high, &low);
            return (uint128_t) high << 64 | (uint128_t) low;
        }

        static uint128_t UStringToUint128(const izenelib::util::UString& ustr)
        {
            std::string str;
            ustr.convertString(str, izenelib::util::UString::UTF_8);
            return StringToUint128(str);
        }

        static std::string Uint128ToString(const uint128_t& val)
        {
            static char tmpstr[33];
            sprintf(tmpstr, "%016llx%016llx", (unsigned long long) (val >> 64), (unsigned long long) val);
            return std::string(reinterpret_cast<const char *>(tmpstr), 32);
        }

        static void GetScdList(const std::string& scd_path, std::vector<std::string>& scd_list)
        {
            namespace bfs = boost::filesystem;
            if(!bfs::exists(scd_path)) return;
            if( bfs::is_regular_file(scd_path) && ScdParser::checkSCDFormat(scd_path))
            {
                scd_list.push_back(scd_path);
            }
            else if(bfs::is_directory(scd_path))
            {
                bfs::path p(scd_path);
                bfs::directory_iterator end;
                for(bfs::directory_iterator it(p);it!=end;it++)
                {
                    if(bfs::is_regular_file(it->path()))
                    {
                        std::string file = it->path().string();
                        if(ScdParser::checkSCDFormat(file))
                        {
                            scd_list.push_back(file);
                        }
                    }
                }
            }
            std::sort(scd_list.begin(), scd_list.end(), ScdParser::compareSCD);
        }

        static void GetIScdList(const std::string& scd_path, std::vector<std::string>& scd_list)
        {
            GetScdList(scd_path, scd_list);
            std::vector<std::string> valid_scd_list;
            for(uint32_t i=0;i<scd_list.size();i++)
            {
                int scd_type = ScdParser::checkSCDType(scd_list[i]);
                if(scd_type==INSERT_SCD)
                {
                    valid_scd_list.push_back(scd_list[i]);
                }
            }
            scd_list.swap(valid_scd_list);
        }

        static void GetIUScdList(const std::string& scd_path, std::vector<std::string>& scd_list)
        {
            GetScdList(scd_path, scd_list);
            std::vector<std::string> valid_scd_list;
            for(uint32_t i=0;i<scd_list.size();i++)
            {
                int scd_type = ScdParser::checkSCDType(scd_list[i]);
                if(scd_type!=DELETE_SCD)
                {
                    valid_scd_list.push_back(scd_list[i]);
                }
            }
            scd_list.swap(valid_scd_list);
        }

        static void PrepareEmptyDir(const std::string& dir)
        {
            boost::filesystem::remove_all(dir);
            boost::filesystem::create_directories(dir);
        }


        static std::string GetRawPath(const std::string& mdb_instance)
        {
            return mdb_instance+"/raw";
        }
        static std::string GetB5moMirrorPath(const std::string& mdb_instance)
        {
            return mdb_instance+"/b5mo_mirror";
        }
        static std::string GetB5moPath(const std::string& mdb_instance)
        {
            return mdb_instance+"/b5mo";
        }
        static std::string GetB5mpPath(const std::string& mdb_instance)
        {
            return mdb_instance+"/b5mp";
        }
        static std::string GetUuePath(const std::string& mdb_instance)
        {
            return mdb_instance+"/uue";
        }
        static std::string GetB5mcPath(const std::string& mdb_instance)
        {
            return mdb_instance+"/b5mc";
        }
        static std::string GetPoMapPath(const std::string& mdb_instance)
        {
            return mdb_instance+"/po_map";
        }
    };

}

#endif

