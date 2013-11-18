#include "AbbrEngine.h"
#include <util/singleton.h>
#include <util/functional.h>
#include <la-manager/AttrTokenizeWrapper.h>

namespace sf1r
{
namespace QA
{
AbbrEngine::AbbrEngine()
    : hot_(NULL)
    , workdir_("")
    , resdir_("")
    , tokenizer_(NULL)
    , isInited_(false)
{
}

AbbrEngine::~AbbrEngine()
{
    if (NULL != hot_)
    {
        delete hot_;
        hot_ = NULL;
    }
}

AbbrEngine* AbbrEngine::get()
{
    return izenelib::util::Singleton<AbbrEngine>::get();
}
    
void AbbrEngine::init(const std::string& workdir, const std::string& resdir)
{
    if (isInited_)
        return;

    workdir_ = workdir;
    resdir_  = resdir;

    hot_ = new HotTokens(workdir_);
    if (hot_->isNeedBuild(resdir_ + "/query_abbreviation/qa_term.txt"))
    {
//        std::cout<<"Build....\n";
        hot_->build(resdir_ + "/query_abbreviation/qa_term.txt");
    }
    else
    {
//        std::cout<<"Load....\n";
        hot_->load();
    }

    tokenizer_ = Recommend::Tokenize::getTokenizer_();
    isInited_ = true; 
}

void AbbrEngine::abbreviation(const std::string& userQuery, std::vector<std::string>& abbrs)
{
    if (NULL == tokenizer_)
        return;

    Recommend::Tokenize::TermVector tv;
    (*tokenizer_)(userQuery, tv);
    //std::vector<std::pair<std::string, int> > tv;
    //AttrTokenizeWrapper::get()->attr_tokenize(userQuery, tv);

    typedef std::pair<std::string, uint32_t> FToken;
    typedef izenelib::util::second_greater<FToken> greater_than;

    std::vector<FToken> ftVector; // factor token vector
    //std::vector<std::pair<std::string, int> >::const_iterator it = tv.begin();
    Recommend::Tokenize::TermVector::iterator it = tv.begin();
    for (; it != tv.end(); ++it)
    {
        std::string term = it->term();
        uint32_t factor = hot_->search(term);
        //std::cout<<term<<"\t=>"<<factor<<"\n";
        if (factor == std::numeric_limits<uint32_t>::max())
            continue;
        ftVector.push_back(std::make_pair(term, factor));
    }
    
    std::sort(ftVector.begin(), ftVector.end(), greater_than());
    std::string abbr = "";
    for (std::size_t i = 0; i < ftVector.size(); ++i)
    {
        abbr += ftVector[i].first;
        abbr += " ";
        abbrs.push_back(abbr);
    }
    std::reverse(abbrs.begin(), abbrs.end());
}

}
}
