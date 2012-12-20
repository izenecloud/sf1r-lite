#include "img_dup_detector.h"
//#include <product-manager/product_term_analyzer.h>
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

bool findTheOneToLeft(sf1r::PsmAttach& attach, std::vector<sf1r::PsmAttach>& match_attach, std::string& toDelete)
{
    uint32_t i;
    for(i=0;i<match_attach.size();i++)
    {
        if(match_attach[i].source_name != toDelete)
            break;
    }
    if(i < match_attach.size())
    {
        if(attach.docid.compare(match_attach[i].docid) == 0)
            return true;
    }
    if(i == match_attach.size())
    {
        if(attach.docid.compare(match_attach[0].docid) == 0)
            return true;
    }
    return false;
}

bool ImgDupDetector::DupDetectByImgUrl(const std::string& scd_path, const std::string& output_path, std::string& toDelete)
{
    ProductTermAnalyzer analyzer(cma_path_);
/*
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.empty())
    {
        LOG(ERROR)<<"scd empty"<<std::endl;
        return false;
    }
*/
    boost::filesystem::create_directories(output_path);
    std::string psm_path = output_path+"/psm";
    B5MHelper::PrepareEmptyDir(psm_path);

    PsmType psm(psm_path);
    psm.SetK(psmk_);
    psm.Open();

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

    while(len = read(fd, buffer, MAX_BUF_SIZE))
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
                std::map<std::string, UString> imgurl_list;
                std::map<std::string, UString> source_name_list;
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
                        std::string key;
                        std::vector<std::pair<std::string, double> > doc_vector;
                        PsmAttach attach;
                        if(!PsmHelper::GetPsmItem(analyzer, doc, key, doc_vector, attach))
                        {
//                          LOG(INFO)<<"Get psm item Error ... "<<std::endl;
                            continue;
                        }
                        psm.Insert(key, doc_vector, attach);
                        imgurl_list[key] =doc["Img"];
                        source_name_list[key] = doc["ShareSourceName"];
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
                        std::string key;
                        std::vector<std::pair<std::string, double> > doc_vector;
                        PsmAttach attach;
                        if(!PsmHelper::GetPsmItem(analyzer, doc, key, doc_vector, attach))
                        {
                            LOG(INFO)<<"Get psm item Error ... "<<std::endl;
                            if(writer.Append(scddoc)) rest++;
                            continue;
                        }
                        std::string match_key;
                        if(!psm.Search(doc_vector, attach, match_key))
                        {
                            if(writer.Append(scddoc)) rest++;
                        }
                        else if(key == match_key)
                        {
                            if(writer.Append(scddoc)) rest++;
                        }
                        else
                        {
                            std::string current_url;
                            std::string match_url;
                            doc["Img"].convertString(current_url,izenelib::util::UString::UTF_8);
                            imgurl_list[match_key].convertString(match_url, izenelib::util::UString::UTF_8);
                            if(current_url.compare(match_url) == 0)
                            {
                                LOG(INFO)<<std::endl<<key<<":  "<<doc["Img"]
                                         <<std::endl<<match_key<<":  "<<imgurl_list[match_key]
                                         <<std::endl<<"Matches "<<std::endl;
                                std::cout<<std::endl<<key<<":  "<<doc["Img"]
                                         <<std::endl<<match_key<<":  "<<imgurl_list[match_key]
                                         <<std::endl<<"Matches "<<std::endl;
                            }
                            else
                            {
                                if(writer.Append(scddoc)) rest++;
                            }
                        }

                    }
/*
                        std::string key;
                        std::vector<std::pair<std::string, double> > doc_vector;
                        PsmAttach attach;
                        if(!PsmHelper::GetPsmItem(analyzer, doc, key, doc_vector, attach))
                        {
                            LOG(INFO)<<"Get psm item Error ... "<<std::endl;
                            if(writer.Append(scddoc)) rest++;
                            continue;
                        }
                        std::vector<std::string> match_key;
                        if(!psm.Search(doc_vector, attach, match_key))
                        {
                            if(writer.Append(scddoc)) rest++;
                        }
                        else if(match_key.size() == 1)
                        {
                            if(writer.Append(scddoc)) rest++;
                        }
                        else
                        {
                            uint32_t i;
                            for(i=0;i<match_key.size();i++)
                            {
                                if(source_name_list.find(match_key[i]) == source_name_list.end() || source_name_list[match_key[i]] != toDelete)
                                    break;
                            }
                            if(i < match_key.size())
                            {
                                if(match_key[i] == key)
                                    if(writer.Append(scddoc)) rest++;
                            }
                            if(i == match_key.size())
                            {
                                if(match_key[0] == key)
                                    if(writer.Append(scddoc)) rest++;
                            }
                        }
                    }
*/
                    LOG(INFO)<<"Total: "<<n<<" Rest: "<<rest<<std::endl;
                }

            }
            index += sizeof(struct inotify_event)+event->len;
        }
    }
/*
    std::map<std::string, UString> imgurl_list;
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
            std::map<std::string, UString> doc;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc[property_name] = p->second;
            }
            std::string key;
            std::vector<std::pair<std::string, double> > doc_vector;
            PsmAttach attach;
            if(!PsmHelper::GetPsmItem(analyzer, doc, key, doc_vector, attach))
            {
//                LOG(INFO)<<"Get psm item Error ... "<<std::endl;
                continue;
            }
            psm.Insert(key, doc_vector, attach);
            imgurl_list[key] =doc["Img"];
        }
    }
    if(!psm.Build())
    {
        LOG(ERROR)<<"psm build error"<<std::endl;
        return false;
    }
    psm.Open();
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
            std::map<std::string, UString> doc;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc[property_name] = p->second;
            }
            std::string key;
            std::vector<std::pair<std::string, double> > doc_vector;
            PsmAttach attach;
            if(!PsmHelper::GetPsmItem(analyzer, doc, key, doc_vector, attach))
            {
                LOG(INFO)<<"Get psm item Error ... "<<std::endl;
                if(writer.Append(scddoc)) rest++;
                continue;
            }
            std::string match_key;
            if(!psm.Search(doc_vector, attach, match_key))
            {
                if(writer.Append(scddoc)) rest++;
            }
            else if(key == match_key)
            {
                if(writer.Append(scddoc)) rest++;
            }
            else
            {
                std::string current_url;
                std::string match_url;
                doc["Img"].convertString(current_url,izenelib::util::UString::UTF_8);
                imgurl_list[match_key].convertString(match_url, izenelib::util::UString::UTF_8);
                if(current_url.compare(match_url) == 0)
                {
                    LOG(INFO)<<std::endl<<key<<":  "<<doc["Img"]
                             <<std::endl<<match_key<<":  "<<imgurl_list[match_key]
                             <<std::endl<<"Matches "<<std::endl;
                    std::cout<<std::endl<<key<<":  "<<doc["Img"]
                             <<std::endl<<match_key<<":  "<<imgurl_list[match_key]
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
    if(!psm.Build())
    {
        LOG(ERROR)<<"psm build error"<<std::endl;
        return false;
    }
*/
    return true;
}

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
