/***************************************************************************
* Copyright (C) 2007 Eddie Carle [eddie@erctech.org]                       *
*                                                                          *
* This file is part of fastcgi++.                                          *
*                                                                          *
* fastcgi++ is free software: you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as  published   *
* by the Free Software Foundation, either version 3 of the License, or (at *
* your option) any later version.                                          *
*                                                                          *
* fastcgi++ is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public     *
* License for more details.                                                *
*                                                                          *
* You should have received a copy of the GNU Lesser General Public License *
* along with fastcgi++.  If not, see <http://www.gnu.org/licenses/>.       *
****************************************************************************/

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <fastcgi++/request.hpp>
#include <fastcgi++/manager.hpp>
#include <string>
#include <net/sf1r/Sf1DriverBase.hpp>
#include <net/sf1r/Sf1Driver.hpp>
#include <net/sf1r/distributed/Sf1DistributedDriver.hpp>
#include <3rdparty/rapidjson/document.h>
#include <3rdparty/rapidjson/stringbuffer.h>
#include <3rdparty/rapidjson/writer.h>

using namespace std;
using namespace izenelib::net::sf1r;
using namespace boost::posix_time;
using boost::asio::ip::udp;

namespace rj = rapidjson;
// I like to have an independent error log file to keep track of exceptions while debugging.
// You might want a different filename. I just picked this because everything has access there.
void error_log(const std::string& msg)
{
	using namespace std;
	using namespace boost;
	static ofstream error;
	if(!error.is_open())
	{
		error.open("/tmp/sf1r_errlog", ios_base::out | ios_base::app);
		error.imbue(locale(error.getloc(), new posix_time::time_facet()));
	}

	error << "PID: " << (long)getpid() << " [" << posix_time::microsec_clock::local_time() << "] " << msg << endl;
}

// Let's make our request handling class. It must do the following:
// 1) Be derived from Fastcgipp::Request
// 2) Define the virtual response() member function from Fastcgipp::Request()

// First things first let's decide on what kind of character set we will use.
// Since we want to be able to echo all languages we will use unicode. The way this
// library handles unicode might be different than some are used to but it is done
// the way it is supposed to be. All internal characters are wide. In this case UTF-32.
// This way we don't have to mess around with variable size characters in our program.
// A string with 10 wchar_ts is ten characters long. Not up in the air as it is with UTF-8.
// Anyway, moving right along, the streams will code convert all the UTF-32 data to UTF-8
// before it is sent out to the client. This way we get the best of both worlds.
//
// So, whenever we are going to use UTF-8, our template parameter for Fastcgipp::Request<charT>
// should be wchar_t. Keep in mind that this suddendly makes
// everything wide character and utf compatible. Including HTTP header data (cookies, urls, yada-yada).

