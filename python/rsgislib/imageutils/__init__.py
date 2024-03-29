"""
The imageutils module contains general utilities for applying to images.
"""
# Maintain python 2 backwards compatibility
from __future__ import print_function
# import the C++ extension into this level
from ._imageutils import *
import rsgislib 

import os.path
import math

import numpy

import osgeo.gdal as gdal
import osgeo.ogr as ogr
import osgeo.osr as osr

haveRIOS = True
try:
    from rios import applier
    from rios import cuiprogress
except ImportError as riosErr:
    haveRIOS = False

# define our own classes
class ImageBandInfo(object):
    """
Create a list of these objects to pass to the extractZoneImageBandValues2HDF function

:param fileName: is the input image file name and path.
:param name: is a name associated with this layer - doesn't really matter what you use but needs to be unique; this is used as a dict key in some functions.
:param bands: is a list of image bands within the fileName to be used for processing (band numbers start at 1).

"""
    def __init__(self, fileName=None, name=None, bands=None):
        """
        :param fileName: is the input image file name and path.
        :param name: is a name associated with this layer - doesn't really matter what you use but needs to be unique; this is used as a dict key in some functions.
        :param bands: is a list of image bands within the fileName to be used for processing (band numbers start at 1).
        """
        self.fileName = fileName
        self.name = name
        self.bands = bands
        
# define our own classes
class SharpBandInfo(object):
    """
Create a list of these objects to pass to the sharpenLowResBands function.

:param band: is the band number (band numbering starts at 1).
:param status: needs to be either rsgislib.SHARP_RES_IGNORE, rsgislib.SHARP_RES_LOW or rsgislib.SHARP_RES_HIGH
               lowres bands will be sharpened using the highres bands and ignored bands
               will just be copied into the output image.
:param name: is a name associated with this image band - doesn't really matter what you put in here.

"""
    def __init__(self, band=None, status=None, name=None):
        """
        :param band: is the band number (band numbering starts at 1).
        :param status: needs to be either 'ignore', 'lowres' or 'highres' - lowres bands will be sharpened using the highres bands and ignored bands will just be copied into the output image.
        :param name: is a name associated with this image band - doesn't really matter what you put in here.

        """
        self.band = band
        self.status = status
        self.name = name

# Define Class for time series fill
class RSGISTimeseriesFillInfo(object):
    """
Create a list of these objects to pass to the fillTimeSeriesGaps function

:param year: year the composite represents.
:param day: the (nominal) day within the year the composite represents (a value of zero and day will not be used)
:param compImg: The input compsite image which has been generated.
:param imgRef:  The reference layer (e.g., from createMaxNDVIComposite or createMaxNDVINDWICompositeLandsat) with zero for no data regions
:param outRef: A boolean variable specify which layer a fill reference layer is to be produced.

"""
    def __init__(self, year=1900, day=0, compImg=None, imgRef=None, outRef=False):
        """
        :param year: year the composite represents.
        :param day: the (nominal) day within the year the composite represents (a value of zero and day will not be used)
        :param compImg: The input compsite image which has been generated.
        :param imgRef:  The reference layer (e.g., from createMaxNDVIComposite or createMaxNDVINDWICompositeLandsat) with zero for no data regions
        :param outRef: A boolean variable specify which layer a fill reference layer is to be produced.

        """
        self.year = year
        self.day = day
        self.compImg = compImg
        self.imgRef = imgRef
        self.outRef = outRef
    
    def __repr__(self):
        return repr((self.year, self.day, self.compImg, self.imgRef, self.outRef))


def setBandNames(inputImage, bandNames, feedback=False):
    """A utility function to set band names.
Where:

:param inImage: is the input image
:param bandNames: is a list of band names
:param feedback: is a boolean specifying whether feedback will be printed to the console (True= Printed / False (default) Not Printed)

Example::

    from rsgislib import imageutils

    inputImage = 'injune_p142_casi_sub_utm.kea'
    bandNames = ['446nm','530nm','549nm','569nm','598nm','633nm','680nm','696nm','714nm','732nm','741nm','752nm','800nm','838nm']
    
    imageutils.setBandNames(inputImage, bandNames)
    
"""
    dataset = gdal.Open(inputImage, gdal.GA_Update)
    
    for i in range(len(bandNames)):
        band = i+1
        bandName = bandNames[i]

        imgBand = dataset.GetRasterBand(band)
        # Check the image band is available
        if not imgBand is None:
            if feedback:
                print('Setting Band {0} to "{1}"'.format(band, bandName))
            imgBand.SetDescription(bandName)
        else:
            raise Exception("Could not open the image band: ", band)

def getBandNames(inputImage):
    """
A utility function to get band names.

Where:

:param inImage: is the input image

:return: list of band names

Example::

    from rsgislib import imageutils

    inputImage = 'injune_p142_casi_sub_utm.kea'
    bandNames = imageutils.getBandNames(inputImage)

"""
    dataset = gdal.Open(inputImage, gdal.GA_Update)
    bandNames = list()
    
    for i in range(dataset.RasterCount):
        imgBand = dataset.GetRasterBand(i+1)
        # Check the image band is available
        if not imgBand is None:
            bandNames.append(imgBand.GetDescription())
        else:
            raise Exception("Could not open the image band: ", band)
    return bandNames


def getRSGISLibDataType(inImg):
    """
Returns the rsgislib datatype ENUM for a raster file

:param inImg: The file to get the datatype for

:return: The rsgislib datatype enum, e.g., rsgislib.TYPE_8INT

"""
    raster = gdal.Open(inImg, gdal.GA_ReadOnly)
    if raster == None:
        raise Exception("Could not open the inImg.")
    band = raster.GetRasterBand(1)
    gdal_dtype = gdal.GetDataTypeName(band.DataType)
    raster = None
    rsgis_utils = rsgislib.RSGISPyUtils()

    return rsgis_utils.getRSGISLibDataType(gdal_dtype)


def getGDALDataType(inImg):
    """
Returns the rsgislib datatype ENUM for a raster file

:param inImg: The file to get the datatype for

:return: The rsgislib datatype enum, e.g., rsgislib.TYPE_8INT

"""
    raster = gdal.Open(inImg, gdal.GA_ReadOnly)
    if raster == None:
        raise Exception("Could not open the inImg.")
    band = raster.GetRasterBand(1)
    gdal_dtype = gdal.GetDataTypeName(band.DataType)
    raster = None
    return gdal_dtype

