#ifndef EDGE_H
#define EDGE_H

#include "Vertex.h"

namespace sf1r
{
namespace text_summarization
{

class Edge
{
public:
    Edge();
    Edge(const Vertex& vertexStart, const Vertex& vertexEnd);
    Edge& operator = (const Edge& otherEdge);
    virtual ~Edge();

    void setWeight(double weight);
    double getWeight() const;

    Vertex getEndVertex() const;
    Vertex getStartVertex() const;
    Vertex getAnotherVertex(const Vertex& vertex);

private:
    Vertex  vertexStart_;  //The start vertex of the edge.
    Vertex  vertexEnd_;  //The end vertex of the edge
    double  weight_;   //The weight of the edge.
};

}
}
#endif
