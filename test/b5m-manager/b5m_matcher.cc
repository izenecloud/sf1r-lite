#include <b5m-manager/raw_scd_generator.h>
#include <b5m-manager/attribute_indexer.h>
#include <b5m-manager/product_matcher.h>
#include <b5m-manager/category_scd_spliter.h>
#include <b5m-manager/b5mo_scd_generator.h>
#include <b5m-manager/b5mo_processor.h>
#include <b5m-manager/log_server_client.h>
#include <b5m-manager/image_server_client.h>
#include <b5m-manager/uue_generator.h>
#include <b5m-manager/complete_matcher.h>
#include <b5m-manager/similarity_matcher.h>
#include <b5m-manager/ticket_processor.h>
#include <b5m-manager/tuan_processor.h>
#include <b5m-manager/uue_worker.h>
#include <b5m-manager/b5mp_processor.h>
#include <b5m-manager/spu_processor.h>
#include <b5m-manager/b5m_mode.h>
#include <b5m-manager/b5mc_scd_generator.h>
#include <b5m-manager/log_server_handler.h>
#include <b5m-manager/product_db.h>
#include <b5m-manager/offer_db.h>
#include <b5m-manager/offer_db_recorder.h>
#include <b5m-manager/brand_db.h>
#include <b5m-manager/comment_db.h>
#include <b5m-manager/history_db_helper.h>
#include <b5m-manager/psm_indexer.h>
#include <b5m-manager/cmatch_generator.h>
#include "../TestResources.h"
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/trie_policy.hpp>
#include <ext/pb_ds/tag_and_trait.hpp>
#include <stack>
#include <boost/network/protocol/http/server.hpp>
#include <boost/network/utils/thread_pool.hpp>
#include <boost/network/uri/uri.hpp>
#include <boost/network/uri/decode.hpp>


using namespace sf1r;
namespace po = boost::program_options;
namespace pbds = __gnu_pbds;
namespace http = boost::network::http;
namespace uri = boost::network::uri;

int do_main(int ac, char** av);

struct ServerHandler;

typedef http::server<ServerHandler> Server;

struct ServerHandler
{
    ServerHandler(const std::string& program_path)
    : program_path_(program_path)
    {
    }
    void operator()(const Server::request& request, Server::response& response)
    {
        typedef Server::request::string_type string_type;
        std::string uri_string = destination(request);
        std::size_t cmd_pos = uri_string.find("cmd=");
        std::string cmd;
        if(cmd_pos!=std::string::npos)
        {
            cmd = uri_string.substr(cmd_pos+4);
        }
        cmd = uri::decoded(cmd);
        //uri::uri _uri(uri_string);
        //string_type q = uri::query(_uri);
        std::cout<<"[cmd]"<<cmd<<std::endl;
        std::vector<std::string> args = po::split_unix(cmd);
        if(args.empty()) return;
        //for(uint32_t i=0;i<args.size();i++)
        //{
            //std::cout<<"arg:"<<args[i]<<std::endl;
        //}
        int dac = args.size()+1;
        char** dav = new char*[dac];
        dav[0] = new char[program_path_.length()+1];
        strcpy(dav[0], program_path_.c_str());
        for(int i=1;i<dac;i++)
        {
            const char* s = args[i-1].c_str();
            dav[i] = new char[strlen(s)+1];
            strcpy(dav[i], s);
        }
        int status = 1;//default to fail
        try{
            status = do_main(dac, dav);
        }
        catch(std::exception& ex)
        {
            status = 1;
            std::cerr<<"do_main exception: "<<ex.what()<<std::endl;
        }
        for(int i=0;i<dac;i++)
        {
            delete[] dav[i];
        }
        delete[] dav;
        std::string sstatus = boost::lexical_cast<std::string>(status);
        if(status==0)//success
        {
            response = Server::response::stock_reply(Server::response::ok, sstatus);
        }
        else
        {
            response = Server::response::stock_reply(Server::response::internal_server_error, sstatus);
        }
    }

    void log(const Server::string_type& info)
    {
        std::cerr<<"[server log]"<<info<<std::endl;
    }
private:
    std::string program_path_;
};