def setImgThematic(imageFile):
    """
Set all image bands to be thematic. 

:param imageFile: The file for which the bands are to be set as thematic

"""
    ds = gdal.Open(imageFile, gdal.GA_Update)
    if ds == None:
        raise Exception("Could not open the imageFile.")
    for bandnum in range(ds.RasterCount):
        band = ds.GetRasterBand(bandnum + 1)
        band.SetMetadataItem('LAYER_TYPE', 'thematic')
    ds = None


def hasGCPs(inImg):
    """
Test whether the input image has GCPs - returns boolean

:param inImg: input image file

:return: boolean True - has GCPs; False - does not have GCPs

"""
    raster = gdal.Open(inImg, gdal.GA_ReadOnly)
    if raster == None:
        raise Exception("Could not open the inImg.")
    numGCPs = raster.GetGCPCount()
    hasGCPs = False
    if numGCPs > 0:
        hasGCPs = True
    raster = None
    return hasGCPs

def copyGCPs(srcImg, destImg):
    """
Copy the GCPs from the srcImg to the destImg

:param srcImg: Raster layer with GCPs
:param destImg: Raster layer to which GCPs will be added
    
"""
    srcDS = gdal.Open(srcImg, gdal.GA_ReadOnly)
    if srcDS == None:
        raise Exception("Could not open the srcImg.")
    destDS = gdal.Open(destImg, gdal.GA_Update)
    if destDS == None:
        raise Exception("Could not open the destImg.")
        srcDS = None

    numGCPs = srcDS.GetGCPCount()
    if numGCPs > 0:
        gcpProj = srcDS.GetGCPProjection()
        gcpList = srcDS.GetGCPs()
        destDS.SetGCPs(gcpList, gcpProj)

    srcDS = None
    destDS = None


def getWKTProjFromImage(inImg):
    """
A function which returns the WKT string representing the projection of the input image.

:param inImg: input image from which WKT string will be read.

"""
    rasterDS = gdal.Open(inImg, gdal.GA_ReadOnly)
    if rasterDS == None:
        raise Exception('Could not open raster image: \'' + inImg+ '\'')
    projStr = rasterDS.GetProjection()
    rasterDS = None
    return projStr


def createBlankImgFromRefVector(inVecFile, inVecLyr, outputImg, outImgRes, outImgNBands, gdalformat, datatype):
    """
A function to create a new image file based on a vector layer to define the extent and projection
of the output image. 

:param inVecFile: input vector file.
:param inVecLyr: name of the vector layer, if None then assume the layer name will be the same as the file
                 name of the input vector file.
:param outputImg: output image file.
:param outImgRes: output image resolution, square pixels so a single value.
:param outImgNBands: the number of image bands in the output image
:param gdalformat: output image file format.
:param datatype: is a rsgislib.TYPE_* value providing the data type of the output image

"""

    rsgisUtils = rsgislib.RSGISPyUtils()

    baseExtent = rsgisUtils.getVecLayerExtent(inVecFile, inVecLyr)
    xMin, xMax, yMin, yMax = rsgisUtils.findExtentOnGrid(baseExtent, outImgRes, fullContain=True)

    tlX = xMin
    tlY = yMax
    
    widthCoord = xMax - xMin
    heightCoord = yMax - yMin
    
    width = int(math.ceil(widthCoord/outImgRes))
    height = int(math.ceil(heightCoord/outImgRes))
    
    wktString = rsgisUtils.getProjWKTFromVec(inVecFile)

    rsgislib.imageutils.createBlankImage(outputImg, outImgNBands, width, height, tlX, tlY, outImgRes, 0.0, '', wktString, gdalformat, datatype)
    

def createCopyImageVecExtentSnap2Grid(inVecFile, inVecLyr, outputImg, outImgRes, outImgNBands, gdalformat, datatype, bufnpxl=0):
    """
A function to create a new image file based on a vector layer to define the extent and projection
of the output image. The image file extent is snapped on to the grid defined by the vector layer.

:param inVecFile: input vector file.
:param inVecLyr: name of the vector layer, if None then assume the layer name will be the same as the file
                 name of the input vector file.
:param outputImg: output image file.
:param outImgRes: output image resolution, square pixels so a single value.
:param outImgNBands: the number of image bands in the output image
:param gdalformat: output image file format.
:param datatype: is a rsgislib.TYPE_* value providing the data type of the output image
:param bufnpxl: is an integer specifying the number of pixels to buffer the vector file extent by.

"""
    rsgisUtils = rsgislib.RSGISPyUtils()
    
    vec_bbox = rsgisUtils.getVecLayerExtent(inVecFile, layerName=inVecLyr, computeIfExp=True)
    xMin = vec_bbox[0] - (outImgRes * bufnpxl)
    xMax = vec_bbox[1] + (outImgRes * bufnpxl)
    yMin = vec_bbox[2] - (outImgRes * bufnpxl)
    yMax = vec_bbox[3] + (outImgRes * bufnpxl)
    xMin, xMax, yMin, yMax = rsgisUtils.findExtentOnWholeNumGrid([xMin, xMax, yMin, yMax], outImgRes, True) 
    
    tlX = xMin
    tlY = yMax
    
    widthCoord = xMax - xMin
    heightCoord = yMax - yMin
    
    width = int(math.ceil(widthCoord/outImgRes))
    height = int(math.ceil(heightCoord/outImgRes))
    
    wktString = rsgisUtils.getProjWKTFromVec(inVecFile)
    
    rsgislib.imageutils.createBlankImage(outputImg, outImgNBands, width, height, tlX, tlY, outImgRes, 0.0, '', wktString, gdalformat, datatype)
    

def createBlankImgFromBBOX(bbox, wktstr, outputImg, outImgRes, outImgPxlVal, outImgNBands, gdalformat, datatype, snap2grid=False):
    """
A function to create a new image file based on a bbox to define the extent. 

:param bbox: bounding box defining the extent of the output image (xMin, xMax, yMin, yMax)
:param wktstr: the WKT string defining the bbox and output image projection.
:param outputImg: output image file.
:param outImgRes: output image resolution, square pixels so a single value.
:param outImgPxlVal: output image pixel value.
:param outImgNBands: the number of image bands in the output image
:param gdalformat: output image file format.
:param datatype: is a rsgislib.TYPE_* value providing the data type of the output image.
:param snap2grid: optional variable to snap the image to a grid of whole numbers with respect to the image pixel resolution.

"""    
    if snap2grid:
        rsgisUtils = rsgislib.RSGISPyUtils()
        bbox = rsgisUtils.findExtentOnGrid(bbox, outImgRes, fullContain=True)

    xMin = bbox[0]
    xMax = bbox[1]
    yMin = bbox[2]
    yMax = bbox[3]

    tlX = xMin
    tlY = yMax
    
    widthCoord = xMax - xMin
    heightCoord = yMax - yMin
    
    width = int(math.ceil(widthCoord/outImgRes))
    height = int(math.ceil(heightCoord/outImgRes))
    
    rsgislib.imageutils.createBlankImage(outputImg, outImgNBands, width, height, tlX, tlY, outImgRes, outImgPxlVal, '', wktstr, gdalformat, datatype)

   
