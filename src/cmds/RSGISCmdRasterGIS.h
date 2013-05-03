/*
 *  RSGISCmdRasterGIS.h
 *
 *
 *  Created by Pete Bunting on 03/05/2013.
 *  Copyright 2013 RSGISLib.
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

#ifndef RSGISCmdRasterGIS_H
#define RSGISCmdRasterGIS_H

#include <iostream>
#include <string>

#include "common/RSGISCommons.h"
#include "RSGISCmdException.h"

namespace rsgis{ namespace cmds {
    
    void executePopulateStats(std::string clumpsImage, bool addColourTable2Img, bool calcImgPyramids)throw(RSGISCmdException);
    
    
}}


#endif

