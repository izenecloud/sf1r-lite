#include "img_dup_detector.h"
#include <util/ClockTimer.h>
#include <util/filesystem.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <dirent.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <boost/lexical_cast.hpp>

#define MAX_BUF_SIZE 1024

using namespace sf1r;

ImgDupDetector::ImgDupDetector()
:psmk_(400)
{
}

ImgDupDetector::ImgDupDetector(std::string sp,
        std::string op,
        std::string sn,
        bool li,
        bool im,
        int con,
        int icl)
:scd_path_(sp), output_path_(op), source_name_(sn), log_info_(li), incremental_mode_(im), controller_(con), image_content_length_(icl), dup_by_url_(false), dup_by_con_(false)
{
    SetPsmK(400);
}

bool ImgDupDetector::SetParams(std::string sp,
        std::string op,
        std::string sn,
        bool li,
        bool im,
        int con,
        int icl)
{
    SetPsmK(400);
    scd_path_ = sp;
    scd_path_ = sp;
    output_path_ = op;
    source_name_ = sn;
    log_info_ = li;
    incremental_mode_ = im;
    controller_ = con;
    image_content_length_ = icl;
    dup_by_url_ = false;
    dup_by_con_ = false;
    return true;
}

ImgDupDetector::~ImgDupDetector()
{
}

uint32_t DocidToUint(const std::string& docid )
{
    return boost::lexical_cast<uint32_t>(docid);
}
std::string UintToDocid(const uint32_t docid)
{
    return boost::lexical_cast<std::string>(docid);
}

bool ImgDupDetector::SetController()
{
    if( (controller_ % 2) == 1)
        dup_by_url_ = true;
    if( ((controller_/2) %2) == 1)
        dup_by_con_ = true;
    return true;
}

bool ImgDupDetector::ClearHistory()
{
    if( !ClearHistoryCon()) return false;
    if( !ClearHistoryUrl()) return false;
    return true;
}

bool ImgDupDetector::ClearHistoryCon()
{
    if( !(con_docid_key_->empty()) )
        con_docid_key_->clear();
    if( !(con_key_docid_->empty()) )
        con_key_docid_->clear();

    if( !(key_con_map_.empty()) )
        key_con_map_.clear();
    if( !(key_url_map_.empty()) )
        key_url_map_.clear();
    con_key_ = 0;
    return true;
}

bool ImgDupDetector::ClearHistoryUrl()
{
    if( !(url_docid_key_->empty()) )
        url_docid_key_->clear();
    if( !(url_key_docid_->empty()) )
        url_key_docid_->clear();

    if( !(key_con_map_.empty()) )
        key_con_map_.clear();
    if( !(key_url_map_.empty()) )
        key_url_map_.clear();
    url_key_ = 0;
    return true;
}

bool ImgDupDetector::SetPath()
{
    scd_temp_path_ = output_path_ + "/temp";

    psm_path_ = output_path_ + "/psm";
    psm_path_incr_ = psm_path_ + "/incr";
    psm_path_noin_ = psm_path_ + "/noin";
    psm_path_incr_url_ = psm_path_ + "/incr/url";
    psm_path_noin_url_ = psm_path_ + "/noin/url";
    psm_path_incr_con_ = psm_path_ + "/incr/con";
    psm_path_noin_con_ = psm_path_ + "/noin/con";

    boost::filesystem::create_directories(output_path_);
    boost::filesystem::create_directories(psm_path_);
    boost::filesystem::create_directories(psm_path_incr_);
    boost::filesystem::create_directories(psm_path_incr_url_);
    boost::filesystem::create_directories(psm_path_incr_con_);

    B5MHelper::PrepareEmptyDir(scd_temp_path_);
    B5MHelper::PrepareEmptyDir(psm_path_noin_);
    B5MHelper::PrepareEmptyDir(psm_path_noin_url_);
    B5MHelper::PrepareEmptyDir(psm_path_noin_con_);

    return true;
}

