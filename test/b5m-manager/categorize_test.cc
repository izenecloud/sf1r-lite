#include <b5m-manager/product_matcher.h>
#include <boost/program_options.hpp>
using namespace sf1r;
namespace po = boost::program_options;

int32_t IsMatch(ProductMatcher* pm, const std::string& text, const std::vector<std::string>& tags)
{
    Document doc;
    doc.property("Title") = UString(text, UString::UTF_8);
    ProductMatcher::Product p;
    pm->Process(doc, p);
    if(p.scategory.empty()) return -1;
    for(uint32_t i=0;i<tags.size();i++)
    {
        if(boost::algorithm::starts_with(p.scategory, tags[i]))
        {
            return 1;
        }
    }
    std::cerr<<"unmatch category "<<p.scategory<<std::endl;
    return 0;
}

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
    matcher.SetUsePriceSim(false);
    std::string must_file = scd_path+"/must";
    std::string accuracy_file = scd_path+"/accuracy";
    std::string line;
    izenelib::util::ClockTimer clocker;
    {
        std::string text;
        std::vector<std::string> tags;
        std::ifstream ifs(must_file.c_str());
        while(getline(ifs, line))
        {
            boost::algorithm::trim(line);
            if(text.empty()) text = line;
            else
            {
                boost::algorithm::split(tags, line, boost::algorithm::is_any_of(","));
                int flag = IsMatch(&matcher, text, tags);
                if(flag!=1)
                {
                    std::cerr<<text<<" not match for "<<line<<std::endl;
                    return EXIT_FAILURE;
                }
                text.clear();
                tags.clear();
            }
        }
        ifs.close();
    }
    {
        std::size_t all_count=0;
        std::size_t correct_count=0;
        std::size_t empty_count=0;
        std::string text;
        std::vector<std::string> tags;
        std::ifstream ifs(accuracy_file.c_str());
        while(getline(ifs, line))
        {
            boost::algorithm::trim(line);
            if(text.empty()) text = line;
            else
            {
                boost::algorithm::split(tags, line, boost::algorithm::is_any_of(","));
                all_count++;
                int flag = IsMatch(&matcher, text, tags);
                if(flag==1)
                {
                    correct_count++;
                }
                else
                {
                    std::cerr<<text<<" not match for "<<line<<std::endl;
                    if(flag==-1) 
                    {
                        std::cerr<<"however empty"<<std::endl;
                        empty_count++;
                    }
                }
                text.clear();
                tags.clear();
            }
        }
        ifs.close();
        std::cerr<<"status "<<all_count<<","<<correct_count<<","<<empty_count<<","<<(double)correct_count/all_count<<std::endl;
    }
    std::cerr<<"clocker used "<<clocker.elapsed()<<std::endl;
}

