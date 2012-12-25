#include "GroupLabelKnowledge.h"
#include "../util/split_ustr.h"
#include <util/ustring/UString.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <glog/logging.h>
#include <fstream>

using namespace sf1r;
namespace bfs = boost::filesystem;

namespace
{
const izenelib::util::UString::EncodingType kEncodingType =
    izenelib::util::UString::UTF_8;

std::string createFileNamePattern(const std::string& targetCollection)
{
    return "_to_" + targetCollection + ".txt";
}

}

GroupLabelKnowledge::GroupLabelKnowledge(
    const std::string& targetCollection,
    const faceted::PropValueTable& categoryValueTable)
    : fileNamePattern_(createFileNamePattern(targetCollection))
    , categoryValueTable_(categoryValueTable)
{
}

bool GroupLabelKnowledge::open(const std::string& dirPath)
{
    std::vector<std::string> matchFileNames;
    std::vector<std::string> matchSourceNames;
    getMatchFileNames_(dirPath, matchFileNames, matchSourceNames);

    const bfs::path baseDir(dirPath);
    const std::size_t matchNum = matchFileNames.size();
    bool result = true;

    for (std::size_t i = 0; i < matchNum; ++i)
    {
        const bfs::path filePath(baseDir / matchFileNames[i]);
        if (!loadFile_(matchSourceNames[i], filePath.string()))
        {
            result = false;
        }
    }

    return result;
}

bool GroupLabelKnowledge::getKnowledgeLabel(
    const std::string& querySource,
    std::vector<LabelId>& labelIdVec) const
{
    SourceLabels::const_iterator it = sourceLabels_.find(querySource);

    if (it == sourceLabels_.end())
        return false;

    labelIdVec = it->second;
    return true;
}

void GroupLabelKnowledge::getMatchFileNames_(
    const std::string& dirPath,
    std::vector<std::string>& matchFileNames,
    std::vector<std::string>& matchSourceNames) const
{
    const bfs::directory_iterator itEnd;
    for (bfs::directory_iterator it(dirPath); it != itEnd; ++it)
    {
        if (!bfs::is_regular_file(it->status()))
            continue;

        std::string fileName = it->path().filename().string();
        std::string sourceName = getMatchSourceName_(fileName);

        if (sourceName.empty())
            continue;

        matchFileNames.push_back(fileName);
        matchSourceNames.push_back(sourceName);
    }
}

std::string GroupLabelKnowledge::getMatchSourceName_(
    const std::string& fileName) const
{
    std::size_t pos = fileName.find(fileNamePattern_);
    std::string matchSource;

    if (pos != std::string::npos &&
        pos + fileNamePattern_.size() == fileName.size())
    {
        matchSource = fileName.substr(0, pos);
    }

    return matchSource;
}

bool GroupLabelKnowledge::loadFile_(
    const std::string& source,
    const std::string& filePath)
{
    std::ifstream ifs(filePath.c_str());
    if (!ifs)
    {
        LOG(ERROR) << "failed to open " << filePath;
        return false;
    }

    std::vector<LabelId>& labels = sourceLabels_[source];
    std::string line;
    while (std::getline(ifs, line))
    {
        boost::trim(line);

        if (line.empty())
            continue;

        const izenelib::util::UString ustrLine(line, kEncodingType);
        std::vector<vector<izenelib::util::UString> > ustrPaths;
        split_group_path(ustrLine, ustrPaths);

        for (std::vector<vector<izenelib::util::UString> >::const_iterator it =
                 ustrPaths.begin(); it != ustrPaths.end(); ++it)
        {
            LabelId labelId = categoryValueTable_.propValueId(*it);
            if (labelId)
            {
                labels.push_back(labelId);
            }
            else
            {
                LOG(WARNING) << "unknown label path: " << line;
            }

        }
    }

    return true;
}
