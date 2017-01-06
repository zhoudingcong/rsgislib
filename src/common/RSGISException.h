/*
 *  RSGISException.h
 *
 *  RSGIS Common
 *
 *	A class providing the base Exception for the 
 *	the RSGIS library
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

#ifndef RSGISException_H
#define RSGISException_H

#include <exception>
#include <iostream>
#include <string>

#include "RSGISCommons.h"

// mark all exported classes/functions with DllExport to have
// them exported by Visual Studio
#ifdef _MSC_VER
    #ifdef rsgis_commons_EXPORTS
        #define DllExport   __declspec( dllexport )
    #else
        #define DllExport   __declspec( dllimport )
    #endif
#else
    #define DllExport
#endif

namespace rsgis
{    
	class DllExport RSGISException : public std::exception
	{
		public:
			RSGISException();
			RSGISException(const char *message);
            RSGISException(std::string message);
			virtual ~RSGISException() throw();
			virtual const char* what() const throw();
		protected:
            std::string msgs;
	};
}


#endif
