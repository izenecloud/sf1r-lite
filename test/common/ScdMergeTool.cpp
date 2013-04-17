#include <common/ScdMerger.h>
#include <iostream>
#include <string>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <util/ClockTimer.h>
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
        ("output-path,O", po::value<std::string>(), "specify output scd path")
        //("properties,P", po::value<std::string>(), "specify properties, comma seperated")
        ("gen-all", "generate possible I,U,D SCD, not I only")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm); 
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }
    std::string input_path;
    std::string output_path;
    //std::string properties;
    bool all_type = false;
    if (vm.count("input-path")) {
        input_path = vm["input-path"].as<std::string>();
    } 
    if (vm.count("output-path")) {
        output_path = vm["output-path"].as<std::string>();
    } 
    //if (vm.count("properties")) {
        //properties = vm["properties"].as<std::string>();
    //} 
    if(input_path.empty()||output_path.empty())
    {
        return EXIT_FAILURE;
    }
    if(vm.count("gen-all"))
    {
        all_type = true;
    }
    //std::vector<std::string> p_vector;
    //if(properties.length()>0)
    //{
        //boost::algorithm::split( p_vector, properties, boost::algorithm::is_any_of(",") );
    //}
    ScdMerger merger(input_path);
    merger.SetAllType(all_type);
    merger.SetOutputPath(output_path);
    merger.Run();
}

