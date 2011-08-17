#include "split_ustr.h"

namespace
{
typedef izenelib::util::UString::const_iterator UStrIter;

const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;

/** colon to split attribute name and value */
const izenelib::util::UString COLON(":", ENCODING_TYPE);

/** vertical line to split attribute values */
const izenelib::util::UString VL("|", ENCODING_TYPE);

/** comma */
const izenelib::util::UString COMMA(",", ENCODING_TYPE);

/** comma and vertical line */
const izenelib::util::UString COMMA_VL(",|", ENCODING_TYPE);

/** quote to embed escape characters */
const izenelib::util::UString QUOTE("\"", ENCODING_TYPE);

/**
 * scans from @p first to @p last until any character in @p delimStr is reached,
 * copy the processed string into @p output.
 * @param first the begin position
 * @param last the end position
 * @param delimStr if any character in @p delimStr is reached, stop the scanning
 * @param output store the sub string
 * @return the last position scaned
 */
UStrIter parse_ustr(
    UStrIter first,
    UStrIter last,
    const izenelib::util::UString& delimStr,
    izenelib::util::UString& output
)
{
    output.clear();

    UStrIter it = first;

    if (first != last && *first == QUOTE[0])
    {
        // escape start quote
        ++it;

        for (; it != last; ++it)
        {
            // candidate end quote
            if (*it == QUOTE[0])
            {
                ++it;

                // convert double quotes to single quote
                if (it != last && *it == QUOTE[0])
                {
                    output.push_back(*it);
                }
                else
                {
                    // end quote escaped
                    break;
                }
            }
            else
            {
                output.push_back(*it);
            }
        }

        // escape all characters between end quote and delimStr
        for (; it != last; ++it)
        {
            if (delimStr.find(*it) != izenelib::util::UString::npos)
                break;
        }
    }
    else
    {
        for (; it != last; ++it)
        {
            if (delimStr.find(*it) != izenelib::util::UString::npos)
                break;

            output.push_back(*it);
        }

        // as no quote "" is surrounded,
        // trim space characters at head and tail
        izenelib::util::UString::algo::compact(output);
    }

    return it;
}
}

namespace sf1r
{

void split_group_value(
    const izenelib::util::UString& src,
    std::vector<izenelib::util::UString>& groupValues
)
{
    UStrIter it = src.begin();
    UStrIter endIt = src.end();

    while (it != endIt)
    {
        izenelib::util::UString value;
        it = parse_ustr(it, endIt, COMMA, value);

        if (!value.empty())
        {
            groupValues.push_back(value);
        }

        if (it != endIt)
        {
            assert(*it == COMMA[0]);
            // escape comma
            ++it;
        }
    }
}

void split_attr_pair(
    const izenelib::util::UString& src,
    std::vector<AttrPair>& attrPairs
)
{
    UStrIter it = src.begin();
    UStrIter endIt = src.end();

    while (it != endIt)
    {
        izenelib::util::UString name;
        it = parse_ustr(it, endIt, COLON, name);
        if (it == endIt || name.empty())
            break;

        AttrPair attrPair;
        attrPair.first = name;
        izenelib::util::UString value;

        // escape colon
        ++it;
        it = parse_ustr(it, endIt, COMMA_VL, value);
        if (!value.empty())
        {
            attrPair.second.push_back(value);
        }

        while (it != endIt && *it == VL[0])
        {
            // escape vertical line
            ++it;
            it = parse_ustr(it, endIt, COMMA_VL, value);
            if (!value.empty())
            {
                attrPair.second.push_back(value);
            }
        }
        if (!attrPair.second.empty())
        {
            attrPairs.push_back(attrPair);
        }

        if (it != endIt)
        {
            assert(*it == COMMA[0]);
            // escape comma
            ++it;
        }
    }
}

}
