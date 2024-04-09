#pragma once

#include "stdafx.h"
 

namespace Cesium {

	struct Cartesian3 {
		double x;
		double y;
		double z;

		static double RX;
		static double RY;
		static double RZ;

		static Cartesian3 wgs84Radii;
		static Cartesian3 wgs84RadiiSquared;
		static Cartesian3 wgs84OneOverRadii;
		static Cartesian3 wgs84OneOverRadiiSquared;
		static double wgs84CenterToleranceSquared;

		Cartesian3();
		Cartesian3(double _x, double _y, double _z);
		static Cartesian3 fromDegree(double lon, double lat, double height = 0);


		static Cartesian3 fromRadians(double longitude, double latitude, double height = 0);

		static Cartesian3 add(Cartesian3 left, Cartesian3 right);


		static double  magnitudeSquared(Cartesian3 cartesian);
 
		static double  magnitude(Cartesian3 cartesian);

		static Cartesian3 normalize(Cartesian3 cartesian);

		static Cartesian3 multiplyComponents(Cartesian3 left, Cartesian3  right);
		static double  dot(Cartesian3 left, Cartesian3  right);
		static Cartesian3 divideByScalar(Cartesian3 cartesian, double scalar);

		static Cartesian3 multiplyByScalar(Cartesian3 cartesian, double  scalar);


		static Cartesian3 scaleToGeodeticSurface(Cartesian3 cartesian, Cartesian3 oneOverRadii, Cartesian3 oneOverRadiiSquared, double centerToleranceSquared);
		void toRadians(double & longitude, double &latitude, double &height);
		void toDegrees(double & longitude, double &latitude, double &height);

		static Cartesian3 subtract(Cartesian3 left, Cartesian3 right);

		static Cartesian3 cross(Cartesian3 left, Cartesian3 right);


		static Cartesian3 computeHorizonCullingPointFromVertices(Cartesian3 directionToPoint, double* vertices, int elementSize, int stride, Cartesian3 center);

		static Cartesian3 magnitudeToPoint(Cartesian3 scaledSpaceDirectionToPoint, double resultMagnitude);

		static Cartesian3  computeScaledSpaceDirectionToPoint(Cartesian3 directionToPoint);

		static Cartesian3  transformPositionToScaledSpace(Cartesian3 directionToPoint);

		static Cartesian3  transformScaledSpaceToPosition(Cartesian3 directionToPoint);

		static double computeMagnitude(Cartesian3 position, Cartesian3 scaledSpaceDirectionToPoint);
 
	};


	struct BoundingSphere {
		Cartesian3 center;
		double radius = 0;
		static BoundingSphere fromVertices(double * positions, int elementSize, int stride = 3);

	};

}