def createImageForEachVecFeat(vectorFile, vectorLyr, fileNameCol, outImgPath, outImgExt, outImgPxlVal, outImgNBands, outImgRes, gdalformat, datatype, snap2grid=False):
    """
A function to create a set of image files representing the extent of each feature in the 
inputted vector file.

:param vectorFile: the input vector file.
:param vectorLyr: the input vector layer
:param fileNameCol: the name of the column in the vector layer which will be used as the file names.
:param outImgPath: output file path (directory) where the images will be saved.
:param outImgExt: the file extension to be added on to the output file names.
:param outImgPxlVal: output image pixel value
:param outImgNBands: the number of image bands in the output image
:param outImgRes: output image resolution, square pixels so a single value
:param gdalformat: output image file format.
:param datatype: is a rsgislib.TYPE_* value providing the data type of the output image.
:param snap2grid: optional variable to snap the image to a grid of whole numbers with respect to the image pixel resolution.

"""
    
    dsVecFile = gdal.OpenEx(vectorFile, gdal.OF_VECTOR )
    if dsVecFile is None:
        raise Exception("Could not open '" + vectorFile + "'")
        
    lyrVecObj = dsVecFile.GetLayerByName( vectorLyr )
    if lyrVecObj is None:
        raise Exception("Could not find layer '" + vectorLyr + "'")
        
    lyrSpatRef = lyrVecObj.GetSpatialRef()
    if lyrSpatRef is not None:
        wktstr = lyrSpatRef.ExportToWkt()
    else:
        wktstr = ''
        
    colExists = False
    feat_idx = 0
    lyrDefn = lyrVecObj.GetLayerDefn()
    for i in range( lyrDefn.GetFieldCount() ):
        if lyrDefn.GetFieldDefn(i).GetName().lower() == fileNameCol.lower():
            feat_idx = i
            colExists = True
            break
    
    if not colExists:
        dsVecFile = None
        raise Exception("The specified column does not exist in the input layer; check case as some drivers are case sensitive.")
    
    lyrVecObj.ResetReading()
    for feat in lyrVecObj:
        geom = feat.GetGeometryRef()
        if geom is not None:
            env = geom.GetEnvelope()
            tilebasename = feat.GetFieldAsString(feat_idx)
            outputImg = os.path.join(outImgPath, "{0}{1}".format(tilebasename, outImgExt))
            print(outputImg)
            createBlankImgFromBBOX(env, wktstr, outputImg, outImgRes, outImgPxlVal, outImgNBands, gdalformat, datatype, snap2grid)


def resampleImage2Match(inRefImg, inProcessImg, outImg, gdalformat, interpMethod, datatype=None, noDataVal=None, multicore=False):
    """
A utility function to resample an existing image to the projection and/or pixel size of another image.

Where:

:param inRefImg: is the input reference image to which the processing image is to resampled to.
:param inProcessImg: is the image which is to be resampled.
:param outImg: is the output image file.
:param gdalformat: is the gdal format for the output image.
:param interpMethod: is the interpolation method used to resample the image [bilinear, lanczos, cubicspline, nearestneighbour, cubic, average, mode]
:param datatype: is the rsgislib datatype of the output image (if none then it will be the same as the input file).
:param multicore: use multiple processing cores (Default = False)

""" 
    rsgisUtils = rsgislib.RSGISPyUtils()
    numBands = rsgisUtils.getImageBandCount(inProcessImg)
    if noDataVal == None:
        noDataVal = rsgisUtils.getImageNoDataValue(inProcessImg)
    
    if datatype == None:
        datatype = rsgisUtils.getRSGISLibDataTypeFromImg(inProcessImg)
        
    interpolationMethod = gdal.GRA_NearestNeighbour
    if interpMethod == 'bilinear':
        interpolationMethod = gdal.GRA_Bilinear 
    elif interpMethod == 'lanczos':
        interpolationMethod = gdal.GRA_Lanczos 
    elif interpMethod == 'cubicspline':
        interpolationMethod = gdal.GRA_CubicSpline 
    elif interpMethod == 'nearestneighbour':
        interpolationMethod = gdal.GRA_NearestNeighbour 
    elif interpMethod == 'cubic':
        interpolationMethod = gdal.GRA_Cubic
    elif interpMethod == 'average':
        interpolationMethod = gdal.GRA_Average
    elif interpMethod == 'mode':
        interpolationMethod = gdal.GRA_Mode
    else:
        raise Exception("Interpolation method was not recognised or known.")
    
    backVal = 0.0
    haveNoData = False
    if noDataVal != None:
        backVal = float(noDataVal)
        haveNoData = True
    
    rsgislib.imageutils.createCopyImage(inRefImg, outImg, numBands, backVal, gdalformat, datatype)

    inFile = gdal.Open(inProcessImg, gdal.GA_ReadOnly)
    outFile = gdal.Open(outImg, gdal.GA_Update)

    wrpOpts = []
    if multicore:
        if haveNoData:
            wrpOpts = gdal.WarpOptions(resampleAlg=interpolationMethod, srcNodata=noDataVal, dstNodata=noDataVal, multithread=True, callback=gdal.TermProgress)    
        else:
            wrpOpts = gdal.WarpOptions(resampleAlg=interpolationMethod, multithread=True, callback=gdal.TermProgress)
    else:
        if haveNoData:
            wrpOpts = gdal.WarpOptions(resampleAlg=interpolationMethod, srcNodata=noDataVal, dstNodata=noDataVal, multithread=False, callback=gdal.TermProgress)    
        else:
            wrpOpts = gdal.WarpOptions(resampleAlg=interpolationMethod, multithread=False, callback=gdal.TermProgress)
    
    gdal.Warp(outFile, inFile, options=wrpOpts)
    
    inFile = None
    outFile = None


