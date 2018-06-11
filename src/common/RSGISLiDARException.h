/*
 *  RSGISLiDARException.h
 *  RSGIS_LIB
 *
 *  Created by Pete Bunting on 28/09/2009.
 *  Copyright 2009 RSGISLib.
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

#ifndef RSGISLiDARException_H
#define RSGISLiDARException_H

#include "RSGISException.h"

namespace rsgis 
{    
	class RSGISLiDARException : public RSGISException
	{
	public:
		RSGISLiDARException();
		RSGISLiDARException(const char* message);
		RSGISLiDARException(std::string message);
	};
}

#endif

