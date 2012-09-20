#ifndef SCDBUILDER_H
#define	SCDBUILDER_H

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

namespace fs = boost::filesystem;

class ScdBuilder
{
public:
    ScdBuilder(const fs::path& filePath, bool appendTrailingNewline = true)
    : appendTrailingNewline_(appendTrailingNewline)
    {
        out_.open(filePath, std::ios_base::out);
    }

    ~ScdBuilder()
    {
        if (appendTrailingNewline_)
        {
            out_ << std::endl;
        }
        out_.close();
    }

    std::size_t written()
    {
        return out_.tellp();
    }

    ScdBuilder& operator()(const std::string propertyName)
    {
        if (out_.tellp() != std::streampos(0))
        {
            out_ << std::endl;
        }

        out_ << "<" << propertyName << ">";
        return *this;
    }

    template<typename T>
    ScdBuilder& operator<<(const T& content)
    {
        out_ << content;
        return *this;
    }

private:
    bool appendTrailingNewline_;
    fs::ofstream out_;
};

#endif	/* SCDBUILDER_H */