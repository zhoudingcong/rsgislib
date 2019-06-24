/*
 *  RSGISRandomColourClumps.h
 *  RSGIS_LIB
 *
 *  Created by Pete Bunting on 19/01/2012.
 *  Copyright 2012 RSGISLib.
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

#ifndef RSGISRandomColourClumps_h
#define RSGISRandomColourClumps_h

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "gdal_priv.h"

#include "img/RSGISImageUtils.h"
#include "img/RSGISImageCalcException.h"
#include "utils/RSGISTextUtils.h"
#include "utils/RSGISTextException.h"

// mark all exported classes/functions with DllExport to have
// them exported by Visual Studio
#undef DllExport
#ifdef _MSC_VER
    #ifdef rsgis_segmentation_EXPORTS
        #define DllExport   __declspec( dllexport )
    #else
        #define DllExport   __declspec( dllimport )
    #endif
#else
    #define DllExport
#endif

namespace rsgis{namespace segment{
    
    struct DllExport ImgClumpRGB
    {
        ImgClumpRGB(unsigned long clumpID)
        {
            this->clumpID = clumpID;
        };
        unsigned int clumpID;
        int red;
        int green;
        int blue;
    };
    
    
    class DllExport RSGISRandomColourClumps
    {
    public:
        RSGISRandomColourClumps();
        void generateRandomColouredClump(GDALDataset *clumps, GDALDataset *colourImg, std::string inputLUTFile, bool importLUT, std::string exportLUTFile, bool exportLUT);
        ~RSGISRandomColourClumps();
    protected:
        std::vector<ImgClumpRGB*>* importLUTFromFile(std::string inFile);
        void exportLUT2File(std::string outFile, std::vector<ImgClumpRGB*> *clumpTab);
    };
    
}}



#endif
