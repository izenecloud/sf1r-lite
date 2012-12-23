#include "img_dup_detector.h"
#include <util/ClockTimer.h>
#include <util/filesystem.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <dirent.h>
#include <unistd.h>
#include <sys/inotify.h>

#define MAX_BUF_SIZE 1024

using namespace sf1r;

ImgDupDetector::ImgDupDetector()
:psmk_(400)
{
}

bool ImgDupDetector::DupDetectByImgUrl(const std::string& scd_path, const std::string& output_path, std::string& toDelete)
{
    LOG(INFO)<<"ShareSourceName to be deleted: " << toDelete << std::endl;
    ProductTermAnalyzer analyzer(cma_path_);
    boost::filesystem::create_directories(output_path);

    int fd, wd;
    int len, index;
    char buffer[1024];
    struct inotify_event *event;
    fd = inotify_init();
    if(fd < 0)
    {
        LOG(ERROR)<<"Failed to initialize inotify."<<std::endl;
        return false;
    }
    wd = inotify_add_watch(fd, scd_path.c_str(), IN_CLOSE_WRITE | IN_CREATE);
    if(wd < 0)
    {
        LOG(ERROR)<<"Can't add watch for: "<<scd_path<<std::endl;
        return false;
    }

    while((len = read(fd, buffer, MAX_BUF_SIZE)) > 0)
    {
        index = 0;
        while(index < len)
        {
            event = (struct inotify_event *)(buffer+index);
            if(event->wd != wd)
                continue;
            if(event -> mask & IN_CLOSE_WRITE)
            {
                LOG(INFO)<<"File "<<event->name<<" is closed for write. " << std::endl;
                std::string psm_path = output_path+"/psm";
                B5MHelper::PrepareEmptyDir(psm_path);

                PsmType psm(psm_path);
                psm.SetK(psmk_);
                psm.Open();

                uint32_t key = 100;
                std::map<std::string, UString> imgurl_list;
                std::map<std::string, uint32_t> docid_key;
                std::map<uint32_t, std::string> key_docid;

                for(uint32_t i=0;i<1;i++)
                {
                    std::string scd_file = scd_path + "/" + std::string(event->name);
                    LOG(INFO)<<"Processing "<<scd_file<<std::endl;
                    ScdParser parser(izenelib::util::UString::UTF_8);
                    parser.load(scd_file);
                    uint32_t n=0;
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
                        std::string sourceName;
                        doc["ShareSourceName"].convertString(sourceName, izenelib::util::UString::UTF_8);
//                        LOG(INFO) << "ShareSourceName: " << sourceName << std::endl;
                        if(sourceName.compare(toDelete) == 0)
                        {
//                            LOG(INFO) << "ShareSourceName Matches!!"<<std::endl;
                            continue;
                        }
                        std::string docID;
                        std::vector<std::pair<std::string, double> > doc_vector;
                        PsmAttach attach;
                        if(!PsmHelper::GetPsmItem(analyzer, doc, docID, doc_vector, attach))
                        {
//                          LOG(INFO)<<"Get psm item Error ... "<<std::endl;
                            continue;
                        }
                        psm.Insert(key, doc_vector, attach);
                        docid_key[docID] = key;
                        key_docid[key] = docID;
                        key++;
                        imgurl_list[docID] =doc["Img"];
                    }
                    n = 0;
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
                        std::string sourceName;
                        doc["ShareSourceName"].convertString(sourceName, izenelib::util::UString::UTF_8);
//                        LOG(INFO) << "ShareSourceName: " << sourceName << std::endl;
                        if(sourceName.compare(toDelete) != 0)
                        {
//                            LOG(INFO) << "ShareSourceName Matches!!"<<std::endl;
                            continue;
                        }
                        std::string docID;
                        std::vector<std::pair<std::string, double> > doc_vector;
                        PsmAttach attach;
                        if(!PsmHelper::GetPsmItem(analyzer, doc, docID, doc_vector, attach))
                        {
//                          LOG(INFO)<<"Get psm item Error ... "<<std::endl;
                            continue;
                        }
                        psm.Insert(key, doc_vector, attach);
                        docid_key[docID] = key;
                        key_docid[key] = docID;
                        key++;
                        imgurl_list[docID] =doc["Img"];
                    }

                }
                if(!psm.Build())
                {
                    LOG(ERROR)<<"psm build error"<<std::endl;
                    return false;
                }
                psm.Open();
                for(uint32_t i=0;i<1;i++)
                {
                    std::string scd_file =  scd_path + "/" + std::string(event->name);
                    LOG(INFO)<<"Processing "<<scd_file<<std::endl;
                    ScdParser parser(izenelib::util::UString::UTF_8);
                    parser.load(scd_file);
                    uint32_t n=0;
                    uint32_t rest=0;
                    ScdWriter writer(output_path, 2);
                    std::string outfilename = std::string(event->name);
                    writer.SetFileName(outfilename);
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
                        std::vector<std::pair<std::string, double> > doc_vector;
                        PsmAttach attach;
                        if(!PsmHelper::GetPsmItem(analyzer, doc, docID, doc_vector, attach))
                        {
                            LOG(INFO)<<"Get psm item Error ... "<<std::endl;
                            if(writer.Append(scddoc)) rest++;
                            continue;
                        }
                        uint32_t match_key;
                        if(!psm.Search(doc_vector, attach, match_key))
                        {
                            if(writer.Append(scddoc)) rest++;
                        }
                        else if(docid_key[docID] == match_key)
                        {
                            if(writer.Append(scddoc)) rest++;
                        }
                        else
                        {
                            std::string current_url;
                            std::string match_url;
                            doc["Img"].convertString(current_url,izenelib::util::UString::UTF_8);
                            imgurl_list[key_docid[match_key]].convertString(match_url, izenelib::util::UString::UTF_8);
                            if(current_url.compare(match_url) == 0)
                            {

                                LOG(INFO)<<std::endl<<docID<<":  "<<doc["Img"]
                                         <<std::endl<<key_docid[match_key]<<":  "<<imgurl_list[key_docid[match_key]]
                                         <<std::endl<<"Matches "<<std::endl;

                            }
                            else
                            {
                                if(writer.Append(scddoc)) rest++;
                            }
                        }

                    }
                    LOG(INFO)<<"Total: "<<n<<" Rest: "<<rest<<std::endl;
                }

            }
            index += sizeof(struct inotify_event)+event->len;
        }
    }
    return true;
}
/*
bool ImgDupDetector::DupDetectByImgSift(const std::string& scd_path, const std::string& output_path, bool test = false)
{
    if(test)
    {

        DIR* dir;
        if(!(dir = opendir(scd_path.c_str())))
        {
            LOG(INFO)<<"empty"<<std::endl;
            return false;
        }
        struct dirent* d_ent;

        boost::filesystem::create_directories(output_path);
        std::string psm_path = output_path+"/psm";
        B5MHelper::PrepareEmptyDir(psm_path);

        idmlib::ise::ProbSimMatch psm(psm_path);
        psm.Init();

        idmlib::ise::Extractor extractor;

        unsigned i = 0;
        std::map<unsigned, std::string> path_list;
        while((d_ent = readdir(dir)) != NULL)
        {
            if(strncmp(d_ent->d_name, ".", 1) == 0 || strncmp(d_ent->d_name, "", 1) == 0 || strncmp(d_ent->d_name, "..", 1) == 0 )
                continue;
            std::string filename(d_ent->d_name);
            std::string path = scd_path + "/" + filename;
            std::vector<idmlib::ise::Sift::Feature> sift;

            extractor.ExtractSift(path, sift);
            LOG(INFO)<<"sift "<<sift[0].desc[0]<<std::endl;

            psm.Insert(i, sift);
            path_list[i] = path;
            i++;

        }
        closedir(dir);
        psm.Finish();

        DIR* match_dir;
        std::string match_path = scd_path + "2";
        if(!(match_dir = opendir(match_path.c_str())))
        {
            LOG(INFO)<<"empty"<<std::endl;
            return false;
        }




        i = 0;
        unsigned matches = 0;
        unsigned no_matches = 0;
        while((d_ent = readdir(match_dir)) != NULL)
        {
            if(strncmp(d_ent->d_name, ".", 1) == 0 || strncmp(d_ent->d_name, "", 1) == 0 || strncmp(d_ent->d_name, "..", 1) == 0 )
                continue;
            std::string filename(d_ent->d_name);
            std::string path = match_path + "/" + filename;
            std::vector<idmlib::ise::Sift::Feature> sift;

            extractor.ExtractSift(path, sift);

            std::vector<unsigned> results;
            psm.Search(sift, results);
            if(results.size() == 0)
            {
                LOG(INFO)<<"no match!!!"<<std::endl;
                no_matches++;
            }
            else if(results.size() > 1)
            {
                unsigned min = i;
                unsigned j;
                for(j=0;j<results.size();j++)
                    if(min > results[j]) break;
                if(j == results.size())
                {
                }
                else
                {
                    LOG(INFO)<<std::endl<<i<<":  "<<path
                             <<std::endl<<results[j]<<":  "<<path_list[results[j]]
                             <<std::endl<<" Matches "<<std::endl;
                    std::cout<<std::endl<<i<<":  "<<path
                             <<std::endl<<results[j]<<":  "<<path_list[results[j]]
                             <<std::endl<<" Matches "<<std::endl;
                    matches++;
                }

            }
            i++;

        }
        LOG(INFO)<<"TOTAL: "<<i<<" REST: "<<i-matches<<" NO MATCH: "<<no_matches<<endl;
        return true;
    }
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.empty())
    {
        LOG(ERROR)<<"scd empty"<<std::endl;
        return false;
    }
    boost::filesystem::create_directories(output_path);
    std::string psm_path = output_path+"/psm";
    B5MHelper::PrepareEmptyDir(psm_path);

    idmlib::ise::ProbSimMatch psm(psm_path);
    psm.Init();

    idmlib::ise::Extractor extractor;

    std::map<unsigned, UString> imgurl_list;
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin();
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%10000==0)
            {
                LOG(INFO)<<"Find Documents "<<n<<std::endl;
            }
            if(n>200)break;
            LOG(INFO)<<"Find Documents "<<n<<std::endl;
            std::map<std::string, UString> doc;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc[property_name] = p->second;
            }
            std::string key_str;
            unsigned key;
            std::string imgurl;
            if(doc["Img"].length()==0)
            {
                LOG(INFO)<<"There is no Img ... "<<std::endl;
                continue;
            }
            std::vector<idmlib::ise::Sift::Feature> sift;
            doc["Img"].convertString(imgurl,izenelib::util::UString::UTF_8);

            extractor.ExtractSift(imgurl, sift);
            LOG(INFO)<<"sift "<<sift[0].desc[0]<<std::endl;

            doc["DOCID"].convertString(key_str, izenelib::util::UString::UTF_8);
            key = atoi(key_str.c_str());
            psm.Insert(key, sift);
            imgurl_list[key] =doc["Img"];
        }
    }
    psm.Finish();
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        uint32_t n=0;
        uint32_t rest=0;
        ScdWriter writer(output_path, 2);
        writer.SetFileName("SCDres.SCD");
        for( ScdParser::iterator doc_iter = parser.begin();
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%10000==0)
            {
                LOG(INFO)<<"Find Documents "<<n<<std::endl;
            }
            if(n>200)break;
            std::map<std::string, UString> doc;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc[property_name] = p->second;
            }
            if(doc["Img"].length()==0)
            {
                LOG(INFO)<<"There is no Img ... "<<std::endl;
                if(writer.Append(scddoc)) rest++;
                continue;
            }

            std::string key_str;
            unsigned key;
            std::string imgurl;
            doc["Img"].convertString(imgurl,izenelib::util::UString::UTF_8);
            std::vector<idmlib::ise::Sift::Feature> sift;

            extractor.ExtractSift(imgurl, sift);

            doc["DOCID"].convertString(key_str, izenelib::util::UString::UTF_8);
            key = atoi(key_str.c_str());

            std::vector<unsigned> results;
            unsigned i;
            psm.Search(sift, results);
            if(results.size() == 0)
            {
                LOG(INFO)<<"Find no match !!!!"<<std::endl;
                if(writer.Append(scddoc)) rest++;
            }
            else if(results.size() == 1)
            {
                if(writer.Append(scddoc)) rest++;
            }
            else if(results.size() > 1)
            {
                unsigned min = key;
                for(i=0;i<results.size();i++)
                    if(min > results[i]) break;
                if(i == results.size())
                {
                    if(writer.Append(scddoc)) rest++;
                }
                else
                {
                    LOG(INFO)<<std::endl<<key<<":  "<<doc["Img"]
                             <<std::endl<<results[i]<<":  "<<imgurl_list[results[i]]
                             <<std::endl<<" Matches "<<std::endl;
                    std::cout<<std::endl<<key<<":  "<<doc["Img"]
                             <<std::endl<<results[i]<<":  "<<imgurl_list[results[i]]
                             <<std::endl<<" Matches "<<std::endl;
                }
            }

        }
        LOG(INFO)<<"Total: "<<n<<" Rest: "<<rest<<std::endl;
    }
    return true;
}
*/