def reprojectImage(inputImage, outputImage, outWKT, gdalformat='KEA', interp='cubic', inWKT=None, noData=0.0, outPxlRes='image', snap2Grid=True, multicore=False):
    """
This function provides a tool which uses the gdalwarp function to reproject an input image.

Where:

:param inputImage: the input image name and path
:param outputImage: the output image name and path
:param outWKT: a WKT file representing the output projection
:param gdalformat: the output image file format (Default is KEA)
:param interp: interpolation algorithm. Options are: near, bilinear, cubic, cubicspline, lanczos, average, mode. (Default is cubic)
:param inWKT: if input image is not well defined this is the input image projection as a WKT file (Default is None, i.e., ignored)
:param noData: float representing the not data value (Default is 0.0)
:param outPxlRes: three inputs can be provided
                  1) 'image' where the output resolution will match the input (Default is image)
                  2) 'auto' where an output resolution maintaining the image size of the input image will be used
                  3) provide a floating point value for the image resolution (note. pixels will be sqaure)
:param snap2Grid: is a boolean specifying whether the TL pixel should be snapped to a multiple of the pixel resolution (Default is True).
:param nCores: the number of processing cores available for processing (-1 is all cores: Default=-1)

    """    
    rsgisUtils = rsgislib.RSGISPyUtils()
    
    eResampleAlg = gdal.GRA_CubicSpline
    if interp == 'near':
        eResampleAlg = gdal.GRA_NearestNeighbour
    elif interp == 'bilinear':
        eResampleAlg = gdal.GRA_Bilinear
    elif interp == 'cubic':
        eResampleAlg = gdal.GRA_Cubic
    elif interp == 'cubicspline':
        eResampleAlg = gdal.GRA_CubicSpline
    elif interp == 'lanczos':
        eResampleAlg = gdal.GRA_Lanczos
    elif interp == 'average':
        eResampleAlg = gdal.GRA_Average
    elif interp == 'mode':
        eResampleAlg = gdal.GRA_Mode
    else:
        raise Exception('The interpolation algorithm was not recogonised: \'' + interp + '\'')
    
    if not os.path.exists(inputImage):
        raise Exception('The input image file does not exist: \'' + inputImage + '\'')
    
    inImgDS = gdal.Open(inputImage, gdal.GA_ReadOnly)
    if inImgDS is None:
        raise Exception('Could not open the Input Image: \'' + inputImage + '\'')    
    
    inImgProj = osr.SpatialReference()
    if not inWKT is None:
        if not os.path.exists(inWKT):
            raise Exception('The input WKT file does not exist: \'' + inWKT + '\'')
        inWKTStr = rsgisUtils.readTextFileNoNewLines(inWKT)
        inImgProj.ImportFromWkt(inWKTStr)
    else:
        inImgProj.ImportFromWkt(inImgDS.GetProjectionRef())
        
    if not os.path.exists(outWKT):
        raise Exception('The output WKT file does not exist: \'' + outWKT + '\'')
    outImgProj = osr.SpatialReference()
    outWKTStr = rsgisUtils.readTextFileNoNewLines(outWKT)
    outImgProj.ImportFromWkt(outWKTStr)
    
    geoTransform = inImgDS.GetGeoTransform()
    if geoTransform is None:
        raise Exception('Could read the geotransform from the Input Image: \'' + inputImage + '\'')
    
    xPxlRes = geoTransform[1]
    yPxlRes = geoTransform[5]
    
    inRes = xPxlRes
    if math.fabs(yPxlRes) < math.fabs(xPxlRes):
        inRes = math.fabs(yPxlRes)
    
    xSize = inImgDS.RasterXSize
    ySize = inImgDS.RasterYSize
    
    tlXIn = geoTransform[0]
    tlYIn = geoTransform[3]
    
    brXIn = tlXIn + (xSize * math.fabs(xPxlRes))
    brYIn = tlYIn - (ySize * math.fabs(yPxlRes))
    
    trXIn = brXIn
    trYIn = tlYIn
    
    blXIn = tlXIn
    blYIn = trYIn
    
    numBands = inImgDS.RasterCount
    
    inImgBand = inImgDS.GetRasterBand( 1 );
    gdalDataType = gdal.GetDataTypeName(inImgBand.DataType)
    rsgisDataType = rsgisUtils.getRSGISLibDataType(gdalDataType)

    tlXOut, tlYOut = rsgisUtils.reprojPoint(inImgProj, outImgProj, tlXIn, tlYIn)
    brXOut, brYOut = rsgisUtils.reprojPoint(inImgProj, outImgProj, brXIn, brYIn)
    trXOut, trYOut = rsgisUtils.reprojPoint(inImgProj, outImgProj, trXIn, trYIn)
    blXOut, blYOut = rsgisUtils.reprojPoint(inImgProj, outImgProj, blXIn, blYIn)

    xValsOut = [tlXOut, brXOut, trXOut, blXOut]
    yValsOut = [tlYOut, brYOut, trYOut, blYOut]
    
    xMax = max(xValsOut)
    xMin = min(xValsOut)
    
    yMax = max(yValsOut)
    yMin = min(yValsOut)
    
    outPxlRes = str(outPxlRes).strip()
    outRes = 0.0
    if rsgisUtils.isNumber(outPxlRes):
        outRes = math.fabs(float(outPxlRes))
    elif outPxlRes == 'image':
        outRes = inRes
    elif outPxlRes == 'auto':
        xOutRes = (brXOut - tlXOut) / xSize
        yOutRes = (tlYOut - brYOut) / ySize
        outRes = xOutRes
        if yOutRes < xOutRes:
            outRes = yOutRes
    else: 
        raise Exception('Was not able to defined the output resolution. Check Input: \'' + outPxlRes + '\'')
    
    outTLX = xMin
    outTLY = yMax
    outWidth = int(round((xMax - xMin) / outRes)) + 1
    outHeight = int(round((yMax - yMin) / outRes)) + 1
    
    if snap2Grid:
    
        xLeft = outTLX % outRes
        yLeft = outTLY % outRes
        
        outTLX = (outTLX-xLeft) - (5 * outRes)
        outTLY = ((outTLY-yLeft) + outRes) + (5 * outRes)
        
        outWidth = int(round((xMax - xMin) / outRes)) + 10
        outHeight = int(round((yMax - yMin) / outRes)) + 10
    
    print('Creating blank image')
    rsgislib.imageutils.createBlankImage(outputImage, numBands, outWidth, outHeight, outTLX, outTLY, outRes, noData, "", outWKTStr, gdalformat, rsgisDataType)

    outImgDS = gdal.Open(outputImage, gdal.GA_Update)
    
    for i in range(numBands):
        outImgDS.GetRasterBand(i+1).SetNoDataValue(noData)
    
    print("Performing the reprojection")
    
    wrpOpts = []
    if multicore:
        wrpOpts = gdal.WarpOptions(resampleAlg=eResampleAlg, srcNodata=noData, dstNodata=noData, multithread=True, callback=gdal.TermProgress)
    else:
        wrpOpts = gdal.WarpOptions(resampleAlg=eResampleAlg, srcNodata=noData, dstNodata=noData, multithread=False, callback=gdal.TermProgress)    

    gdal.Warp(outImgDS, inImgDS, options=wrpOpts)

    inImgDS = None
    outImgDS = None    

