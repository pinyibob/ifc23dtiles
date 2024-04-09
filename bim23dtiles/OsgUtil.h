#pragma once

#include "stdafx.h"

#include <osg/BoundingBox>
#include <osg/Matrix>
namespace XBSJ {

	class XbsjOsgUtil
	{
	public:
		XbsjOsgUtil();
		~XbsjOsgUtil();


		//static osg::Matrix eastNorthUpMatrix(double lon, double lat, double height);
		static osg::Matrix eastNorthUpMatrix(double lon, double lat, double height,double angle);
		static osg::Matrix eastNorthUpMatrix(osg::Vec3d &center);

		static osg::Matrix getRotationMatrix(double angle);



		static osg::BoundingBox boxFromJson(json & conf);
		static osg::Matrix      matFromJson(json & conf);
		static json toJson(osg::BoundingBoxd &boundingbox);
		static json toJson(osg::BoundingBox &boundingbox);
		static json toJson(osg::Matrix &matrix);


		static json tianAnMen();

		static bool parse(string & str, osg::Vec3d & v);

		static osg::BoundingBoxd  tranformBox(osg::Matrixd &matrix, osg::BoundingBoxd & bbox);

	};


}