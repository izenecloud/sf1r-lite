#ifndef IMG_DUP_FM_H_
#define IMG_DUP_FM_H_

#include "img_dup_helper.h"
#include "img_dup_fujimap.h"
#include "img_dup_leveldb.h"
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
        DocidImgDbTable* docidImgDbTable;
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

        ImgDupFileManager(std::string ip,
                std::string op,
                ImgDupFujiMap* gm,
                ImgDupFujiMap* dd,
                DocidImgDbTable* dt)
        {
            input_path_ = ip;
            output_path_ = op;
            gid_memcount_ = gm;
            docid_docid_ = dd;
            docidImgDbTable = dt;
        }
        bool SetParam(std::string ip,
                std::string op,
                ImgDupFujiMap* gm,
                ImgDupFujiMap* dd,
                DocidImgDbTable* dt)
        {
            input_path_ = ip;
            output_path_ = op;
            gid_memcount_ = gm;
            docid_docid_ = dd;
            docidImgDbTable = dt;
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

        bool GetFileNameFromPathName(const std::string& pathname, std::string& filename)
        {
            filename = pathname.substr(pathname.find_last_of("/")+1);
            return true;
        }
        bool ReBuildFile(const std::string& filename)
        {

            std::string scd_file = filename;
            LOG(INFO)<<"ReBuild "<<scd_file<<std::endl;
            ScdParser parser(izenelib::util::UString::UTF_8);
            parser.load(scd_file);
            uint32_t n=0;
            uint32_t rest=0;

            boost::filesystem::create_directories(current_output_path_+"/0");
            boost::filesystem::create_directories(current_output_path_+"/1");

            ScdWriter writer0(current_output_path_+"/0", UPDATE_SCD);
            ScdWriter writer1(current_output_path_+"/1", UPDATE_SCD);


            std::string writefile;
            GetFileNameFromPathName(filename, writefile);

            writer0.SetFileName(writefile);
            writer1.SetFileName(writefile);
            LOG(INFO) << "WriteFileName: " << writefile << std::endl;

            for( ScdParser::iterator doc_iter = parser.begin();
                doc_iter!= parser.end(); ++doc_iter, ++n)
            {
                if(n%10000==0)
                {
                    LOG(INFO)<<"Find Documents "<<n<<std::endl;
                }
                std::map<std::string, Document::doc_prop_value_strtype> doc;
                SCDDoc& scddoc = *(*doc_iter);
                SCDDoc::iterator p = scddoc.begin();
                for(; p!=scddoc.end(); ++p)
                {
                    const std::string& property_name = p->first;
                    doc[property_name] = p->second;
                }
                std::string docID = propstr_to_str(doc["DOCID"]);
                uint32_t current_docid = DocidToUint(docID);
                uint32_t match_docid;
                if( docid_docid_->get(current_docid, match_docid))
                {
                    //deleted
                    std::string gid;
                    gid.assign(UintToDocid(match_docid));

                    scddoc.push_back(std::pair<std::string, Document::doc_prop_value_strtype>("GID", str_to_propstr(gid)));
                    std::string guangURL;
                    if(!docidImgDbTable->get_item(match_docid, guangURL))
                    {
                        LOG(INFO) << "Find no img url..." << endl;
                    }
                    else
                    {
                        scddoc.push_back(std::pair<std::string, Document::doc_prop_value_strtype>("guangURL", str_to_propstr(guangURL)));
                    }
                    writer0.Append(scddoc);
                }
                else
                {
                    //saved
                    rest++;
                    scddoc.push_back(std::pair<std::string, Document::doc_prop_value_strtype>("GID", doc["DOCID"]));
                    scddoc.push_back(std::pair<std::string, Document::doc_prop_value_strtype>("guangURL", doc["Img"]));
                    writer0.Append(scddoc);

                    uint32_t count;
                    if( !gid_memcount_->get(current_docid, count))
                    {
                        scddoc.push_back(std::pair<std::string, Document::doc_prop_value_strtype>("GMemCount", str_to_propstr("1")));
                        writer1.Append(scddoc);
                    }
                    else
                    {
                        std::string count_str = boost::lexical_cast<std::string> (count+1);
                        scddoc.push_back(std::pair<std::string, Document::doc_prop_value_strtype>("GMemCount", str_to_propstr(count_str)));
                        writer1.Append(scddoc);
                    }
                }
            }
            LOG(INFO)<<"Total: "<<n<<" Rest: "<<rest<< std::endl;
            return true;
        }

        bool ReBuildAll()
        {
            docid_docid_->load(output_path_+"/../fujimap/tmp4.index");
            gid_memcount_->load(output_path_+"/../fujimap/tmp5.index");

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