bool ImgDupDetector::InitFujiMap()
{
    con_docid_key_ = new ImgDupFujiMap(output_path_ + "/fujimap/tmp0.kf");
    con_docid_key_->open();

    con_key_docid_ = new ImgDupFujiMap(output_path_ + "/fujimap/tmp1.kf");
    con_key_docid_->open();

    url_docid_key_ = new ImgDupFujiMap(output_path_ + "/fujimap/tmp2.kf");
    url_docid_key_->open();

    url_key_docid_ = new ImgDupFujiMap(output_path_ + "/fujimap/tmp3.kf");
    url_key_docid_->open();

    docid_docid_ = new ImgDupFujiMap(output_path_ + "/fujimap/tmp4.kf");
    docid_docid_->open();

    return true;
}

bool ImgDupDetector::DupDetectorMain()
{
    ImgDupDetector::SetController();
    ImgDupDetector::SetPath();
    ImgDupDetector::InitFujiMap();
    ImgDupDetector::ClearHistory();


    if(log_info_ || !log_info_)
    {
        if(incremental_mode_)
            LOG(INFO) << "The image dd server works in incremental mode! " << std::endl;
        else
            LOG(INFO) << "The image dd server does not work in incremental mode! " << std::endl;
        if(dup_by_url_)
            LOG(INFO) << "Dup detected by image url ... " << std::endl;
        if(dup_by_con_)
            LOG(INFO) << "Dup detected by image content ..." << std::endl;

        LOG(INFO)<<"ShareSourceName to be deleted: " << source_name_ << std::endl;

        LOG(INFO)<<"Image content length: " << image_content_length_ << std::endl;
    }

    int fd, wd;
    int len, index;
    char buffer[1024];
    struct inotify_event *event;
    fd = inotify_init();

    if(fd < 0)
    {
        LOG(ERROR) << "Failed to initialize inotify." << std::endl;
        return false;
    }
    wd = inotify_add_watch(fd, scd_path_.c_str(), IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_TO);

    if(wd < 0)
    {
        LOG(ERROR)<<"Can't add watch for: "<<scd_path_<<std::endl;
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
            if(event -> mask & IN_CLOSE_WRITE || event -> mask & IN_MOVED_TO)
            {
                std::string filename = std::string(event->name);
                BeginToDupDetect(filename);
                LOG(INFO)<<"Finish processing: "<<filename<<std::endl;
            }
            index += sizeof(struct inotify_event)+event->len;
        }
    }

    return true;
}

