#include "img_dup_detector.h"
#include <util/ClockTimer.h>
#include <util/filesystem.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <dirent.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <boost/lexical_cast.hpp>
#include <fstream>

#define MAX_BUF_SIZE 1024

using namespace sf1r;
uint32_t history = 0;

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
        int icl,
        std::string icn)
:scd_path_(sp), output_path_(op), source_name_(sn), log_info_(li), incremental_mode_(im), controller_(con), image_content_length_(icl), img_content_name_(icn), dup_by_url_(false), dup_by_con_(false)
{
    SetPsmK(400);
}

bool ImgDupDetector::SetParams(std::string sp,
        std::string op,
        std::string sn,
        bool li,
        bool im,
        int con,
        int icl,
        std::string icn)
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
    img_content_name_ = icn;
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
    scd_temp_path_ = output_path_ + "/../temp";

    psm_path_ = output_path_ + "/../psm";
    psm_path_incr_ = psm_path_ + "/incr";
    psm_path_noin_ = psm_path_ + "/noin";
    psm_path_incr_url_ = psm_path_ + "/incr/url";
    psm_path_noin_url_ = psm_path_ + "/noin/url";
    psm_path_incr_con_ = psm_path_ + "/incr/con";
    psm_path_noin_con_ = psm_path_ + "/noin/con";

    boost::filesystem::create_directories(output_path_);
    boost::filesystem::create_directories(output_path_+"/../leveldb");
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
    boost::filesystem::create_directories(output_path_+"/../fujimap");
    con_docid_key_ = new ImgDupFujiMap(output_path_ + "/../fujimap/tmp0.kf");
    con_docid_key_->open();

    con_key_docid_ = new ImgDupFujiMap(output_path_ + "/../fujimap/tmp1.kf");
    con_key_docid_->open();

    url_docid_key_ = new ImgDupFujiMap(output_path_ + "/../fujimap/tmp2.kf");
    url_docid_key_->open();

    url_key_docid_ = new ImgDupFujiMap(output_path_ + "/../fujimap/tmp3.kf");
    url_key_docid_->open();

    docid_docid_ = new ImgDupFujiMap(output_path_ + "/../fujimap/tmp4.kf");
    docid_docid_->open();

    gid_memcount_ = new ImgDupFujiMap(output_path_ + "/../fujimap/tmp5.kf");
    gid_memcount_->open();

    return true;
}

bool ImgDupDetector::InitLevelDb()
{
    docidImgDbTable = new DocidImgDbTable(output_path_+"/../leveldb/docid_imgurl.db");
    docidImgDbTable->open();
    return true;
}
bool ImgDupDetector::SaveFujiMap()
{
/*
    con_docid_key_->save(output_path_ + "/../fujimap/tmp0.index");
    con_key_docid_->save(output_path_ + "/../fujimap/tmp1.index");
    url_docid_key_->save(output_path_ + "/../fujimap/tmp2.index");
    url_key_docid_->save(output_path_ + "/../fujimap/tmp3.index");
*/
    docid_docid_->save(output_path_ + "/../fujimap/tmp4.index");
//    gid_memcount_->save(output_path_ + "/../fujimap/tmp5.index");

    std::string statefile = output_path_ + "/../fujimap/state";
    ofstream fout(statefile.c_str());
    fout << url_key_ << " " << con_key_ << " " << history;

    return true;
}

bool ImgDupDetector::LoadFujiMap()
{
    con_docid_key_->load(output_path_ + "/../fujimap/tmp0.index");
    con_key_docid_->load(output_path_ + "/../fujimap/tmp1.index");
    url_docid_key_->load(output_path_ + "/../fujimap/tmp2.index");
    url_key_docid_->load(output_path_ + "/../fujimap/tmp3.index");
    docid_docid_->load(output_path_ + "/../fujimap/tmp4.index");
    gid_memcount_->load(output_path_ + "/../fujimap/tmp5.index");

    std::string statefile = output_path_ + "/../fujimap/state";
    ifstream fin(statefile.c_str());
    fin >> url_key_ >> con_key_;

    return true;
}

