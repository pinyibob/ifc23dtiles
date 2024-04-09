
#include "Cesium.h"


namespace Cesium {
	 double Cartesian3::RX = 6378137.0;
	 double Cartesian3::RY = 6378137.0;
	 double Cartesian3::RZ = 6356752.3142451793;

	const  double  EPSILON1 = 0.1;
	const  double  EPSILON2 = 0.01;
	const  double  EPSILON12 = 0.000000000001;
	Cartesian3 Cartesian3::wgs84Radii = Cartesian3(RX, RY, RZ);
	Cartesian3 Cartesian3::wgs84RadiiSquared = Cartesian3(RX * RX, RY * RY, RZ*RZ);
	Cartesian3 Cartesian3::wgs84OneOverRadii = Cartesian3(1 / RX, 1 / RY, 1 / RZ);
	Cartesian3 Cartesian3::wgs84OneOverRadiiSquared = Cartesian3(1 / (RX *RX), 1 / (RY*RY), 1 / (RZ*RZ));
	double Cartesian3::wgs84CenterToleranceSquared = EPSILON1;


	double  sign(double value) {
		if (value > 0) {
			return 1;
		}
		if (value < 0) {
			return -1;
		}
		return 0;
	};



	Cartesian3::Cartesian3() {
		x = y = z = 0;
	}
	Cartesian3::Cartesian3(double _x, double _y, double _z) {
		x = _x;
		y = _y;
		z = _z;
	}
	Cartesian3 Cartesian3::fromDegree(double lon, double lat, double height) {

		return fromRadians(lon * M_PI / 180, lat * M_PI / 180, height);
	}

	Cartesian3 Cartesian3::fromRadians(double longitude, double latitude, double height) {

		Cartesian3 radiiSquared = wgs84RadiiSquared;

		double cosLatitude = cos(latitude);

		Cartesian3 scratchN;
		Cartesian3 scratchK;


		scratchN.x = cosLatitude * cos(longitude);
		scratchN.y = cosLatitude * sin(longitude);
		scratchN.z = sin(latitude);
		scratchN = normalize(scratchN);

		scratchK = multiplyComponents(radiiSquared, scratchN);
		double gamma = sqrt(dot(scratchN, scratchK));
		scratchK = divideByScalar(scratchK, gamma);
		scratchN = multiplyByScalar(scratchN, height);

		return move(add(scratchK, scratchN));

	}

	Cartesian3 Cartesian3::add(Cartesian3 left, Cartesian3 right) {
		Cartesian3 result;

		result.x = left.x + right.x;
		result.y = left.y + right.y;
		result.z = left.z + right.z;
		return move(result);
	};


	double  Cartesian3::magnitudeSquared(Cartesian3 cartesian) {


		return cartesian.x * cartesian.x + cartesian.y * cartesian.y + cartesian.z * cartesian.z;
	};

	/**
	* Computes the Cartesian's magnitude (length).
	*
	* @param {Cartesian3} cartesian The Cartesian instance whose magnitude is to be computed.
	* @returns {Number} The magnitude.
	*/
	double  Cartesian3::magnitude(Cartesian3 cartesian) {
		return  sqrt(magnitudeSquared(cartesian));
	};

	Cartesian3 Cartesian3::normalize(Cartesian3 cartesian) {


		double  magnitude = Cartesian3::magnitude(cartesian);

		Cartesian3 result;

		result.x = cartesian.x / magnitude;
		result.y = cartesian.y / magnitude;
		result.z = cartesian.z / magnitude;


		return move(result);
	};

	Cartesian3 Cartesian3::multiplyComponents(Cartesian3 left, Cartesian3  right) {

		//>>includeEnd('debug');
		Cartesian3 result;
		result.x = left.x * right.x;
		result.y = left.y * right.y;
		result.z = left.z * right.z;
		return move(result);
	};
	double  Cartesian3::dot(Cartesian3 left, Cartesian3  right) {

		return left.x * right.x + left.y * right.y + left.z * right.z;
	};

	Cartesian3 Cartesian3::divideByScalar(Cartesian3 cartesian, double scalar) {

		Cartesian3 result;
		result.x = cartesian.x / scalar;
		result.y = cartesian.y / scalar;
		result.z = cartesian.z / scalar;
		return move(result);
	};

	Cartesian3 Cartesian3::multiplyByScalar(Cartesian3 cartesian, double  scalar) {

		Cartesian3 result;
		result.x = cartesian.x * scalar;
		result.y = cartesian.y * scalar;
		result.z = cartesian.z * scalar;
		return move(result);
	};


