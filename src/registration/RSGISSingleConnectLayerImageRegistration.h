/*
 *  RSGISSingleConnectLayerImageRegistration.h
 *  RSGIS_LIB
 *
 *  Created by Pete Bunting on 03/09/2010.
 *  Copyright 2010 RSGISLib. All rights reserved.
 *
 * This file is part of RSGISLib.
 * 
 * RSGISLib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RSGISLib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RSGISLib.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#ifndef RSGISSingleConnectLayerImageRegistration_H
#define RSGISSingleConnectLayerImageRegistration_H

#include <iostream>
#include <string>
#include <math.h>
#include <list>

#include "gdal_priv.h"
#include "ogrsf_frmts.h"

#include "common/RSGISRegistrationException.h"

#include "registration/RSGISImageRegistration.h"

namespace rsgis{namespace reg{
    
	class DllExport RSGISSingleConnectLayerImageRegistration : public RSGISImageRegistration
	{
	public:
		
		struct DllExport TiePointInSingleLayer
		{
			TiePoint *tiePt;
			std::list<TiePoint*> *nrTiePts;
		};
		
		RSGISSingleConnectLayerImageRegistration(GDALDataset *reference, GDALDataset *floating, unsigned int gap, float metricThreshold, unsigned int windowSize, unsigned int searchArea, RSGISImageSimilarityMetric *metric, float stdDevRefThreshold, float stdDevFloatThreshold, unsigned int subPixelResolution, float distanceThreshold, unsigned int maxNumIterations, float moveChangeThreshold, float pSmoothness);
		void initRegistration()throw(RSGISRegistrationException);
		void executeRegistration()throw(RSGISRegistrationException);
		void finaliseRegistration()throw(RSGISRegistrationException);
		void exportTiePointsENVIImage2Map(std::string filepath)throw(RSGISRegistrationException);
		void exportTiePointsENVIImage2Image(std::string filepath)throw(RSGISRegistrationException);
		void exportTiePointsRSGISImage2Map(std::string filepath)throw(RSGISRegistrationException);
        void exportTiePointsRSGISMapOffs(std::string filepath)throw(RSGISRegistrationException);
		~RSGISSingleConnectLayerImageRegistration();
	private:
		std::list<TiePointInSingleLayer*> *tiePoints;
		unsigned int gap;
		float metricThreshold;
		bool initExecuted;
		unsigned int windowSize;
		unsigned int searchArea;
		RSGISImageSimilarityMetric *metric;
		float stdDevRefThreshold;
		float stdDevFloatThreshold;
		unsigned int subPixelResolution;
		float distanceThreshold;
		unsigned int maxNumIterations;
		float moveChangeThreshold;
		float pSmoothness;
	};
}}

#endif