def subsetPxlBBox(inputimage, outputimage, gdalformat, datatype, xMinPxl, xMaxPxl, yMinPxl, yMaxPxl):
    """
Function to subset an input image using a defined pixel bbox.

:param inputimage: input image to be subset.
:param outputimage: output image file.
:param gdalformat: output image file format
:param datatype: datatype is a rsgislib.TYPE_* value providing the data type of the output image.
:param xMinPxl: min x in pixels
:param xMaxPxl: max x in pixels
:param yMinPxl: min y in pixels
:param yMaxPxl: max y in pixels

"""
    rsgis_utils = rsgislib.RSGISPyUtils()
    bbox = rsgis_utils.getImageBBOX(inputimage)
    xRes, yRes = rsgis_utils.getImageRes(inputimage)
    xSize, ySize = rsgis_utils.getImageSize(inputimage)
    
    if (xMaxPxl > xSize) or (yMaxPxl > ySize):
        raise Exception("The pixel extent defined is bigger than the input image.")
    
    xMin = bbox[0] + (xMinPxl * xRes)
    xMax = bbox[0] + (xMaxPxl * xRes)
    yMin = bbox[2] + (yMinPxl * yRes)
    yMax = bbox[2] + (yMaxPxl * yRes)
    
    rsgislib.imageutils.subsetbbox(inputimage, outputimage, gdalformat, datatype, xMin, xMax, yMin, yMax)

def _runSubset(tileinfo):
    """ Internal function for createTilesMultiCore for multiprocessing Pool. """
    subsetPxlBBox(tileinfo['inputimage'], tileinfo['outfile'], tileinfo['gdalformat'], tileinfo['datatype'], tileinfo['bbox'][0], tileinfo['bbox'][1], tileinfo['bbox'][2], tileinfo['bbox'][3])


def createTilesMultiCore(inputimage, baseimage, width, height, gdalformat, datatype, ext, ncores=1):
    """
Function to generate a set of tiles for the input image.

:param inputimage: input image to be subset.
:param baseimage: output image files base path.
:param width: width in pixels of the tiles.
:param height: height in pixels of the tiles.
:param gdalformat: output image file format
:param datatype: datatype is a rsgislib.TYPE_* value providing the data type of the output image.
:param ext: output file extension to be added to the baseimage path (e.g., kea)
:param ncores: number of cores to be used; uses python multiprocessing module.

"""
    import multiprocessing
    rsgis_utils = rsgislib.RSGISPyUtils()
    xSize, ySize = rsgis_utils.getImageSize(inputimage)
    
    n_full_xtiles = math.floor(xSize/width)
    x_remain_width = xSize - (n_full_xtiles * width)
    n_full_ytiles = math.floor(ySize/height)
    y_remain_height = ySize - (n_full_ytiles * height)
    
    tiles = []
    
    for ytile in range(n_full_ytiles):
        y_pxl_min = ytile * height
        y_pxl_max = y_pxl_min + height
    
        for xtile in range(n_full_xtiles):
            x_pxl_min = xtile * width
            x_pxl_max = x_pxl_min + width
            tiles.append({'tile':'x{0}y{1}'.format(xtile+1, ytile+1), 'bbox':[x_pxl_min, x_pxl_max, y_pxl_min, y_pxl_max]})

        if x_remain_width > 0:
            x_pxl_min = n_full_xtiles * width
            x_pxl_max = x_pxl_min + x_remain_width
            tiles.append({'tile':'x{0}y{1}'.format(n_full_xtiles+1, ytile+1), 'bbox':[x_pxl_min, x_pxl_max, y_pxl_min, y_pxl_max]})
    
    if y_remain_height > 0:
        y_pxl_min = n_full_ytiles * height
        y_pxl_max = y_pxl_min + y_remain_height
        
        for xtile in range(n_full_xtiles):
            x_pxl_min = xtile * width
            x_pxl_max = x_pxl_min + width
            tiles.append({'tile':'x{0}y{1}'.format(xtile+1, n_full_ytiles+1), 'bbox':[x_pxl_min, x_pxl_max, y_pxl_min, y_pxl_max]})

        if x_remain_width > 0:
            x_pxl_min = n_full_xtiles * width
            x_pxl_max = x_pxl_min + x_remain_width
            tiles.append({'tile':'x{0}y{1}'.format(n_full_xtiles+1, n_full_ytiles+1), 'bbox':[x_pxl_min, x_pxl_max, y_pxl_min, y_pxl_max]})
    
    for tile in tiles:
        tile['inputimage'] = inputimage
        tile['outfile'] = "{0}_{1}.{2}".format(baseimage, tile['tile'], ext)
        tile['gdalformat'] = gdalformat
        tile['datatype'] = datatype
    
    poolobj = multiprocessing.Pool(ncores)
    poolobj.map(_runSubset, tiles)


def subsetImgs2CommonExtent(inImagesDict, outShpEnv, gdalformat):
    """
A command to subset a set of images to the same overlapped extent.

Where:

:param inImagesDict: is a list of dictionaries containing values for IN (input image) OUT (output image) and TYPE (data type for output)
:param outShpEnv: is a file path for the output shapefile representing the overlap extent.
:param gdalformat: is the gdal format of the output images.

Example::
    
    from rsgislib import imageutils
    
    inImagesDict = []
    inImagesDict.append({'IN': './Images/Lifeformclip.tif', 'OUT':'./Subsets/Lifeformclip_sub.kea', 'TYPE':rsgislib.TYPE_32INT})
    inImagesDict.append({'IN': './Images/chmclip.tif', 'OUT':'./Subsets/chmclip_sub.kea', 'TYPE':rsgislib.TYPE_32FLOAT})
    inImagesDict.append({'IN': './Images/peakBGclip.tif', 'OUT':'./Subsets/peakBGclip_sub.kea', 'TYPE':rsgislib.TYPE_32FLOAT})
    
    outputVector = 'imgSubExtent.shp'
    imageutils.subsetImgs2CommonExtent(inImagesDict, outputVector, 'KEA')
    
"""
    import rsgislib.vectorutils
    
    inImages = []
    for inImgDict in inImagesDict:
        inImages.append(inImgDict['IN'])
    
    rsgislib.vectorutils.findCommonImgExtent(inImages, outShpEnv, True)
    
    for inImgDict in inImagesDict:
        rsgislib.imageutils.subset(inImgDict['IN'], outShpEnv, inImgDict['OUT'], gdalformat, inImgDict['TYPE'])
    



