 /*
 *  RSGISImageWarpException.h
 *  RSGIS_LIB
 *
 *  Created by Pete Bunting on 04/09/2010.
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

#ifndef RSGISImageWarpException_H
#define RSGISImageWarpException_H

#include "common/RSGISRegistrationException.h"

namespace rsgis{namespace reg{
	
	class DllExport RSGISImageWarpException : public rsgis::RSGISRegistrationException
	{
	public:
		RSGISImageWarpException();
		RSGISImageWarpException(const char* message);
		RSGISImageWarpException(std::string message);
	};

}}

#endif



