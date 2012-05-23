#include "uue_generator.h"
#include "log_server_client.h"

using namespace sf1r;

UueGenerator::UueGenerator(OfferDb* odb)
:odb_(odb)
{
}

bool UueGenerator::Generate(const std::string& mdb_instance)
{
    if(!odb_->is_open())
    {
        if(!odb_->open())
        {
            LOG(ERROR)<<"odb open fail"<<std::endl;
            return false;
        }
    }
    namespace bfs = boost::filesystem;
    std::string b5mo_scd = B5MHelper::GetB5moPath(mdb_instance);
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

    std::ofstream ofs(B5MHelper::GetUuePath(mdb_instance).c_str());
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"processing "<<scd_file<<std::endl;
        int scd_type = ScdParser::checkSCDType(scd_list[i]);
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
            Document doc;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
            }
            UString docid;
            UString pid;
            std::string sdocid;
            std::string spid;
            doc.getProperty("DOCID", docid);
            doc.getProperty("uuid", pid);
            docid.convertString(sdocid, izenelib::util::UString::UTF_8);
            pid.convertString(spid, izenelib::util::UString::UTF_8);
            if( sdocid.empty() || spid.empty() ) continue;
            UueItem uue;
            uue.docid = sdocid;
            if(scd_type==INSERT_SCD)
            {
                uue.from_to.to = spid;
            }
            else if(scd_type==UPDATE_SCD)
            {
                uue.from_to.from = spid;
                uue.from_to.to = spid;
            }
            else
            {
                uue.from_to.from = spid;
            }
            ofs<<uue.docid<<","<<uue.from_to.from<<","<<uue.from_to.to<<std::endl;
        }
    }
    ofs.close();
    return true;
}

