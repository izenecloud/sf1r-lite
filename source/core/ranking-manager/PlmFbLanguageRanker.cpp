/**
 * @file PlmFbLanguageRanker.cpp
 * @author Ian Yang
 * @date Created <2009-09-24 10:38:02>
 * @date Updated <2010-03-24 14:57:28>
 */
#include "PlmFbLanguageRanker.h"

#include <stdexcept>

namespace sf1r {

float PlmFbLanguageRanker::getScore(
    const RankQueryProperty& queryProperty,
    const RankDocumentProperty& documentProperty
) const
{
    throw std::logic_error("PlmFbLanguageRanker not implemented");
    return PlmLanguageRanker::getScore(queryProperty, documentProperty);
} // end PlmFbLanguageRanker::getScore

PlmFbLanguageRanker* PlmFbLanguageRanker::clone() const
{
    return new PlmFbLanguageRanker(*this);
}

} // namespace sf1r
