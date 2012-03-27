#include "uue_generator.h"
#include "log_server_client.h"

using namespace sf1r;

UueGenerator::UueGenerator(const LogServerConnectionConfig& config, OfferDb* odb)
:logserver_config_(config), odb_(odb)
{
}

UueGenerator::UueGenerator(OfferDb* odb)
:odb_(odb)
{
}

bool UueGenerator::Generate(const std::string& b5mo_scd, const std::string& result)
{
    //if(!LogServerClient::Init(logserver_config_))
    //{
        //std::cout<<"log server init failed"<<std::endl;
        //return false;
    //}
    //if(!LogServerClient::Test())
    //{
        //std::cout<<"log server test connection failed"<<std::endl;
        //return false;
    //}
    namespace bfs = boost::filesystem;
    std::vector<std::string> scd_list;
    B5MHelper::GetScdList(b5mo_scd, scd_list);
    if(scd_list.empty()) return false;
    //LOG(INFO)<<"loading odb to memory"<<std::endl;
    //boost::unordered_map<std::string, std::string> mem_odb;
    //OfferDb::cursor_type it = odb_->begin();
    //std::string key;
    //std::string value;
    //while(true)
    //{
        //if(!odb_->fetch(it, key, value)) break;
        //mem_odb.insert(std::make_pair(key, value));
        //odb_->iterNext(it);
    //}

    std::ofstream ofs(result.c_str());
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"processing "<<scd_file<<std::endl;
        int scd_type = ScdParser::checkSCDType(scd_list[i]);
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin(B5MHelper::B5MO_PROPERTY_LIST);
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%10000==0)
            {
                LOG(INFO)<<"Find Documents "<<n<<std::endl;
            }
            Document doc;
            SCDDoc& scddoc = *(*doc_iter);
            std::vector<std::pair<izenelib::util::UString, izenelib::util::UString> >::iterator p;
            for(p=scddoc.begin(); p!=scddoc.end(); ++p)
            {
                std::string property_name;
                p->first.convertString(property_name, izenelib::util::UString::UTF_8);
                doc.property(property_name) = p->second;
            }
            UString docid;
            UString pid;
            std::string sdocid;
            std::string spid;
            doc.getProperty("DOCID", docid);
            doc.getProperty("uuid", pid);
            if( docid.length()==0 ) continue;
            docid.convertString(sdocid, izenelib::util::UString::UTF_8);
            pid.convertString(spid, izenelib::util::UString::UTF_8);
            std::string old_spid;
            UueItem uue;
            uue.docid = sdocid;
            if(scd_type==INSERT_SCD)
            {
                if(odb_->get(sdocid, old_spid)) continue;
                uue.from_to.to = spid;
                odb_->update(sdocid, spid);
            }
            else if(scd_type==UPDATE_SCD)
            {
                if(!odb_->get(sdocid, old_spid)) continue;
                uue.from_to.from = old_spid;
                if(spid.length()>0)
                {
                    uue.from_to.to = spid;
                    odb_->update(sdocid, spid);
                }
                else
                {
                    uue.from_to.to = old_spid; //pid not changed, but flag it as an update.
                }
            }
            else
            {
                if(!odb_->get(sdocid, old_spid)) continue;
                uue.from_to.from = old_spid;
                odb_->del(sdocid);
            }
            ofs<<uue.docid<<","<<uue.from_to.from<<","<<uue.from_to.to<<std::endl;
        }
    }
    odb_->flush();
    ofs.close();
    return true;
}

