#include <b5m-manager/product_matcher.h>
#include <common/ScdWriter.h>
#include <boost/program_options.hpp>
using namespace sf1r;
namespace po = boost::program_options;

int main(int ac, char** av)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("knowledge-dir,K", po::value<std::string>(), "specify knowledge dir")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm); 
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }
    std::string knowledge_dir;
    if (vm.count("knowledge-dir")) {
        knowledge_dir = vm["knowledge-dir"].as<std::string>();
        std::cout << "knowledge-dir: " << knowledge_dir <<std::endl;
    } 
    if( knowledge_dir.empty())
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
    std::string line;
    while( getline(std::cin, line) )
    {
        UString text(line, UString::UTF_8);
        ProductMatcher::KeywordTag keyword;
        if(matcher.GetKeyword(text, keyword))
        {
            for(uint32_t i=0;i<keyword.category_name_apps.size();i++)
            {
                const ProductMatcher::Category& c = matcher.GetCategory(keyword.category_name_apps[i].cid);
                std::cout<<"CategoryApp "<<c.name<<std::endl;
            }
            for(uint32_t i=0;i<keyword.attribute_apps.size();i++)
            {
                const ProductMatcher::Product& p = matcher.GetProduct(keyword.attribute_apps[i].spu_id);
                std::cout<<"AttributeApp "<<p.stitle<<":"<<keyword.attribute_apps[i].attribute_name<<std::endl;
            }
        }
    }
    return EXIT_SUCCESS;
}


