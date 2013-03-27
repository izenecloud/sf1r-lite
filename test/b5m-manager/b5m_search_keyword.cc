#include <b5m-manager/product_matcher.h>
#include "../TestResources.h"
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/network/protocol/http/server.hpp>
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
    ServerHandler(ProductMatcher* matcher)
    : matcher_(matcher)
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
        std::cout<<"cmd:"<<cmd<<std::endl;
        UString text(cmd, UString::UTF_8);
        typedef std::list<std::pair<UString, double> > Hits;
        Hits hits;
        typedef std::list<UString> Left;
        Left left;
        matcher_->GetSearchKeywords(text, hits, left);
        int status = 0;
        std::string sresult;

        for(Hits::const_iterator it = hits.begin();it!=hits.end();++it)
        {
            const std::pair<UString, double>& v = *it;
            std::string str;
            v.first.convertString(str, UString::UTF_8);
            sresult+=str;
            sresult+=",";
            sresult+=boost::lexical_cast<std::string>(v.second);
            sresult+="|";
        }
        //for(Left::const_iterator it = left.begin();it!=left.end();++it)
        //{
            //std::string str;
            //(*it).convertString(str, UString::UTF_8);
            //std::cout<<"[LEFT]"<<str<<std::endl;
        //}
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
    ProductMatcher* matcher_;
};

int main(int ac, char** av)
{
    std::string knowledge_dir(av[1]);
    ProductMatcher matcher;
    if(!matcher.Open(knowledge_dir))
    {
        return EXIT_FAILURE;
    }
    matcher.SetUsePriceSim(false);

    ServerHandler handler(&matcher);
    Server server("0.0.0.0", "18192", handler);
    server.run();

    return EXIT_SUCCESS;
}