class Sf1rProcess: public Fastcgipp::Request<char>
{
	bool response()
	{
		using namespace Fastcgipp;

        if(environment().requestMethod != Http::HTTP_METHOD_GET && 
            environment().requestMethod != Http::HTTP_METHOD_POST )
        {
            out << "Status: 403 Forbidden\r\n\r\n";
            return true;
        }

        std::string requri = environment().requestUri;
        //error_log("got sf1r request: " + requri);
        if (requri.find("/sf1r") == 0)
            requri.replace(0, 5, "");
        std::string tokens = environment().findCustomHeader("HTTP_X_IZENESOFT_ACL_TOKENS");
        if (!tokens.empty())
        {
            error_log("using tokens : " + tokens);
        }
        std::string body = environment().raw_post.raw_post_data;
        if(!body.empty() && environment().raw_post.type == Http::Post<char>::raw)
        {
            //error_log("post raw data: " + body);
        }
		else
        {
            out << "Status: 400 Bad Request\r\n\r\n";
            return true;
        }

        std::string response;
        assert(driver);
        try
        {
            std::string log_content;
            rj::Document document;
            rj::Document::AllocatorType& allocator = document.GetAllocator();
            document.SetObject();
            rj::Value& top = document;
            rj::Value tag(log_tag.c_str(), log_tag.length(), allocator);
            top.AddMember("tag", tag, allocator);
            rj::Value request_body(body.c_str(), body.length(), allocator);
            top.AddMember("request", request_body, allocator);
            rj::Value request_uri(requri.c_str(), requri.length(), allocator);
            top.AddMember("uri", request_uri, allocator);
            static ptime epoch(boost::gregorian::date(1970, 1, 1));
            ptime current_time = microsec_clock::universal_time();
            time_duration td = current_time - epoch;
            std::string time_str = boost::lexical_cast<std::string>(double(td.total_milliseconds())/1000);
            rj::Value msec(time_str.c_str(), time_str.length(), allocator);
            top.AddMember("msec", msec, allocator);

            rj::StringBuffer stream;
            rj::Writer<rj::StringBuffer> writer(stream);
            document.Accept(writer);
            log_content.assign(stream.GetString(), stream.GetSize());
            boost::system::error_code ec;
            s.send_to(boost::asio::buffer(log_content.data(), log_content.size()),
                destination, 0, ec);

            if(ec)
            {
                error_log("send log to fluentd failed.");
            }

            response = driver->call(requri, tokens, body);
        }
        catch (ClientError& e) {
            error_log(std::string("ClientError: ") + e.what());
            out << "Status: 400 Bad Request\r\n\r\n";
            return true;
        } catch (ServerError& e) {
            error_log(std::string("ServerError: ") + e.what());
            out << "Status: 502 Bad GateWay\r\n\r\n";
            return true;
        } catch (RoutingError& e) {
            error_log(std::string("RoutingError: ") + e.what());
            out << "Status: 503 Service Unavailable\r\n\r\n";
            return true;
        } catch (NetworkError& e) {
            error_log(std::string("NetworkError: ") + e.what());
            out << "Status: 504 GateWay Time out\r\n\r\n";
            return true;
        } catch (std::exception& e) {
            error_log(std::string("Exception: ") + e.what());
            out << "Status: 500 Internal Server Error\r\n\r\n";
            return true;
        }

        out << "Content-Type: application/json\r\nContent-Length: " << response.size() << "\r\n\r\n";
        out.dump((const char*)response.data(), response.size());

		return true;
	}
public:
    static Sf1DriverBase* driver;
    static boost::asio::io_service io_service;
    static boost::asio::ip::udp::endpoint destination;
    static udp::socket s;
    static std::string log_tag;
};

Sf1DriverBase* Sf1rProcess::driver = NULL;
boost::asio::io_service Sf1rProcess::io_service;
udp::socket Sf1rProcess::s(Sf1rProcess::io_service);
boost::asio::ip::udp::endpoint Sf1rProcess::destination;
std::string Sf1rProcess::log_tag;


int main(int argc, char* argv[])
{
	try
	{
        if (argc < 5)
        {
            error_log("arg not enough. sf1rhost:sf1rport fluent_ip fluent_port fluent_tag ");
            return -1;
        }
        std::string host( "180.153.140.110:2181,180.153.140.111:2181,180.153.140.112:2181" );
        host = argv[1];
        std::string fluent_ip = argv[2];
        std::string fluent_port = argv[3];
        Sf1rProcess::log_tag = argv[4];
        std::string distributed;
        if(argc > 5) 
        {
            distributed = argv[5];
        }
        std::string match_master;
        if (argc > 6)
        {
            match_master = argv[6];
        }
        error_log("args:" + host + "," + fluent_ip + "," + fluent_port + "," + Sf1rProcess::log_tag +
            "," + distributed + "," + match_master);
        if(distributed.empty())
        {
            Sf1Config conf;
            conf.initialSize = 2;
            conf.resize = 2;
            conf.maxSize = 4;
            conf.timeout = 30;
            Sf1rProcess::driver = new Sf1Driver(host, conf);
        }
        else
        {
            Sf1DistributedConfig dconf;
            dconf.initialSize = 2;
            dconf.resize = 2;
            dconf.maxSize = 4;
            dconf.timeout = 30;
            dconf.zkTimeout = 2000;  //ms
            dconf.match_master_name = match_master;
            Sf1rProcess::driver = new Sf1DistributedDriver(host, dconf);
            error_log("config as distributed sf1r node");
        }

        Sf1rProcess::destination.address(boost::asio::ip::address::from_string(fluent_ip));
        Sf1rProcess::destination.port(boost::lexical_cast<unsigned short>(fluent_port));
        Sf1rProcess::s.open(udp::v4());
		// First we make a Fastcgipp::Manager object, with our request handling class
		// as a template parameter.
		Fastcgipp::Manager<Sf1rProcess> fcgi;
		// Now just call the object handler function. It will sleep quietly when there
		// are no requests and efficiently manage them when there are many.
		fcgi.handler();
	}
	catch(std::exception& e)
	{
        error_log("sf1r cgi process init failed, exited.");
        error_log(e.what());
	}
    if (Sf1rProcess::driver)
        delete Sf1rProcess::driver;
    return -1;
}
