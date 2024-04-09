#pragma once

#include<vcg/complex/complex.h>
#include <wrap/io_trimesh/import.h>
#include <wrap/io_trimesh/export.h>
#include <vcg/complex/algorithms/local_optimization.h>
#include <vcg/complex/algorithms/local_optimization/tri_edge_collapse_quadric.h>

class MyVertex; class MyEdge; class MyFace;
struct MyUsedTypes : public vcg::UsedTypes<vcg::Use<MyVertex>   ::AsVertexType,
	vcg::Use<MyEdge>     ::AsEdgeType,
	vcg::Use<MyFace>     ::AsFaceType> {};
class MyVertex : public vcg::Vertex < MyUsedTypes, vcg::vertex::Coord3d, vcg::vertex::VFAdj, vcg::vertex::Mark, vcg::vertex::Normal3d, vcg::vertex::TexCoord2d, vcg::vertex::Color4b, vcg::vertex::BitFlags > {};
class MyFace : public vcg::Face<   MyUsedTypes, vcg::face::FFAdj, vcg::face::VFAdj, vcg::face::Mark, vcg::face::VertexRef, vcg::face::BitFlags > {};
class MyEdge : public vcg::Edge<   MyUsedTypes> {};
class MyMesh : public vcg::tri::TriMesh< std::vector<MyVertex>, std::vector<MyFace>, std::vector<MyEdge>  > {};

typedef	vcg::SimpleTempData<MyMesh::VertContainer, vcg::math::Quadric<double> > QuadricTemp;



class QHelper
{
public:
	QHelper() {}
	static void Init() {}
	static vcg::math::Quadric<double> &Qd(MyVertex &v) { return TD()[v]; }
	static vcg::math::Quadric<double> &Qd(MyVertex *v) { return TD()[*v]; }
	static MyVertex::ScalarType W(MyVertex *) { return 1.0; }
	static MyVertex::ScalarType W(MyVertex &) { return 1.0; }
	static void Merge(MyVertex &, MyVertex const &) {}
	static QuadricTemp* &TDp() { static QuadricTemp *td; return td; }
	static QuadricTemp &TD() { return *TDp(); }
};

typedef vcg::tri::BasicVertexPair<MyVertex> VertexPair;

class MyTriEdgeCollapse : public vcg::tri::TriEdgeCollapseQuadric< MyMesh, VertexPair, MyTriEdgeCollapse, QHelper > {
public:
	typedef  vcg::tri::TriEdgeCollapseQuadric< MyMesh, VertexPair, MyTriEdgeCollapse, QHelper> TECQ;
	inline MyTriEdgeCollapse(const VertexPair &p, int i, vcg::BaseParameterClass *pp) :TECQ(p, i, pp) {}
};

