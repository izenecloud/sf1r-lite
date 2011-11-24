#include "Graph.h"

#include <math.h>

#include <map>
#include <vector>
#include <set>
#include <fstream>
#include <iostream>
#include <algorithm>

using namespace std;
namespace sf1r
{
namespace text_summarization
{

bool Graph:: isFirstInstantiate_=true;


Graph::Graph()
{
    if (isFirstInstantiate_ == true)
    {
        isFirstInstantiate_ = false;
    }

    threshold_ = 0.000001;
    nDirect_ = 0;
    d_ = 0.85;
}

void Graph::initVertices(const vector<Sentence>& input)
{
    for (size_t i=0; i<input.size(); i++)
    {

        Vertex vertexNew(i, input[i]);
        vecVertices_.push_back(vertexNew);
    }
}

void Graph::initEdges()
{
    if (vecVertices_.size()<=1)
    {
        return;
    }
    vector<Vertex>::iterator iteri=vecVertices_.begin();
    for (; iteri!=vecVertices_.end(); iteri++)
    {
        vector<Vertex>::iterator iterj=iteri;
        for (iterj++; iterj!=vecVertices_.end(); iterj++)
        {
            if (nDirect_==UNDIRECTED || nDirect_==DIRECTED_FORWARD)
            {
                Edge edgeNew(*iteri, *iterj);
                vecEdges_.push_back(edgeNew);
            }
            else
            {
                Edge edgeNew(*iterj,*iteri);
                vecEdges_.push_back(edgeNew);
            }
        }
    }
    calcWeights();
}

void Graph::searchByStart(Vertex& vertexBegin, vector<Edge>& vecEdges)
{
    vector<Edge>::iterator iterEdge=vecEdges_.begin();
    for (; iterEdge!=vecEdges_.end(); iterEdge++)
    {
        Edge edge=*iterEdge;
        if ((edge.getWeight()>0 && edge.getStartVertex()==vertexBegin)
                ||(nDirect_==UNDIRECTED && edge.getEndVertex()==vertexBegin))
        {
            vecEdges.push_back(edge);
        }
    }
}

void Graph::calcWeights()
{
    vector<double> vecWeights;
    vecWeights.reserve( vecEdges_.size() );
    double dSum = 0.0;
    for (size_t i=0; i<vecEdges_.size(); i++)
    {
        vector<TermID> vecStartTokens;
        vector<TermID> vecEndTokens;

        vecStartTokens = vecEdges_[i].getStartVertex().getSentence();
        vecEndTokens = vecEdges_[i].getEndVertex().getSentence();

        int counter=0;

        vector<TermID>::iterator iterA=vecStartTokens.begin();
        vector<TermID>::iterator iterB;

        map<TermID, int > countMap1, countMap2;
        map<TermID, int >::iterator it;
        for ( ;iterA != vecStartTokens.end(); iterA++)
        {
            countMap1[*iterA]++;
        }
        for (iterB = vecEndTokens.begin(); iterB != vecEndTokens.end(); iterB++)
        {
            countMap2[*iterB]++;
        }

        for (it = countMap1.begin(); it != countMap1.end(); it++)
        {
            if ( countMap2[(*it).first] > 0 )
            {
                counter += countMap1[(*it).second]*countMap2[(*it).second];
            }
        }

        /*
        vector<TermID>::iterator iterA=vecStartTokens.begin();
        vector<TermID>::iterator iterB;
        for(;iterA!=vecStartTokens.end();iterA++){
            for(iterB=vecEndTokens.begin();iterB!=vecEndTokens.end();iterB++){
                if(*iterA==*iterB) counter++;
            }
        } */

        vecWeights[i] = ((double) counter) / (log(vecStartTokens.size())
                                              + log(vecEndTokens.size() ));
        dSum += vecWeights[i] * vecWeights[i];
    }
    dSum = sqrt(dSum);

    for (size_t k=0;k<vecEdges_.size();k++)
    {
        vecEdges_[k].setWeight(vecWeights[k]/dSum);
    }
}


void Graph::calcPageRank()
{
    double d;
    int step=0;
    do
    {
        d = 0.0;
        step++;
        for (size_t i=0;i<vecVertices_.size();i++)
        {
            vector<Edge> ejs;
            searchByEnd(vecVertices_[i],ejs);
            double pr = 0.0;
            for (size_t j=0;j<ejs.size();j++)
            {
                Edge ej=ejs[j];
                Vertex vj=ej.getAnotherVertex(vecVertices_[i]);
                vector<Edge> eks;
                searchByStart(vj,eks);
                double wkj = 0.0;
                for (size_t k=0;k<eks.size();k++)
                {
                    Edge ek = eks[k];
                    wkj+=ek.getWeight();
                }
                pr+= ej.getWeight() * vecVertices_[vj.getId()-1].getPageRank() / wkj;
            }
            pr = 1 - d_ + d_ * pr;
            double temp=fabs(pr-vecVertices_[i].getPageRank());
            if (d<temp) d=temp;
            vecVertices_[i].setPageRank(pr);
        }

    }
    while (d>threshold_);
}

void Graph::setProperties(double threshold, int nDirect, double d )
{
    threshold_ = threshold;
    nDirect_ = nDirect;
    d_ = d;
}


bool Graph::compare(const Vertex& vertLeft, const Vertex& vertRight)
{
    return( vertLeft.getPageRank() > vertRight.getPageRank() );
}


void Graph::searchByEnd(Vertex& vertexEnd, vector<Edge> & vecEdges)
{
    vector<Edge>::iterator iterEdge = vecEdges_.begin();
    for (; iterEdge!=vecEdges_.end(); iterEdge++)
    {
        Edge edge = *iterEdge;
        if (edge.getWeight()>0&&(edge.getEndVertex()==vertexEnd
                                 ||(nDirect_==UNDIRECTED && edge.getStartVertex()==vertexEnd)))
        {
            vecEdges.push_back(edge);
        }
    }
}


void Graph::doTs(vector<Sentence>& sentences)
{
    if (isFirstInstantiate_) isFirstInstantiate_ = false;

    initVertices(sentences);
    initEdges();
    calcPageRank();
}


void Graph::copyResult(vector<SentenceNO> &result, int cnt)
{
    result.clear();
    sort(vecVertices_.begin(), vecVertices_.end(), Graph::compare);
    for (vector<Vertex>::iterator it=vecVertices_.begin();
            it!= vecVertices_.end() && cnt >0; it++,cnt--)
    {
        result.push_back( it->getId() );
    }
}

}
}