def buildImgSubDict(globFindImgsStr, outDir, suffix, ext):
    """
Automate building the dictionary of image to be used within the 
subsetImgs2CommonExtent(inImagesDict, outShpEnv, imgFormat) function.

Where:

:param globFindImgsStr: is a string to be passed to the glob module to find the input image files.
:param outDir: is the output directory path for the images.
:param suffix: is a suffix to be appended on to the end of the file name (can be a blank string, i.e., '')
:param ext: is a string with the output file extension

Example::
    
    from rsgislib import imageutils
    
    inImagesDict = imageutils.buildImgSubDict("./Images/*.tif", "./Subsets/", "_sub", ".kea")
    print(inImagesDict)
    
    outputVector = 'imgSubExtent.shp'
    imageutils.subsetImgs2CommonExtent(inImagesDict, outputVector, 'KEA')

"""
    import glob
    import os.path
        
    inImagesDict = []
    
    inputImages = glob.glob(globFindImgsStr)
    if len(inputImages) == 0:
        raise Exception("No images were found using \'" + globFindImgsStr + "\'")
    
    for image in inputImages:
        dataset = gdal.Open(image, gdal.GA_ReadOnly)
        gdalDType = dataset.GetRasterBand(1).DataType
        dataset = None
        datatype = rsgislib.TYPE_32FLOAT
        if gdalDType == gdal.GDT_Byte:
            datatype = rsgislib.TYPE_8UINT
        elif gdalDType == gdal.GDT_Int16:
            datatype = rsgislib.TYPE_16INT
        elif gdalDType == gdal.GDT_Int32:
            datatype = rsgislib.TYPE_32INT
        elif gdalDType == gdal.GDT_UInt16:
            datatype = rsgislib.TYPE_16UINT
        elif gdalDType == gdal.GDT_UInt32:
            datatype = rsgislib.TYPE_32UINT         
        elif gdalDType == gdal.GDT_Float32:
            datatype = rsgislib.TYPE_32FLOAT
        elif gdalDType == gdal.GDT_Float64:
            datatype = rsgislib.TYPE_64FLOAT
        else:
            raise Exception("Data type of the input file was not recognised or known.")
            
        imgBase = os.path.splitext(os.path.basename(image))[0]
        outImg = os.path.join(outDir, (imgBase+suffix+ext))
        inImagesDict.append({'IN':image, 'OUT':outImg, 'TYPE':datatype})

    return inImagesDict


def calcPixelLocations(inputImg, outputImg, gdalformat):
    """
Function which produces a 2 band output image with the X and Y locations of the image pixels.

Where:

:param inputImg: the input reference image
:param outputImg: the output image file name and path (will be same dimensions as the input)
:param gdalformat: the GDAL image file format of the output image file.

"""
    if not haveRIOS:
        raise Exception("The RIOS module required for this function could not be imported\n\t" + riosErr)

    infiles = applier.FilenameAssociations()
    infiles.image1 = inputImg
    outfiles = applier.FilenameAssociations()
    outfiles.outimage = outputImg
    otherargs = applier.OtherInputs()
    aControls = applier.ApplierControls()
    aControls.progress = cuiprogress.CUIProgressBar()
    aControls.drivername = gdalformat
    aControls.omitPyramids = True
    aControls.calcStats = False
    
    def _getXYPxlLocs(info, inputs, outputs, otherargs):
        """
        This is an internal rios function 
        """
        xBlock, yBlock = info.getBlockCoordArrays()
        outputs.outimage = numpy.stack((xBlock,yBlock))

    applier.apply(_getXYPxlLocs, infiles, outfiles, otherargs, controls=aControls)

def mergeExtractedHDF5Data(h5Files, outH5File):
    """
A function to merge a list of HDF files (e.g., from rsgislib.imageutils.extractZoneImageBandValues2HDF)
with the same number of variables (i.e., columns) into a single file. For example, if class training
regions have been sourced from multiple images. 

:param h5Files: a list of input files.
:param outH5File: the output file.

Example::

    inTrainSamples = ['MSS_CloudTrain1.h5', 'MSS_CloudTrain2.h5', 'MSS_CloudTrain3.h5']
    cloudTrainSamples = 'LandsatMSS_CloudTrainingSamples.h5'
    rsgislib.imageutils.mergeExtractedHDF5Data(inTrainSamples, cloudTrainSamples)

"""
    import h5py
    
    first = True
    numVars = 0
    numVals = 0
    for h5File in h5Files:
        fH5 = h5py.File(h5File)
        dataShp = fH5['DATA/DATA'].shape
        if first:
            numVars = dataShp[1]
            first = False
        elif numVars is not dataShp[1]:
            raise rsgislib.RSGISPyException("The number of variables within the inputted HDF5 files was not the same.")
        numVals += dataShp[0]
        fH5.close()
    
    dataArr = numpy.zeros([numVals, numVars], dtype=float)
    
    rowInit = 0
    for h5File in h5Files:
        fH5 = h5py.File(h5File)
        numRows = fH5['DATA/DATA'].shape[0]
        dataArr[rowInit:(rowInit+numRows)] = fH5['DATA/DATA']
        rowInit += numRows
        fH5.close()
    
    fH5Out = h5py.File(outH5File,'w')
    dataGrp = fH5Out.create_group("DATA")
    metaGrp = fH5Out.create_group("META-DATA")
    dataGrp.create_dataset('DATA', data=dataArr, chunks=True, compression="gzip", shuffle=True)
    describDS = metaGrp.create_dataset("DESCRIPTION", (1,), dtype="S10")
    describDS[0] = 'Merged'.encode()
    fH5Out.close()


