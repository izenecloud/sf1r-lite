#include "Edge.h"

namespace sf1r
{
namespace text_summarization
{


Edge::Edge(const Vertex& vertexStart, const Vertex& vertexEnd)
{
    vertexStart_ = vertexStart;
    vertexEnd_ = vertexEnd;
    weight_ = 1.0;
}

Edge& Edge::operator = (const Edge& otherEdge)
{
    weight_ = otherEdge.getWeight();
    vertexStart_ = otherEdge.getStartVertex();
    vertexEnd_ = otherEdge.getEndVertex();
    return *this;
}

Edge::Edge()
{

}

Edge::~Edge()
{

}

Vertex Edge::getStartVertex() const
{
    return vertexStart_;
}

Vertex Edge::getEndVertex() const
{
    return vertexEnd_;
}

double Edge::getWeight() const
{
    return weight_;
}

void Edge::setWeight(double weight)
{
    weight_ = weight;
}

Vertex Edge::getAnotherVertex(const Vertex& vertex)
{
    /*double test = vertex.getPageRank();*/
    if (vertexStart_ == vertex)
    {
        return vertexEnd_;
    }
    else
    {
        return vertexStart_;
    }
}
}

}

