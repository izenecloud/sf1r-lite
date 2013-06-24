#include <b5m-manager/product_matcher.h>
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
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm); 
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
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
    std::ifstream ifs(scd_path.c_str());
    std::string line;
    while( getline(ifs, line) )
    {
        boost::algorithm::trim(line);
        UString query(line, UString::UTF_8);

        typedef std::list<std::pair<UString, double> > Hits;
        Hits hits;
        Hits left_hits;
        typedef std::list<UString> Left;
        Left left;
        izenelib::util::ClockTimer clocker;
        matcher.GetSearchKeywords(query, hits, left_hits, left);
        std::cout<<"[QUERY]"<<line<<" : "<<clocker.elapsed()<<std::endl;
        for(Hits::const_iterator it = hits.begin();it!=hits.end();++it)
        {
            const std::pair<UString, double>& v = *it;
            std::string str;
            v.first.convertString(str, UString::UTF_8);
            std::cout<<"[HITS]"<<str<<","<<v.second<<std::endl;
        }
        for(Hits::const_iterator it = left_hits.begin();it!=left_hits.end();++it)
        {
            const std::pair<UString, double>& v = *it;
            std::string str;
            v.first.convertString(str, UString::UTF_8);
            std::cout<<"[LEFT-HITS]"<<str<<","<<v.second<<std::endl;
        }
        for(Left::const_iterator it = left.begin();it!=left.end();++it)
        {
            std::string str;
            (*it).convertString(str, UString::UTF_8);
            std::cout<<"[LEFT]"<<str<<std::endl;
        }
    }
    ifs.close();
    
    return EXIT_SUCCESS;
}