def doImagesOverlap(image1, image2, overThres=0.0):
    """
Function to test whether two images overlap with one another.
If the images have a difference projection/coordinate system then corners 

:param image1: path to first image
:param image2: path to second image
:param overThres: the amount of overlap required to return true (e.g., at least 1 pixel)

:return: Boolean specifying whether they overlap or not.

Example::

    import rsgislib.imageutils
    img = "/Users/pete/Temp/LandsatStatsImgs/MSS/ClearSkyMsks/LS1MSS_19720823_lat52lon114_r24p218_osgb_clearsky.tif"
    tile = "/Users/pete/Temp/LandsatStatsImgs/MSS/RefImages/LandsatWalesRegion_60m_tile8.kea"
    
    overlap = rsgislib.imageutils.doImagesOverlap(tile, img)
    print("Images Overlap: " + str(overlap))

"""
    overlap = True
    
    projSame = False
    rsgisUtils = rsgislib.RSGISPyUtils()
    if rsgisUtils.doGDALLayersHaveSameProj(image1, image2):
        projSame = True
    
    img1DS = gdal.Open(image1, gdal.GA_ReadOnly)
    if img1DS is None:
        raise rsgislib.RSGISPyException('Could not open image: ' + image1)
        
    img2DS = gdal.Open(image2, gdal.GA_ReadOnly)
    if img2DS is None:
        raise rsgislib.RSGISPyException('Could not open image: ' + image2)

    img1GeoTransform = img1DS.GetGeoTransform()
    if img1GeoTransform is None:
        img1DS = None
        img2DS = None
        raise rsgislib.RSGISPyException('Could not get geotransform: ' + image1)
        
    img2GeoTransform = img2DS.GetGeoTransform()
    if img2GeoTransform is None:
        img1DS = None
        img2DS = None
        raise rsgislib.RSGISPyException('Could not get geotransform: ' + image2)
    
    img1TLX = img1GeoTransform[0]
    img1TLY = img1GeoTransform[3]
    
    img1BRX = img1GeoTransform[0] + (img1DS.RasterXSize * img1GeoTransform[1])
    img1BRY = img1GeoTransform[3] + (img1DS.RasterYSize * img1GeoTransform[5])
    
    img2TLX_orig = img2GeoTransform[0]
    img2TLY_orig = img2GeoTransform[3]
    
    img2BRX_orig = img2GeoTransform[0] + (img2DS.RasterXSize * img2GeoTransform[1])
    img2BRY_orig = img2GeoTransform[3] + (img2DS.RasterYSize * img2GeoTransform[5])
    
    img1EPSG = rsgisUtils.getEPSGCode(image1)
    img2EPSG = rsgisUtils.getEPSGCode(image2)
    
    if projSame:
        img2TLX = img2GeoTransform[0]
        img2TLY = img2GeoTransform[3]
        
        img2BRX = img2GeoTransform[0] + (img2DS.RasterXSize * img2GeoTransform[1])
        img2BRY = img2GeoTransform[3] + (img2DS.RasterYSize * img2GeoTransform[5])
    else:
        inProj = osr.SpatialReference()
        
        if img2EPSG is None:
            wktImg2 = rsgisUtils.getWKTProjFromImage(image2)
            if (wktImg2 is None) or (wktImg2 == ""):
                raise rsgislib.RSGISPyException('Could not retrieve EPSG or WKT for image: ' + image2)
            inProj.ImportFromWkt(wktImg2)
        else:
            inProj.ImportFromEPSG(int(img2EPSG))
        
        outProj = osr.SpatialReference()
        if img1EPSG is None:
            wktImg1 = rsgisUtils.getWKTProjFromImage(image1)
            if (wktImg1 is None) or (wktImg1 == ""):
                raise rsgislib.RSGISPyException('Could not retrieve EPSG or WKT for image: ' + image1)
            outProj.ImportFromWkt(wktImg1)
        else:
            outProj.ImportFromEPSG(int(img1EPSG))
        
        if img1EPSG is None:
            img1EPSG = 0

        img2TLX, img2TLY = rsgisUtils.reprojPoint(inProj, outProj, img2TLX_orig, img2TLY_orig)
        img2BRX, img2BRY = rsgisUtils.reprojPoint(inProj, outProj, img2BRX_orig, img2BRY_orig)
    
    xMin = img1TLX
    xMax = img1BRX
    yMin = img1BRY
    yMax = img1TLY
    
    if img2TLX > xMin:
        xMin = img2TLX
    if img2BRX < xMax:
        xMax = img2BRX
    if img2BRY > yMin:
        yMin = img2BRY
    if img2TLY < yMax:
        yMax = img2TLY
        
    if xMax - xMin <= overThres:
        overlap = False
    elif yMax - yMin <= overThres:
        overlap = False

    return overlap


def generateRandomPxlValsImg(inputImg, outputImg, gdalformat, lowVal, upVal):
    """
Function which produces a 1 band image with random values between lowVal and upVal.

Where:

:param inputImg: the input reference image
:param outputImg: the output image file name and path (will be same dimensions as the input)
:param gdalformat: the GDAL image file format of the output image file.
:param lowVal: lower value
:param upVal: upper value

"""
    if not haveRIOS:
        raise Exception("The RIOS module required for this function could not be imported\n\t" + riosErr)

    infiles = applier.FilenameAssociations()
    infiles.inImg = inputImg
    outfiles = applier.FilenameAssociations()
    outfiles.outimage = outputImg
    otherargs = applier.OtherInputs()
    otherargs.lowVal = lowVal
    otherargs.upVal = upVal
    aControls = applier.ApplierControls()
    aControls.progress = cuiprogress.CUIProgressBar()
    aControls.drivername = gdalformat
    aControls.omitPyramids = True
    aControls.calcStats = False
    
    def _popPxlsRanVals(info, inputs, outputs, otherargs):
        """
        This is an internal rios function for generateRandomPxlValsImg()
        """
        outputs.outimage = numpy.random.random_integers(otherargs.lowVal, high=otherargs.upVal, size=inputs.inImg.shape)
        outputs.outimage = outputs.outimage.astype(numpy.int32, copy=False)
    
    applier.apply(_popPxlsRanVals, infiles, outfiles, otherargs, controls=aControls)


