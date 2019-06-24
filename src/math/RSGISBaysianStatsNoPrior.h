/*
 *  RSGISBaysianStats.h
 *  RSGIS_LIB
 *
 *  Created by Daniel Clewley on 12/12/2008.
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

#ifndef RSGISBaysianStatsNoPrior_H
#define RSGISBaysianStatsNoPrior_H

#include "math/RSGISBaysianIntergrateFunctionNoPrior.h"
#include "math/RSGISMathFunction.h"
#include "math/RSGISIntergration.h"
#include "math/RSGISBaysianStatsException.h"
#include "math/RSGISBaysianDeltaType.h"

// mark all exported classes/functions with DllExport to have
// them exported by Visual Studio
#undef DllExport
#ifdef _MSC_VER
    #ifdef rsgis_maths_EXPORTS
        #define DllExport   __declspec( dllexport )
    #else
        #define DllExport   __declspec( dllimport )
    #endif
#else
    #define DllExport
#endif

namespace rsgis
{
	namespace math
	{		
		class DllExport RSGISBaysianStatsNoPrior
			{
			public:
				RSGISBaysianStatsNoPrior(RSGISMathFunction *function, double variance, double interval, double minVal, double maxVal, double lowerLimit, double upperLimit, deltatypedef deltatype);
				virtual double* calcImageValueNoPrior(float value);
				virtual ~RSGISBaysianStatsNoPrior(){};
			protected:
				deltatypedef deltatype;
				double variance;
				double interval;
				double minVal;
				double maxVal;
				double upperLimit;
				double lowerLimit;
				float value;
				RSGISBaysianIntergrateFunctionNoPrior *baysianFunction;
			};
		
	}
}

#endif
