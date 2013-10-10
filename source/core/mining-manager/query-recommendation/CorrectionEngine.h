#ifndef CORRECTION_ENGINE_H
#define CORRECTION_ENGINE_H

#include <string>
#include "PinyinTable.h"
#include "PrefixTable.h"
#include "parser/Parser.h"
#include "parser/ParserFactory.h"
#include "pinyin/PinYin.h"
#include "Filter.h"

namespace sf1r
{
namespace Recommend
{
class CorrectionEngine
{
public:
    CorrectionEngine(const std::string& workdir);
    ~CorrectionEngine();
public:
    const PinYinConverter* pinYinConverter() const
    {
        return pyConverter_;
    }

    void setPinYinConverter(PinYinConverter* pyConverter)
    {
        pyConverter_ = pyConverter;
    }

    bool correct(const std::string& userQuery, std::string& results, double& freq ) const;

    void buildEngine(const std::string& path = "");
    bool isNeedBuild(const std::string& path = "") const;

    void evaluate(const std::string& path = "") const;
    
    void flush() const;
    void clear();
private:
    DISALLOW_COPY_AND_ASSIGN(CorrectionEngine);

    void processQuery(const std::string& userQuery, const uint32_t freq);

private:
    PinyinTable* pinyin_;
    PrefixTable* prefix_;
    Filter*      filter_; 
    ParserFactory* parsers_;
    PinYinConverter* pyConverter_;
    time_t timestamp_;
    std::string workdir_;
};
}
}

#endif
