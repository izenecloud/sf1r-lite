#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <common/ScdWriterController.h>
#include <common/Utilities.h>
#include <document-manager/Document.h>
#include <iostream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <am/luxio/BTree.h>
#include <am/tc/BTree.h>
#include <am/leveldb/Table.h>
#include <am/range/AmIterator.h>
#include <am/sequence_file/ssfr.h>
#include <ir/id_manager/IDManager.h>
#include <util/ClockTimer.h>
using namespace sf1r;
namespace bfs = boost::filesystem;
using namespace std;

int main(int argc, char** argv)
{
    bool sync = true;
    if(argc>=4)
    {
        std::string p = argv[3];
        if(p == "--no-sync")
        {
            sync = false;
        }
    }
    if(!sync)
    {
        std::ios::sync_with_stdio(false);
        LOG(INFO)<<"set sync_with_stdio(false)"<<std::endl;
    }
    std::string scdPath = argv[1];
    std::string properties = argv[2];
    std::vector<std::string> p_vector;
    boost::algorithm::split( p_vector, properties, boost::algorithm::is_any_of(",") );
    std::vector<std::string> scd_list;
    ScdParser::getScdList(scdPath, scd_list);
    if(scd_list.empty()) return 0;
    LOG(INFO)<<"total "<<scd_list.size()<<" SCDs"<<std::endl;

    std::string id_dir = "./test_scd_id_manager";
    boost::filesystem::remove_all(id_dir);
    boost::filesystem::create_directories(id_dir);
    id_dir = id_dir+"/id";
    izenelib::ir::idmanager::IDManager id_manager(id_dir);

    izenelib::util::ClockTimer timer;
    double cost = 0.0;
    uint64_t n=0;
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        LOG(INFO)<<"Processing SCD "<<scd_list[i]<<std::endl;
        timer.restart();
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_list[i]);
        ScdParser::iterator doc_iter = parser.begin(p_vector);
        cost += timer.elapsed();
        while(true)
        {
            if(doc_iter==parser.end()) break;
            SCDDoc doc = *(*doc_iter);
            izenelib::util::UString docname;

            SCDDoc::iterator p = doc.begin();

            for (p = doc.begin(); p != doc.end(); p++)
            {
                const std::string& property_name = p->first;
                if(property_name=="DOCID")
                {
                    docname = p->second;
                    break;
                }
            }
            uint32_t did = 0;
            std::string sdocname;
            docname.convertString(sdocname, izenelib::util::UString::UTF_8);
            id_manager.getDocIdByDocName(Utilities::md5ToUint128(sdocname), did);
            //std::cout<<sdocname<<","<<did<<std::endl;
            if(n%100000==0)
            {
                LOG(INFO)<<"read "<<n<<" docs, "<<cost<<"\t"<<cost/n<<std::endl;
            }
            timer.restart();
            ++doc_iter;
            cost += timer.elapsed();
            ++n;
        }
    }
    LOG(INFO)<<"finished, cost "<<cost<<std::endl;
}