bool ImgDupDetector::BeginToDupDetect(const std::string& filename)
{
    if(dup_by_url_ && !dup_by_con_)
    {
        std::string scd_file = scd_path_ + "/" + filename;
        std::string res_file = filename;
        std::string output_path = output_path_;
        if(incremental_mode_)
        {
            ImgDupDetector::BuildUrlIndex(scd_file, psm_path_incr_url_);
            ImgDupDetector::DetectUrl(scd_file, psm_path_incr_url_, res_file, output_path);
            ImgDupDetector::WriteFile(filename);
        }
        else
        {
            ImgDupDetector::BuildUrlIndex(scd_file, psm_path_noin_url_);
            ImgDupDetector::DetectUrl(scd_file, psm_path_noin_url_, res_file, output_path);
            ImgDupDetector::WriteFile(filename);
            B5MHelper::PrepareEmptyDir(psm_path_noin_url_);
            ImgDupDetector::ClearHistoryUrl();
        }
    }

    if(dup_by_url_ && dup_by_con_)
    {
        std::string scd_file = scd_path_ + "/" + filename;
        std::string res_file = filename;
        std::string output_path = scd_temp_path_;
        if(incremental_mode_)
        {
            ImgDupDetector::BuildUrlIndex(scd_file, psm_path_incr_url_);
            ImgDupDetector::DetectUrl(scd_file, psm_path_incr_url_, res_file, output_path);
        }
        else
        {
            ImgDupDetector::BuildUrlIndex(scd_file, psm_path_noin_url_);
            ImgDupDetector::DetectUrl(scd_file, psm_path_noin_url_, res_file, output_path);
            B5MHelper::PrepareEmptyDir(psm_path_noin_url_);
            ImgDupDetector::ClearHistoryUrl();
        }

        scd_file = output_path + "/" + res_file;
        res_file = filename;
        output_path = output_path_;

        if(incremental_mode_)
        {
            ImgDupDetector::BuildConIndex(scd_file, psm_path_incr_con_);
            ImgDupDetector::DetectCon(scd_file, psm_path_incr_con_, res_file, output_path);
            ImgDupDetector::WriteFile(filename);
        }
        else
        {
            ImgDupDetector::BuildConIndex(scd_file, psm_path_noin_con_);
            ImgDupDetector::DetectCon(scd_file, psm_path_noin_con_, res_file, output_path);
            ImgDupDetector::WriteFile(filename);
            B5MHelper::PrepareEmptyDir(psm_path_noin_con_);
            ImgDupDetector::ClearHistoryCon();
        }

//        B5MHelper::PrepareEmptyDir(scd_temp_path_);

    }

    if(!dup_by_url_ && dup_by_con_)
    {
        std::string scd_file = scd_path_ + "/" + filename;
        std::string res_file = filename;
        std::string output_path = output_path_;
        if(incremental_mode_)
        {
            ImgDupDetector::BuildConIndex(scd_file, psm_path_incr_con_);
            ImgDupDetector::DetectCon(scd_file, psm_path_incr_con_, res_file, output_path);
            ImgDupDetector::WriteFile(filename);
        }
        else
        {
            ImgDupDetector::BuildConIndex(scd_file, psm_path_noin_con_);
            ImgDupDetector::DetectCon(scd_file, psm_path_noin_con_, res_file, output_path);
            ImgDupDetector::WriteFile(filename);
            B5MHelper::PrepareEmptyDir(psm_path_noin_con_);
            ImgDupDetector::ClearHistoryCon();
        }
    }
    return true;
}

bool ImgDupDetector::BuildUrlIndex(const std::string& scd_file, const std::string& psm_path)
{
    ProductTermAnalyzer analyzer(cma_path_);
    PsmType psm(psm_path);
    psm.SetK(psmk_);
    psm.Open();

    LOG(INFO) << "Processing " << scd_file << std::endl;

    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    uint32_t n = 0;

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
        if(sourceName.compare(source_name_) == 0)
        {
            continue;
        }

        std::string docID;
        std::vector<std::pair<std::string, double> > doc_vector;
        PsmAttach attach;
        if(!PsmHelper::GetPsmItem(analyzer, doc, docID, doc_vector, attach))
        {
            continue;
        }
        psm.Insert(url_key_, doc_vector, attach);
        //url_docid_key_[boost::lexical_cast<uint32_t>(docID)] = url_key_;
        //url_key_docid_[url_key_] = boost::lexical_cast<uint32_t>(docID);
        url_key_docid_->insert(url_key_, DocidToUint(docID));
        url_docid_key_->insert(DocidToUint(docID), url_key_);
        if(log_info_ && !log_info_)
        {
            key_url_map_[url_key_] = doc["Img"];
            key_con_map_[url_key_] = doc["Content"];
        }
        url_key_++;
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
        if(sourceName.compare(source_name_) != 0)
        {
            continue;
        }
        std::string docID;
        std::vector<std::pair<std::string, double> > doc_vector;
        PsmAttach attach;
        if(!PsmHelper::GetPsmItem(analyzer, doc, docID, doc_vector, attach))
        {
            continue;
        }
        psm.Insert(url_key_, doc_vector, attach);
        //url_docid_key_[boost::lexical_cast<uint32_t>(docID)] = url_key_;
        //url_key_docid_[url_key_] = boost::lexical_cast<uint32_t>(docID);
        url_key_docid_->insert(url_key_, DocidToUint(docID));
        url_docid_key_->insert(DocidToUint(docID), url_key_);
        if(log_info_ && !log_info_)
        {
            key_url_map_[url_key_] = doc["Img"];
            key_con_map_[url_key_] = doc["Content"];
        }
        url_key_++;
    }


    if(!psm.Build())
    {
        LOG(ERROR) << "psm build error " << std::endl;
        return false;
    }
