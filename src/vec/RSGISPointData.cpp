/*
 *  RSGISPointData.cpp
 *  RSGIS_LIB
 *
 *  Created by Pete Bunting on 11/01/2009.
 *  Copyright 2009 RSGISLib. All rights reserved.
 *  This file is part of RSGISLib.
 * 
 *  RSGISLib is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RSGISLib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RSGISLib.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "RSGISPointData.h"

namespace rsgis{namespace vec{
	
	RSGISPointData::RSGISPointData()
	{
		pointGeom = NULL;
	}
	
	void RSGISPointData::readPoint(OGRPoint *point)
	{
		RSGISVectorUtils vecUtils;
		pointGeom = vecUtils.convertOGRPoint2GEOSPoint(point);
	}

	double RSGISPointData::distance(RSGISPolygonData *data)
	{
		return pointGeom->distance(data->getGeometry());
	}
									 
	double RSGISPointData::distance(RSGISPointData *data)
	{
		return pointGeom->distance(data->getPoint());
	}
	
	void RSGISPointData::printGeometry()
	{
		std::cout << pointGeom->toString() << std::endl;
	}
	
	geos::geom::Point* RSGISPointData::getPoint()
	{
		return pointGeom;
	}
	
	void RSGISPointData::setPoint(geos::geom::Point *point)
	{
		this->pointGeom = dynamic_cast<geos::geom::Point*>(point->clone());
	}
	
	RSGISPointData::~RSGISPointData()
	{
		delete pointGeom;
	}
	
}}

