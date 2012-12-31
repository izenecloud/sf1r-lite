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
    if( !(con_docid_key_.empty()) )
        con_docid_key_.clear();
    if( !(con_key_docid_.empty()) )
        con_key_docid_.clear();
    con_key_ = 0;
    return true;
}

bool ImgDupDetector::ClearHistoryUrl()
{
    if( !(url_docid_key_.empty()) )
        url_docid_key_.clear();
    if( !(url_key_docid_.empty()) )
        url_key_docid_.clear();
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

bool ImgDupDetector::DupDetectorMain()
{
    ImgDupDetector::SetController();
    ImgDupDetector::SetPath();
    ImgDupDetector::ClearHistory();

    if(log_info_)
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
        }
        else
        {
            ImgDupDetector::BuildUrlIndex(scd_file, psm_path_noin_url_);
            ImgDupDetector::DetectUrl(scd_file, psm_path_noin_url_, res_file, output_path);
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
        }
        else
        {
            ImgDupDetector::BuildConIndex(scd_file, psm_path_noin_con_);
            ImgDupDetector::DetectCon(scd_file, psm_path_noin_con_, res_file, output_path);
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
        }
        else
        {
            ImgDupDetector::BuildConIndex(scd_file, psm_path_noin_con_);
            ImgDupDetector::DetectCon(scd_file, psm_path_noin_con_, res_file, output_path);
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
        url_docid_key_[docID] = url_key_;
        url_key_docid_[url_key_] = docID;
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
        url_docid_key_[docID] = url_key_;
        url_key_docid_[url_key_] = docID;
        url_key_++;
    }

    if(!psm.Build())
    {
        LOG(ERROR) << "psm build error " << std::endl;
        return false;
    }
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
        con_docid_key_[docID] = con_key_;
        con_key_docid_[con_key_] = docID;
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
        con_docid_key_[docID] = con_key_;
        con_key_docid_[con_key_] = docID;
        con_key_++;
    }

    if(!psm.Build())
    {
        LOG(ERROR) << "psm build error " << std::endl;
        return false;
    }
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
        if(!psm.Search(doc_vector, attach, match_key))
        {
            if(writer.Append(scddoc)) rest++;
        }
        else if(url_docid_key_[docID] == match_key)
        {
            if(writer.Append(scddoc)) rest++;
        }
        else
        {
            /****Matches****/
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
        if(!psm.Search(doc_vector, attach, match_key))
        {
            if(writer.Append(scddoc)) rest++;
        }
        else if(con_docid_key_[docID] == match_key)
        {
            if(writer.Append(scddoc)) rest++;
        }
        else
        {
            /****Matches****/
        }
    }
    LOG(INFO)<<"Total: "<<n<<" Rest: "<<rest<< std::endl;
    return true;
}
