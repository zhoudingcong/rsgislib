/*
 *  RSGISAddBands.h
 *  
 *	RSGIS IMG
 *
 *	A class to stack image bands.
 *
 *  Created by Pete Bunting on 04/02/2008.
 *  Copyright 2008 RSGISLib.
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

#ifndef RSGISAddBands_H
#define RSGISAddBands_H

#include <iostream>
#include <string>
#include "gdal_priv.h"
#include "img/RSGISImageBandException.h"
#include "img/RSGISImageUtils.h"
#include "img/RSGISImageCalcException.h"
#include "img/RSGISCalcImageValue.h"

#include "math/RSGISMathsUtils.h"

// mark all exported classes/functions with DllExport to have
// them exported by Visual Studio
#undef DllExport
#ifdef _MSC_VER
    #ifdef rsgis_img_EXPORTS
        #define DllExport   __declspec( dllexport )
    #else
        #define DllExport   __declspec( dllimport )
    #endif
#else
    #define DllExport
#endif

namespace rsgis 
{
	namespace img
	{
		class DllExport RSGISAddBands
		{
			public:
				RSGISAddBands();
				void addMultipleBands(GDALDataset *input, GDALDataset **toAdd, std::string *outputFile, int *band, int numAddBands);
				void addBandToFile(GDALDataset *input, GDALDataset *toAdd, std::string *outputFile, int band);
				void stackImages(GDALDataset **datasets, int numDS, std::string outputImage, std::string *imageBandNames, bool skipPixels, float skipValue = 0, float noDataValue = 0, std::string gdalFormat="ENVI", GDALDataType gdalDataType=GDT_Float32, bool replaceBandNames=false);
				~RSGISAddBands();
		};
        
        
        
        class DllExport RSGISCombineImagesIgnoreNoData : public RSGISCalcImageValue
        {
        public:
            RSGISCombineImagesIgnoreNoData(float noDataVal);
            void calcImageValue(float *bandValues, int numBands, double *output);
            ~RSGISCombineImagesIgnoreNoData();
        protected:
            float noDataVal;
        };
	}
}

#endif

