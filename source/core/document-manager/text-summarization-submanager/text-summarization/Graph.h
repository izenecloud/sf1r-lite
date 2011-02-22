#ifndef GRAPH_H
#define GRAPH_H

#include "ts_types.h"
#include "Vertex.h"
#include "Edge.h"

//#include <list>
#include <vector>
//#include <map>
//using namespace std;

namespace sf1r
{
namespace text_summarization
{

class Graph
{
    enum {UNDIRECTED, DIRECTED_FORWARD, DIRECTED_BACKWARD};
public:
    Graph();

    //Print the sentences in descending pageRank to file.
    //void printOrderedVertices(const string& strFileName);

    static bool compare(const Vertex& vertLeft, const Vertex& vertRight);

    /* Added by wps, for use this class in class TextSummarization.
     * 	4.27, 2008
     */
public:
    //Read program properties.
    void setProperties(double threshold, int nDirect, double d );
    void doTs(std::vector<Sentence>& sentences);
    void copyResult(std::vector<SentenceNO> &result, int cnt);

private:
    std::vector<Vertex> vecVertices_; //The set of vertices.
    std::vector<Edge> vecEdges_;   //The set of edges.

    double threshold_; //The parameter used to decide if the iteration should stop, default is 0.000001
    int nDirect_;    //The parameter that indicates if the graph is directed, default is 0.
    //0: undirected
    //1: directed forward
    //2: directed backward
    double  d_;// The parameter that integrating random jumps into the random walking model, default is 0.85.

    static bool isFirstInstantiate_;


    //Calculate the weight of all the edges.
    void calcWeights();

    //Calculate the pagerank of all the vertices.
    void calcPageRank();

    void initEdges();

    //Initialize the vertices according to the input file.
    void initVertices(const std::vector<Sentence>& input);

    // Search all the out edges.
    void searchByStart(Vertex& vertexBegin, std::vector<Edge> & vecEdges);

    //Search all the in edges.
    void searchByEnd(Vertex& vertexEnd, std::vector<Edge> & vecEdges);

};

}
}
#endif

