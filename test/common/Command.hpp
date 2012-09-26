/*
 * File:   Command.hpp
 * Author: Paolo D'Apice
 *
 * Created on August 22, 2012, 02:26 PM
 */

#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <sstream>

/// Execute a command.
class Command {
    typedef boost::iostreams::stream< boost::iostreams::file_descriptor_source > ifdstream;
public:
    Command(const std::string& cmd) {
        FILE* pipe = popen(cmd.c_str(),"r");
        if (pipe == NULL)
            //throw std::runtime_error("Error exectung command: " + cmd);
            throw 0;
        ifdstream in(fileno(pipe), boost::iostreams::close_handle);

        for (std::string line; not in.eof();) {
            getline(in, line);
            sstream << line << "\n";
        }
    }
    /// Get the stream to the command output.
    std::istream& stream() {
        return sstream;
    }
private:
    std::stringstream sstream;
};

#endif /* COMMAND_HPP */
