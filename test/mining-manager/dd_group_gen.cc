#include <mining-manager/duplicate-detection-submanager/group_table.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
using namespace sf1r;
int main(int ac, char** av)
{
    std::string input(av[1]);
    std::string output(av[2]);
    GroupTable group(output);
    group.Load();
    std::ifstream ifs(input.c_str());
    std::string line;
    std::vector<uint32_t> in_group;
    while ( getline ( ifs,line ) )
    {
        boost::algorithm::trim(line);
        if(line.length()>0 && line[0]=='#' ) line = "";
        if(line.length()==0 && in_group.size()>0)
        {
            //process in_group
            for(uint32_t i=1;i<in_group.size();i++)
            {
                group.AddDoc(in_group[0], in_group[i]);
            }
            
            in_group.resize(0);
            continue;
        }
        if(line.length()>0)
        {
            std::vector<std::string> vec;
            boost::algorithm::split( vec, line, boost::algorithm::is_any_of("\t") );
//                 std::cout<<"XXX "<<vec[0]<<std::endl;
            uint32_t docid = boost::lexical_cast<uint32_t>(vec[0]);
            in_group.push_back(docid);
        }
        
    }
    ifs.close();
    //process in_group
    if(in_group.size()>0)
    {
        for(uint32_t i=1;i<in_group.size();i++)
        {
            group.AddDoc(in_group[0], in_group[i]);
        }
    }
    group.Flush();
}
