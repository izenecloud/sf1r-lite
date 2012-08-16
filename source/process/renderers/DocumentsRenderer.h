#ifndef BUNDLES_INDEX_RENDERERS_DOCUMENTS_RENDERER_H
#define BUNDLES_INDEX_RENDERERS_DOCUMENTS_RENDERER_H
/**
 * @file core/common/renderers/DocumentsRenderer.h
 * @author Ian Yang
 * @date Created <2010-06-11 13:01:17>
 */
#include "SplitPropValueRenderer.h"
#include <util/driver/Renderer.h>

#include <common/ResultType.h>

#include <util/ustring/UString.h>

#include <string>
#include <vector>

namespace sf1r
{
class DisplayProperty;
class MiningSchema;

/**
 * @brief Render documents in response
 *
 * Dummy token @@ALL@@ is removed from ACL_ALLOW.
 */
class DocumentsRenderer : public izenelib::driver::Renderer
{
public:
    SplitPropValueRenderer splitRenderer_;

    DocumentsRenderer(const MiningSchema& miningSchema, int topKNum = 0);

    void renderDocuments(
        const std::vector<DisplayProperty>& propertyList,
        const RawTextResultFromMIA& result,
        izenelib::driver::Value& resources
    );

    void renderDocuments(
        const std::vector<DisplayProperty>& propertyList,
        const KeywordSearchResult& searchResult,
        izenelib::driver::Value& resources
    );

    void renderRelatedQueries(const KeywordSearchResult& miaResult,
                              izenelib::driver::Value& relatedQueries);

//     void renderPopularQueries(const KeywordSearchResult& miaResult,
//                               izenelib::driver::Value& popularQueries);
//
//     void renderRealTimeQueries(const KeywordSearchResult& miaResult,
//                                izenelib::driver::Value& realTimeQueries);

    void renderTaxonomy(const KeywordSearchResult& miaResult,
                        izenelib::driver::Value& taxonomy);

    void renderNameEntity(const KeywordSearchResult& miaResult,
                          izenelib::driver::Value& nameEntity);

    void renderFaceted(const KeywordSearchResult& miaResult,
                          izenelib::driver::Value& facetedEntity);

    void renderGroup(const KeywordSearchResult& miaResult,
                          izenelib::driver::Value& groupResult);

    void renderAttr(const KeywordSearchResult& miaResult,
                          izenelib::driver::Value& attrResult);

private:
    int TOP_K_NUM;
};

} // namespace sf1r

#endif // BUNDLES_INDEX_RENDERERS_DOCUMENTS_RENDERER_H