bool ImgDupDetector::DupDetectorMain()
{
    ImgDupDetector::SetController();
    ImgDupDetector::SetPath();
    ImgDupDetector::InitFujiMap();
    ImgDupDetector::InitLevelDb();
    ImgDupDetector::ClearHistory();
    ImgDupFileManager::get()->SetParam(scd_path_, output_path_, docid_docid_, gid_memcount_, docidImgDbTable);


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

        LOG(INFO) <<"Image Content name: " << img_content_name_ << endl;
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
                if(incremental_mode_)
                    ImgDupDetector::LoadFujiMap();
                std::string filename = std::string(event->name);
                BeginToDupDetect(filename);
                LOG(INFO)<<"Finish processing: "<<filename<<std::endl;
                if(incremental_mode_)
                {
                    ImgDupDetector::BuildGidMem();
                    ImgDupDetector::SaveFujiMap();
                }
//                if(incremental_mode_)
//                    ImgDupFileManager::get()->ReBuildAll();
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
            ImgDupDetector::WriteCurrentFile(filename);
        }
        else
        {
            ImgDupDetector::BuildUrlIndex(scd_file, psm_path_noin_url_);
            ImgDupDetector::DetectUrl(scd_file, psm_path_noin_url_, res_file, output_path);
            ImgDupDetector::WriteCurrentFile(filename);
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
            url_docid_key_->save(output_path_ + "/../fujimap/tmp2.index");
            url_key_docid_->save(output_path_ + "/../fujimap/tmp3.index");
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
            con_docid_key_->save(output_path_ + "/../fujimap/tmp0.index");
            con_key_docid_->save(output_path_ + "/../fujimap/tmp1.index");
            ImgDupDetector::WriteCurrentFile(filename);
        }
        else
        {
            ImgDupDetector::BuildConIndex(scd_file, psm_path_noin_con_);
            ImgDupDetector::DetectCon(scd_file, psm_path_noin_con_, res_file, output_path);
            ImgDupDetector::WriteCurrentFile(filename);
            B5MHelper::PrepareEmptyDir(psm_path_noin_con_);
            ImgDupDetector::ClearHistoryCon();
        }

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
            ImgDupDetector::WriteCurrentFile(filename);
        }
        else
        {
            ImgDupDetector::BuildConIndex(scd_file, psm_path_noin_con_);
            ImgDupDetector::DetectCon(scd_file, psm_path_noin_con_, res_file, output_path);
            ImgDupDetector::WriteCurrentFile(filename);
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
        if( !url_key_docid_->insert(url_key_, DocidToUint(docID)))
            LOG(INFO) << "Item exist! " << std::endl;
        url_docid_key_->insert(DocidToUint(docID), url_key_);
        if(log_info_ && !log_info_)
        {
            key_url_map_[url_key_] = doc["Img"];
            key_con_map_[url_key_] = doc[img_content_name_];
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
        url_key_docid_->insert(url_key_, DocidToUint(docID));
        url_docid_key_->insert(DocidToUint(docID), url_key_);
        if(log_info_ && !log_info_)
        {
            key_url_map_[url_key_] = doc["Img"];
            key_con_map_[url_key_] = doc[img_content_name_];
        }
        url_key_++;
    }

    if(!psm.Build())
    {
        LOG(ERROR) << "psm build error " << std::endl;
        return false;
    }

    if( url_docid_key_->build() < 0 )
    {
        LOG(ERROR) << "FujiMap build error [url_docid_key_]" << endl;
        LOG(ERROR) << "Error info: " << url_docid_key_->what() << endl;
        return false;
    }

    if( url_key_docid_->build() < 0 )
    {
        LOG(ERROR) << "FujiMap build error [url_key_docid_]" <<endl;
        LOG(ERROR) << "Error info: " << url_key_docid_->what() << endl;
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
        if(!PsmHelper::GetPsmItemCon(analyzer, doc, docID, doc_vector, attach, image_content_length_, img_content_name_))
        {
            continue;
        }
        psm.Insert(con_key_, doc_vector, attach);
        con_key_docid_->insert(con_key_, DocidToUint(docID));
        con_docid_key_->insert(DocidToUint(docID), con_key_);
        if(log_info_)
        {
            key_con_map_[con_key_] = doc[img_content_name_];
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
        if(!PsmHelper::GetPsmItemCon(analyzer, doc, docID, doc_vector, attach, image_content_length_, img_content_name_))
        {
            continue;
        }
        psm.Insert(con_key_, doc_vector, attach);
        con_key_docid_->insert(con_key_, DocidToUint(docID));
        con_docid_key_->insert(DocidToUint(docID), con_key_);
        if(log_info_)
        {
            key_con_map_[con_key_] = doc[img_content_name_];
            key_url_map_[con_key_] = doc["Img"];
        }
        con_key_++;
    }

    if(!psm.Build())
    {
        LOG(ERROR) << "psm build error " << std::endl;
        return false;
    }

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
    ScdWriter writer(output_path, UPDATE_SCD);
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
            writer.Append(scddoc);
            rest++;
            continue;
        }
        uint32_t match_key;
        uint32_t current_key;
        uint32_t current_docid = DocidToUint(docID);
        if(!psm.Search(doc_vector, attach, match_key))
        {
            writer.Append(scddoc);
            rest++;
        }
        else if(!url_docid_key_->get(current_docid, current_key))
        {
            LOG(INFO) << "FujiMap Get ERROR: docID[" << docID << "]"
                      << "current_key[" << current_key <<"]"
                      << "match_key" << match_key <<"]" <<endl;
            writer.Append(scddoc);
            rest++;
        }
        else if(current_key == match_key)
        {
            writer.Append(scddoc);
            rest++;
        }
        else
        {
              /****Matches****/
            uint32_t match_docid;
            if( docid_docid_->get(current_docid, match_docid) )
            {
//                LOG(INFO) << current_docid << endl;
                continue;
            }

            if( !url_key_docid_->get(match_key, match_docid))
            {
                LOG(INFO) << "FujiMap Get ERROR: match_key[" << match_key <<"]"
                          << "match_docid["<<match_docid<<"]"<<endl;
            }

            uint32_t match_match;
            if( docid_docid_->get(match_docid, match_match) )
            {
                match_docid = match_match;
            }
            if(current_docid == match_docid)
            {
//                LOG(INFO) << current_docid <<endl;
                writer.Append(scddoc);
                rest++;
                history++;
                continue;
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
                    docid_docid_->update(iter->second[i], match_docid);
                }
                gid_docids_.erase(iter);
            }

            uint32_t gmemcount;
            if( gid_memcount_->get(current_docid, gmemcount) )
            {
                gid_memcount_->update(current_docid, 0);
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
    ScdWriter writer(output_path, UPDATE_SCD);
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
        if(!PsmHelper::GetPsmItemCon(analyzer, doc, docID, doc_vector, attach, image_content_length_, img_content_name_))
        {
            rest++;
            continue;
        }
        uint32_t match_key;
        uint32_t current_key;
        uint32_t current_docid = DocidToUint(docID);
        std::string img_url_str;
        doc["Img"].convertString(img_url_str, izenelib::util::UString::UTF_8);
        docidImgDbTable->add_item(current_docid, img_url_str);

        if(!psm.Search(doc_vector, attach, match_key))
        {
            rest++;
        }
        else if( !con_docid_key_->get(current_docid, current_key) )
        {
            LOG(INFO) << "FujiMap Get ERROR: docID[" << docID << "]"
                      << "current_key[" << current_key <<"]"
                      << "match_key" << match_key <<"]" <<endl;
            rest++;
        }
        else if(current_key == match_key)
        {
            rest++;
        }
        else
        {
            /****Matches****/
            uint32_t match_docid;
            if( docid_docid_->get(current_docid, match_docid) )
            {
//                LOG(INFO) << current_docid << endl;
                continue;
            }
            if( !con_key_docid_->get(match_key, match_docid))
            {
                LOG(INFO) << "FujiMap Get ERROR: match_key[" << match_key <<"]"
                          << "match_docid["<<match_docid<<"]"<<endl;
            }

            uint32_t match_match;
            if( docid_docid_->get(match_docid, match_match))
            {
                match_docid = match_match;
            }

            if(current_docid == match_docid)
            {
//                LOG(INFO) << current_docid << endl;
                history++;
                rest++;
                continue;
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
                    docid_docid_->update(iter->second[i], match_docid);
                }
                gid_docids_.erase(iter);
            }

            uint32_t gmemcount;
            if( gid_memcount_->get(current_docid, gmemcount) )
            {
                gid_memcount_->update(current_docid, 0);
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
        }
    }
    LOG(INFO)<<"Total: "<<n<<" Rest: "<<rest<< std::endl;
    return true;
}

