#include <b5m-manager/attribute_indexer.h>
#include <b5m-manager/scd_generator.h>
#include <b5m-manager/complete_matcher.h>
#include <b5m-manager/similarity_matcher.h>
#include "../TestResources.h"
#include <boost/program_options.hpp>

using namespace sf1r;
namespace po = boost::program_options;


int main(int ac, char** av)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("attribute-index,A", "build attribute index")
        ("b5m-match,B", "make b5m matching")
        ("complete-match,M", "attribute complete matching")
        ("similarity-match,I", "title based similarity matching")
        ("knowledge-dir,K", po::value<std::string>(), "specify knowledge dir")
        ("synonym,Y", po::value<std::string>(), "specify synonym file")
        ("scd-path,S", po::value<std::string>(), "specify scd path")
        ("category-regex,R", po::value<std::string>(), "specify category regex string")
        ("output-match,O", po::value<std::string>(), "specify output match path")
        ("cma-path,C", po::value<std::string>(), "manually specify cma path")
        ("scd-generate,G", po::value<std::string>(), "generate b5mo b5mp scd files")
        ("exclude,E", "do not generate non matched categories")
        ("scd-split,P", "split scd files for each categories.")
        ("work-dir,W", po::value<std::string>(), "specify temp working directory")
        ("match-test,T", "b5m matching test")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm); 
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }
    std::string scd_path;
    std::string output_match;
    std::string knowledge_dir;
    std::string synonym_file;
    std::string category_regex_str;
    std::string cma_path = IZENECMA_KNOWLEDGE ;
    std::string work_dir;
    if (vm.count("scd-path")) {
        scd_path = vm["scd-path"].as<std::string>();
        std::cout << "scd-path: " << scd_path <<std::endl;
    } 
    if (vm.count("output-match")) {
        output_match = vm["output-match"].as<std::string>();
        std::cout << "output-match: " << output_match <<std::endl;
    } 
    if (vm.count("knowledge-dir")) {
        knowledge_dir = vm["knowledge-dir"].as<std::string>();
        std::cout << "knowledge-dir: " << knowledge_dir <<std::endl;
    } 
    if(vm.count("synonym"))
    {
        synonym_file = vm["synonym"].as<std::string>();
        std::cout<< "synonym file set to "<<synonym_file<<std::endl;
    }
    if(vm.count("category-regex"))
    {
        category_regex_str = vm["category-regex"].as<std::string>();
        std::cout<< "category_regex set to "<<category_regex_str<<std::endl;
    }
    if(vm.count("cma-path"))
    {
        cma_path = vm["cma-path"].as<std::string>();
    }
    if(vm.count("work-dir"))
    {
        work_dir = vm["work-dir"].as<std::string>();
        std::cout<< "work-dir set to "<<work_dir<<std::endl;
    }
    std::cout<<"cma-path is "<<cma_path<<std::endl;
    if (vm.count("attribute-index")) {
        if( scd_path.empty() || knowledge_dir.empty() )
        {
            return EXIT_FAILURE;
        }
        AttributeIndexer indexer;
        indexer.SetCmaPath(cma_path);
        if(!synonym_file.empty())
        {
            indexer.LoadSynonym(synonym_file);
        }
        if(!category_regex_str.empty())
        {
            indexer.SetCategoryRegex(category_regex_str);
        }
        if(!indexer.Index(scd_path, knowledge_dir))
        {
            return EXIT_FAILURE;
        }
        if(!indexer.TrainSVM())
        {
            return EXIT_FAILURE;
        }
    } 
    else if(vm.count("b5m-match"))
    {
        if( scd_path.empty() || knowledge_dir.empty() )
        {
            return EXIT_FAILURE;
        }
        AttributeIndexer indexer;
        indexer.SetCmaPath(cma_path);
        if(!synonym_file.empty())
        {
            indexer.LoadSynonym(synonym_file);
        }
        if(!category_regex_str.empty())
        {
            indexer.SetCategoryRegex(category_regex_str);
        }
        if(!indexer.Open(knowledge_dir))
        {
            return EXIT_FAILURE;
        }
        indexer.ProductMatchingSVM(scd_path);
    }
    else if(vm.count("complete-match"))
    {
        if( scd_path.empty() || knowledge_dir.empty() )
        {
            return EXIT_FAILURE;
        }
        CompleteMatcher matcher;
        matcher.Index(scd_path, knowledge_dir);
    }
    else if(vm.count("similarity-match"))
    {
        if( scd_path.empty() || knowledge_dir.empty() )
        {
            return EXIT_FAILURE;
        }
        SimilarityMatcher matcher;
        matcher.SetCmaPath(cma_path);
        matcher.Index(scd_path, knowledge_dir);
    }
    else if(vm.count("scd-split"))
    {
        if( scd_path.empty() || knowledge_dir.empty() )
        {
            return EXIT_FAILURE;
        }
        AttributeIndexer indexer;
        if(!indexer.Open(knowledge_dir))
        {
            return EXIT_FAILURE;
        }
        if(!indexer.SplitScd(scd_path))
        {
            return EXIT_FAILURE;
        }
    }
    else if(vm.count("scd-generate"))
    {
        std::string output_dir = vm["scd-generate"].as<std::string>();
        if( scd_path.empty() || knowledge_dir.empty() || output_dir.empty() )
        {
            return EXIT_FAILURE;
        }
        ScdGenerator gen;
        if(vm.count("exclude"))
        {
            std::cout<<"scd generate exclude"<<std::endl;
            gen.SetExclude();
        }
        if(!gen.Load(knowledge_dir))
        {
            return EXIT_FAILURE;
        }
        if(!gen.Generate(scd_path, output_dir, work_dir))
        {
            return EXIT_FAILURE;
        }
    }
    else if(vm.count("match-test"))
    {
        if( knowledge_dir.empty() )
        {
            return EXIT_FAILURE;
        }
        AttributeIndexer indexer;
        indexer.SetCmaPath(cma_path);
        if(!synonym_file.empty())
        {
            indexer.LoadSynonym(synonym_file);
        }
        if(!category_regex_str.empty())
        {
            indexer.SetCategoryRegex(category_regex_str);
        }
        indexer.Open(knowledge_dir);
        std::cout<<"Input Product Title:"<<std::endl;
        std::string line;
        while( getline(std::cin, line) )
        {
            if(line.empty()) break;
            izenelib::util::UString category;
            izenelib::util::UString ustr;
            std::size_t split_index = line.find("|");
            if(split_index!=std::string::npos)
            {
                category.append( izenelib::util::UString(line.substr(0, split_index), izenelib::util::UString::UTF_8) );
                ustr.append( izenelib::util::UString(line.substr(split_index+1), izenelib::util::UString::UTF_8) );
            }
            else
            {
                ustr.append( izenelib::util::UString(line, izenelib::util::UString::UTF_8) );
            }
            std::vector<uint32_t> aid_list;
            indexer.GetAttribIdList(category, ustr, aid_list);
            for(std::size_t i=0;i<aid_list.size();i++)
            {
                std::cout<<"["<<aid_list[i]<<",";
                izenelib::util::UString arep;
                indexer.GetAttribRep(aid_list[i], arep);
                if(arep.length()>0)
                {
                    std::string sarep;
                    arep.convertString(sarep, izenelib::util::UString::UTF_8);
                    std::cout<<sarep;
                }
                std::cout<<"]"<<std::endl;
            }
        }
    }
    return EXIT_SUCCESS;
}