	Cartesian3 Cartesian3::scaleToGeodeticSurface(Cartesian3 cartesian, Cartesian3 oneOverRadii, Cartesian3 oneOverRadiiSquared, double centerToleranceSquared) {

		//>>includeEnd('debug');

		auto positionX = cartesian.x;
		auto positionY = cartesian.y;
		auto positionZ = cartesian.z;

		auto oneOverRadiiX = oneOverRadii.x;
		auto oneOverRadiiY = oneOverRadii.y;
		auto oneOverRadiiZ = oneOverRadii.z;

		auto x2 = positionX * positionX * oneOverRadiiX * oneOverRadiiX;
		auto y2 = positionY * positionY * oneOverRadiiY * oneOverRadiiY;
		auto z2 = positionZ * positionZ * oneOverRadiiZ * oneOverRadiiZ;

		// Compute the squared ellipsoid norm.
		auto squaredNorm = x2 + y2 + z2;
		auto ratio = sqrt(1.0 / squaredNorm);

		// As an initial approximation, assume that the radial intersection is the projection point.
		auto intersection = multiplyByScalar(cartesian, ratio);

		// If the position is near the center, the iteration will not converge.
		if (squaredNorm < centerToleranceSquared) {
			return  intersection;
		}

		auto oneOverRadiiSquaredX = oneOverRadiiSquared.x;
		auto oneOverRadiiSquaredY = oneOverRadiiSquared.y;
		auto oneOverRadiiSquaredZ = oneOverRadiiSquared.z;

		// Use the gradient at the intersection point in place of the true unit normal.
		// The difference in magnitude will be absorbed in the multiplier.
		Cartesian3 gradient;
		gradient.x = intersection.x * oneOverRadiiSquaredX * 2.0;
		gradient.y = intersection.y * oneOverRadiiSquaredY * 2.0;
		gradient.z = intersection.z * oneOverRadiiSquaredZ * 2.0;

		// Compute the initial guess at the normal vector multiplier, lambda.
		auto lambda = (1.0 - ratio) * magnitude(cartesian) / (0.5 *  magnitude(gradient));
		auto correction = 0.0;

		double func;
		double denominator;
		double xMultiplier;
		double yMultiplier;
		double zMultiplier;
		double xMultiplier2;
		double yMultiplier2;
		double zMultiplier2;
		double xMultiplier3;
		double yMultiplier3;
		double zMultiplier3;

		do {
			lambda -= correction;

			xMultiplier = 1.0 / (1.0 + lambda * oneOverRadiiSquaredX);
			yMultiplier = 1.0 / (1.0 + lambda * oneOverRadiiSquaredY);
			zMultiplier = 1.0 / (1.0 + lambda * oneOverRadiiSquaredZ);

			xMultiplier2 = xMultiplier * xMultiplier;
			yMultiplier2 = yMultiplier * yMultiplier;
			zMultiplier2 = zMultiplier * zMultiplier;

			xMultiplier3 = xMultiplier2 * xMultiplier;
			yMultiplier3 = yMultiplier2 * yMultiplier;
			zMultiplier3 = zMultiplier2 * zMultiplier;

			func = x2 * xMultiplier2 + y2 * yMultiplier2 + z2 * zMultiplier2 - 1.0;

			// "denominator" here refers to the use of this expression in the velocity and acceleration
			// computations in the sections to follow.
			denominator = x2 * xMultiplier3 * oneOverRadiiSquaredX + y2 * yMultiplier3 * oneOverRadiiSquaredY + z2 * zMultiplier3 * oneOverRadiiSquaredZ;

			auto derivative = -2.0 * denominator;

			correction = func / derivative;
		} while (abs(func) > EPSILON12);

		Cartesian3 result;
		result.x = positionX * xMultiplier;
		result.y = positionY * yMultiplier;
		result.z = positionZ * zMultiplier;
		return move(result);
	}
	void Cartesian3::toRadians(double & longitude, double &latitude, double &height) {

		Cartesian3 cartesian = *this;
		auto & oneOverRadii = wgs84OneOverRadii;
		auto & oneOverRadiiSquared = wgs84OneOverRadiiSquared;
		auto & centerToleranceSquared = wgs84CenterToleranceSquared;
		auto p = scaleToGeodeticSurface(cartesian, oneOverRadii, oneOverRadiiSquared, centerToleranceSquared);


		auto n = multiplyComponents(p, oneOverRadiiSquared);
		n = normalize(n);

		auto h = subtract(cartesian, p);

		longitude = atan2(n.y, n.x);
		latitude = asin(n.z);
		height = sign(dot(h, cartesian)) * magnitude(h);
		return;
	}
	void Cartesian3::toDegrees(double & longitude, double &latitude, double &height) {

		toRadians(longitude, latitude, height);
		longitude = longitude * 180 / M_PI;
		latitude = latitude * 180 / M_PI;
	}



