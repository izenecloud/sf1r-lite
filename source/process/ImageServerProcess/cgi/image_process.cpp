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

#include <fastcgi++/request.hpp>
#include <fastcgi++/manager.hpp>
#include <Magick++.h>
#include <tfs_client_api.h>
#include <func.h>
#include <string>

using namespace std;
using namespace Magick;
using namespace tfs::client;
using namespace tfs::common;

// I like to have an independent error log file to keep track of exceptions while debugging.
// You might want a different filename. I just picked this because everything has access there.
void error_log(const std::string& msg)
{
	using namespace std;
	using namespace boost;
	static ofstream error;
	if(!error.is_open())
	{
		error.open("/tmp/errlog", ios_base::out | ios_base::app);
		error.imbue(locale(error.getloc(), new posix_time::time_facet()));
	}

	error << '[' << posix_time::second_clock::local_time() << "] " << msg << endl;
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

class ImageProcess: public Fastcgipp::Request<char>
{
	bool response()
	{
		using namespace Fastcgipp;

        if(environment().requestMethod != Http::HTTP_METHOD_GET && 
            environment().requestMethod != Http::HTTP_METHOD_HEAD )
        {
            out << "Status: 403 Forbidden\r\n\r\n";
            return true;
        }

        std::string img_file;
        std::string zoom_param;
		if(environment().gets.size() >= 1)
        {
			for(Http::Environment<char>::Gets::const_iterator it=environment().gets.begin(); it!=environment().gets.end(); ++it)
            {
                if(it->first == "filename")
                {
                    img_file = it->second;
                }
                else if(it->first == "zoom")
                {
                    zoom_param = it->second;
                }
           }
        }
		else
        {
            out << "Status: 400 Bad Request\r\n\r\n";
            return true;
        }

        if(img_file.empty())
        {
            out << "Status: 400 Bad Request\r\n\r\n";
            return true;
        }

        // read file of file system test
        //std::string root_img = "/home/lscm/imgresize/";
        //img_file = root_img + img_file;
        //error_log((img_file + zoom_param).c_str());
        ////if(access(std::string(img_file + zoom_param).c_str(), F_OK) != 0)
        //{
        //    Image image;
        //    try
        //    {
        //        image.read(img_file);
        //        image.scale(zoom_param);
        //        if(access(std::string(img_file + zoom_param).c_str(), F_OK) != 0)
        //        {
        //            image.write(img_file + zoom_param);
        //        }
        //        img_file = img_file + zoom_param;
        //    }
        //    catch( Exception &e)
        //    {
        //        out << "Status: 500 Internal Server Error\r\n";
        //        const char err[] = "Process Image Error in Magick!";
        //        out << "Content-Type: text/plain\r\nContent-Length: " << sizeof(err) - 1 <<"\r\n\r\n";
        //        out << err;
        //        return true;
        //    }
        //}

        //int fd = ::open(img_file.c_str(), O_RDONLY);
        //if(fd >=0)
        //{
        //    size_t total = lseek(fd, 0, SEEK_END);
        //    lseek(fd, 0, SEEK_SET);
        //    out << "Content-Type: text/plain\r\nContent-Length: " << total << "\r\n\r\n";
        //    char* buf = new char[total + 1];
        //    
        //    int retlen = 0;
        //    int readed = 0;
        //    while(( retlen = read(fd, buf + readed, total)) > 0 && total > 0)
        //    {
        //        out.dump(buf + readed, retlen);
        //        readed += retlen;
        //        total -= retlen;
        //    }
        //    delete[] buf;
        //}
        //else
        //{
        //    out << "Status: 500 Internal Server Error\r\n";
        //    const char err[] = "Read Image Error!";
        //    out << "Content-Type: text/plain\r\nContent-Length: " << sizeof(err) - 1 <<"\r\n\r\n";
		//	out << err;
        //}
        //error_log(img_file + zoom_param);
        int fd = -1;
        int ret = 0;
        fd = tfsclient->open(img_file.c_str(), NULL, T_READ);
        if( fd <= 0 )
        {
            error_log("tfs open file failed : " + img_file);
            out << "Status: 404 Not Found\r\n\r\n";
            return true;
        }
        TfsFileStat fstat;
        ret = tfsclient->fstat(fd, &fstat);
        if( ret != TFS_SUCCESS || fstat.size_ <= 0)
        {
            error_log("tfs get stat failed : " + img_file);
            out << "Status: 404 Not Found\r\n\r\n";
            return true;
        }

        char* buffer = new char[fstat.size_];
        int read = 0;
        uint32_t crc = 0;
        while(read < fstat.size_)
        {
            ret = tfsclient->read(fd, buffer + read, fstat.size_ - read);
            if(ret < 0)
            {
                break;
            }
            else
            {
                crc = Func::crc(crc, buffer + read, ret);
                read += ret;
            }
        }
        if(ret < 0 || crc != fstat.crc_)
        {
            error_log("tfs read file data error: " + img_file);
            out << "Status: 404 Not Found\r\n\r\n";
            tfsclient->close(fd);
            delete[] buffer;
            return true;
        }
        tfsclient->close(fd);

        if(zoom_param.empty())
        {
            out << "Content-Type: text/plain\r\nContent-Length: " << fstat.size_ << "\r\n\r\n";
            out.dump(buffer, fstat.size_);
            delete[] buffer;
            return true;
        }
        Blob imgdata;
        // imgdata will take the owner of buffer, so buffer should not been deleted by others.
        imgdata.updateNoCopy(buffer, fstat.size_);

        Image image;
        Blob zoomed_imgdata;
        try
        {
            image.read(imgdata);
            // only zoom out not zoom in.
            if(Geometry(zoom_param) <= image.size())
            {
                image.scale(zoom_param);
            }
            image.write(&zoomed_imgdata);
        }
        catch( Magick::Exception &e)
        {
            out << "Status: 500 Internal Server Error\r\n\r\n";
            error_log(std::string("Process Image Error in Magick, err: ") + e.what());
            return true;
        }
        catch( ... )
        {
            out << "Status: 500 Internal Server Error\r\n\r\n";
            error_log( "Unknown exception!" );
            return true;
        }
        if(zoomed_imgdata.data() == NULL)
        {
            out << "Status: 500 Internal Server Error\r\n\r\n";
            error_log( "zoomed image data is null!" );
            return true;
        }

        out << "Content-Type: text/plain\r\nContent-Length: " << zoomed_imgdata.length() << "\r\n\r\n";
        out.dump((const char*)zoomed_imgdata.data(), zoomed_imgdata.length());

		return true;
	}
public:
    static TfsClient* tfsclient;
};

TfsClient* ImageProcess::tfsclient = NULL;

int main(int argc, char* argv[])
{
	try
	{
        std::string nsip( "127.0.0.1:8108" );
        if(argc >= 2)
        {
            nsip = argv[1];
        }
        ImageProcess::tfsclient = TfsClient::Instance();
        int ret = ImageProcess::tfsclient->initialize(nsip.c_str());
        if(ret != TFS_SUCCESS)
        {
            perror("Error: connect to tfs nameserver failed.");
            error_log("Init TFS Client failed. nsip: " + nsip);
            return -1;
        }
        error_log("Init TFS Client OK. ns: " + nsip);

        InitializeMagick(NULL);
		// First we make a Fastcgipp::Manager object, with our request handling class
		// as a template parameter.
		Fastcgipp::Manager<ImageProcess> fcgi;
		// Now just call the object handler function. It will sleep quietly when there
		// are no requests and efficiently manage them when there are many.
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}
