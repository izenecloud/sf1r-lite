#include <b5m-manager/product_matcher.h>
#include <common/ScdWriter.h>
#include <common/ScdTypeWriter.h>
#include <boost/program_options.hpp>
using namespace sf1r;
namespace po = boost::program_options;

int main(int ac, char** av)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("knowledge-dir,K", po::value<std::string>(), "specify knowledge dir")
        ("scd-path,S", po::value<std::string>(), "specify scd path")
        ("discover", po::value<std::string>(),  "specify whether do the new spu discover and the output directory")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm); 
    if (vm.count("help")) {
        std::cout << desc << std::endl; return 1;
    }
    std::string scd_path;
    std::string knowledge_dir;
    if (vm.count("scd-path")) {
        scd_path = vm["scd-path"].as<std::string>();
        std::cout << "scd-path: " << scd_path <<std::endl;
    } 
    if (vm.count("knowledge-dir")) {
        knowledge_dir = vm["knowledge-dir"].as<std::string>();
        std::cout << "knowledge-dir: " << knowledge_dir <<std::endl;
    } 
    std::string discover;
    if(vm.count("discover"))
    {
        discover = vm["discover"].as<std::string>();
        std::cout << "discover: " << discover <<std::endl;
    }
    if( knowledge_dir.empty()||scd_path.empty())
    {
        return EXIT_FAILURE;
    }
    ProductMatcher matcher;
    //matcher.SetCmaPath(cma_path);
    if(!matcher.Open(knowledge_dir))
    {
        LOG(ERROR)<<"matcher open failed"<<std::endl;
        return EXIT_FAILURE;
    }
    std::vector<std::string> scd_list;
    ScdParser::getScdList(scd_path, scd_list);
    izenelib::util::ClockTimer clocker;
    uint32_t all_count = 0;
    uint32_t match_count = 0;
    boost::shared_ptr<ScdTypeWriter> discover_writer;
    if(!discover.empty())
    {
        boost::filesystem::remove_all(discover);
        boost::filesystem::create_directories(discover);
        discover_writer.reset(new ScdTypeWriter(discover));
    }
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
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            Document doc;
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
            }
            Document::doc_prop_value_strtype e_dattrib;
            Document::doc_prop_value_strtype e_fattrib;
            doc.getProperty("DisplayAttribute", e_dattrib);
            doc.getProperty("FilterAttribute", e_fattrib);
            std::string stitle;
            doc.getString("Title", stitle);
            ProductMatcher::Product result_product;
            matcher.Process(doc, result_product, true);
            all_count++;
            bool spu_match = false;
            if(!result_product.stitle.empty())
            {
                spu_match = true;
                match_count++;
            }
            if(!spu_match)
            {
                LOG(ERROR)<<"unmatch for "<<stitle<<std::endl;
                if(discover_writer)
                {
                    discover_writer->Append(doc, INSERT_SCD);
                }
                //return EXIT_FAILURE;
            }
            else
            {
                LOG(INFO)<<"spumatch for "<<stitle<<" to "<<result_product.stitle<<std::endl;
                if( discover_writer && (!e_dattrib.empty()||!e_fattrib.empty()) && (result_product.display_attributes.empty()&&result_product.filter_attributes.empty()))
                {
                    Document sdoc;
                    sdoc.property("DOCID") = str_to_propstr(result_product.spid, UString::UTF_8);
                    sdoc.property("Title") = str_to_propstr(result_product.stitle, UString::UTF_8);
                    if(!e_dattrib.empty())
                    {
                        sdoc.property("DisplayAttribute") = e_dattrib;
                    }
                    if(!e_fattrib.empty())
                    {
                        sdoc.property("FilterAttribute") = e_fattrib;
                    }
                    discover_writer->Append(sdoc, UPDATE_SCD); 
                }
            }
        }
        
    }
    if(discover_writer)
    {
        discover_writer->Close();
    }
    std::cerr<<"clocker used "<<clocker.elapsed()<<std::endl;
    std::cerr<<"stat "<<all_count<<","<<match_count<<","<<all_count-match_count<<std::endl;
    return EXIT_SUCCESS;
}