def extractImgPxlSample(inputImg, pxlNSample, noData=None):
    """
A function which extracts a sample of pixels from the 
input image file to a number array.

:param inputImg: the image from which the random sample will be taken.
:param pxlNSample: the sample to be taken (e.g., a value of 100 will sample every 100th, valid (if noData specified), pixel)
:param noData: provide a no data value which is to be ignored during processing. If None then ignored (Default: None)

:return: outputs a numpy array (n sampled values, n bands)

""" 
    # Import the RIOS image reader
    from rios.imagereader import ImageReader
    
    first = True
    reader = ImageReader(inputImg, windowxsize=200, windowysize=200)
    print('Started .0.', end='', flush=True)
    outCount = 10
    for (info, block) in reader:
        if info.getPercent() > outCount:
            print('.'+str(int(outCount))+'.', end='', flush=True)
            outCount = outCount + 10
                
        blkShape = block.shape
        blkBands = block.reshape((blkShape[0], (blkShape[1]*blkShape[2])))
        
        blkBandsTrans = numpy.transpose(blkBands)
        
        if noData is not None:
            blkBandsTrans = blkBandsTrans[(blkBandsTrans!=noData).all(axis=1)]
        
        if blkBandsTrans.shape[0] > 0:
            nSamp = int((blkBandsTrans.shape[0])/pxlNSample)
            nSampRange = numpy.arange(0, nSamp, 1)*pxlNSample
            blkBandsTransSamp = blkBandsTrans[nSampRange]
            
            if first:
                outArr = blkBandsTransSamp
                first = False
            else:
                outArr = numpy.concatenate((outArr, blkBandsTransSamp), axis=0)
    print('. Completed')
    return outArr


def extractImgPxlValsInMsk(img, img_bands, img_mask, img_mask_val, no_data=None):
    """
A function which extracts the image values within a mask for the specified image bands.

:param img: the image from which the random sample will be taken.
:param img_bands: the image bands the values are to be read from.
:param img_mask: the image mask specifying the regions of interest.
:param img_mask_val: the pixel value within the mask defining the region of interest.

:return: outputs a numpy array (n values, n bands)

"""
    # Import the RIOS image reader
    from rios.imagereader import ImageReader
    outArr = None
    first = True
    reader = ImageReader([img, img_mask], windowxsize=200, windowysize=200)
    print('Started .0.', end='', flush=True)
    outCount = 10
    for (info, block) in reader:
        if info.getPercent() > outCount:
            print('.' + str(int(outCount)) + '.', end='', flush=True)
            outCount = outCount + 10

        blk_img = block[0]
        blk_msk = block[1].flatten()
        blk_img_shape = blk_img.shape

        blk_bands = blk_img.reshape((blk_img_shape[0], (blk_img_shape[1] * blk_img_shape[2])))
        band_lst = []
        for band in img_bands:
            if (band > 0) and (band <= blk_bands.shape[0]):
                band_lst.append(blk_bands[band - 1])
            else:
                raise Exception("Band ({}) specified is not within the image".format(band))
        blk_bands_sel = numpy.stack(band_lst, axis=0)
        blk_bands_trans = numpy.transpose(blk_bands_sel)

        if no_data is not None:
            blk_msk = blk_msk[(blk_bands_trans != no_data).all(axis=1)]
            blk_bands_trans = blk_bands_trans[(blk_bands_trans != no_data).all(axis=1)]

        if blk_bands_trans.shape[0] > 0:
            blk_bands_trans = blk_bands_trans[blk_msk == img_mask_val]
            if first:
                out_arr = blk_bands_trans
                first = False
            else:
                out_arr = numpy.concatenate((out_arr, blk_bands_trans), axis=0)
    print('. Completed')
    return out_arr


def getUniqueValues(img, img_band=1):
    """
Find the unique image values within an image band.
Note, the whole image band gets read into memory.

:param img: input image file path
:param img_band: image band to be processed (starts at 1)

:return: array of unique values.

"""
    imgDS = gdal.Open(img)
    if imgDS is None:
        raise Exception("Could not open output image")
    imgBand = imgDS.GetRasterBand(img_band)
    if imgBand is None:
        raise Exception("Could not open output image band ({})".format(img_band))
    valsArr = imgBand.ReadAsArray()
    imgDS = None
    
    uniq_vals = numpy.unique(valsArr)

    return uniq_vals
    
def combineBinaryMasks(msk_imgs_dict, out_img, output_lut, gdalformat='KEA'):
    """
A function which combines up to 8 binary image masks to create a single 
output image with a unique value for each combination of intersecting 
masks. A JSON LUT is also generated to identify the image values to a
'class'.

:param msk_imgs_dict: dict of input images.
:param out_img: output image file.
:param output_lut: output file path to JSON LUT file identifying the image values.
:param gdalformat: output GDAL format (e.g., KEA)

""" 
    import json
    rsgis_utils = rsgislib.RSGISPyUtils()

    in_vals_dict = dict()
    msk_imgs = list()
    for key in msk_imgs_dict.keys():
        msk_imgs.append(msk_imgs_dict[key])
        in_vals_dict[key] = [0,1]
    
    # Generated the combined mask.
    infiles = applier.FilenameAssociations()
    infiles.msk_imgs = msk_imgs
    outfiles = applier.FilenameAssociations()
    outfiles.outimage = out_img
    otherargs = applier.OtherInputs()
    aControls = applier.ApplierControls()
    aControls.progress = cuiprogress.CUIProgressBar()
    aControls.drivername = gdalformat
    aControls.omitPyramids = False
    aControls.calcStats = False
    
    def _combineMsks(info, inputs, outputs, otherargs):
        out_arr = numpy.zeros_like(inputs.msk_imgs[0], dtype=numpy.uint8)        
        out_bit_arr = numpy.unpackbits(out_arr, axis=2)
        img_n = 0
        for img in inputs.msk_imgs:
            for x in range(img.shape[1]):
                for y in range(img.shape[2]):
                    if img[0,x,y] > 1:
                        out_bit_arr[0,x,(8*y)+img_n] = 1
            img_n = img_n + 1
        
        out_arr = numpy.packbits(out_bit_arr, axis=2)
        
        outputs.outimage = out_arr
    applier.apply(_combineMsks, infiles, outfiles, otherargs, controls=aControls)
    
    # find the unique output image files.
    uniq_vals = getUniqueValues(out_img, img_band=1)
    
    # find the powerset of the inputs
    possible_outputs = rsgis_utils.createVarList(in_vals_dict, val_dict=None)
    
    out_poss_lut = dict()
    for poss in possible_outputs:
        val = numpy.zeros(1, dtype=numpy.uint8)
        val_bit_arr = numpy.unpackbits(val, axis=0)
        i = 0
        for key in msk_imgs_dict.keys():
            val_bit_arr[i] = poss[key]
            i = i + 1
        out_arr = numpy.packbits(val_bit_arr)
        if out_arr[0] in uniq_vals:
            out_poss_lut[str(out_arr[0])] = poss
        
    with open(output_lut, 'w') as outJSONfile:
        json.dump(out_poss_lut, outJSONfile, sort_keys=True,indent=4, separators=(',', ': '), ensure_ascii=False)



