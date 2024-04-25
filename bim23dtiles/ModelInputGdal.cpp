/*
	part of modelinput.h aming at deal with codinate convert with gdal lib
*/

#include "ModelInput.h"

#include "util.h"
#include <gdal.h>
#include <ogr_api.h>
#include <ogr_srs_api.h>
#include <ogrsf_frmts.h>
#include "OsgUtil.h"
#include "Cesium.h"
using namespace Cesium;

namespace XBSJ {

	bool importFromStr(OGRSpatialReference & src, char * str) {


		string srs = str;
		string lowwer = srs;
		::transform(srs.begin(), srs.end(), lowwer.begin(), ::tolower);
		if (lowwer.find("epsg") == 0)
		{
			auto ret = OSRSetFromUserInput(&src, str);
			if (ret) {
				LOG(ERROR) << "OSRSetFromUserInput failed��" << srs;
				return false;
			}

			return true;
		}
		else
		{
			auto ret = OSRImportFromWkt(&src, &str);
			if (ret) {
				LOG(ERROR) << "OSRImportFromWkt failed��" << srs;
				return false;
			}
			//��仰��ô�ؼ� ��Ϊ��Щwkt ����gdal���ϣ�����ת�ɼ�����ʽ��
			ret = OSRMorphFromESRI(&src);
			if (ret) {
				LOG(ERROR) << "OSRMorphFromESRI failed��" << srs;
				return false;
			}

			return true;
		}

	}

	bool ModelInput::initGdal() {

		GDALAllRegister();
		OGRRegisterAll();
		return true;
	}
	bool ModelInput::destroyGdal() {

		OGRCleanupAll();
		GDALDestroy();

		return true;
	}

	bool parse(string & str, osg::Vec3d & v) {

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

	// get pos convert matrix with lib gdal
	bool ModelInput::configSrs(json& cinput) {
		// get model positon eg: ENU:22.599869,113.986992
		// ENU means east north up cordinate, usually refers to WGS84
		auto csrs = cinput.get<json>()["srs"];
		if (csrs.is_string()) {
			srs = csrs.get<string>();
		}
		auto corigin = cinput.get<json>()["srsorigin"];
		if (corigin.is_string()) {
			auto str = corigin.get<string>();
			if (!parse(str, srsorigin)) {
				LOG(WARNING) << "origin miss";
			}
		}

		//parse enu or t4326 pos
		if (srs.find("ENU:") == 0) {
			//parse data
			auto idx = srs.find(",");
			if (idx == string::npos) {
				LOG(ERROR) << "position parse error";
				return false;
			}
			auto lat = atof(srs.substr(4, idx - 4).c_str());
			auto lon = atof(srs.substr(idx + 1).c_str());
			if (isnan(lat) || isnan(lon)) {
				LOG(ERROR) << "position parse error2";
				return false;
			}
			 
			enuMatrix = XbsjOsgUtil::eastNorthUpMatrix(lon, lat, 0,0);
			enutrans = true;
		}
		else 
		{
			OGRSpatialReference refs;
			if (!importFromStr(refs, const_cast<char*>(srs.c_str()))) {

				LOG(ERROR) << "in_srs�������ô���";
				return false;
			}
			OGRSpatialReference t4326;
			t4326.importFromEPSG(4326);
			auto trans = OGRCreateCoordinateTransformation((OGRSpatialReference*)&refs, &t4326);
			if (!trans) {
				LOG(ERROR) << "in_srs��������ת����wgs84���޷�����";
				return false;
			}
			transform4326 = trans;
			
		}

		return true;
	}

	bool ModelInput::setSrs(string crs ,string coords,string angles) {

		OGRSpatialReference refs;
		if (!importFromStr(refs, const_cast<char*>(crs.c_str()))) {

			LOG(ERROR) << "in_srs�������ô���";
			return false;
		}
		OGRSpatialReference t4326;
		t4326.importFromEPSG(4326);
		auto trans = OGRCreateCoordinateTransformation((OGRSpatialReference*)&refs, &t4326);
		if (!trans) {
			LOG(ERROR) << "in_srs��������ת����wgs84���޷�����";
			return false;
		}
		

		//����������γ��
		auto idx = coords.find(",");
		if (idx == string::npos) {
			LOG(ERROR) << "�����������";
			return false;
		}


		//double *x,*y;
		double x=atof(coords.substr(0, idx).c_str());
		double y = atof(coords.substr(idx + 1).c_str());
		if (isnan(x) || isnan(y)) {
			LOG(ERROR) << "�����������";
			return false;
		}

		//������ת�Ƕ�
		auto ida = angles.find(",");
		if (ida == string::npos) {
			LOG(ERROR) << "�ǶȲ�������";
			return false;
		}


		//double *x,*y;
		double xa = atof(angles.substr(0, ida).c_str());
		double ya = atof(angles.substr(ida + 1).c_str());
		/*if (isnan(ida) || isnan(ida)) {
			LOG(ERROR) << "�ǶȲ�������";
			return false;
		}*/

		double angle = atan2(ya, xa);

		trans->Transform(1, &x, &y, 0);

		enuMatrix = XbsjOsgUtil::eastNorthUpMatrix(x, y, 0, angle);

		enutrans = true;

		return true;
	}
	bool   ModelInput::setSrs(string crs, vector<double> mapCoords) {


		OGRSpatialReference refs;
		if (!importFromStr(refs, const_cast<char*>(crs.c_str()))) {

			LOG(ERROR) << "in_srs�������ô���";
			return false;
		}
		OGRSpatialReference t4326;
		t4326.importFromEPSG(4326);
		auto trans = OGRCreateCoordinateTransformation((OGRSpatialReference*)&refs, &t4326);
		if (!trans) {
			LOG(ERROR) << "in_srs��������ת����wgs84���޷�����";
			return false;
		}


		//����������γ��
		
		//double *x,*y;
		double x = mapCoords[0];
		double y = mapCoords[1];
		double z = mapCoords[2];
		if (isnan(x) || isnan(y)) {
			LOG(ERROR) << "�����������";
			return false;
		}

		//������ת�Ƕ�
		
		//double *x,*y;
		double xa = mapCoords[3];
		double ya = mapCoords[4];
		
		double angle = atan2(ya, xa);
#ifndef WIN32
		trans->Transform(1, &y, &x, 0);
#else
		trans->Transform(1, &x, &y, 0);
#endif

		enuMatrix = XbsjOsgUtil::eastNorthUpMatrix(x, y, z, angle);

		enutrans = true;

		return true;
	}
	osg::Vec3d  ModelInput::toGloble(osg::Vec3d v) {
		// ��Ϊassimp Ĭ����y���ϣ����������ȶ�v�� y �� z ����
		auto t = v.y();
		v.y() = -v.z();
		v.z() = t;

		//1, ����srsorigin
		v = v + srsorigin;
		//2, ���enu ֱ�ӳ��Ծ��󼴿�
		if (enutrans) {
			v = v * enuMatrix;
			return move(v);
		}

		//3, �����ȱ任��4326
		auto trans = (OGRCoordinateTransformation *)transform4326;
		//�޸�Ϊ��zֵ���任
		trans->Transform(1, &v.x(), &v.y(), 0);
		//3, 4326 �任�� ȫ����������
		auto d = Cartesian3::fromDegree(v.x(), v.y(), v.z());

		return move(osg::Vec3d(d.x,d.y,d.z));
	}

}