/* Simple wrapper around OGDF 
 * - making a weaker dependency, especially for memory management
 * - also designed for Dia's need to simply try out some layout algorithms
 * - might even become a C or D interface to OGDF?
 *
 * Author:
 *   Hans Breuer <hans@breuer.org>
 *
 * This code is in public domain
 */

#include <ogdf/energybased/GEMLayout.h>
#include <ogdf/energybased/FMMMLayout.h>
#include <ogdf/energybased/FastMultipoleEmbedder.h>
#include <ogdf/misclayout/BalloonLayout.h>
#include <ogdf/misclayout/CircularLayout.h>
#include <ogdf/energybased/DavidsonHarelLayout.h>
#include <ogdf/energybased/StressMajorizationSimple.h>
#include <ogdf/energybased/MultilevelLayout.h>
#include <ogdf/layered/SugiyamaLayout.h>
#include <ogdf/layered/OptimalHierarchyLayout.h>
#include <ogdf/upward/DominanceLayout.h>
#include <ogdf/upward/UpwardPlanarizationLayout.h>
#include <ogdf/upward/VisibilityLayout.h>
#include <ogdf/energybased/SpringEmbedderFR.h>
#include <ogdf/energybased/SpringEmbedderKK.h>
#include <ogdf/tree/TreeLayout.h>
#include <ogdf/tree/RadialTreeLayout.h>
#include <ogdf/planarlayout/MixedModelLayout.h>
#include <ogdf/planarlayout/PlanarDrawLayout.h>
#include <ogdf/planarlayout/PlanarStraightLayout.h>
#include <ogdf/planarity/PlanarizationLayout.h>
#include <ogdf/planarity/PlanarizationGridLayout.h>
// new with v2012.06
#include <ogdf/planarlayout/FPPLayout.h>
#include <ogdf/planarlayout/SchnyderLayout.h>

#include <ogdf/energybased/multilevelmixer/MMMExampleFastLayout.h>
#include <ogdf/energybased/multilevelmixer/MMMExampleNiceLayout.h>
#include <ogdf/energybased/multilevelmixer/MMMExampleNoTwistLayout.h>
#include <ogdf/energybased/multilevelmixer/MixedForceLayout.h>

#include <ogdf/orthogonal/OrthoLayout.h>

#ifdef _MSC_VER
#define DLL_EXPORT __declspec(dllexport)
#endif
#include "ogdf-simple.h"

namespace {

class KGraph : public IGraph
{
    //! mapping indices to node pointers
    std::vector<ogdf::node> m_nodes;
    //! averege node size updated during graph constructions
    double m_avgNodeSize;
    // mapping edges
    std::vector<ogdf::edge> m_edges;
    //! average edge length (direct line) updated during graph constructions
    double m_avgEdgeLen;
    //! the basic graph
    ogdf::Graph *m_pGraph;
    //! including the geometric information 
    ogdf::GraphAttributes *m_pGraphAttrs;
    //! valid after Layout method call
    ogdf::LayoutModule *m_pLayout;

    //! use IGrap:Release()
    ~KGraph ();
    
    //! just scale by 
    static const int m_scale = 20;
public :
    //! Clean up and destroy the graph
    void Release ();
    //! Create a new node with bounding box and return it's index
    int AddNode (double left, double top, double right, double bottom);
    //! Create a new edge and return it's index
    int AddEdge (int srcNode, int destNode, double* points, int len);

    //! layout the graph
    eResult Layout (const char *module);

    // Layout result left,top
    bool GetNodePosition (int node, double* x, double* y);
    int  GetEdgeBends (int e, double *coords, int len);

