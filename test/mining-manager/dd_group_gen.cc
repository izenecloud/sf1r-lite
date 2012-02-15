#include <idmlib/duplicate-detection/group_table.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>

int main(int ac, char** av)
{
    std::string input(av[1]);
    std::string output(av[2]);
    boost::filesystem::remove_all(output);
    idmlib::dd::GroupTable<uint32_t, uint32_t> group(output);
    group.Load();
    std::ifstream ifs(input.c_str());
    std::string line;
    std::vector<uint32_t> in_group;
    izenelib::am::rde_hash<uint32_t, std::string> apps;
    while ( getline ( ifs,line ) )
    {
        boost::algorithm::trim(line);
//         std::cout<<line<<std::endl;
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
            uint32_t docid = boost::lexical_cast<uint32_t>(vec[0]);
            if( apps.find(docid)!=NULL )
            {
                std::cout<<"!!!! docid "<<docid<<" duplicated in file."<<std::endl;
            }
            apps.insert(docid, line);
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

//     apps.clear();
    const std::vector<std::vector<uint32_t> >& group_info = group.GetGroupInfo();
    std::cout<<"[TOTAL GROUP SIZE] : "<<group_info.size()<<std::endl;
    for(uint32_t group_id = 0;group_id<group_info.size();group_id++)
    {
        const std::vector<uint32_t>& in_group = group_info[group_id];
        for(uint32_t i=0;i<in_group.size();i++)
        {
            uint32_t docid = in_group[i];
            std::string* line = apps.find(docid);
            if( line!=NULL )
            {
                std::cout<<*line<<std::endl;

            }
            else
            {
                std::cout<<"???? docid "<<docid<<" error."<<std::endl;
            }


        }
        std::cout<<std::endl<<std::endl;
    }
}
