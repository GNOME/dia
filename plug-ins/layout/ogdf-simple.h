/*!
 * A very simple interface to use graph layout algorithms with Dia
 *
 * For OGDF ( http://www.ogdf.net ) main problem to solve is the 
 * compiler mismatch on Windows.
 * Dia (and GTK+) are still bound to use vc6, but that's not enough 
 * C++ to compile OGDF.
 *
 * But this is so simple that it should be possible to wrap other graph
 * layout libraries with the same interface, too.
 *
 * Author:
 *   Hans Breuer <hans@breuer.org>
 *
 * This code is in public domain
 */
#ifndef OGDF_SIMPLE_H
#define OGDF_SIMPLE_H

#ifndef DLL_EXPORT 
#define DLL_EXPORT 
#endif

/*!
 * \brief Abstract graph interface to be fed by Dia
 *
 * This interface must not expose any allocation/deallocations and it shall
 * not let pass any exceptions, because the consumer probaly can't catch them.
 */
class IGraph
{
public :
    //! Clean up and destroy the graph
    virtual void Release () = 0;
    //! Create a new node with bounding box and return it's index
    virtual int AddNode (double left, double top, double right, double bottom) = 0;
    //! Create a new edge and return it's index
    virtual int AddEdge (int srcNode, int destNode, double* points, int len) = 0;

    //! some hints what was going wrong
    typedef enum {
	SUCCESS = 0,
	NO_MODULE,
	OUT_OF_MEMORY,
	NO_TREE,
	NO_FOREST,
	FAILED_PRECONDITION,
	FAILED_ALGORITHM,
	FAILED,
	CRASHED // catch (...)
    } eResult;
    //! layout the graph
    virtual eResult Layout (const char *module) = 0;

    // Layout result left,top
    virtual bool GetNodePosition (int node, double* x, double* y) = 0;
    // Layout result bends - fill sized array of coords
    virtual int GetEdgeBends (int e, double *coords, int len) = 0;
protected :
    //! use factory function
    IGraph () {}
    //! use Release() instead
    ~IGraph () {}
private :
    //! no copy construction
    IGraph (const IGraph&);
};

DLL_EXPORT IGraph *CreateGraph ();

#endif
