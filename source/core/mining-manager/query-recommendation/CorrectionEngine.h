#ifndef SF1R_RECOMMEND_CORRECTION_ENGINE_H
#define SF1R_RECOMMEND_CORRECTION_ENGINE_H

#include <string>
#include "PinyinTable.h"
#include "PrefixTable.h"
#include "parser/Parser.h"
#include "parser/ParserFactory.h"
#include "pinyin/PinYin.h"
#include "Filter.h"
#include "UQCateEngine.h"

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
    
    const PinYinConverter* pinYinApproximator() const
    {
        return pyApproximator_;
    }

    void setPinYinApproximator(PinYinConverter* pyConverter)
    {
        pyApproximator_ = pyConverter;
    }


    bool correct(const std::string& userQuery, std::string& results, double& freq ) const;

    void buildEngine(const std::string& path = "");
    bool isNeedBuild(const std::string& path = "") const;

    void evaluate(const std::string& path, std::string& sResult) const;
    
    void flush() const;
    void clear();
private:
    DISALLOW_COPY_AND_ASSIGN(CorrectionEngine);

    void processQuery(const std::string& userQuery, const std::string& category, const uint32_t freq);

private:
    PinyinTable* pinyin_;
    PrefixTable* prefix_;
    Filter*      filter_; 
    ParserFactory* parsers_;
    PinYinConverter* pyConverter_;
    PinYinConverter* pyApproximator_;
    time_t timestamp_;
    std::string workdir_;
};
}
}

#endif