int main(int ac, char** av)
{
    if(ac>1)
    {
        return do_main(ac, av);
    }
    std::string program_path(av[0]);

    ServerHandler handler(program_path);
    Server server("0.0.0.0", "18190", handler);
    static const uint32_t thread_count = 2;
    std::vector<boost::thread*> threads;
    for(uint32_t i=0;i<thread_count;i++)
    {
        boost::thread* t = new boost::thread(boost::bind(&Server::run, &server));
        threads.push_back(t);
    }
    for(uint32_t i=0;i<thread_count;i++)
    {
        threads[i]->join();
    }
    for(uint32_t i=0;i<thread_count;i++)
    {
        delete threads[i];
    }
    //server.run();



    //boost::filesystem::path p(program_path);
    //p = p.parent_path();
    //p /= "daemon";
    //if(!boost::filesystem::exists(p))
    //{
        //std::cerr<<"daemon file not exists"<<std::endl;
        //return EXIT_FAILURE;
    //}
    //std::cout<<"daemon file:"<<p.string()<<std::endl;
    //key_t msgkey = ftok(p.string().c_str(), 0);
    //int qid;
    //if( (qid=msgget(msgkey, IPC_CREAT | 0660)) == -1 )
    //{
        //std::cerr<<"open msg queue error"<<std::endl;
        //return EXIT_FAILURE;
    //}
    ////std::cerr<<"sizeof "<<sizeof(b5m_msgbuf)<<","<<sizeof(long)<<std::endl;
    //int length = sizeof(b5m_msgbuf)-sizeof(long);
    //int result = 0;
    //int mtype = 1;

    //b5m_msgbuf msg;
    //b5m_msgbuf result_msg;
    //while(true)
    //{
        //msg.clear();
        //result_msg.clear();
        //result=msgrcv(qid, &msg, length, 0, IPC_NOWAIT);
        //if(result<0)
        //{
            //break;
        //}
        //else
        //{
            //std::cerr<<"ignoring "<<msg.cmd<<std::endl;
        //}
    //}
    ////msg.mtype = mtype;
    ////char cmd[] = "--product-train -K \"./T/P\" -S ./T";
    ////strcpy(msg.cmd, cmd);
    ////if((result=msgsnd(qid,&msg,length,0))==-1)
    ////{
        ////std::cerr<<"msg send error"<<std::endl;
        ////return EXIT_FAILURE;
    ////}

    ////return EXIT_SUCCESS;
    //while(true)
    //{
        //msg.clear();
        //result_msg.clear();
        //result=msgrcv(qid, &msg, length, mtype, 0);
        //if(result==-1)
        //{
            //std::cerr<<"msg rcv error"<<std::endl;
            //continue;
        //}
        //std::string cmd_str(msg.cmd);
        //boost::algorithm::trim(cmd_str);
        //std::cout<<"cmd string:"<<cmd_str<<std::endl;
        //std::cout<<"start task:"<<msg.cmd<<std::endl;
        //std::vector<std::string> args = po::split_unix(msg.cmd);
        //if(args.empty()) continue;
        ////for(uint32_t i=0;i<args.size();i++)
        ////{
            ////std::cout<<"arg:"<<args[i]<<std::endl;
        ////}
        //int dac = args.size()+1;
        //char** dav = new char*[dac];
        //dav[0] = new char[strlen(program_path)+1];
        //strcpy(dav[0], program_path);
        //for(int i=1;i<dac;i++)
        //{
            //const char* s = args[i-1].c_str();
            //dav[i] = new char[strlen(s)+1];
            //strcpy(dav[i], s);
        //}
        //int status = 1;//default to fail
        //try{
            //status = do_main(dac, dav);
        //}
        //catch(std::exception& ex)
        //{
            //status = 1;
            //std::cerr<<"do_main exception: "<<ex.what()<<std::endl;
        //}
        //for(int i=0;i<dac;i++)
        //{
            //delete[] dav[i];
        //}
        //delete[] dav;
        //if(status>0)
        //{
            //std::cerr<<msg.cmd<<" return status fail "<<status<<std::endl;
            //result_msg.mtype = 2;
            //char str[] = "fail";
            //strcpy(result_msg.cmd, str);
        //}
        //else
        //{
            //std::cerr<<"task:"<<msg.cmd<<" finished"<<std::endl;
            //result_msg.mtype = 2;
            //char str[] = "succ";
            //strcpy(result_msg.cmd, str);
        //}
        //if((result=msgsnd(qid,&result_msg,length,0))==-1)
        //{
            //std::cerr<<"result msg send error"<<std::endl;
            //continue;
        //}
    //}
    return EXIT_SUCCESS;
}

