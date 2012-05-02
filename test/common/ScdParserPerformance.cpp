#include <common/ScdParser.h>
#include <document-manager/Document.h>
#include <iostream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <util/ClockTimer.h>
#include <glog/logging.h>
using namespace sf1r;
namespace bfs = boost::filesystem;
namespace po = boost::program_options;
using namespace std;

int main(int ac, char** av)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("input-path,I", po::value<std::string>(), "specify input scd path")
        ("properties,P", po::value<std::string>(), "specify properties, comma seperated")
        ("verbose,V", "verbose mode, output scd value")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm); 
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }
    std::string input_path;
    std::string properties;
    bool verbose = false;
    if (vm.count("input-path")) {
        input_path = vm["input-path"].as<std::string>();
    } 
    if (vm.count("properties")) {
        properties = vm["properties"].as<std::string>();
    } 
    if (vm.count("verbose"))
    {
        verbose = true;
    }
    if(input_path.empty())
    {
        return EXIT_FAILURE;
    }
    std::vector<std::string> p_vector;
    if(properties.length()>0)
    {
        boost::algorithm::split( p_vector, properties, boost::algorithm::is_any_of(",") );
    }
    std::vector<std::string> scd_list;
    ScdParser::getScdList(input_path, scd_list);
    if(scd_list.empty()) return 0;
    LOG(INFO)<<"total "<<scd_list.size()<<" SCDs"<<std::endl;

    izenelib::util::ClockTimer timer;
    double cost = 0.0;
    uint64_t n=0;
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        LOG(INFO)<<"Processing SCD "<<scd_list[i]<<std::endl;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_list[i]);
        ScdParser::iterator doc_iter = parser.begin(p_vector);
        while(true)
        {
            if(doc_iter==parser.end()) break;
            SCDDoc doc = *(*doc_iter);

            if(verbose)
            {
                std::vector<std::pair<std::string, izenelib::util::UString> >::iterator p;

                for (p = doc.begin(); p != doc.end(); p++)
                {
                    const std::string& property_name = p->first;
                    std::string property_value;
                    p->second.convertString(property_value, izenelib::util::UString::UTF_8);
                    std::cout<<property_name<<":"<<property_value<<std::endl;
                }
            }
            if(n%100000==0)
            {
                double cost = timer.elapsed();
                LOG(INFO)<<"read "<<n<<" docs, "<<cost<<"\t"<<cost/n<<std::endl;
            }
            ++doc_iter;
            ++n;
        }
    }
    LOG(INFO)<<"finished, cost "<<cost<<std::endl;
}