/*
    if( url_docid_key_->build() < 0 )
    {
        LOG(ERROR) << "FujiMap build error [url_docid_key_]" <<endl;
        LOG(ERROR) << "Error info: " << url_docid_key_->what() << endl;
        return false;
    }

    if( url_key_docid_->build() < 0 )
    {
        LOG(ERROR) << "FujiMap build error [url_key_docid_]" <<endl;
        LOG(ERROR) << "Error info: " << url_key_docid_->what() << endl;
        return false;
    }
*/
    return true;
}

bool ImgDupDetector::BuildConIndex(const std::string& scd_file, const std::string& psm_path)
{
    ProductTermAnalyzer analyzer(cma_path_);
    PsmType psm(psm_path);
    psm.SetK(psmk_);
    psm.Open();

    LOG(INFO) << "Processing " << scd_file << std::endl;

    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    uint32_t n = 0;

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
        if(sourceName.compare(source_name_) == 0)
        {
            continue;
        }
        std::string docID;
        std::vector<std::pair<std::string, double> > doc_vector;
        PsmAttach attach;
        if(!PsmHelper::GetPsmItemCon(analyzer, doc, docID, doc_vector, attach, image_content_length_))
        {
            continue;
        }
        psm.Insert(con_key_, doc_vector, attach);
        //con_docid_key_[boost::lexical_cast<uint32_t>(docID)] = con_key_;
        //con_key_docid_[con_key_] = boost::lexical_cast<uint32_t>(docID);
        con_key_docid_->insert(con_key_, DocidToUint(docID));
        con_docid_key_->insert(DocidToUint(docID), con_key_);
        if(log_info_)
        {
            key_con_map_[con_key_] = doc["Content"];
            key_url_map_[con_key_] = doc["Img"];
        }
        con_key_++;
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
        if(sourceName.compare(source_name_) != 0)
        {
            continue;
        }
        std::string docID;
        std::vector<std::pair<std::string, double> > doc_vector;
        PsmAttach attach;
        if(!PsmHelper::GetPsmItemCon(analyzer, doc, docID, doc_vector, attach, image_content_length_))
        {
            continue;
        }
        psm.Insert(con_key_, doc_vector, attach);
        //con_docid_key_[boost::lexical_cast<uint32_t>(docID)] = con_key_;
        //con_key_docid_[con_key_] = boost::lexical_cast<uint32_t>(docID);
        con_key_docid_->insert(con_key_, DocidToUint(docID));
        con_docid_key_->insert(DocidToUint(docID), con_key_);
        if(log_info_)
        {
            key_con_map_[con_key_] = doc["Content"];
            key_url_map_[con_key_] = doc["Img"];
        }
        con_key_++;
    }

    if(!psm.Build())
    {
        LOG(ERROR) << "psm build error " << std::endl;
        return false;
    }
/*
    if( con_key_docid_->build() < 0 )
    {
        LOG(ERROR) <<"FujiMap build error [con_key_docid_]"<<endl;
        LOG(ERROR) <<"Error info: " << con_key_docid_->what() <<endl;
        return false;
    }
    if( con_docid_key_->build() < 0 )
    {
        LOG(ERROR) <<"FujiMap build error [con_docid_key_]" <<endl;
        LOG(ERROR) <<"Error info: " <<con_docid_key_->what() <<endl;
        return false;
    }
*/
    return true;
}

