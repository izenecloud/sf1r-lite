#ifndef _SNIPPET_GENERATOR_SUBMANAGER_H
#define _SNIPPET_GENERATOR_SUBMANAGER_H

/**
 * @file SnippetGeneratorSubManager.h
 * @brief header file of SnippetGeneratorSubManager
 * @author MyungHyun (Kent)
 * @date Created <2008-06-05>
 *
 * History:
 * - 2009-09-05 Deepesh Shrestha 
 *   Modified interfaces to generate snippet of text and 
 *   sentence offsets instead of word offsets for analyzed
 *   queryterms from index manager.
 * - 2009-11-13 Peiseng Wang 
 *   getSnippet() modified to use dynamic programming for
 *   selecting the two highest score sentences
 * - 2009-11-20 Deepesh Shrestha 
 *   computeInverseSentenceFrequency() for computing tf*ISF 
 *   as a sentence metric for selecting snippet
 *
 */

#include <common/type_defs.h>
#include <document-manager/highlighter/Highlighter.h>

#include <util/ustring/UString.h>
#include <vector>

using namespace sf1r;

/**
 * @brief Generates snippets (query-biased summary) from a given text
 *
 * @details
 * SnippetGeneratorSubManager creates snippets by selecting query biased  
 * sentences snippet sentences and comparing their priorities in the following order:
 *      1. Based on the sequenc of query terms, sentences are scored. 
 *      2. Scoring mertric is either based on distance (D) between query terms or inverse.
 *      3. Sentence frequency (ISF) and term frequency (TF).
 *      4. Compute the score of each sentence.
 *      5. Pick two sentence(s) with the highest score to be presentend as a snippet.
 *
 */

class SnippetGeneratorSubManager {

    protected:
        typedef izenelib::util::UString::size_t CharacterOffset;

    public:

        SnippetGeneratorSubManager() :
            highlighter_() {
            }

        ~SnippetGeneratorSubManager() {
        }

        /**
         * @brief getSnippet() interface returns snippet based on query terms and string of content string.
         *
         * @param text              The text body that is subjected to snippet generation
         * @param offsetPairs       The offfset lists of starting and ending offset pair of sentences
         *                          (computed in TextSummarization.cpp in summarization module)
         * @param queryTerms    The is the query term string list (Analyzed + origianal query term string)
         * @param maxSnippetLength  The length of the snippet to be generated (set in config.xml)
         * @param bHighlight        If want to highlight resut snippet, true. (set in display property)
         * @param encodingType      common encoding type used while indexing and querying (set in config.xml)
         * @param snippetText       The result snippet string.
         * @return                  true if snippet generation is successful, else false
         */
        bool getSnippet(const izenelib::util::UString& text,
                const std::vector<uint32_t>& offsetPairs,
                const std::vector<izenelib::util::UString>& queryTerms,
                const unsigned int maxSnippetLength, 
                const bool bHighlight,
                izenelib::util::UString::EncodingType& encodingType,
                izenelib::util::UString& snippetText);

    private:

        /**
         * @brief getSnippetText_() method for cropping snippet text from snippet sentence.
         *
         * @param text              The text body that is subjected to snippet generation
         * @param queryTerms        The is the query term string list (Analyzed + origianal query term string)
         * @param maxSnippetLength  The length of the snippet to be generated (set in config.xml)
         * @param bHighlight        If want to highlight resut snippet, true. (set in display property)
         * @param encodingType      common encoding type used while indexing and querying (set in config.xml)
         * @param snippetText       The result snippet string.
         */

        bool getSnippetText_(
                const izenelib::util::UString& text,
                const std::vector<izenelib::util::UString>& queryTerms, 
                int snippetStarts,
                int nextSnippetStarts,
                const unsigned int maxSnippetLength, 
                const bool bHighlight,
                izenelib::util::UString::EncodingType& encodingType,
                izenelib::util::UString& snippetText);

        /**
         * @brief computeInverseSentenceFrequency_() method for computing inverse sentence frequency.
         *        TODO: score metrics to use ISF tf*computeInverseSentenceFrequency_(offsetPairs, termOffsets),
         *        currently the usage is based on term frequency and span of terms in computeScore_() method
         *        
         * @param offsetPairs               The offfset lists of starting and ending offset pair of sentences
         *                                  (computed in TextSummarization.cpp in summarization module)
         * @param termOffsets            List of character offset of query terms in the sentence text.
         * @return inverseSentenceFrequency Zero if @param termOffsetList is zero, else log2(N)-log2(ni) + 1
         *                                  where N is total number of sentences in the document,
         *                                        ni is the number of sentences with the query terms
         *
         */
        double computeInverseSentenceFrequency_(
                const std::vector<uint32_t>& offsetPairs,
                const std::vector<CharacterOffset>& termOffsets);

        /**
         * @brief computeScore_() method for computing inverse sentence frequency.
         *
         * @param tf               Term frequency
         * @param start             startOffset 
         * @param end               end offset of sentence
         * @return                  score of sentence
         */

        double computeScore_(
                const unsigned int tf,
                const unsigned int start, 
                const unsigned int end);

    private:

        ///maximum number of sentences to be selected for generating
        //snippet
        static const unsigned int NUM_OF_SENTENCES;

        ///number of prefixes to be added in the snippet sentence
        static const unsigned int NUM_OF_PREFIX_WORD;

        //number of prefixes to be added in the snippet sentence
        static const unsigned int MAX_TERM_FREQUENCY;

        //to if highlighting is enabled, method to highlight
        //is directly called from snippet
        Highlighter highlighter_;

};

#endif //_SNIPPET_GENERATOR_SUBMANAGER_H
