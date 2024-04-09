#include "OsgUtil.h"
#include "Cesium.h"
using namespace Cesium;

#include "util.h"

namespace XBSJ {
	XbsjOsgUtil::XbsjOsgUtil()
	{
	}


	XbsjOsgUtil::~XbsjOsgUtil()
	{
	}

	osg::Matrix   XbsjOsgUtil::matFromJson(json & conf) {

		osg::Matrix mat = osg::Matrix::identity();

		if (!conf.is_array()) {
			return move(mat);
		}
		mat(0, 0) = conf[0].get<double>();
		mat(0, 1) = conf[1].get<double>();
		mat(0, 2) = conf[2].get<double>();
		mat(0, 3) = conf[3].get<double>();
		mat(1, 0) = conf[4].get<double>();
		mat(1, 1) = conf[5].get<double>();
		mat(1, 2) = conf[6].get<double>();
		mat(1, 3) = conf[7].get<double>();
		mat(2, 0) = conf[8].get<double>();
		mat(2, 1) = conf[9].get<double>();
		mat(2, 2) = conf[10].get<double>();
		mat(2, 3) = conf[11].get<double>();
		mat(3, 0) = conf[12].get<double>();
		mat(3, 1) = conf[13].get<double>();
		mat(3, 2) = conf[14].get<double>();
		mat(3, 3) = conf[15].get<double>();


		return move(mat);
	}

	osg::BoundingBox XbsjOsgUtil::boxFromJson(json & conf) {

		osg::BoundingBox box;
		double wx = conf[3].get < double>();
		double wy = conf[7].get < double>();
		double wz = conf[11].get < double>();

		double cx = conf[0].get < double>();
		double cy = conf[1].get < double>();
		double cz = conf[2].get < double>();

		box.xMin() = cx - wx; box.xMax() = cx + wx;
		box.yMin() = cy - wy; box.yMax() = cy + wy;
		box.zMin() = cz - wz; box.zMax() = cz + wz;

		return move(box);
	}

	json XbsjOsgUtil::toJson(osg::BoundingBoxd &boundingbox) {

		json ret;
		double wx = boundingbox.xMax() - boundingbox.xMin();
		double wy = boundingbox.yMax() - boundingbox.yMin();
		double wz = boundingbox.zMax() - boundingbox.zMin();
		auto center = boundingbox.center();
		ret = {
			//ǰ����Ϊ���ĵ�
			center.x(),
			center.y(),
			center.z(),
			//����Ϊx����
			wx * 0.5,0,0,
			//����Ϊy����
			0,wy * 0.5,0,
			//����Ϊz����
			0,0,wz * 0.5
		};

		return move(ret);
	}
	json XbsjOsgUtil::toJson(osg::BoundingBox &boundingbox) {

		json ret;
		double wx = boundingbox.xMax() - boundingbox.xMin();
		double wy = boundingbox.yMax() - boundingbox.yMin();
		double wz = boundingbox.zMax() - boundingbox.zMin();
		auto center = boundingbox.center();
		ret = {
			//ǰ����Ϊ���ĵ�
			center.x(),
			center.y(),
			center.z(),
			//����Ϊx����
			wx * 0.5,0,0,
			//����Ϊy����
			0,wy * 0.5,0,
			//����Ϊz����
			0,0,wz * 0.5
		};

		return move(ret);
	}

