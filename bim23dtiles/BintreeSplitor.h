#pragma once

#include <osg/Vec3d>
#include <list>
using namespace std;

namespace XBSJ {

	//标量分割
	struct SplitScalar{
		void * ref = nullptr;
		double value = 0;
	};

	//空间点分割
	struct SplitVector {
		void* ref = nullptr;
		double value = 0;
		osg::Vec3d pos = osg::Vec3d(0,0,0);
	};
	
	class BintreeSplitor
	{
	public:
		bool split(list<SplitScalar> &input, list<SplitScalar> &c0, list<SplitScalar> &c1);
		bool split(list<SplitVector> &input, list<SplitVector> &c0, list<SplitVector> &c1);

	private:
		bool split(list<SplitVector> &input, int axis, list<SplitVector> &c0, list<SplitVector> &c1,double & bias);
	 
	};
}