int do_main(int ac, char** av)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("raw-generate", "generate standard raw scd")
        ("attribute-index,A", "build attribute index")
        ("product-train", "do product training")
        ("product-match", "do product matching test")
        ("fuzzy-diff", "test the fuzzy matching diff from no fuzzy")
        ("b5m-match,B", "make b5m matching")
        ("psm-index", "psm index")
        ("psm-match", "psm match")
        ("complete-match,M", "attribute complete matching")
        ("similarity-match,I", "title based similarity matching")
        ("ticket-generate", "do directly ticket matching")
        ("tuan-generate", "do directly tuan matching")
        ("cmatch-generate", "match to cmatch")
        ("b5mo-generate", "generate b5mo scd")
        ("uue-generate", "generate uue")
        ("b5mp-generate", "generate b5mp scd")
        ("b5mc-generate", "generate b5mc scd")
        ("spu-process", "process and merge all spus")
        ("logserver-update", "update logserver")
        ("match-test,T", "b5m matching test")
        ("frontend-test", "the frontend categorizing")
        ("search-keyword", "get search keywords")
        ("mdb-instance", po::value<std::string>(), "specify mdb instance")
        ("last-mdb-instance", po::value<std::string>(), "specify last mdb instance")
        ("mode", po::value<int>(), "specify mode")
        ("knowledge-dir,K", po::value<std::string>(), "specify knowledge dir")
        ("pdb", po::value<std::string>(), "specify product db path")
        ("odb", po::value<std::string>(), "specify offer db path")
        ("last-odb", po::value<std::string>(), "specify last offer db path")
        ("bdb", po::value<std::string>(), "specify brand db path")
        ("cdb", po::value<std::string>(), "specify comment db path")
        ("historydb", po::value<std::string>(), "specify offer history db path")
        ("synonym,Y", po::value<std::string>(), "specify synonym file")
        ("scd-path,S", po::value<std::string>(), "specify scd path")
        ("old-scd-path", po::value<std::string>(), "specify old processed scd path")
        ("raw", po::value<std::string>(), "specify raw scd path")
        ("b5mo", po::value<std::string>(), "specify b5mo scd path")
        ("b5mp", po::value<std::string>(), "specify b5mp scd path")
        ("b5mc", po::value<std::string>(), "specify b5mc scd path")
        ("uue", po::value<std::string>(), "uue path")
        ("spu", po::value<std::string>(), "spu path")
        //("category-group", po::value<std::string>(), "specify category group file")
        ("output-match,O", po::value<std::string>(), "specify output match path")
        ("cma-path,C", po::value<std::string>(), "manually specify cma path")
        ("dictionary", po::value<std::string>(), "specify dictionary path")
        ("mobile-source", po::value<std::string>(), "specify mobile source list file")
        ("human-match", po::value<std::string>(), "specify human edit match file")
        ("logserver-config,L", po::value<std::string>(), "log server config string")
        ("imgserver-config,L", po::value<std::string>(), "image server config string")
        ("exclude,E", "do not generate non matched categories")
        ("scd-split,P", "split scd files for each categories.")
        ("name,N", po::value<std::string>(), "specify the name")
        ("work-dir,W", po::value<std::string>(), "specify temp working directory")
        ("test", "specify test flag")
        ("noprice", "no price flag")
        ("depth", po::value<uint16_t>(), "specify category max depth while categorizing")
        ("force", "specify force flag")
        ("trie", "do trie test")
        ("cr-train", "do category recognizer training")
        ("cr", "do category recognizer")
        ("odb-test", "do odb test")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm); 
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }
    std::string mdb_instance;
    std::string last_mdb_instance;
    int mode = B5MMode::INC;
    std::string scd_path;
    std::string old_scd_path;
    std::string raw;
    std::string b5mo;
    std::string b5mp;
    std::string b5mc;
    std::string uue;
    std::string output_match;
    std::string knowledge_dir;
    boost::shared_ptr<OfferDb> odb;
    boost::shared_ptr<OfferDb> last_odb;
    boost::shared_ptr<BrandDb> bdb;
    boost::shared_ptr<CommentDb> cdb;
    boost::shared_ptr<B5MHistoryDBHelper> historydb;

    boost::shared_ptr<LogServerConnectionConfig> logserver_config;
    boost::shared_ptr<RpcServerConnectionConfig> imgserver_config;
    std::string synonym_file;
    //std::string category_group;
    std::string dictionary;
    std::string mobile_source;
    std::string human_match;
    std::string cma_path = IZENECMA_KNOWLEDGE ;
    std::string work_dir;
    std::string name;
    std::string spu;
    bool test_flag = false;
    bool force_flag = false;
    bool noprice = false;
    uint16_t max_depth = 0;
    if (vm.count("mdb-instance")) {
        mdb_instance = vm["mdb-instance"].as<std::string>();
    } 
    if (vm.count("last-mdb-instance")) {
        last_mdb_instance = vm["last-mdb-instance"].as<std::string>();
    } 
    if (vm.count("mode")) {
        mode = vm["mode"].as<int>();
    } 
    if (vm.count("scd-path")) {
        scd_path = vm["scd-path"].as<std::string>();
        std::cout << "scd-path: " << scd_path <<std::endl;
    } 
    if (vm.count("old-scd-path")) {
        old_scd_path = vm["old-scd-path"].as<std::string>();
        std::cout << "old-scd-path: " << old_scd_path <<std::endl;
    } 
    if (vm.count("raw")) {
        raw = vm["raw"].as<std::string>();
    } 
    if (vm.count("b5mo")) {
        b5mo = vm["b5mo"].as<std::string>();
    } 
    if (vm.count("b5mp")) {
        b5mp = vm["b5mp"].as<std::string>();
    } 
    if (vm.count("b5mc")) {
        b5mc = vm["b5mc"].as<std::string>();
    } 
    if (vm.count("uue")) {
        uue = vm["uue"].as<std::string>();
    } 
    if (vm.count("spu")) {
        spu = vm["spu"].as<std::string>();
    } 
    if (vm.count("output-match")) {
        output_match = vm["output-match"].as<std::string>();
        std::cout << "output-match: " << output_match <<std::endl;
    } 
    if (vm.count("knowledge-dir")) {
        knowledge_dir = vm["knowledge-dir"].as<std::string>();
        std::cout << "knowledge-dir: " << knowledge_dir <<std::endl;
    } 
    //if (vm.count("pdb")) {
        //std::string pdb_path = vm["pdb"].as<std::string>();
        //std::cout << "open pdb path: " << pdb_path <<std::endl;
        //pdb.reset(new ProductDb(pdb_path));
        //pdb->open();
    //} 
    if (vm.count("odb")) {
        std::string odb_path = vm["odb"].as<std::string>();
        std::cout << "odb path: " << odb_path <<std::endl;
        odb.reset(new OfferDb(odb_path));
    } 
    if(vm.count("last-odb"))
    {
        std::string last_odb_path = vm["last-odb"].as<std::string>();
        std::cout << "last odb path: " << last_odb_path <<std::endl;
        last_odb.reset(new OfferDb(last_odb_path));
    }
    if (vm.count("bdb")) {
        std::string bdb_path = vm["bdb"].as<std::string>();
        std::cout << "bdb path: " << bdb_path <<std::endl;
        bdb.reset(new BrandDb(bdb_path));
    } 
    if (vm.count("cdb")) {
        std::string cdb_path = vm["cdb"].as<std::string>();
        std::cout << "cdb path: " << cdb_path <<std::endl;
        cdb.reset(new CommentDb(cdb_path));
    } 
    if (vm.count("historydb")) {
        std::string hdb_path = vm["historydb"].as<std::string>();
        std::cout << "historydb path: " << hdb_path <<std::endl;
        if(hdb_path != "////" && hdb_path != "")
        {
            historydb.reset(new B5MHistoryDBHelper(hdb_path));
        }
    } 

    if(vm.count("logserver-config"))
    {
        std::string config_string = vm["logserver-config"].as<std::string>();
        std::vector<std::string> vec;
        boost::algorithm::split( vec, config_string, boost::algorithm::is_any_of("|") );
        if(vec.size()==4)
        {
            std::string host = vec[0];
            uint32_t rpc_port = boost::lexical_cast<uint32_t>(vec[1]);
            uint32_t rpc_thread_num = boost::lexical_cast<uint32_t>(vec[2]);
            uint32_t driver_port = boost::lexical_cast<uint32_t>(vec[3]);
            logserver_config.reset(new LogServerConnectionConfig(host, rpc_port, rpc_thread_num, driver_port));
            cout << "using log server: " << host << "," << rpc_port << "," << driver_port << std::endl;
        }
        else
        {
            return EXIT_FAILURE;
        }
    }
    if(vm.count("imgserver-config"))
    {
        std::string config_string = vm["imgserver-config"].as<std::string>();
        std::vector<std::string> vec;
        boost::algorithm::split( vec, config_string, boost::algorithm::is_any_of("|") );
        if(vec.size()==3)
        {
            std::string host = vec[0];
            uint32_t rpc_port = boost::lexical_cast<uint32_t>(vec[1]);
            uint32_t rpc_thread_num = boost::lexical_cast<uint32_t>(vec[2]);
            imgserver_config.reset(new RpcServerConnectionConfig(host, rpc_port, rpc_thread_num));
        }
        else
        {
            return EXIT_FAILURE;
        }
    }

    if(vm.count("synonym"))
    {
        synonym_file = vm["synonym"].as<std::string>();
        std::cout<< "synonym file set to "<<synonym_file<<std::endl;
    }
    //if(vm.count("category-group"))
    //{
        //category_group = vm["category-group"].as<std::string>();
        //std::cout<< "category_group set to "<<category_group<<std::endl;
    //}
    if(vm.count("cma-path"))
    {
        cma_path = vm["cma-path"].as<std::string>();
    }
    if(vm.count("dictionary"))
    {
        dictionary = vm["dictionary"].as<std::string>();
    }
    if(vm.count("mobile-source"))
    {
        mobile_source = vm["mobile-source"].as<std::string>();
    }
    if(vm.count("human-match"))
    {
        human_match = vm["human-match"].as<std::string>();
    }
    if(vm.count("work-dir"))
    {
        work_dir = vm["work-dir"].as<std::string>();
        std::cout<< "work-dir set to "<<work_dir<<std::endl;
    }
    if(vm.count("name"))
    {
        name = vm["name"].as<std::string>();
        std::cout<< "name set to "<<name<<std::endl;
    }
    if(vm.count("test"))
    {
        test_flag = true;
    }
    if(vm.count("force"))
    {
        force_flag = true;
    }
    if(vm.count("noprice"))
    {
        noprice = true;
    }
    if(vm.count("depth"))
    {
        max_depth = vm["depth"].as<uint16_t>();
    }
    std::cout<<"cma-path is "<<cma_path<<std::endl;

    if(vm.count("odb-test"))
    {
        if(!odb->is_open())
        {
            odb->open();
        }
        std::string pid;
        odb->get(name, pid);
        std::cout<<"pid:"<<pid<<std::endl;
    }

    if(vm.count("raw-generate"))
    {
        if( scd_path.empty() || mdb_instance.empty() || !odb )
        {
            return EXIT_FAILURE;
        }
        LOG(INFO)<<"raw generator, mode: "<<mode<<std::endl;
        RawScdGenerator generator(odb.get(), mode, imgserver_config.get());
        if(!mobile_source.empty())
        {
            if(boost::filesystem::exists(mobile_source))
            {
                LOG(INFO)<<"raw generator loading mobile source "<<mobile_source<<std::endl;
                generator.LoadMobileSource(mobile_source);
            }
        }
        if(!generator.Generate(scd_path, mdb_instance))
        {
            return EXIT_FAILURE;
        }
        LOG(INFO)<<"raw generator successfully"<<std::endl;
    }
    if(vm.count("scd-split"))
    {
        if( scd_path.empty() || knowledge_dir.empty() || name.empty())
        {
            return EXIT_FAILURE;
        }
        CategoryScdSpliter spliter;
        spliter.Load(knowledge_dir,name);
        if(!spliter.Split(scd_path))
        {
            return EXIT_FAILURE;
        }
    }
    if (vm.count("attribute-index")) {
        if( knowledge_dir.empty() )
        {
            return EXIT_FAILURE;
        }
        if(force_flag)
        {
            boost::filesystem::remove_all(knowledge_dir+"/index.done");
        }
        AttributeIndexer indexer(knowledge_dir);
        indexer.SetCmaPath(cma_path);
        if(!synonym_file.empty())
        {
            indexer.LoadSynonym(synonym_file);
        }
        //if(!category_regex.empty())
        //{
            //indexer.SetCategoryRegex(category_regex);
        //}
        if(!indexer.Index())
        {
            return EXIT_FAILURE;
        }
        //std::cout<<"attribute-index end"<<std::endl;
        //if(!indexer.TrainSVM())
        //{
            //return EXIT_FAILURE;
        //}
    } 
    if (vm.count("product-train")) {
        if( knowledge_dir.empty()||scd_path.empty())
        {
            return EXIT_FAILURE;
        }
        //ProductMatcher::Clear(knowledge_dir, mode);
        ProductMatcher matcher;
        matcher.SetCmaPath(cma_path);
        //if(!matcher.Open(knowledge_dir))
        //{
            //LOG(ERROR)<<"matcher open failed"<<std::endl;
            //return EXIT_FAILURE;
        //}
        //if(!category_group.empty()&&boost::filesystem::exists(category_group))
        //{
            //matcher.LoadCategoryGroup(category_group);
        //}
        if(!matcher.Index(knowledge_dir, scd_path, mode))
        {
            return EXIT_FAILURE;
        }
    } 
    if (vm.count("product-match")) {
        if( knowledge_dir.empty()||scd_path.empty())
        {
            return EXIT_FAILURE;
        }
        ProductMatcher matcher;
        matcher.SetCmaPath(cma_path);
        if(!matcher.Open(knowledge_dir))
        {
            LOG(ERROR)<<"matcher open failed"<<std::endl;
            return EXIT_FAILURE;
        }
        if(!matcher.DoMatch(scd_path, output_match))
        {
            return EXIT_FAILURE;
        }
    } 
    if (vm.count("fuzzy-diff")) {
        if( knowledge_dir.empty()||scd_path.empty())
        {
            return EXIT_FAILURE;
        }
        ProductMatcher matcher;
        matcher.SetCmaPath(cma_path);
        if(!matcher.Open(knowledge_dir))
        {
            LOG(ERROR)<<"matcher open failed"<<std::endl;
            return EXIT_FAILURE;
        }
        if(!matcher.FuzzyDiff(scd_path, output_match))
        {
            return EXIT_FAILURE;
        }
    } 
    if (vm.count("frontend-test")) {
        if( knowledge_dir.empty())
        {
            return EXIT_FAILURE;
        }
        ProductMatcher matcher;
        matcher.SetCmaPath(cma_path);
        if(!matcher.Open(knowledge_dir))
        {
            LOG(ERROR)<<"matcher open failed"<<std::endl;
            return EXIT_FAILURE;
        }
        matcher.SetUsePriceSim(false);
        if(max_depth>0)
        {
            matcher.SetCategoryMaxDepth(max_depth);
        }
        while(true)
        {
            std::string line;
            std::cerr<<"input text:"<<std::endl;
            getline(std::cin, line);
            boost::algorithm::trim(line);
            UString query(line, UString::UTF_8);
            uint32_t limit = 3;
            std::vector<UString> results;
            matcher.GetFrontendCategory(query, limit, results);
            for(uint32_t i=0;i<results.size();i++)
            {
                std::string str;
                results[i].convertString(str, UString::UTF_8);
                std::cout<<"[FCATEGORY]"<<str<<std::endl;
            }
        }
    }
    if (vm.count("search-keyword")) {
        if( knowledge_dir.empty())
        {
            return EXIT_FAILURE;
        }
        ProductMatcher matcher;
        matcher.SetCmaPath(cma_path);
        if(!matcher.Open(knowledge_dir))
        {
            LOG(ERROR)<<"matcher open failed"<<std::endl;
            return EXIT_FAILURE;
        }
        matcher.SetUsePriceSim(false);
        while(true)
        {
            std::string line;
            std::cerr<<"input text:"<<std::endl;
            getline(std::cin, line);
            boost::algorithm::trim(line);
            UString query(line, UString::UTF_8);

            typedef std::list<std::pair<UString, double> > Hits;
            Hits hits;
            typedef std::list<UString> Left;
            Left left;
            matcher.GetSearchKeywords(query, hits, left);
            for(Hits::const_iterator it = hits.begin();it!=hits.end();++it)
            {
                const std::pair<UString, double>& v = *it;
                std::string str;
                v.first.convertString(str, UString::UTF_8);
                std::cout<<"[HITS]"<<str<<","<<v.second<<std::endl;
            }
            for(Left::const_iterator it = left.begin();it!=left.end();++it)
            {
                std::string str;
                (*it).convertString(str, UString::UTF_8);
                std::cout<<"[LEFT]"<<str<<std::endl;
            }
        }
    }
    if (vm.count("match-test")) {
        if( knowledge_dir.empty())
        {
            return EXIT_FAILURE;
        }
        ProductMatcher matcher;
        matcher.SetCmaPath(cma_path);
        if(!matcher.Open(knowledge_dir))
        {
            LOG(ERROR)<<"matcher open failed"<<std::endl;
            return EXIT_FAILURE;
        }
        if(noprice)
        {
            matcher.SetUsePriceSim(false);
        }
        if(max_depth>0)
        {
            matcher.SetCategoryMaxDepth(max_depth);
        }
        if(!scd_path.empty())
        {
            matcher.Test(scd_path);
        }
        else
        {
            matcher.SetUsePriceSim(false);
            while(true)
            {
                std::string line;
                std::cerr<<"input text:"<<std::endl;
                getline(std::cin, line);
                boost::algorithm::trim(line);
                Document doc;
                doc.property("Title") = UString(line, UString::UTF_8);
                uint32_t limit = 3;
                std::vector<ProductMatcher::Product> products;
                matcher.Process(doc, limit, products);
                for(uint32_t i=0;i<products.size();i++)
                {
                    std::cout<<"[CATEGORY]"<<products[i].scategory<<std::endl;
                }
            }
        }
    } 
    if (vm.count("spu-process")) {
        if(spu.empty())
        {
            return EXIT_FAILURE;
        }
        SpuProcessor processor(spu);

        if(!processor.Upgrade())
        {
            return EXIT_FAILURE;
        }
    } 
    if(vm.count("b5m-match"))
    {
        if( knowledge_dir.empty() )
        {
            return EXIT_FAILURE;
        }
        AttributeIndexer indexer(knowledge_dir);
        indexer.SetCmaPath(cma_path);
        if(!synonym_file.empty())
        {
            indexer.LoadSynonym(synonym_file);
        }
        //if(!category_regex.empty())
        //{
            //indexer.SetCategoryRegex(category_regex);
        //}
        if(!indexer.Open())
        {
            return EXIT_FAILURE;
        }
        indexer.ProductMatchingSVM();
    }
    if(vm.count("psm-index"))
    {
        if( scd_path.empty() || knowledge_dir.empty())
        {
            return EXIT_FAILURE;
        }
        PsmIndexer psm(cma_path);
        psm.Index(scd_path, knowledge_dir, test_flag);
    }
    if(vm.count("psm-match"))
    {
        if( scd_path.empty() || knowledge_dir.empty())
        {
            return EXIT_FAILURE;
        }
        PsmIndexer psm(cma_path);
        if(!psm.DoMatch(scd_path, knowledge_dir))
        {
            LOG(ERROR)<<"psm matching fail"<<std::endl;
            return EXIT_FAILURE;
        }
    }
    if(vm.count("complete-match"))
    {
        if( scd_path.empty() || knowledge_dir.empty() )
        {
            return EXIT_FAILURE;
        }
        CompleteMatcher matcher;
        matcher.Index(scd_path, knowledge_dir);
    }
    if(vm.count("similarity-match"))
    {
        if( scd_path.empty() || knowledge_dir.empty() || dictionary.empty())
        {
            return EXIT_FAILURE;
        }
        SimilarityMatcher matcher;
        matcher.SetCmaPath(cma_path);
        PidDictionary dic;
        matcher.SetPidDictionary(dictionary);
        matcher.Index(scd_path, knowledge_dir);
    }
    if(vm.count("ticket-generate"))
    {
        if( scd_path.empty() || mdb_instance.empty())
        {
            return EXIT_FAILURE;
        }
        TicketProcessor processor;
        processor.SetCmaPath(cma_path);
        if(!processor.Generate(scd_path, mdb_instance))
        {
            std::cout<<"ticket generator fail"<<std::endl;
            return EXIT_FAILURE;
        }
    }
    if(vm.count("tuan-generate"))
    {
        if( scd_path.empty() || mdb_instance.empty())
        {
            return EXIT_FAILURE;
        }
        TuanProcessor processor;
        processor.SetCmaPath(cma_path);
        if(!processor.Generate(scd_path, mdb_instance))
        {
            std::cout<<"tuan generator fail"<<std::endl;
            return EXIT_FAILURE;
        }
    }
    if(vm.count("cmatch-generate"))
    {
        if( !odb || mdb_instance.empty())
        {
            return EXIT_FAILURE;
        }
        CMatchGenerator generator(odb.get());
        if(!generator.Generate(mdb_instance))
        {
            return EXIT_FAILURE;
        }
    }
    if(vm.count("b5mo-generate") && scd_path.empty())
    {
        if( !odb || mdb_instance.empty())
        {
            return EXIT_FAILURE;
        }
        B5moScdGenerator generator(odb.get());
        if(!generator.Generate(mdb_instance, last_mdb_instance))
        {
            return EXIT_FAILURE;
        }
    }
    if(vm.count("b5mo-generate") && !scd_path.empty())
    {
        if( scd_path.empty() || !odb || mdb_instance.empty() || knowledge_dir.empty())
        {
            return EXIT_FAILURE;
        }
        LOG(INFO)<<"b5mo generator, mode: "<<mode<<std::endl;
        boost::shared_ptr<ProductMatcher> matcher(new ProductMatcher);
        matcher->SetCmaPath(cma_path);
        if(!matcher->Open(knowledge_dir))
        {
            LOG(ERROR)<<"matcher open failed"<<std::endl;
            return EXIT_FAILURE;
        }
        B5moProcessor processor(odb.get(), matcher.get(), bdb.get(), mode, imgserver_config.get());
        if(!mobile_source.empty())
        {
            if(boost::filesystem::exists(mobile_source))
            {
                LOG(INFO)<<"b5mo processor loading mobile source "<<mobile_source<<std::endl;
                processor.LoadMobileSource(mobile_source);
            }
        }
        if(!human_match.empty() && boost::filesystem::exists(human_match))
        {
            processor.SetHumanMatchFile(human_match);
        }
        if(!processor.Generate(scd_path, mdb_instance, last_mdb_instance))
        {
            return EXIT_FAILURE;
        }
        LOG(INFO)<<"b5mo processor successfully"<<std::endl;
    }
    if(vm.count("uue-generate"))
    {
        if( mdb_instance.empty() || !odb )
        {
            return EXIT_FAILURE;
        }
        UueGenerator generator(odb.get());
        if(!generator.Generate(mdb_instance))
        {
            return EXIT_FAILURE;
        }
    }
    if(vm.count("b5mp-generate"))
    {
        if( mdb_instance.empty() )
        {
            return EXIT_FAILURE;
        }
        B5mpProcessor processor(mdb_instance, last_mdb_instance);
        if(!processor.Generate())
        {
            std::cout<<"b5mp processor failed"<<std::endl;
            return EXIT_FAILURE;
        }
    }
    if(vm.count("b5mc-generate"))
    {
        if( !odb || !cdb || scd_path.empty() || mdb_instance.empty())
        {
            return EXIT_FAILURE;
        }
        boost::shared_ptr<ProductMatcher> matcher;
        if(!knowledge_dir.empty())
        {
            matcher.reset(new ProductMatcher);
            if(!matcher->Open(knowledge_dir)) 
            {
                LOG(ERROR)<<"matcher open failed"<<std::endl;
                return EXIT_FAILURE;
            }
            matcher->SetCmaPath(cma_path);
        }

        boost::shared_ptr<OfferDbRecorder> odbr(new OfferDbRecorder(odb.get(), last_odb.get()));
        B5mcScdGenerator generator(cdb.get(), odbr.get(), bdb.get(), matcher.get(), mode);
        if(!generator.Generate(scd_path, mdb_instance))
        {
            return EXIT_FAILURE;
        }
    }
    if(vm.count("logserver-update"))
    {
        if( mdb_instance.empty() || !logserver_config)
        {
            return EXIT_FAILURE;
        }
        LogServerHandler handler(*logserver_config);
        if(!handler.Generate(mdb_instance))
        {
            return EXIT_FAILURE;
        }
    }
    //if(vm.count("cr-train"))
    //{
        //if( knowledge_dir.empty())
        //{
            //return EXIT_FAILURE;
        //}
        //ProductMatcher matcher(knowledge_dir);
        //matcher.SetCmaPath(cma_path);
        //if(!matcher.Open())
        //{
            //return EXIT_FAILURE;
        //}
        //if(!matcher.IndexCR())
        //{
            //return EXIT_FAILURE;
        //}
    //}
    //if(vm.count("cr"))
    //{
        //if( scd_path.empty() || knowledge_dir.empty())
        //{
            //return EXIT_FAILURE;
        //}
        //ProductMatcher matcher(knowledge_dir);
        //matcher.SetCmaPath(cma_path);
        //if(!matcher.Open())
        //{
            //return EXIT_FAILURE;
        //}
        //if(!matcher.DoCR(scd_path))
        //{
            //return EXIT_FAILURE;
        //}
    //}
    if(vm.count("trie"))
    {
        typedef izenelib::util::UString UString;
        //typedef pbds::trie_string_access_traits<> cmp_fn;
        //typedef pbds::pat_trie_tag tag_type;
        //typedef pbds::trie<std::string, UString, cmp_fn, tag_type, pbds::trie_prefix_search_node_update> TrieType;
        typedef pbds::trie<std::string, std::string> TrieType;
        TrieType t;
        t["a"] = "qwe";
        t["as"] = "asd";
        //t["421"] = "zxc";
        for(TrieType::const_iterator it = t.begin();it!=t.end();it++)
        {
            std::cout<<it->first<<","<<it->second<<std::endl;
        }
        std::cout<<"nodes:"<<std::endl;
        //const TrieType::e_access_traits& at = t.get_e_access_traits();
        typedef TrieType::e_access_traits::const_iterator e_iterator;
        typedef TrieType::const_node_iterator node_iterator;
        std::stack<node_iterator> stack;
        stack.push(t.node_begin());
        while(!stack.empty())
        {
            node_iterator it = stack.top();
            stack.pop();
            std::pair<e_iterator, e_iterator> e_pair = it.valid_prefix();
            std::cout<<"ready to output"<<std::endl;
            for(e_iterator eit = e_pair.first; eit!=e_pair.second;++eit)
            {
                std::cout<<"eit:"<<*eit<<","<<stack.size()<<std::endl;
            }
            std::cout<<"it has "<<it.num_children()<<" children"<<std::endl;
            for(std::size_t i=0;i<it.num_children();i++)
            {
                stack.push(it.get_child(i));
            }
        }
        //node_iterator nit = t.node_begin();
        //std::cout<<nit.num_children()<<std::endl;
        //nit = nit.get_child(0);
        //std::cout<<(*nit)->first<<std::endl;
        std::cout<<"nodes end"<<std::endl;
    }
    //if(vm.count("match-test"))
    //{
        //if( knowledge_dir.empty() )
        //{
            //return EXIT_FAILURE;
        //}
        //AttributeIndexer indexer;
        //indexer.SetCmaPath(cma_path);
        //if(!synonym_file.empty())
        //{
            //indexer.LoadSynonym(synonym_file);
        //}
        //if(!category_regex.empty())
        //{
            //indexer.SetCategoryRegex(category_regex);
        //}
        //indexer.Open(knowledge_dir);
        //std::cout<<"Input Product Title:"<<std::endl;
        //std::string line;
        //while( getline(std::cin, line) )
        //{
            //if(line.empty()) break;
            //izenelib::util::UString category;
            //izenelib::util::UString ustr;
            //std::size_t split_index = line.find("|");
            //if(split_index!=std::string::npos)
            //{
                //category.append( izenelib::util::UString(line.substr(0, split_index), izenelib::util::UString::UTF_8) );
                //ustr.append( izenelib::util::UString(line.substr(split_index+1), izenelib::util::UString::UTF_8) );
            //}
            //else
            //{
                //ustr.append( izenelib::util::UString(line, izenelib::util::UString::UTF_8) );
            //}
            //std::vector<uint32_t> aid_list;
            //indexer.GetAttribIdList(category, ustr, aid_list);
            //for(std::size_t i=0;i<aid_list.size();i++)
            //{
                //std::cout<<"["<<aid_list[i]<<",";
                //izenelib::util::UString arep;
                //indexer.GetAttribRep(aid_list[i], arep);
                //if(arep.length()>0)
                //{
                    //std::string sarep;
                    //arep.convertString(sarep, izenelib::util::UString::UTF_8);
                    //std::cout<<sarep;
                //}
                //std::cout<<"]"<<std::endl;
            //}
        //}
    //}

    //if(odb)
    //{
        //odb->flush();
    //}
    return EXIT_SUCCESS;
}