	Cartesian3 Cartesian3::subtract(Cartesian3 left, Cartesian3 right) {

		Cartesian3 result;

		result.x = left.x - right.x;
		result.y = left.y - right.y;
		result.z = left.z - right.z;
		return move(result);
	};

	Cartesian3 Cartesian3::cross(Cartesian3 left, Cartesian3 right) {

		Cartesian3 result;

		auto leftX = left.x;
		auto leftY = left.y;
		auto leftZ = left.z;
		auto rightX = right.x;
		auto rightY = right.y;
		auto rightZ = right.z;

		auto x = leftY * rightZ - leftZ * rightY;
		auto y = leftZ * rightX - leftX * rightZ;
		auto z = leftX * rightY - leftY * rightX;

		result.x = x;
		result.y = y;
		result.z = z;
		return move(result);
	};


	Cartesian3 Cartesian3::computeHorizonCullingPointFromVertices(Cartesian3 directionToPoint, double* vertices, int elementSize, int stride, Cartesian3 center) {


		auto scaledSpaceDirectionToPoint = computeScaledSpaceDirectionToPoint(directionToPoint);
		auto resultMagnitude = 0.0;

		Cartesian3 positionScratch;

		for (int i = 0; i < elementSize; i += stride) {
			positionScratch.x = vertices[i] + center.x;
			positionScratch.y = vertices[i + 1] + center.y;
			positionScratch.z = vertices[i + 2] + center.z;

			double candidateMagnitude = computeMagnitude(positionScratch, scaledSpaceDirectionToPoint);
			resultMagnitude = fmax(resultMagnitude, candidateMagnitude);
		}

		return magnitudeToPoint(scaledSpaceDirectionToPoint, resultMagnitude);
	};

	Cartesian3 Cartesian3::magnitudeToPoint(Cartesian3 scaledSpaceDirectionToPoint, double resultMagnitude) {
		// The horizon culling point is undefined if there were no positions from which to compute it,
		// the directionToPoint is pointing opposite all of the positions,  or if we computed NaN or infinity.
		if (resultMagnitude <= 0.0 || isnan(resultMagnitude)) {
			return Cartesian3();
		}

		return  multiplyByScalar(scaledSpaceDirectionToPoint, resultMagnitude);
	}

	Cartesian3  Cartesian3::computeScaledSpaceDirectionToPoint(Cartesian3 directionToPoint) {

		Cartesian3 directionToPointScratch = transformPositionToScaledSpace(directionToPoint);
		return   normalize(directionToPointScratch);
	}

	Cartesian3 Cartesian3::transformPositionToScaledSpace(Cartesian3 directionToPoint) {

		return multiplyComponents(directionToPoint, wgs84OneOverRadii);
	}
	Cartesian3 Cartesian3::transformScaledSpaceToPosition(Cartesian3 directionToPoint) {
		return multiplyComponents(directionToPoint, wgs84Radii);
	}

	double Cartesian3::computeMagnitude(Cartesian3 position, Cartesian3 scaledSpaceDirectionToPoint) {
		auto scaledSpacePosition = transformPositionToScaledSpace(position);
		auto magnitudeSquared = Cartesian3::magnitudeSquared(scaledSpacePosition);
		auto magnitude = sqrt(magnitudeSquared);
		auto direction = divideByScalar(scaledSpacePosition, magnitude);

		// For the purpose of this computation, points below the ellipsoid are consider to be on it instead.
		magnitudeSquared = fmax(1.0, magnitudeSquared);
		magnitude = fmax(1.0, magnitude);

		auto cosAlpha = dot(direction, scaledSpaceDirectionToPoint);
		auto sinAlpha = Cartesian3::magnitude(cross(direction, scaledSpaceDirectionToPoint));
		auto cosBeta = 1.0 / magnitude;
		auto sinBeta = sqrt(magnitudeSquared - 1.0) * cosBeta;

		return 1.0 / (cosAlpha * cosBeta - sinAlpha * sinBeta);
	}





