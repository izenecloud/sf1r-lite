#include <b5m-manager/category_mapper.h>
#include "../TestResources.h"
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/network/protocol/http/server.hpp>
#include <boost/network/uri/uri.hpp>
#include <boost/network/uri/decode.hpp>


using namespace sf1r;
namespace po = boost::program_options;
namespace http = boost::network::http;
namespace uri = boost::network::uri;

int do_main(int ac, char** av);

struct ServerHandler;

typedef http::server<ServerHandler> Server;

struct ServerHandler
{
    ServerHandler(CategoryMapper* mapper)
    : mapper_(mapper)
    {
    }
    void operator()(const Server::request& request, Server::response& response)
    {
        typedef Server::request::string_type string_type;
        typedef CategoryMapper::ResultItem ResultItem;
        std::string uri_string = destination(request);
        std::size_t cmd_pos = uri_string.find("cmd=");
        std::string cmd;
        if(cmd_pos!=std::string::npos)
        {
            cmd = uri_string.substr(cmd_pos+4);
        }
        cmd = uri::decoded(cmd);
        //std::cout<<"cmd:"<<cmd<<std::endl;
        UString text(cmd, UString::UTF_8);
        std::vector<ResultItem> results;
        mapper_->Map(text, results);
        int status = 0;
        std::string sresult;
        for(uint32_t i=0;i<results.size();i++)
        {
            std::string str;
            results[i].category.convertString(str, UString::UTF_8);
            //std::cout<<"[F]"<<str<<std::endl;
            if(!sresult.empty()) sresult+=",";
            sresult+=str;
        }
        if(status==0)//success
        {
            response = Server::response::stock_reply(Server::response::ok, sresult);
        }
        else
        {
            response = Server::response::stock_reply(Server::response::internal_server_error, sresult);
        }
    }

    void log(const Server::string_type& info)
    {
        std::cerr<<"[server log]"<<info<<std::endl;
    }
private:
    CategoryMapper* mapper_;
};

int main(int ac, char** av)
{
    std::string knowledge_dir(av[1]);
    CategoryMapper mapper;
    if(!mapper.Open(knowledge_dir))
    {
        return EXIT_FAILURE;
    }

    ServerHandler handler(&mapper);
    Server server("0.0.0.0", "18193", handler);
    server.run();

    return EXIT_SUCCESS;
}