bool ImgDupDetector::BuildGidMem()
{
    std::map<uint32_t, std::vector<uint32_t> >::iterator iter = gid_docids_.begin();
    for(;iter != gid_docids_.end(); iter++)
    {
        uint32_t count;
        if(gid_memcount_->get(iter->first, count))
        {
            gid_memcount_->update(iter->first, count + iter->second.size());
        }
        else
        {
            gid_memcount_->update(iter->first, iter->second.size());
        }
    }
    gid_memcount_->build();
    gid_memcount_->save(output_path_ + "/../fujimap/tmp5.index");
    if(!gid_docids_.empty())
        gid_docids_.clear();
    return true;
}

bool ImgDupDetector::WriteCurrentFile(const std::string& filename)
{
    if( docid_docid_->build() < 0 )
    {
        LOG(ERROR) <<"FujiMap build error [docid_docid_]" <<endl;
        LOG(ERROR) <<"Error info: " <<docid_docid_->what() <<endl;
        return false;
    }

    std::string scd_file = scd_path_ + "/" + filename;
    LOG(INFO)<<"Processing (WriteFile) "<<scd_file<<std::endl;
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    uint32_t n=0;
    uint32_t rest=0;

    boost::filesystem::create_directories(output_path_+"/0");
    boost::filesystem::create_directories(output_path_+"/1");

    ScdWriter writer0(output_path_+"/0", UPDATE_SCD);
    ScdWriter writer1(output_path_+"/1", UPDATE_SCD);

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
            std::string guangURL;
            if(!docidImgDbTable->get_item(match_docid, guangURL))
            {
                LOG(INFO) << "Find no img url..." << endl;
                scddoc.puch_back(std::pair<std::string, UString>("guangURL", doc["Img"]));
            }
            else
            {
                UString gURL;
                gURL.assign(guangURL, izenelib::util::UString::UTF_8);
                scddoc.push_back(std::pair<std::string, UString>("guangURL", gURL));
            }
            writer0.Append(scddoc);
        }
        else
        {
            //saved
            rest++;
            scddoc.push_back(std::pair<std::string, UString>("GID", doc["DOCID"]));
            scddoc.push_back(std::pair<std::string, UString>("guangURL", doc["Img"]));
            writer0.Append(scddoc);

            std::map<uint32_t, std::vector<uint32_t> >::iterator iter = gid_docids_.find(current_docid);
            if(iter == gid_docids_.end())
            {
                UString gmemcount;
                gmemcount.assign("1", izenelib::util::UString::UTF_8);
                scddoc.push_back(std::pair<std::string, UString>("GMemCount", gmemcount));
                writer1.Append(scddoc);
            }
            else
            {
                UString gmemcount;
                std::string count = boost::lexical_cast<std::string> (iter->second.size()+1);
                gmemcount.assign(count, izenelib::util::UString::UTF_8);
                scddoc.push_back(std::pair<std::string, UString>("GMemCount", gmemcount));
                writer1.Append(scddoc);
            }
        }
    }
    LOG(INFO)<<"Total: "<<n<<" Rest: "<<rest<< std::endl;
    return true;
}
