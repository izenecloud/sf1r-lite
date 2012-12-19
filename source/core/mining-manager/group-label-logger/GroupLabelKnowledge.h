/**
 * @file GroupLabelKnowledge.h
 * @brief the knowledge of top labels for category boosting.
 * @author Jun Jiang
 * @date Created 2012-12-18
 */

#ifndef SF1R_GROUP_LABEL_KNOWLEDGE_H
#define SF1R_GROUP_LABEL_KNOWLEDGE_H

#include "../group-manager/PropValueTable.h"
#include <string>
#include <vector>
#include <map>

namespace sf1r
{

class GroupLabelKnowledge
{
public:
    typedef faceted::PropValueTable::pvid_t LabelId;

    GroupLabelKnowledge(
        const std::string& targetCollection,
        const faceted::PropValueTable& categoryValueTable);

    /**
     * It reads files under directory @p dirPath with the file name pattern
     * "source_to_target.txt", and loads each line as a group label.
     *
     * Each line is a label path starting from root node, for example:
     * 手机数码>手机通讯>手机.
     *
     * @return true for success, false for failure
     */
    bool open(const std::string& dirPath);

    /**
     * Get the top labels in knowledge.
     * @param querySource where does the query come from
     * @param labelIdVec store the ids of each top label
     * @return true for found, false for not found
     */
    bool getKnowledgeLabel(
        const std::string& querySource,
        std::vector<LabelId>& labelIdVec) const;

private:
    void getMatchFileNames_(
        const std::string& dirPath,
        std::vector<std::string>& matchFileNames,
        std::vector<std::string>& matchSourceNames) const;

    std::string getMatchSourceName_(
        const std::string& fileName) const;

    bool loadFile_(
        const std::string& source,
        const std::string& filePath);

private:
    const std::string fileNamePattern_;
    const faceted::PropValueTable& categoryValueTable_;

    /** query source -> labels*/
    typedef std::map<std::string, std::vector<LabelId> > SourceLabels;
    SourceLabels sourceLabels_;
};

} // namespace sf1r

#endif // SF1R_GROUP_LABEL_KNOWLEDGE_H