    //! only internally used by factory function
    KGraph ();
protected :
    //! internal layout function w/o exception handler
    bool RealLayout (const char *module);
};

KGraph::KGraph() :
    m_pGraph(new ogdf::Graph()),
    m_pGraphAttrs(new ogdf::GraphAttributes(*m_pGraph, 
					    ogdf::GraphAttributes::nodeGraphics 
					    | ogdf::GraphAttributes::edgeGraphics
					    | ogdf::GraphAttributes::edgeType)),
    m_pLayout(NULL),
    m_avgNodeSize(0),
    m_avgEdgeLen(0)
{
}
KGraph::~KGraph()
{
    if (m_pGraphAttrs)
        delete m_pGraphAttrs;
    if (m_pGraph)
        delete m_pGraph;
    if (m_pLayout)
        delete m_pLayout;
}
void
KGraph::Release ()
{
    delete this;
}

int 
KGraph::AddNode (double left, double top, double right, double bottom)
{
    ogdf::node node = m_pGraph->newNode();
    double w = (right - left);
    double h = (bottom - top);

    // apparently we have to give the center of the node
    m_pGraphAttrs->x(node) = (left + w/2) * m_scale;
    m_pGraphAttrs->y(node) = (top + h/2) * m_scale;
    m_pGraphAttrs->width(node) = w * m_scale;
    m_pGraphAttrs->height(node) = h * m_scale;

    m_avgNodeSize = ((m_avgNodeSize * m_nodes.size()) + (w * m_scale) * (h * m_scale)) / (m_nodes.size() + 1);
    m_nodes.push_back (node);

    return m_nodes.size();
}

int 
KGraph::AddEdge (int srcNode, int dstNode, double* coords, int len)
{
    ogdf::edge edge;
    ogdf::node src;
    ogdf::node dst;
  
    if (srcNode < 0 || srcNode >= m_nodes.size())
	return -1;
    if (dstNode < 0 || dstNode >= m_nodes.size())
	return -2;

    src = m_nodes[srcNode];
    dst = m_nodes[dstNode];
    double xs, xd, ys, yd;
    double dist;
    if (GetNodePosition (srcNode, &xs, &ys) && GetNodePosition (dstNode, &xd, &yd))
    	dist = sqrt((xd - xs) * (xd - xs) + (yd - ys) * (yd - ys)) * m_scale;
    else
	dist = 0.0;

    edge = m_pGraph->newEdge(src, dst);

    // initialization of points
    if (len > 0)
    {
	ogdf::DPolyline &poly = m_pGraphAttrs->bends(edge);
	for (int i = 0; i < len; i+=2)
	     poly.pushBack (ogdf::DPoint (coords[i] * m_scale, coords[i+1] * m_scale));
    }
    m_avgEdgeLen = (m_avgEdgeLen * m_edges.size() + dist) / (m_edges.size() + 1);
    m_edges.push_back (edge);

    return m_edges.size();
}

bool
KGraph::RealLayout (const char *module)
{
    // improve default initialization - distance from node size
    const double distFactor = 1.5;
#ifdef OGDF_DEBUG
    m_pGraphAttrs->writeGML ("d:\\temp\\ogdf-simple-pre.gml");
#endif
    if (strcmp ("Balloon", module) == 0)
        m_pLayout = new ogdf::BalloonLayout ();
    else if (strcmp ("Circular", module) == 0)
        m_pLayout = new ogdf::CircularLayout ();
    else if (strcmp ("DavidsonHarel", module) == 0)
    {
        ogdf::DavidsonHarelLayout *pLayout = new ogdf::DavidsonHarelLayout ();
        pLayout->setPreferredEdgeLength (std::max(m_avgEdgeLen, sqrt(m_avgNodeSize)*distFactor));
        m_pLayout = pLayout;
    }
    else if (strcmp ("Dominance", module) == 0)
    {
        ogdf::DominanceLayout *pLayout = new ogdf::DominanceLayout ();
	pLayout->setMinGridDistance (std::max(m_avgEdgeLen, sqrt(m_avgNodeSize)*distFactor));
        m_pLayout = pLayout;
    }
    else if (strcmp ("FMMM", module) == 0)
    {
        ogdf::FMMMLayout *pLayout = new ogdf::FMMMLayout ();
        pLayout->unitEdgeLength (std::max(m_avgEdgeLen, sqrt(m_avgNodeSize)*distFactor));
        m_pLayout = pLayout;
    }
    else if (strcmp ("FMME", module) == 0)
        m_pLayout = new ogdf::FastMultipoleMultilevelEmbedder ();
    else if (strcmp ("FPP", module) == 0)
        m_pLayout = new ogdf::FPPLayout ();
    else if (strcmp ("GEM", module) == 0)
    {
        ogdf::GEMLayout *pLayout = new ogdf::GEMLayout ();
        pLayout->minDistCC (std::max(m_avgEdgeLen, sqrt(m_avgNodeSize)*distFactor));
        m_pLayout = pLayout;new ogdf::GEMLayout ();
    }
    else if (strcmp ("MixedForce", module) == 0)
        m_pLayout = new ogdf::MixedForceLayout ();
    else if (strcmp ("MixedModel", module) == 0)
        m_pLayout = new ogdf::MixedModelLayout();
    else if (strcmp ("Nice", module) == 0)
        m_pLayout = new ogdf::MMMExampleNiceLayout ();
    else if (strcmp ("Fast", module) == 0)
	m_pLayout = new ogdf::MMMExampleFastLayout();
    else if (strcmp ("NoTwist", module) == 0)
	m_pLayout = new ogdf::MMMExampleNoTwistLayout();
    else if (strcmp ("Planarization", module) == 0)
        m_pLayout = new ogdf::PlanarizationLayout ();
    else if (strcmp ("PlanarDraw", module) == 0)
	m_pLayout = new ogdf::PlanarDrawLayout();
    else if (strcmp ("PlanarStraight", module) == 0)
	m_pLayout = new ogdf::PlanarStraightLayout();
    else if (strcmp ("PlanarizationGrid", module) == 0)
	m_pLayout = new ogdf::PlanarizationGridLayout();
    else if (strcmp ("RadialTree", module) == 0)
        m_pLayout = new ogdf::RadialTreeLayout ();
    else if (strcmp ("Schnyder", module) == 0)
        m_pLayout = new ogdf::SchnyderLayout ();
    else if (strcmp ("SpringEmbedderFR", module) == 0)
    {
        ogdf::SpringEmbedderFR *pLayout = new ogdf::SpringEmbedderFR ();
        pLayout->minDistCC (std::max(m_avgEdgeLen, sqrt(m_avgNodeSize)*distFactor));
        m_pLayout = pLayout;
    }
    else if (strcmp ("SpringEmbedderKK", module) == 0)
    {
        ogdf::SpringEmbedderKK *pLayout = new ogdf::SpringEmbedderKK ();
	pLayout->setDesLength (std::max(m_avgEdgeLen, sqrt(m_avgNodeSize)*distFactor));
        m_pLayout = pLayout;
    }
    else if (strcmp ("StressMajorization", module) == 0)
    {
        ogdf::StressMajorization *pLayout = new ogdf::StressMajorization ();
	//private: pLayout->desMinLength (std::max(m_avgEdgeLen, sqrt(m_avgNodeSize)*distFactor));
	pLayout->setUseLayout (true);
        m_pLayout = pLayout;
    }
    else if (strcmp ("Sugiyama", module) == 0)
    {
        ogdf::SugiyamaLayout *pLayout = new ogdf::SugiyamaLayout ();
        pLayout->setRanking (new ogdf::OptimalRanking ());
        m_pLayout = new ogdf::SugiyamaLayout ();
    }
    else if (strcmp ("TreeStraight", module) == 0)
        m_pLayout = new ogdf::TreeLayout ();
    else if (strcmp ("TreeOrthogonal", module) == 0)
    {
        ogdf::TreeLayout *pLayout = new ogdf::TreeLayout ();
        pLayout->orthogonalLayout(true);
        m_pLayout = pLayout;
    }
    else if (strcmp ("UpwardPlanarization", module) == 0)
        m_pLayout = new ogdf::UpwardPlanarizationLayout ();
    else if (strcmp ("Visibility", module) == 0)
        m_pLayout = new ogdf::VisibilityLayout ();
    else
        m_pLayout = NULL;

    if (!m_pLayout)
        return false;

    m_pLayout->call (*m_pGraphAttrs);
    m_pGraphAttrs->removeUnnecessaryBendsHV ();
#ifdef OGDF_DEBUG
    //FIXME: debugging
    m_pGraphAttrs->writeGML ("d:\\temp\\ogdf-simple-post.gml");
#endif
    return true;
}

//! API function which should filter all exceptions
IGraph::eResult
KGraph::Layout (const char *module)
{
    try
    {
	if (RealLayout (module))
	    return SUCCESS;
	else
	    return NO_MODULE;
    }
    catch (ogdf::InsufficientMemoryException &e)
    {
	return OUT_OF_MEMORY;
    }
    catch (ogdf::PreconditionViolatedException &e)
    {
        fprintf (stderr, "odgf:Precontion %d failed", e.exceptionCode());
	switch (e.exceptionCode())
	{
	case ogdf::pvcTree : return NO_TREE;
	case ogdf::pvcForest : return NO_FOREST;
	case ogdf::pvcConnected :
	case ogdf::pvcOrthogonal :
	case ogdf::pvcClusterPlanar :
	case ogdf::pvcSelfLoop :
	case ogdf::pvcPlanar :
	default : return  FAILED_PRECONDITION;
	}
    }
    catch (ogdf::AlgorithmFailureException &e)
    {
        fprintf (stderr, "odgf:Algorithm %d failed", e.exceptionCode());
        return FAILED_ALGORITHM;
    }
    catch (ogdf::Exception& e)
    {
	if (e.file())
	    fprintf (stderr, "odgf:Exception: %s:%d\n", e.file(), e.line());
	return FAILED;
    }
    catch (std::bad_alloc &e)
    {
	fprintf (stderr, "Out of memory: %s\n", e.what());
	return OUT_OF_MEMORY;
    }
    catch (std::exception &e)
    {
	fprintf (stderr, "Failed: %s\n", e.what());
	return FAILED;
    }
    catch (...)
    {
	return CRASHED;
    }
}

bool
KGraph::GetNodePosition (int n, double* x, double* y)
{
    if (!m_pLayout)
        return false;
    
    if (n < 0 || n >= m_nodes.size())
        return false;

    ogdf::node node = m_nodes[n];
    
    if (x)
        *x = (m_pGraphAttrs->x(node) - m_pGraphAttrs->width(node)/2) / m_scale;
    if (y)
        *y = (m_pGraphAttrs->y(node) - m_pGraphAttrs->height(node)/2) / m_scale;

    return true;
}

int
KGraph::GetEdgeBends (int e, double *coords, int len)
{
    if (!m_pLayout)
        return 0;
    
    if (e < 0 || e >= m_edges.size())
        return 0;

    ogdf::edge edge = m_edges[e];
    const ogdf::DPolyline& poly = m_pGraphAttrs->bends(edge);

    if (!coords || len <= 0)
        return  poly.size() * 2;

    for (int i = 0; i < poly.size()*2 && (i <len); i+=2)
    {
        ogdf::ListConstIterator<ogdf::DPoint> it = poly.get(i/2);
        coords[i  ] = (*it).m_x / m_scale;
	coords[i+1] = (*it).m_y / m_scale;
    }
    
    return poly.size() * 2;
}

} // namespace

/*!
 * Factory function to construct a graph
 */
DLL_EXPORT IGraph *
CreateGraph ()
{
    IGraph *pg = new KGraph();
    
    return pg;
}
