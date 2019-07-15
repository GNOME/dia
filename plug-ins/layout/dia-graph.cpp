/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dia-graph.cpp -  simple graph algortihms without external help
 *
 * Copyright (c) 2012 Hans Breuer <hans@breuer.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <config.h>

#include "ogdf-simple.h"

#include "dia-graph.h"

#include <stdio.h>
#include <string.h>
#include <vector>

/*!
 * \brief A simple x,y coodinate
 * \ingroup LayoutPlugin
 */
struct Point
{
  double x;
  double y;
  Point (double _x, double _y) : x(_x), y(_y) {}
};

/*!
 * \brief Another node representation
 * \ingroup LayoutPlugin
 */
struct Node
{
  Point center;
  double width, height;
  Node (double x, double y, double w, double h) :
    center(x, y), width(w), height(h) {}
};

typedef std::vector<Node> Nodes;
/*!
 * \brief The Edge object is just stroing bends here
 * \ingroup LayoutPlugin
 */
typedef std::vector<Point> Edge;
typedef std::vector<Edge> Edges;

/*!
 * \brief Implementing the IGraph interface for simple layout algorithms
 *
 * \ingroup LayoutPlugin
 */
class DiaGraph : public IGraph
{
public :
  void Release ();
  int AddNode (double left, double top, double right, double bottom);
  int AddEdge (int srcNode, int destNode, double* points, int len);
  eResult Layout (const char *module);
  bool GetNodePosition (int node, double* x, double* y);
  int GetEdgeBends (int e, double *coords, int len);
  virtual ~DiaGraph();
protected :
  bool Scale (double xfactor, double yfactor);
private :
  Nodes m_nodes;
  Edges m_edges;
};

DiaGraph::~DiaGraph() {

}

IGraph *
dia_graph_create ()
{
  return new DiaGraph ();
}

void
DiaGraph::Release ()
{
  delete this;
}

int
DiaGraph::AddNode (double left, double top, double right, double bottom)
{
  m_nodes.push_back (Node ((left + right) / 2, (top + bottom) / 2, right - left, bottom - top));
  return m_nodes.size() - 1;
}

int
DiaGraph::AddEdge (int srcNode, int destNode, double* points, int len)
{
  int pos;
  m_edges.push_back (Edge());
  pos = m_edges.size() - 1;
  for (int i = 0; i < len; i+=2)
    m_edges[pos].push_back (Point(points[i], points[i+1]));
  return pos;
}

/*!
 * \brief Invoke the given layout algoritm
 *
 * \ingroup LayoutPlugin
 */
IGraph::eResult
DiaGraph::Layout (const char *module)
{
  // double p1, p2;
  // int n;

  if (strcmp(module, "Grow") == 0)
    return Scale (1.4142, 1.4142) ? SUCCESS : FAILED_ALGORITHM;
  else if (strcmp(module, "Shrink") == 0)
    return Scale (0.7071, 0.7071) ? SUCCESS : FAILED_ALGORITHM;
  else if (strcmp(module, "Heighten") == 0)
    return Scale (1.0, 1.4142) ? SUCCESS : FAILED_ALGORITHM;
  else if (strcmp(module, "Widen") == 0)
    return Scale (1.4142, 1.0) ? SUCCESS : FAILED_ALGORITHM;

  return NO_MODULE;
}

bool
DiaGraph::GetNodePosition (int node, double* x, double* y)
{
  if (node >= 0 && ((size_t) node) < m_nodes.size()) {
    Node &n = m_nodes[node];
    if (x)
      *x = n.center.x - n.width / 2;
    if (y)
      *y = n.center.y - n.height / 2;
    return true;
  }
  return false;
}

int
DiaGraph::GetEdgeBends (int e, double *coords, int len)
{
  if (((size_t) e) >= m_edges.size() || e < 0)
    return 0;
  Edge &edge = m_edges[e];
  if (coords && len > 0) {
    for (size_t i = 0, j = 0; i < ((size_t) len) && j < edge.size(); i+=2, ++j) {
      coords[i  ] = edge[j].x;
      coords[i+1] = edge[j].y;
    }
  }
  return edge.size();
}

/*!
 * \brief Just resizing the graph according to the given scale
 *
 * First the center of gravity is calculated, than nodes and
 * edge bends are moved according to the given scale.
 */
bool
DiaGraph::Scale (double xfactor, double yfactor)
{
  Point cog(0,0);
  double weight(0);
  Nodes::iterator itn;
  Edges::iterator ite;

  for (itn = m_nodes.begin(); itn != m_nodes.end(); ++itn) {
    double w = (*itn).width * (*itn).height;

    cog.x += (w * (*itn).center.x);
    cog.y += (w * (*itn).center.y);
    weight += w;
  }
  cog.x /= weight;
  cog.y /= weight;

  for (itn = m_nodes.begin(); itn != m_nodes.end(); ++itn) {
    (*itn).center.x = cog.x + ((*itn).center.x - cog.x) * xfactor;
    (*itn).center.y = cog.y + ((*itn).center.y - cog.y) * yfactor;
  }
  for (ite = m_edges.begin(); ite != m_edges.end(); ++ite) {
    Edge &e = (*ite);
    Edge::iterator it;
    for (it = e.begin(); it != e.end(); ++it) {
      (*it).x = cog.x + ((*it).x - cog.x) * xfactor;
      (*it).y = cog.y + ((*it).y - cog.y) * yfactor;
    }
  }

  return true;
}
