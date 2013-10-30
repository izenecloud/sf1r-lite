#ifndef SF1R_B5MMANAGER_B5MHELPER_H_
#define SF1R_B5MMANAGER_B5MHELPER_H_
#include "b5m_types.h"
#include <string>
#include <vector>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <common/ScdParser.h>
#include <common/Utilities.h>
#include <mining-manager/util/split_ustr.h>
#include <types.h>
#include <3rdparty/udt/md5.h>

NS_SF1R_B5M_BEGIN

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
        if( bfs::is_regular_file(scd_path) && boost::algorithm::ends_with(scd_path, ".SCD"))
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


    static void SplitAttributeValue(const std::string& str, std::vector<std::string>& str_list)
    {
        boost::algorithm::split(str_list, str, boost::algorithm::is_any_of("/"));
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
    static std::string GetB5moBlockPath(const std::string& mdb_instance)
    {
        return mdb_instance+"/b5mo_block";
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
    static std::string GetTmpPath(const std::string& mdb_instance)
    {
        return mdb_instance+"/tmp";
    }
    static std::string GetPoMapPath(const std::string& mdb_instance)
    {
        return mdb_instance+"/po_map";
    }

    static std::string GetOMapperPath(const std::string& mdb_instance)
    {
        return mdb_instance+"/omapper";
    }

    static std::string GetBrandPropertyName()
    {
        static std::string p("Brand");
        return p;
    }

    static std::string GetSPTPropertyName()
    {
        static std::string p("SPTitle");
        return p;
    }
    static std::string GetSPPicPropertyName()
    {
        static std::string p("SPPicture");
        return p;
    }
    static std::string GetSPUrlPropertyName()
    {
        static std::string p("SPUrl");
        return p;
    }
    static std::string GetTargetCategoryPropertyName()
    {
        static std::string p("TargetCategory");
        return p;
    }
    static std::string GetSubDocsPropertyName()
    {
        static std::string p("SubDocs");
        return p;
    }

    static std::string GetSalesAmountPropertyName()
    {
        static std::string p("SalesAmount");
        return p;
    }

    static std::string BookCategoryName()
    {
        static std::string name("书籍/杂志/报纸");
        return name;
    }

    static std::string GetPidByIsbn(const std::string& isbn)
    {
        //static const int MD5_DIGEST_LENGTH = 32;
        std::string url = "http://www.b5m.com/spuid/isbn/"+isbn;
        return Utilities::generateMD5(url);

        //md5_state_t st;
        //md5_init(&st);
        //md5_append(&st, (const md5_byte_t*)(url.c_str()), url.size());
        //union
        //{
            //md5_byte_t digest[MD5_DIGEST_LENGTH];
            //uint128_t md5_int_value;
        //} digest_union;			
        //memset(digest_union.digest, 0, sizeof(digest_union.digest));
        //md5_finish(&st,digest_union.digest);

        ////uint128_t pid = izenelib::util::HashFunction<UString>::generateHash128(UString(pid_str, UString::UTF_8));
        //return B5MHelper::Uint128ToString(digest_union.md5_int_value);
    }

    static std::string GetPidByUrl(const std::string& url)
    {
        return Utilities::generateMD5(url);
    }

    static void ParseAttributes(const izenelib::util::UString& ustr, std::vector<b5m::Attribute>& attributes)
    {
        typedef izenelib::util::UString UString;
        std::vector<AttrPair> attrib_list;
        std::vector<std::pair<UString, std::vector<UString> > > my_attrib_list;
        split_attr_pair(ustr, attrib_list);
        for(std::size_t i=0;i<attrib_list.size();i++)
        {
            b5m::Attribute attribute;
            attribute.is_optional = false;
            const std::vector<izenelib::util::UString>& attrib_value_list = attrib_list[i].second;
            if(attrib_value_list.empty()) continue;
            izenelib::util::UString attrib_value = attrib_value_list[0];
            for(uint32_t a=1;a<attrib_value_list.size();a++)
            {
                attrib_value.append(UString(" ",UString::UTF_8));
                attrib_value.append(attrib_value_list[a]);
            }
            if(attrib_value.empty()) continue;
            //if(attrib_value_list.size()!=1) continue; //ignore empty value attrib and multi value attribs
            izenelib::util::UString attrib_name = attrib_list[i].first;
            //if(attrib_value.length()==0 || attrib_value.length()>30) continue;
            attrib_name.convertString(attribute.name, UString::UTF_8);
            boost::algorithm::trim(attribute.name);
            if(boost::algorithm::ends_with(attribute.name, "_optional"))
            {
                attribute.is_optional = true;
                attribute.name = attribute.name.substr(0, attribute.name.find("_optional"));
            }
            std::vector<std::string> value_list;
            std::string svalue;
            attrib_value.convertString(svalue, izenelib::util::UString::UTF_8);
            boost::algorithm::split(attribute.values, svalue, boost::algorithm::is_any_of("/"));
            for(uint32_t v=0;v<attribute.values.size();v++)
            {
                boost::algorithm::trim(attribute.values[v]);
            }
            //if(attribute.name=="容量" && attribute.values.size()==1 && boost::algorithm::ends_with(attribute.values[0], "G"))
            //{
                //std::string gb_value = attribute.values[0]+"B";
                //attribute.values.push_back(gb_value);
            //}
            attributes.push_back(attribute);
        }
    }

};

NS_SF1R_B5M_END

#endif