bool ImgDupDetector::DetectUrl(const std::string& scd_file, const std::string& psm_path, const std::string& res_file, const std::string& output_path)
{
    ProductTermAnalyzer analyzer(cma_path_);
    PsmType psm(psm_path);
    psm.SetK(psmk_);
    psm.Open();

    LOG(INFO)<<"Processing "<<scd_file<<std::endl;

    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    uint32_t n=0;
    uint32_t rest=0;
    ScdWriter writer(output_path, 2);
    writer.SetFileName(res_file);

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
        uint32_t current_key;
        uint32_t current_docid = DocidToUint(docID);
        if(!psm.Search(doc_vector, attach, match_key))
        {
            if(writer.Append(scddoc)) rest++;
        }
        else if(!url_docid_key_->get(current_docid, current_key))
        {
            LOG(INFO) << "FujiMap Get ERROR: docID[" << docID << "]"
                      << "current_key[" << current_key <<"]"
                      << "match_key" << match_key <<"]" <<endl;
            if(writer.Append(scddoc)) rest++;
        }
        else if(current_key == match_key)
        {
            if(writer.Append(scddoc)) rest++;
        }
        else
        {
              /****Matches****/
            uint32_t match_docid;
            if( !url_key_docid_->get(match_key, match_docid))
            {
                LOG(INFO) << "FujiMap Get ERROR: match_key[" << match_key <<"]"
                          << "match_docid["<<match_docid<<"]"<<endl;
            }
            docid_docid_->update(current_docid, match_docid);
            std::map<uint32_t, std::vector<uint32_t> >::iterator iter = gid_docids_.find(current_docid);
            std::vector<uint32_t> temp_v;
            temp_v.push_back(current_docid);

            if( iter != gid_docids_.end() )
            {
                uint32_t i;
                for(i=0;i<iter->second.size();i++)
                {
                    docid_docid_->update(iter->second[i], match_docid);
                    temp_v.push_back(iter->second[i]);
                }
                gid_docids_.erase(iter);
            }
            iter = gid_docids_.find(match_docid);
            if(iter != gid_docids_.end() )
            {
                uint32_t i;
                for(i=0;i<temp_v.size();i++)
                {
                    iter->second.push_back(temp_v[i]);
                }
            }
            else
            {
                gid_docids_[match_docid] = temp_v;
            }

/*
            if(log_info_ && !log_info_)
            {
                std::string current_url;
                std::string current_content;
                std::string match_url;
                std::string match_content;
                doc["Img"].convertString(current_url,izenelib::util::UString::UTF_8);
                doc["Content"].convertString(current_content, izenelib::util::UString::UTF_8);
                key_url_map_[match_key].convertString(match_url, izenelib::util::UString::UTF_8);
                key_con_map_[match_key].convertString(match_content, izenelib::util::UString::UTF_8);


                LOG(INFO)<<std::endl<<"DOCID: "<<docID
                         <<std::endl<<"ImgUrl: "<<current_url
                         <<std::endl<<"Content: "<<current_content
                         <<std::endl<<std::endl<<"DOCID: "<<url_key_docid_[match_key]
                         <<std::endl<<"ImgUrl: "<<match_url
                         <<std::endl<<"Content: "<<match_content
                         <<std::endl<<"Image URL Matches "<<std::endl<<std::endl<<std::endl;


            }
*/
        }
    }
    LOG(INFO)<<"Total: "<<n<<" Rest: "<<rest<< std::endl;
    return true;
}

