#ifndef BUNDLES_INDEX_RENDERERS_DOCUMENTS_RENDERER_H
#define BUNDLES_INDEX_RENDERERS_DOCUMENTS_RENDERER_H
/**
 * @file core/common/renderers/DocumentsRenderer.h
 * @author Ian Yang
 * @date Created <2010-06-11 13:01:17>
 */
#include <util/driver/Renderer.h>

#include <common/ResultType.h>

#include <util/ustring/UString.h>

#include <string>
#include <vector>

namespace sf1r {
class DisplayProperty;
using namespace izenelib;
using namespace izenelib::driver;
/**
 * @brief Render documents in response
 *
 * Dummy token @@ALL@@ is removed from ACL_ALLOW.
 */
class DocumentsRenderer : public driver::Renderer
{
    static const izenelib::util::UString::EncodingType kEncoding;

public:
    void renderDocuments(
        const std::vector<DisplayProperty> propertyList,
        const RawTextResultFromMIA& result,
        Value& resources
    );

    void renderDocuments(
        const std::vector<DisplayProperty> propertyList,
        const KeywordSearchResult& searchResult,
        Value& resources
    );

    void renderRelatedQueries(const KeywordSearchResult& miaResult,
                              Value& relatedQueries);

//     void renderPopularQueries(const KeywordSearchResult& miaResult,
//                               Value& popularQueries);
// 
//     void renderRealTimeQueries(const KeywordSearchResult& miaResult,
//                                Value& realTimeQueries);

    void renderTaxonomy(const KeywordSearchResult& miaResult,
                        Value& taxonomy);

    void renderNameEntity(const KeywordSearchResult& miaResult,
                          Value& nameEntity);

    void renderFaceted(const KeywordSearchResult& miaResult,
                          Value& facetedEntity);

    void renderGroup(const KeywordSearchResult& miaResult,
                          Value& groupResult);

    void renderAttr(const KeywordSearchResult& miaResult,
                          Value& attrResult);
};

} // namespace sf1r

#endif // BUNDLES_INDEX_RENDERERS_DOCUMENTS_RENDERER_H
