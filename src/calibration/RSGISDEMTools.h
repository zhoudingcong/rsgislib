/*
 *  RSGISDEMTools.h
 *  RSGIS_LIB
 *
 *  Created by Pete Bunting on 01/08/2011.
 *  Copyright 2011 RSGISLib. All rights reserved.
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

#ifndef RSGISDEMTools_h
#define RSGISDEMTools_h

#include <iostream>
#include <string>
#include <math.h>

#include "gdal_priv.h"

#include "geos/geom/Envelope.h"
#include "geos/geom/Point.h"
#include "geos/geom/Polygon.h"
#include "geos/geom/Coordinate.h"
#include "geos/geom/CoordinateArraySequence.h"
#include "geos/geom/PrecisionModel.h"

#include "img/RSGISImageCalcException.h"
#include "img/RSGISCalcImageValue.h"
#include "img/RSGISImageUtils.h"
#include "img/RSGISExtractImagePixelsInPolygon.h"

#include "utils/RSGISGEOSFactoryGenerator.h"

#include "math/RSGISMathsUtils.h"

#include <boost/math/special_functions/fpclassify.hpp>

#ifndef M_PI
# define M_PI  3.1415926535897932384626433832795
#endif

#ifndef ASTROUNIT
# define ASTROUNIT  149597870.7
#endif

namespace rsgis{namespace calib{
    
    
    /***
     * If output type is set to 1 then output is radians if 0 then degrees.
     *
     */
	class DllExport RSGISCalcSlope: public rsgis::img::RSGISCalcImageValue
	{
	public: 
		RSGISCalcSlope(int numberOutBands, unsigned int band, float ewRes, float nsRes, int outType);
		void calcImageValue(float *bandValues, int numBands, double *output) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
		void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException);
        void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		~RSGISCalcSlope();
    private:
        unsigned int band;
        float ewRes;
        float nsRes;
        int outType;
	};
    
    class DllExport RSGISCalcAspect : public rsgis::img::RSGISCalcImageValue
	{
	public: 
		RSGISCalcAspect(int numberOutBands, unsigned int band, float ewRes, float nsRes);
		void calcImageValue(float *bandValues, int numBands, double *output) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
		void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException);
        void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		~RSGISCalcAspect();
    private:
        unsigned int band;
        float ewRes;
        float nsRes;
	};
    
    class DllExport RSGISRecodeAspect : public rsgis::img::RSGISCalcImageValue
	{
	public:
		RSGISRecodeAspect();
		void calcImageValue(float *bandValues, int numBands, double *output) throw(rsgis::img::RSGISImageCalcException);
		void calcImageValue(float *bandValues, int numBands) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
		void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
        void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		~RSGISRecodeAspect();
	};
    
    class DllExport RSGISCalcSlopeAspect : public rsgis::img::RSGISCalcImageValue
	{
	public: 
		RSGISCalcSlopeAspect(int numberOutBands, unsigned int band, float ewRes, float nsRes);
		void calcImageValue(float *bandValues, int numBands, double *output) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
		void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException);
        void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		~RSGISCalcSlopeAspect();
    private:
        unsigned int band;
        float ewRes;
        float nsRes;
	};
    
    
	class DllExport RSGISCalcHillShade : public rsgis::img::RSGISCalcImageValue
	{
	public: 
		RSGISCalcHillShade(int numberOutBands, unsigned int band, float ewRes, float nsRes, float sunZenith, float sunAzimuth);
		void calcImageValue(float *bandValues, int numBands, double *output) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
		void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException);
        void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		~RSGISCalcHillShade();
    private:
        unsigned int band;
        float ewRes;
        float nsRes;
        float sunZenith;
        float sunAzimuth;
	};
    
    class DllExport RSGISCalcShadowBinaryMask : public rsgis::img::RSGISCalcImageValue
	{
	public: 
		RSGISCalcShadowBinaryMask(int numberOutBands, GDALDataset *inputImage, unsigned int band, float ewRes, float nsRes, float sunZenith, float sunAzimuth, float maxElevHeight, double noDataVal);
		void calcImageValue(float *bandValues, int numBands, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
		void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands, double *output, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException);
		void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
        void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		~RSGISCalcShadowBinaryMask();
    private:
        unsigned int band;
        float ewRes;
        float nsRes;
        float sunZenith;
        float sunAzimuth;
        double sunRange;
        double demWidth;
        double demHeight;
        float maxElevHeight;
        GDALDataset *inputImage;
        double noDataVal;
	};
    
    class DllExport RSGISCalcRayIncidentAngle : public rsgis::img::RSGISCalcImageValue
	{
	public: 
		RSGISCalcRayIncidentAngle(int numberOutBands, unsigned int band, float ewRes, float nsRes, float sunZenith, float sunAzimuth);
		void calcImageValue(float *bandValues, int numBands, double *output) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
		void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException);
        void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		~RSGISCalcRayIncidentAngle();
    private:
        unsigned int band;
        float ewRes;
        float nsRes;
        float sunZenith;
        float sunAzimuth;
	};
    
    class DllExport RSGISCalcRayExitanceAngle : public rsgis::img::RSGISCalcImageValue
	{
	public: 
		RSGISCalcRayExitanceAngle(int numberOutBands, unsigned int band, float ewRes, float nsRes, float viewZenith, float viewAzimuth);
		void calcImageValue(float *bandValues, int numBands, double *output) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
		void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException);
        void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		~RSGISCalcRayExitanceAngle();
    private:
        unsigned int band;
        float ewRes;
        float nsRes;
        float viewZenith;
        float viewAzimuth;
	};
    
    class DllExport RSGISCalcRayIncidentAndExitanceAngles : public rsgis::img::RSGISCalcImageValue
	{
	public: 
		RSGISCalcRayIncidentAndExitanceAngles(int numberOutBands, unsigned int band, float ewRes, float nsRes, float sunZenith, float sunAzimuth, float viewZenith, float viewAzimuth);
		void calcImageValue(float *bandValues, int numBands, double *output) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
		void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException);
        void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		~RSGISCalcRayIncidentAndExitanceAngles();
    private:
        unsigned int band;
        float ewRes;
        float nsRes;
        float sunZenith;
        float sunAzimuth;
        float viewZenith;
        float viewAzimuth;
	};
    
    
    
    
    class DllExport RSGISFillDEMHoles : public rsgis::img::RSGISCalcImageValue
	{
	public:
		RSGISFillDEMHoles(float holeValue, float nodata);
		void calcImageValue(float *bandValues, int numBands, double *output) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
		void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException);
        void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		bool changeOccurred();
        void resetChange();
        ~RSGISFillDEMHoles();
    private:
        float holeValue;
        float nodata;
        bool change;
	};

    
    
    class DllExport RSGISInFillDerivedHoles : public rsgis::img::RSGISCalcImageValue
	{
	public:
		RSGISInFillDerivedHoles(float holeValue);
		void calcImageValue(float *bandValues, int numBands, double *output) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
		void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException);
        void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
        ~RSGISInFillDerivedHoles();
    private:
        float holeValue;
	};
    
    class DllExport RSGISFilterDTMWithAspectMedianFilter : public rsgis::img::RSGISCalcImageValue
	{
	public:
		RSGISFilterDTMWithAspectMedianFilter(float aspectRange);
		void calcImageValue(float *bandValues, int numBands, double *output) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
		void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException) {throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float *bandValues, int numBands, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException);
        void calcImageValue(float ***dataBlock, int numBands, int winSize, double *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
		bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, double *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("No implemented");};
        ~RSGISFilterDTMWithAspectMedianFilter();
    private:
        float aspectRange;
	};

    
	
}}

#endif