	BoundingSphere BoundingSphere::fromVertices(double * positions, int elementSize, int stride) {

		Cartesian3 center;
		Cartesian3 currentPos;
		currentPos.x = positions[0] + center.x;
		currentPos.y = positions[1] + center.y;
		currentPos.z = positions[2] + center.z;


		Cartesian3 xMin = currentPos;
		Cartesian3 yMin = currentPos;
		Cartesian3 zMin = currentPos;

		Cartesian3 xMax = currentPos;
		Cartesian3 yMax = currentPos;
		Cartesian3 zMax = currentPos;


		for (int i = 0; i < elementSize; i += stride) {
			auto x = positions[i] + center.x;
			auto y = positions[i + 1] + center.y;
			auto z = positions[i + 2] + center.z;

			currentPos.x = x;
			currentPos.y = y;
			currentPos.z = z;

			// Store points containing the the smallest and largest components
			if (x < xMin.x) {
				xMin = currentPos;
			}

			if (x > xMax.x) {
				xMax = currentPos;
			}

			if (y < yMin.y) {
				yMin = currentPos;
			}

			if (y > yMax.y) {
				yMax = currentPos;

			}

			if (z < zMin.z) {
				zMin = currentPos;
			}

			if (z > zMax.z) {
				zMax = currentPos;
			}
		}

		// Compute x-, y-, and z-spans (Squared distances b/n each component's min. and max.).
		auto xSpan = Cartesian3::magnitudeSquared(Cartesian3::subtract(xMax, xMin));
		auto ySpan = Cartesian3::magnitudeSquared(Cartesian3::subtract(yMax, yMin));
		auto zSpan = Cartesian3::magnitudeSquared(Cartesian3::subtract(zMax, zMin));

		// Set the diameter endpoints to the largest span.
		auto diameter1 = xMin;
		auto diameter2 = xMax;
		auto maxSpan = xSpan;
		if (ySpan > maxSpan) {
			maxSpan = ySpan;
			diameter1 = yMin;
			diameter2 = yMax;
		}
		if (zSpan > maxSpan) {
			maxSpan = zSpan;
			diameter1 = zMin;
			diameter2 = zMax;
		}

		// Calculate the center of the initial sphere found by Ritter's algorithm
		Cartesian3 ritterCenter;
		ritterCenter.x = (diameter1.x + diameter2.x) * 0.5;
		ritterCenter.y = (diameter1.y + diameter2.y) * 0.5;
		ritterCenter.z = (diameter1.z + diameter2.z) * 0.5;

		// Calculate the radius of the initial sphere found by Ritter's algorithm
		auto radiusSquared = Cartesian3::magnitudeSquared(Cartesian3::subtract(diameter2, ritterCenter));
		auto ritterRadius = sqrt(radiusSquared);

		// Find the center of the sphere found using the Naive method.
		Cartesian3 minBoxPt;
		minBoxPt.x = xMin.x;
		minBoxPt.y = yMin.y;
		minBoxPt.z = zMin.z;

		Cartesian3 maxBoxPt;
		maxBoxPt.x = xMax.x;
		maxBoxPt.y = yMax.y;
		maxBoxPt.z = zMax.z;

		auto naiveCenter = Cartesian3::multiplyByScalar(Cartesian3::add(minBoxPt, maxBoxPt), 0.5);

		// Begin 2nd pass to find naive radius and modify the ritter sphere.
		double naiveRadius = 0;
		for (int i = 0; i < elementSize; i += stride) {
			currentPos.x = positions[i] + center.x;
			currentPos.y = positions[i + 1] + center.y;
			currentPos.z = positions[i + 2] + center.z;

			// Find the furthest point from the naive center to calculate the naive radius.
			auto r = Cartesian3::magnitude(Cartesian3::subtract(currentPos, naiveCenter));
			if (r > naiveRadius) {
				naiveRadius = r;
			}

			// Make adjustments to the Ritter Sphere to include all points.
			auto oldCenterToPointSquared = Cartesian3::magnitudeSquared(Cartesian3::subtract(currentPos, ritterCenter));
			if (oldCenterToPointSquared > radiusSquared) {
				auto oldCenterToPoint = sqrt(oldCenterToPointSquared);
				// Calculate new radius to include the point that lies outside
				ritterRadius = (ritterRadius + oldCenterToPoint) * 0.5;
				radiusSquared = ritterRadius * ritterRadius;
				// Calculate center of new Ritter sphere
				auto oldToNew = oldCenterToPoint - ritterRadius;
				ritterCenter.x = (ritterRadius * ritterCenter.x + oldToNew * currentPos.x) / oldCenterToPoint;
				ritterCenter.y = (ritterRadius * ritterCenter.y + oldToNew * currentPos.y) / oldCenterToPoint;
				ritterCenter.z = (ritterRadius * ritterCenter.z + oldToNew * currentPos.z) / oldCenterToPoint;
			}
		}

		BoundingSphere result;
		if (ritterRadius < naiveRadius) {
			result.center = ritterCenter;
			result.radius = ritterRadius;
		}
		else {
			result.center = naiveCenter;
			result.radius = naiveRadius;
		}

		return result;
	};


}