bool ImgDupDetector::DetectCon(const std::string& scd_file, const std::string& psm_path, const std::string& res_file, const std::string& output_path)
{
    ProductTermAnalyzer analyzer(cma_path_);
    PsmType psm(psm_path);
    psm.SetK(psmk_);
    psm.Open();

    LOG(INFO)<<"Processing "<<scd_file<<std::endl;

    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    uint32_t n=0;
    uint32_t rest=0;
    ScdWriter writer(output_path, 2);
    writer.SetFileName(res_file);

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
        if(!PsmHelper::GetPsmItemCon(analyzer, doc, docID, doc_vector, attach, image_content_length_))
        {
            if(writer.Append(scddoc)) rest++;
            continue;
        }
        uint32_t match_key;
        uint32_t current_key;
        uint32_t current_docid = DocidToUint(docID);
        if(!psm.Search(doc_vector, attach, match_key))
        {
            if(writer.Append(scddoc)) rest++;
        }
        else if( !con_docid_key_->get(current_docid, current_key) )
        {
            LOG(INFO) << "FujiMap Get ERROR: docID[" << docID << "]"
                      << "current_key[" << current_key <<"]"
                      << "match_key" << match_key <<"]" <<endl;
            if(writer.Append(scddoc)) rest++;
        }
        else if(current_key == match_key)
        {
            if(writer.Append(scddoc)) rest++;
        }
        else
        {
            /****Matches****/
            uint32_t match_docid;
            if( !con_key_docid_->get(match_key, match_docid))
            {
                LOG(INFO) << "FujiMap Get ERROR: match_key[" << match_key <<"]"
                          << "match_docid["<<match_docid<<"]"<<endl;
            }
            docid_docid_->update(current_docid, match_docid);
            std::map<uint32_t, std::vector<uint32_t> >::iterator iter = gid_docids_.find(current_docid);
            std::vector<uint32_t> temp_v;
            temp_v.push_back(current_docid);

            if( iter != gid_docids_.end() )
            {
                uint32_t i;
                for(i=0;i<iter->second.size();i++)
                {
                    docid_docid_->update(iter->second[i], match_docid);
                    temp_v.push_back(iter->second[i]);
                }
                gid_docids_.erase(iter);
            }

            iter = gid_docids_.find(match_docid);
            if(iter != gid_docids_.end() )
            {
                uint32_t i;
                for(i=0;i<temp_v.size();i++)
                {
                    iter->second.push_back(temp_v[i]);
                }
            }
            else
            {
                gid_docids_[match_docid] = temp_v;
            }
/*
            if(log_info_)
            {
                std::string current_url;
                std::string current_content;
                std::string match_url;
                std::string match_content;
                doc["Img"].convertString(current_url,izenelib::util::UString::UTF_8);
                doc["Content"].convertString(current_content, izenelib::util::UString::UTF_8);
                key_url_map_[match_key].convertString(match_url, izenelib::util::UString::UTF_8);
                key_con_map_[match_key].convertString(match_content, izenelib::util::UString::UTF_8);


                LOG(INFO)<<std::endl<<"DOCID: "<<docID
                         <<std::endl<<"ImgUrl: "<<current_url
                         <<std::endl<<"Content: "<<current_content
                         <<std::endl<<std::endl<<"DOCID: "<<con_key_docid_[match_key]
                         <<std::endl<<"ImgUrl: "<<match_url
                         <<std::endl<<"Content: "<<match_content
                         <<std::endl<<"Image Content Matches "<<std::endl<<std::endl<<std::endl;


            }
*/
        }
    }
    LOG(INFO)<<"Total: "<<n<<" Rest: "<<rest<< std::endl;
    return true;
}

bool ImgDupDetector::WriteFile(const std::string& filename)
{
    std::string scd_file = scd_path_ + "/" + filename;
    LOG(INFO)<<"Processing (WriteFile) "<<scd_file<<std::endl;
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    uint32_t n=0;
    uint32_t rest=0;

    boost::filesystem::create_directories(output_path_+"/0");
    boost::filesystem::create_directories(output_path_+"/1");

    ScdWriter writer0(output_path_+"/0", 2);
    ScdWriter writer1(output_path_+"/1", 2);

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

            std::map<uint32_t, std::vector<uint32_t> >::iterator iter = gid_docids_.find(current_docid);
            if(iter == gid_docids_.end())
            {
                UString itemcount;
                itemcount.assign("1", izenelib::util::UString::UTF_8);
                scddoc.push_back(std::pair<std::string, UString>("ItemCount", itemcount));
                writer1.Append(scddoc);
            }
            else
            {
                UString itemcount;
                std::string count = boost::lexical_cast<std::string> (iter->second.size()+1);
                itemcount.assign(count, izenelib::util::UString::UTF_8);
                scddoc.push_back(std::pair<std::string, UString>("ItemCount", itemcount));
                writer1.Append(scddoc);
            }
        }
    }
    LOG(INFO)<<"Total: "<<n<<" Rest: "<<rest<< std::endl;
    return true;
}
