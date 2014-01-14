#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/network/protocol/http/server.hpp>
#include <boost/network/utils/thread_pool.hpp>
#include <boost/network/uri/uri.hpp>
#include <boost/network/uri/decode.hpp>
#include <b5m-manager/address_extract.h>

using namespace sf1r;
using namespace sf1r::b5m;
namespace po = boost::program_options;
namespace http = boost::network::http;
namespace uri = boost::network::uri;

int do_main(int ac, char** av);

struct ServerHandler;

typedef http::server<ServerHandler> Server;

struct ServerHandler
{
    ServerHandler(AddressExtract* an): an_(an)
    {
    }
    void operator()(const Server::request& request, Server::response& response)
    {
        typedef Server::request::string_type string_type;
        std::string uri_string = destination(request);
        std::size_t addr_pos = uri_string.find("addr=");
        std::string addr;
        if(addr_pos!=std::string::npos)
        {
            addr = uri_string.substr(addr_pos+5);
        }
        addr = uri::decoded(addr);
        //uri::uri _uri(uri_string);
        //string_type q = uri::query(_uri);
        std::vector<std::string> args = po::split_unix(addr);
        if(args.empty()) return;
        addr = args.front();
        std::cout<<"[addr]"<<addr<<std::endl;
        std::string n = an_->Evaluate(addr);
        int status = 0;
        //std::string sstatus = boost::lexical_cast<std::string>(status);
        std::string sstatus = n;
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
    AddressExtract* an_;
};

int main(int ac, char** av)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("knowledge,K", po::value<std::string>(), "specify knowledge dir")
        ("address,A", po::value<std::string>(), "specify the address, for test")
        ("thread-num,T", po::value<int>(), "specify server thread num")
        ("test", "do test")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm); 
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }
    std::string knowledge;
    std::string address;
    int thread_num = 50;
    bool dotest = false;
    if(vm.count("test"))
    {
        dotest = true;
    }
    if (vm.count("knowledge")) {
        knowledge = vm["knowledge"].as<std::string>();
        std::cerr << "knowledge: " << knowledge <<std::endl;
    } 
    if (vm.count("address")) {
        address = vm["address"].as<std::string>();
        std::cerr << "address: " << address<<std::endl;
    } 
    if(vm.count("thread-num")) {
        thread_num = vm["thread-num"].as<int>();
    }
    std::cerr<<"thread-num: "<<thread_num<<std::endl;
    if(knowledge.empty())
    {
        LOG(ERROR)<<"Knowledge path not specified"<<std::endl;
        return EXIT_FAILURE;
    }
    AddressExtract an;
    if(dotest)
    {
        an.Process(knowledge);
        return EXIT_SUCCESS;
    }
    if(!an.Train(knowledge))
    {
        LOG(ERROR)<<"train error"<<std::endl;
        return EXIT_FAILURE;
    }
    LOG(INFO)<<"train finished"<<std::endl;
    if(!address.empty())
    {
        std::string n = an.Evaluate(address);
        std::cout<<n<<std::endl;
        return EXIT_SUCCESS;
    }
    ServerHandler handler(&an);
    Server server("0.0.0.0", "18199", handler);
    std::vector<boost::thread*> threads;
    for(int i=0;i<thread_num;i++)
    {
        boost::thread* t = new boost::thread(boost::bind(&Server::run, &server));
        threads.push_back(t);
    }
    for(int i=0;i<thread_num;i++)
    {
        threads[i]->join();
    }
    for(int i=0;i<thread_num;i++)
    {
        delete threads[i];
    }
    return EXIT_SUCCESS;
}

