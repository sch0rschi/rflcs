#ifndef GRAPHIZ_UTILS_HPP
#define GRAPHIZ_UTILS_HPP

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>

typedef boost::property<boost::vertex_name_t, std::string> VertexProperty;
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, VertexProperty> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;

#endif
