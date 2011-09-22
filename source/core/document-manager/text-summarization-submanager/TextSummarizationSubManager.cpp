/**
 * @file TextSummarizationSubManager.cpp
 * @brief header file of TextSummarizationSubManager class
 * @author MyungHyun (Kent)
 * @date 2008-06-05
 * @details
 * - Log
 * @author Deepesh Shrestha
 *   Modified interfaces to generate sentence blocks for
 *   snippet and summarization
 */

#include "TextSummarizationSubManager.h"
#include "text-summarization/TextSummarization.h"

using namespace std;
using namespace izenelib::util;     //for UString
using namespace sf1r::text_summarization;

namespace sf1r
{

TextSummarizationSubManager::TextSummarizationSubManager()
{}

TextSummarizationSubManager::~TextSummarizationSubManager()
{}

void TextSummarizationSubManager::init(
    ilplib::langid::Analyzer* langIdAnalyzer,
    boost::shared_ptr<izenelib::ir::idmanager::IDManager> idManager 
)
{
    langIdAnalyzer_ = langIdAnalyzer;
    idManager_ = idManager;
}

////@brief get requests for getting summary, snippet offsets, when given a text
bool TextSummarizationSubManager::getOffsetPairs(  
    const izenelib::util::UString& textBody,
    const unsigned int maxDisplayLength,
    const unsigned int numOfSentence,
    std::vector<CharacterOffset>& offsetPairs
)
{
    //clear out structure
    offsetPairs.clear();

    std::vector<std::vector<TermId> > sentenceListInTermId;
    std::vector<std::pair<CharacterOffset, CharacterOffset> > sentencesOffsetPairs;

    UString sentence;
    CharacterOffset startPos = 0;
    while (std::size_t len = langIdAnalyzer_->sentenceLength(textBody, startPos))
    {
        sentence.assign(textBody, startPos, len);

        /// Replace the old inefficient API to the new one. - Wei, 2010.08.25
        TermId id;
        sentenceListInTermId.push_back(vector<TermId>());
        vector<TermId>& sentenceIds = sentenceListInTermId.back();

        tokenizer_.tokenize(sentence);
        while (tokenizer_.nextToken())
        {
            if (!tokenizer_.isDelimiter())
            {
                idManager_->getTermIdByTermString(tokenizer_.getToken(), tokenizer_.getLength(), id);
                sentenceIds.push_back(id);
            }
        }

        sentencesOffsetPairs.push_back(make_pair(startPos, startPos+len));
        startPos += len;
    }

    //initialize first value to be 0 by default
    offsetPairs.push_back(0);

    //append summary offset
    vector<unsigned int> sentenceOrder;
    if (numOfSentence > 0)
    {
        TextSummarization ts( numOfSentence );
        ts.computeSummaryRM(numOfSentence, sentenceListInTermId, sentenceOrder);

        //adding summary sentence Offset pair
        unsigned int i=0;
        while ( ( i < sentenceOrder.size()) && (i < numOfSentence))
        {
            if (sentenceOrder[i] < sentencesOffsetPairs.size())
            {
                offsetPairs.push_back(sentencesOffsetPairs[sentenceOrder[i]].first);
                offsetPairs.push_back(sentencesOffsetPairs[sentenceOrder[i]].second);
            }
            i++;
        }

        if (numOfSentence < sentenceOrder.size())
            offsetPairs[0] = numOfSentence;
        else
            offsetPairs[0] = sentenceOrder.size();


        sentenceOrder.clear();
    }

    //adding snippet sentence offset pair
    unsigned int j = 0;
    while ( j < sentencesOffsetPairs.size())
    {
        offsetPairs.push_back(sentencesOffsetPairs[j].first);
        offsetPairs.push_back(sentencesOffsetPairs[j].second);
        j++;
    }

    if (offsetPairs.size() > 0)
        return true;
    else
        return false;
}


}

