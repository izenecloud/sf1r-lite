#ifndef _HIGHLIGHTER_H_
#define _HIGHLIGHTER_H_

/**
 * @file Highlighter.h
 * @brief header file of Highlighter
 * @author MyungHyun (Kent)
 * @date 2008-09-04
 *
 * History:
 *  -interfaces to produce highlighting tags based
 *  on string matching on text since word offsets
 *  are no longer availed from document manager
 */

#include <common/type_defs.h>
#include <util/ustring/UString.h>
#include <vector>

using namespace sf1r;

class Highlighter
{
protected:

    typedef izenelib::util::UString::size_t CharacterOffset;
    typedef unsigned int OffsetFlag;


public:
    Highlighter() {}

    ~Highlighter() {}

    /**
     * @brief getHighlightedText() Gets the text with highlighting tags.
     * @param textFragment      The actual text to be highlighted.
     * @param termStringList    The terms to be highlighted.
     * @param encodingType      The encoding type used during indexing and querying
     * @param highlightedText   The result of highlighting. The text fragment with
     */
    bool getHighlightedText(
        const izenelib::util::UString& textFragment,
        const std::vector<izenelib::util::UString>& termStringList,
        const izenelib::util::UString::EncodingType& encodingType,
        izenelib::util::UString& highlightedText
    ) const;

private:

    /**
     * @brief  getHighlightTagPositions() gets the start and end  positon of a term from
     *         the query string. The positions are marked with START_FLAG and END_FLAG
     * @param textFragment  The actual text to be highlighted.
     * @param termString    term from the query string that requires highlighting
     * @param highlightTagPositions   The character offset pairs including start and end tag
     *                                for a textFragment.
     */
    void getHighlightTagPositions(
        const izenelib::util::UString& textFragment,
        const izenelib::util::UString& termString,
        std::vector<std::pair<CharacterOffset, OffsetFlag> >& highlightTagPositions
    ) const;

    /**
     * @brief Inserts highlighting tags <!HS> and <!HE> around the terms in the text.
     * @param text  The text body to be highlighted with the query terms
     * @param highlightTagPositions  Offsets where the highlighting tags should be inserted
     * @param highlightedText  Resulting highlighted text.
     */
    void setHighlightTags(
        const izenelib::util::UString& text,
        const std::vector<std::pair<CharacterOffset, OffsetFlag> >& highlightTagPositions,
        const izenelib::util::UString::EncodingType& encodingType,
        izenelib::util::UString& highlightedText
    ) const;


private:
    /// @brief  Indicates that a given offset belongs to an start tag, in highlightTagPositions
    static const unsigned int START_FLAG;

    /// @brief  Indicates that a given offset belongs to an end tag, in highlightTagPositions
    static const unsigned int END_FLAG;
};

#endif //_HIGHLIGHTER_H_
