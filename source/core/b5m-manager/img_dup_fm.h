#ifndef IMG_DUP_FM_H_
#define IMG_DUP_FM_H_

#include "img_dup_helper.h"
#include "img_dup_fujimap.h"
#include <string>
#include <vector>
#include "b5m_types.h"
#include "b5m_helper.h"
#include <types.h>
#include <util/singleton.h>
#include <glog/logging.h>
#include <common/ScdWriter.h>


namespace sf1r {

    class ImgDupFileManager
    {
        std::string input_path_;
        std::string output_path_;
        std::string current_output_path_;
        ImgDupFujiMap* gid_memcount_;
        ImgDupFujiMap* docid_docid_;
    public:
        ImgDupFileManager()
        {
        }
        ~ImgDupFileManager()
        {
        }

        static ImgDupFileManager* get()
        {
            return izenelib::util::Singleton<ImgDupFileManager>::get();
        }

        bool SetParam(std::string ip,
                std::string op,
                ImgDupFujiMap* gm,
                ImgDupFujiMap* dd)
        {
            input_path_ = ip;
            output_path_ = op;
            gid_memcount_ = gm;
            docid_docid_ = dd;
            return true;
        }

        uint32_t DocidToUint(const std::string& docid )
        {
            return boost::lexical_cast<uint32_t>(docid);
        }
        std::string UintToDocid(const uint32_t docid)
        {
            return boost::lexical_cast<std::string>(docid);
        }

        bool GetCurrentTime(std::string& str)
        {
            struct tm* newtime;
            char buf[128];
            time_t t;
            time(&t);
            newtime = localtime(&t);
            strftime(buf, 128, "%Y-%m-%d[%X]", newtime);
            str = std::string(buf);
            return true;
        }

        bool GetCurrentOutputPath(std::string& output_path)
        {
            std::string current_time;
            if( !ImgDupFileManager::GetCurrentTime(current_time))
            {
                LOG(INFO) << "Get Current Time Error" << endl;
                return false;
            }
            output_path = output_path_ + "/" + current_time;
            return true;
        }

        bool ReBuildFile(const std::string& filename)
        {

            std::string scd_file = input_path_ + "/" + filename;
            LOG(INFO)<<"ReBuild "<<scd_file<<std::endl;
            ScdParser parser(izenelib::util::UString::UTF_8);
            parser.load(scd_file);
            uint32_t n=0;
            uint32_t rest=0;

            boost::filesystem::create_directories(current_output_path_+"/0");
            boost::filesystem::create_directories(current_output_path_+"/1");

            ScdWriter writer0(current_output_path_+"/0", 2);
            ScdWriter writer1(current_output_path_+"/1", 2);

            writer0.SetFileName(filename);
            writer1.SetFileName(filename);

            for( ScdParser::iterator doc_iter = parser.begin();
                doc_iter!= parser.end(); ++doc_iter, ++n)
            {
                if(n%10000==0)
                {
                    LOG(INFO)<<"Find Documents "<<n<<std::endl;
                }
                std::map<std::string, UString> doc;
                SCDDoc& scddoc = *(*doc_iter);
                SCDDoc::iterator p = scddoc.begin();
                for(; p!=scddoc.end(); ++p)
                {
                    const std::string& property_name = p->first;
                    doc[property_name] = p->second;
                }
                std::string docID;
                doc["DOCID"].convertString(docID, izenelib::util::UString::UTF_8);
                uint32_t current_docid = DocidToUint(docID);
                uint32_t match_docid;
                if( docid_docid_->get(current_docid, match_docid) )
                {
                    //deleted
                    UString gid;
                    gid.assign(UintToDocid(match_docid), izenelib::util::UString::UTF_8);

                    scddoc.push_back(std::pair<std::string, UString>("GID", gid));
                    writer0.Append(scddoc);
                }
                else
                {
                    //saved
                    rest++;
                    scddoc.push_back(std::pair<std::string, UString>("GID", doc["DOCID"]));
                    writer0.Append(scddoc);

                    uint32_t count;
                    if( !gid_memcount_->get(current_docid, count))
                    {
                        UString gmemcount;
                        gmemcount.assign("1", izenelib::util::UString::UTF_8);
                        scddoc.push_back(std::pair<std::string, UString>("GMemCount", gmemcount));
                        writer1.Append(scddoc);
                    }
                    else
                    {
                        UString gmemcount;
                        std::string count_str = boost::lexical_cast<std::string> (count+1);
                        gmemcount.assign(count_str, izenelib::util::UString::UTF_8);
                        scddoc.push_back(std::pair<std::string, UString>("GMemCount", gmemcount));
                        writer1.Append(scddoc);
                    }
                }
            }
            LOG(INFO)<<"Total: "<<n<<" Rest: "<<rest<< std::endl;
            return true;
        }

        bool ReBuildAll()
        {
            if( docid_docid_->build() < 0 )
            {
                LOG(ERROR) <<"FujiMap build error [docid_docid_]" <<endl;
                LOG(ERROR) <<"Error info: " <<docid_docid_->what() <<endl;
                return false;
            }
            if( gid_memcount_->build() < 0 )
            {
                LOG(INFO) << "FUjiMap build error [gid_memcount_]" <<endl;
                LOG(INFO) << "Error info: " <<gid_memcount_->what() <<endl;
                return false;
            }

            ImgDupFileManager::GetCurrentOutputPath(current_output_path_);

            std::vector<std::string> scd_list;
            B5MHelper::GetIUScdList(input_path_, scd_list);
            if(scd_list.empty())
            {
                LOG(ERROR) << "scd empty" <<std::endl;
                return false;
            }
            for(uint32_t i=0;i<scd_list.size();i++)
            {
                std::string scd_file = scd_list[i];
                if(!ImgDupFileManager::ReBuildFile(scd_file))
                    return false;
            }
            return true;
        }

    };
}

#endif // IMG_DUP_FM_H_
