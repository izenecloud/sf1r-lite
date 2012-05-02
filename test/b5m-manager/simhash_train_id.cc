#include <b5m-manager/similarity_matcher.h>
#include <b5m-manager/pid_dictionary.h>
#include "../TestResources.h"
#include <boost/program_options.hpp>

using namespace sf1r;
namespace po = boost::program_options;
using namespace idmlib::dd;


int main(int ac, char** av)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("group-file,G", po::value<std::string>(), "specify group file location")
        ("pid-dictionary,P", po::value<std::string>(), "specify pid dictionary path")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm); 
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }
    std::string group_file;
    std::string pid_dic_file;
    PidDictionary dic;

    if (vm.count("group-file")) {
        group_file = vm["group-file"].as<std::string>();
    } 
    if (vm.count("pid-dictionary")) {
        pid_dic_file = vm["pid-dictionary"].as<std::string>();
    } 
    if(group_file.empty()||pid_dic_file.empty())
    {
        return EXIT_FAILURE;
    }
    GroupTable<std::string, uint32_t> gt(group_file);
    if(!gt.Load())
    {
        std::cerr<<"load group file error"<<std::endl;
    }
    const std::vector<std::vector<std::string> >& group_info = gt.GetGroupInfo();
    for(uint32_t gid=0;gid<group_info.size();gid++)
    {
        std::vector<std::string> in_group = group_info[gid];
        std::sort(in_group.begin(), in_group.end());
        if(in_group.size()>0)
        {
            dic.Add(in_group[0]);
        }
    }
    if(!dic.Save(pid_dic_file))
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