	json XbsjOsgUtil::toJson(osg::Matrix &matrix) {

		std::vector<double> trans;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				trans.push_back(matrix(i,j));
			}
		}

		json ret = trans;

		return move(ret);

	}

	//osg::Matrix XbsjOsgUtil::eastNorthUpMatrix(double lon, double lat, double height) {


	//	auto pos = Cartesian3::fromDegree(lon, lat, height);

	//	//return eastNorthUpMatrix(osg::Vec3d(pos.x,pos.y,pos.z));
	//}

	osg::Matrix XbsjOsgUtil::eastNorthUpMatrix(double lon, double lat, double height ,double angle) {


		auto pos = Cartesian3::fromDegree(lon,lat,height);
		auto iv = osg::Vec3d(pos.x, pos.y, pos.z);
		osg::Matrixd eastNorthUp = eastNorthUpMatrix(iv);
		osg::Matrixd rotation = getRotationMatrix(-angle);
		// postMult  preMult
		eastNorthUp.preMult(rotation);
		return eastNorthUp;
 
		//return eastNorthUpMatrix(osg::Vec3d(pos.x,pos.y,pos.z));
	}

	osg::Matrix XbsjOsgUtil::eastNorthUpMatrix(osg::Vec3d &center) {


		auto pos = Cartesian3(center.x(),center.y(), center.z());
		auto up = Cartesian3::normalize(pos);
		Cartesian3 east;
		east.x = -pos.y;
		east.y = pos.x;
		east.z = 0;
		east = Cartesian3::normalize(east);
		auto north = Cartesian3::cross(up, east);
		auto mat = osg::Matrix::identity();

		mat(0, 0) = east.x;
		mat(0, 1) = east.y;
		mat(0, 2) = east.z;
		mat(0, 3) = 0.0;
		mat(1, 0) = north.x;
		mat(1, 1) = north.y;
		mat(1, 2) = north.z;
		mat(1, 3) = 0.0;
		mat(2, 0) = up.x;
		mat(2, 1) = up.y;
		mat(2, 2) = up.z;
		mat(2, 3) = 0.0;
		mat(3, 0) = pos.x;
		mat(3, 1) = pos.y;
		mat(3, 2) = pos.z;
		mat(3, 3) = 1.0;

		return move(mat);
	}

	osg::Matrix XbsjOsgUtil::getRotationMatrix(double angle) {

		auto mat = osg::Matrix::identity();

		auto cosAngle=cos(angle);
		auto sinAngle=sin(angle);

		mat(0, 0) = cosAngle;
		mat(0, 1) = sinAngle;
		mat(0, 2) = 0.0;
		mat(0, 3) = 0.0;
		mat(1, 0) = -sinAngle;
		mat(1, 1) = cosAngle;
		mat(1, 2) = 0.0;
		mat(1, 3) = 0.0;
		mat(2, 0) = 0.0;
		mat(2, 1) = 0.0;
		mat(2, 2) = 1.0;
		mat(2, 3) = 0.0;
		mat(3, 0) = 0.0;
		mat(3, 1) = 0.0;
		mat(3, 2) = 0.0;
		mat(3, 3) = 1.0;

		/*auto pos = Cartesian3(center.x(), center.y(), center.z());
		auto up = Cartesian3::normalize(pos);
		Cartesian3 east;
		east.x = -pos.y;
		east.y = pos.x;
		east.z = 0;
		east = Cartesian3::normalize(east);
		auto north = Cartesian3::cross(up, east);
		auto mat = osg::Matrix::identity();

		mat(0, 0) = east.x;
		mat(0, 1) = east.y;
		mat(0, 2) = east.z;
		mat(0, 3) = 0.0;
		mat(1, 0) = north.x;
		mat(1, 1) = north.y;
		mat(1, 2) = north.z;
		mat(1, 3) = 0.0;
		mat(2, 0) = up.x;
		mat(2, 1) = up.y;
		mat(2, 2) = up.z;
		mat(2, 3) = 0.0;
		mat(3, 0) = pos.x;
		mat(3, 1) = pos.y;
		mat(3, 2) = pos.z;
		mat(3, 3) = 1.0;*/

		return move(mat);
	}

	json XbsjOsgUtil::tianAnMen() {
 
		auto mat = eastNorthUpMatrix(116.39123, 39.90691, 0,0);

		return toJson(mat);
	}


	bool XbsjOsgUtil::parse(string & str, osg::Vec3d & v) {

		vector<string> tt;
		split(str, tt, ",");
		if (tt.size() != 3)
			return false;

		double x = atoi(tt[0].c_str());
		double y = atoi(tt[1].c_str());
		double z = atoi(tt[2].c_str());
		if (isnan(x) || isnan(y) || isnan(z))
			return false;
		v.x() = x;
		v.y() = y;
		v.z() = z;
		return true;
	}

	osg::BoundingBoxd  XbsjOsgUtil::tranformBox(osg::Matrixd &matrix, osg::BoundingBoxd & bbox) {
		osg::BoundingBoxd box;

		box.expandBy(bbox.corner(0) * matrix);
		box.expandBy(bbox.corner(1) * matrix);
		box.expandBy(bbox.corner(2) * matrix);
		box.expandBy(bbox.corner(3) * matrix);
		box.expandBy(bbox.corner(4) * matrix);
		box.expandBy(bbox.corner(5) * matrix);
		box.expandBy(bbox.corner(6) * matrix);
		box.expandBy(bbox.corner(7) * matrix);

		return move(box);
	}
}