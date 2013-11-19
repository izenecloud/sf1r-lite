#include <b5m-manager/product_matcher.h>
#include <b5m-manager/product_discover.h>
#include <b5m-manager/category_mapper.h>
#include <b5m-manager/b5mo_processor.h>
#include <b5m-manager/image_server_client.h>
#include <b5m-manager/ticket_processor.h>
#include <b5m-manager/tuan_processor.h>
#include <b5m-manager/tour_processor.h>
#include <b5m-manager/b5mp_processor2.h>
#include <b5m-manager/b5m_mode.h>
#include <b5m-manager/b5mc_scd_generator.h>
#include <b5m-manager/product_db.h>
#include <b5m-manager/offer_db.h>
#include <b5m-manager/offer_db_recorder.h>
#include <b5m-manager/brand_db.h>
#include <b5m-manager/comment_db.h>
#include <b5m-manager/psm_indexer.h>
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
using namespace sf1r::b5m;
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
        ("attribute-index", "build attribute index")
        ("product-train", "do product training")
        ("product-train-post", "do product post training")
        ("product-discover", "do product discover")
        ("product-match", "do product matching test")
        ("output-categorymap", "output category map info from SCD")
        ("map-index", "do category mapper index")
        ("map-test", "do category mapper test")
        ("fuzzy-diff", "test the fuzzy matching diff from no fuzzy")
        ("b5m-match", "make b5m matching")
        ("psm-index", "psm index")
        ("psm-match", "psm match")
        ("complete-match", "attribute complete matching")
        ("similarity-match", "title based similarity matching")
        ("ticket-generate", "do directly ticket matching")
        ("tuan-generate", "do directly tuan matching")
        ("tour-generate", "do directly tour matching")
        ("cmatch-generate", "match to cmatch")
        ("b5mo-generate", "generate b5mo scd")
        ("uue-generate", "generate uue")
        ("b5mp-generate", "generate b5mp scd")
        ("b5mc-generate", "generate b5mc scd")
        ("logserver-update", "update logserver")
        ("match-test", "b5m matching test")
        ("isbn-test", "b5m isbn pid test")
        ("frontend-test", "the frontend categorizing")
        ("search-keyword", "get search keywords")
        ("scd-merge", "merge scd")
        ("imc", po::value<uint32_t>(), "specify in memory doc count of scd merger")
        ("mdb-instance", po::value<std::string>(), "specify mdb instance")
        ("last-mdb-instance", po::value<std::string>(), "specify last mdb instance")
        ("mode", po::value<int>(), "specify mode")
        ("thread-num", po::value<int>(), "specify thread num")
        ("buffer-size", po::value<std::string>(), "specify in memory buffer size")
        ("knowledge-dir,K", po::value<std::string>(), "specify knowledge dir")
        ("pdb", po::value<std::string>(), "specify product db path")
        ("odb", po::value<std::string>(), "specify offer db path")
        ("last-odb", po::value<std::string>(), "specify last offer db path")
        ("bdb", po::value<std::string>(), "specify brand db path")
        ("cdb", po::value<std::string>(), "specify comment db path")
        ("synonym,Y", po::value<std::string>(), "specify synonym file")
        ("scd-path,S", po::value<std::string>(), "specify scd path")
        ("old-scd-path", po::value<std::string>(), "specify old processed scd path")
        ("b5mo", po::value<std::string>(), "specify b5mo scd path")
        ("b5mp", po::value<std::string>(), "specify b5mp scd path")
        ("b5mc", po::value<std::string>(), "specify b5mc scd path")
        ("uue", po::value<std::string>(), "uue path")
        //("category-group", po::value<std::string>(), "specify category group file")
        ("input,I", po::value<std::string>(), "specify input path")
        ("output,O", po::value<std::string>(), "specify output path")
        ("cma-path,C", po::value<std::string>(), "manually specify cma path")
        ("dictionary", po::value<std::string>(), "specify dictionary path")
        ("mobile-source", po::value<std::string>(), "specify mobile source list file")
        ("human-match", po::value<std::string>(), "specify human edit match file")
        ("logserver-config", po::value<std::string>(), "log server config string")
        ("imgserver-config", po::value<std::string>(), "image server config string")
        ("exclude", "do not generate non matched categories")
        ("scd-split", "split scd files for each categories.")
        ("name,N", po::value<std::string>(), "specify the name")
        ("work-dir,W", po::value<std::string>(), "specify temp working directory")
        ("sorter-bin", po::value<std::string>(), "specify third-party sorter binary")
        ("all", "specify all flag")
        ("test", "specify test flag")
        ("noprice", "no price flag")
        ("spu-only", "spu only flag")
        ("use-psm", "use psm flag")
        ("no-avg-price", "no avg price flag")
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
    std::string b5mo;
    std::string b5mp;
    std::string b5mc;
    std::string uue;
    std::string input;
    std::string output;
    std::string knowledge_dir;
    boost::shared_ptr<OfferDb> odb;
    boost::shared_ptr<OfferDb> last_odb;
    boost::shared_ptr<BrandDb> bdb;
    boost::shared_ptr<CommentDb> cdb;

    boost::shared_ptr<sf1r::RpcServerConnectionConfig> imgserver_config;
    std::string synonym_file;
    //std::string category_group;
    std::string dictionary;
    std::string mobile_source;
    std::string human_match;
    std::string cma_path = IZENECMA_KNOWLEDGE ;
    std::string work_dir;
    std::string name;
    bool all_flag = false;
    bool test_flag = false;
    bool force_flag = false;
    bool noprice = false;
    bool spu_only = false;
    bool use_psm = false;
    bool use_avg_price = true;
    uint16_t max_depth = 0;
    int thread_num = 1;
    uint32_t imc = 0;
    std::string buffer_size;
    std::string sorter_bin;
    if (vm.count("mdb-instance")) {
        mdb_instance = vm["mdb-instance"].as<std::string>();
    } 
    if (vm.count("last-mdb-instance")) {
        last_mdb_instance = vm["last-mdb-instance"].as<std::string>();
    } 
    if (vm.count("mode")) {
        mode = vm["mode"].as<int>();
    } 
    if (vm.count("thread-num")) {
        thread_num = vm["thread-num"].as<int>();
        std::cout<<"thread_num:"<<thread_num<<std::endl;
    } 
    if (vm.count("buffer-size")) {
        buffer_size = vm["buffer-size"].as<std::string>();
        std::cout<<"buffer_size:"<<buffer_size<<std::endl;
    } 
    if (vm.count("sorter-bin")) {
        sorter_bin = vm["sorter-bin"].as<std::string>();
        std::cout<<"sorter_bin:"<<sorter_bin<<std::endl;
    } 
    if (vm.count("scd-path")) {
        scd_path = vm["scd-path"].as<std::string>();
        std::cout << "scd-path: " << scd_path <<std::endl;
    } 
    if (vm.count("old-scd-path")) {
        old_scd_path = vm["old-scd-path"].as<std::string>();
        std::cout << "old-scd-path: " << old_scd_path <<std::endl;
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
    if (vm.count("input")) {
        input = vm["input"].as<std::string>();
        std::cout << "input: " << input <<std::endl;
    } 
    if (vm.count("output")) {
        output = vm["output"].as<std::string>();
        std::cout << "output: " << output <<std::endl;
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
            imgserver_config.reset(new sf1r::RpcServerConnectionConfig(host, rpc_port, rpc_thread_num));
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
    if(vm.count("all"))
    {
        all_flag = true;
    }
    if(vm.count("force"))
    {
        force_flag = true;
    }
    if(vm.count("noprice"))
    {
        noprice = true;
    }
    if(vm.count("spu-only"))
    {
        spu_only = true;
    }
    if(vm.count("use-psm"))
    {
        use_psm = true;
    }
    if(vm.count("no-avg-price"))
    {
        use_avg_price = false;
    }
    if(vm.count("depth"))
    {
        max_depth = vm["depth"].as<uint16_t>();
    }
    if(vm.count("imc"))
    {
        imc = vm["imc"].as<uint32_t>();
        LOG(INFO)<<"set imc to "<<imc<<std::endl;
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
    if(vm.count("isbn-test"))
    {
        name = "9787532744237";
        std::string pid = B5MHelper::GetPidByIsbn(name);
        std::cout<<"pid:"<<pid<<std::endl;
    }

    if(vm.count("scd-merge"))
    {
        if( input.empty()||output.empty())
        {
            return EXIT_FAILURE;
        }
        ScdMerger merger(input);
        if(imc>0) merger.SetInMCount(imc);
        merger.SetAllType(all_flag);
        merger.SetOutputPath(output);
        merger.Run();
    }
    if (vm.count("product-train")) {
        if( knowledge_dir.empty()||scd_path.empty())
        {
            return EXIT_FAILURE;
        }
        //ProductMatcher::Clear(knowledge_dir, mode);
        {
            ProductMatcher matcher;
            matcher.SetCmaPath(cma_path);
            use_psm = true;
            if(use_psm)
            {
                matcher.SetUsePsm(true);
            }
            if(!matcher.Index(knowledge_dir, scd_path, mode, thread_num))
            {
                return EXIT_FAILURE;
            }
        }
        {
            ProductMatcher matcher;
            matcher.SetCmaPath(cma_path);
            if(!matcher.IndexPost(knowledge_dir, scd_path, thread_num))
            {
                return EXIT_FAILURE;
            }
        }
    } 
    if (vm.count("product-train-post")) {
        if( knowledge_dir.empty()||scd_path.empty())
        {
            return EXIT_FAILURE;
        }
        ProductMatcher matcher;
        matcher.SetCmaPath(cma_path);
        if(!matcher.IndexPost(knowledge_dir, scd_path, thread_num))
        {
            return EXIT_FAILURE;
        }
    } 
    if (vm.count("product-discover")) {
        if( knowledge_dir.empty()||scd_path.empty())
        {
            return EXIT_FAILURE;
        }
        //ProductMatcher::Clear(knowledge_dir, mode);
        ProductMatcher matcher;
        matcher.SetCmaPath(cma_path);
        if(!matcher.Open(knowledge_dir))
        {
            LOG(ERROR)<<"matcher open failed"<<std::endl;
            return EXIT_FAILURE;
        }
        ProductDiscover pd(&matcher);
        if(!pd.Process(scd_path, output, thread_num))
        {
            LOG(ERROR)<<"discover process failed"<<std::endl;
            return EXIT_FAILURE;
        }
    } 
    if (vm.count("map-index")) {
        if( knowledge_dir.empty()||scd_path.empty())
        {
            return EXIT_FAILURE;
        }
        //ProductMatcher::Clear(knowledge_dir, mode);
        CategoryMapper mapper;
        mapper.SetCmaPath(cma_path);
        if(!mapper.Index(knowledge_dir, scd_path))
        {
            return EXIT_FAILURE;
        }
    } 
    if (vm.count("map-test")) {
        if( knowledge_dir.empty()||scd_path.empty())
        {
            return EXIT_FAILURE;
        }
        //ProductMatcher::Clear(knowledge_dir, mode);
        CategoryMapper mapper;
        mapper.SetCmaPath(cma_path);
        if(!mapper.Open(knowledge_dir))
        {
            return EXIT_FAILURE;
        }
        if(!mapper.DoMap(scd_path))
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
        if(!matcher.DoMatch(scd_path, output))
        {
            return EXIT_FAILURE;
        }
    } 
    if (vm.count("output-categorymap")) {
        if( knowledge_dir.empty()||scd_path.empty()||output.empty())
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
        if(!matcher.OutputCategoryMap(scd_path, output))
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
        if(!matcher.FuzzyDiff(scd_path, output))
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
        if(scd_path.empty())
        {
            while(true)
            {
                std::string line;
                std::cerr<<"input text:"<<std::endl;
                getline(std::cin, line);
                boost::algorithm::trim(line);
                UString query(line, UString::UTF_8);

                typedef std::list<std::pair<UString, double> > Hits;
                Hits hits;
                Hits left_hits;
                typedef std::list<UString> Left;
                Left left;
                izenelib::util::ClockTimer clocker;
                matcher.GetSearchKeywords(query, hits, left_hits, left);
                std::cout<<"[COST]"<<clocker.elapsed()<<std::endl;
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
        }
        else
        {
            std::vector<std::string> scd_list;
            B5MHelper::GetIUScdList(scd_path, scd_list);
            for(uint32_t i=0;i<scd_list.size();i++)
            {
                std::string scd_file = scd_list[i];
                LOG(INFO)<<"Processing "<<scd_file<<std::endl;
                ScdParser parser(izenelib::util::UString::UTF_8);
                parser.load(scd_file);
                uint32_t n=0;
                for( ScdParser::iterator doc_iter = parser.begin();
                  doc_iter!= parser.end(); ++doc_iter, ++n)
                {
                    if(n%10000==0)
                    {
                        LOG(INFO)<<"Find Documents "<<n<<std::endl;
                    }
                    Document::doc_prop_value_strtype title;
                    SCDDoc& scddoc = *(*doc_iter);
                    SCDDoc::iterator p = scddoc.begin();
                    for(; p!=scddoc.end(); ++p)
                    {
                        const std::string& property_name = p->first;
                        if(property_name=="Title")
                        {
                            title = p->second;
                            break;
                        }
                    }
                    if(title.empty()) continue;
                    typedef std::list<std::pair<UString, double> > Hits;
                    Hits hits;
                    Hits left_hits;
                    typedef std::list<UString> Left;
                    Left left;
                    matcher.GetSearchKeywords(propstr_to_ustr(title), hits, left_hits, left);
                    std::string stitle = propstr_to_str(title);
                    std::cout<<"[TITLE]"<<stitle<<std::endl;
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
                doc.property("Title") = str_to_propstr(line, UString::UTF_8);
                uint32_t limit = 3;
                std::vector<Product> products;
                matcher.Process(doc, limit, products);
                for(uint32_t i=0;i<products.size();i++)
                {
                    std::cout<<"[CATEGORY]"<<products[i].scategory<<std::endl;
                }
            }
        }
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
    if(vm.count("tour-generate"))
    {
        if( scd_path.empty() || mdb_instance.empty())
        {
            return EXIT_FAILURE;
        }
        TourProcessor processor;
        if(!processor.Generate(scd_path, mdb_instance))
        {
            std::cout<<"tour generator fail"<<std::endl;
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
        B5moProcessor processor(odb.get(), matcher.get(), mode, imgserver_config.get());
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
        if(!processor.Generate(scd_path, mdb_instance, last_mdb_instance, thread_num))
        {
            return EXIT_FAILURE;
        }
        LOG(INFO)<<"b5mo processor successfully"<<std::endl;
    }
    if(vm.count("b5mp-generate"))
    {
        if( mdb_instance.empty() )
        {
            return EXIT_FAILURE;
        }
        B5mpProcessor2 processor(mdb_instance, last_mdb_instance);
        if(!buffer_size.empty())
        {
            processor.SetBufferSize(buffer_size);
        }
        if(!sorter_bin.empty())
        {
            processor.SetSorterBin(sorter_bin);
        }
        if(!processor.Generate(spu_only, thread_num))
        {
            std::cout<<"b5mp processor failed"<<std::endl;
            return EXIT_FAILURE;
        }
    }
    if(vm.count("b5mc-generate"))
    {
        if( scd_path.empty() || mdb_instance.empty())
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

        //boost::shared_ptr<OfferDbRecorder> odbr(new OfferDbRecorder(odb.get(), last_odb.get()));
        B5mcScdGenerator generator(mode, matcher.get());
        if(!generator.Generate(scd_path, mdb_instance, last_mdb_instance, thread_num))
        {
            return EXIT_FAILURE;
        }
    }
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
    return EXIT_SUCCESS;
}

