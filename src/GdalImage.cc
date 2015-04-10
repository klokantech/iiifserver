/*  IIP Server: GDAL handler



    Copyright (C) 2009-2015 IIPImage.
    Author: Ruven Pillay

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
*/


#include "GdalImage.h"
#include <cmath>
#include <sstream>

bool GdalImage::initialised = false;

#include "Timer.h"
#define DEBUG 1


using namespace std;


#ifdef DEBUG
extern std::ofstream logfile;
#endif

/**
 * Test if file is supported
 * @param      std::string       Path to the file
 */
int GdalImage::IsFileSupported(std::string path)
{
  if(!initialised) {
    GdalImage::InitialiseLibrary();
    initialised = 1;
  }

  GDALDataset * ds = (GDALDataset *) GDALOpen(path.c_str(), GA_ReadOnly);
#ifdef DEBUG
    logfile << "GDAL :: IsFileSupported() " << path << ": "<< (ds != NULL) << endl;
#endif

  if(ds == NULL)
    return 0;
  GDALClose(ds);
  return 1;
}
// ~ static int GdalImage::IsFileSupported(std::string path)


void GdalImage::openImage() throw (file_error)
{
  string filename = getFileName( currentX, currentY );

  // Update our timestamp
  updateTimestamp( filename );

#ifdef DEBUG
  Timer timer;
  timer.start();
#endif

  _ds = (GDALDataset *) GDALOpen(filename.c_str(), GA_ReadOnly);
  if(_ds == NULL) {
    throw file_error( "GDAL :: Unable to open '"+filename+"', maybe not supported.");
  }

  // Load our metadata if not already loaded
  if( bpc == 0 ) {
    loadImageInfo( currentX, currentY );
  }

  if(needOverviews()) {
    throw file_error( "GDAL :: File requires overviews for fast performance!" );
  }

#ifdef DEBUG
  logfile << "GDAL :: openImage() :: " << timer.getTime() << " microseconds" << endl;
#endif

}


/**
 * Dynamic decision of need for Overviews
 *  - small images, or images with overviews
 *  - or not compressed TIF
 */
bool GdalImage::needOverviews()
{
  // Should not happen!
  if(_ds == NULL || _ds->GetRasterCount() < 1)
    return false;

  GDALRasterBand *band = _ds->GetRasterBand(1);

  // TODO test this values
  unsigned int w = _ds->GetRasterXSize();
  unsigned int h = _ds->GetRasterYSize();
  long unsigned MPx = (w * h)/1000000;
#ifdef DEBUG
  logfile << " GDAL :: needOverviews() :: MPx = " << MPx << endl;
#endif
  if(MPx < 25)  // images less than 25MPx are easy and fast
    return false;

  // Get info about Compression
  const char *compr = _ds->GetMetadataItem("COMPRESSION", "IMAGE_STRUCTURE");
  const char *ftype = GDALGetDriverShortName(_ds->GetDriver());
#ifdef DEBUG
  logfile << " GDAL :: needOverviews() :: compression metadate: " << compr << endl;
  logfile << " GDAL :: needOverviews() :: ftype: " << ftype << endl;
#endif

  // Some formats depends on compression method
  if(strcmp(ftype, "GTiff") == 0) {

    if(compr == NULL || strcmp(compr, "JPEG") == 0) {
      // None compress or JPEG are very fast, 300MPx images are quick shown
      if(MPx < 300) {
        return false;
      }
    }
    // Others compression are enough for images lower than 100MPx
    else if(MPx < 100) {
      return false;
    }
  }
  // PDF using Pdfium driver doesn't need overviews for images lower than 200MPx
  else if(strcmp(ftype, "PDF") == 0 && MPx < 200) {
    return false;
  }
  // JPEG files are fast enough if large
  else if(strcmp(ftype, "JPEG") == 0) {
    // JPEG could have overviews, but large JPEGs are too slow
    // Dynamic overviews will handle this properly.
    return (MPx > 200);
  }
  // BSB/KAP format is very fast without overviews
  else if(strcmp(ftype, "BSB") == 0) {
    // TODO this 500MPx is test value, need confirmation
    if(MPx < 500)
      return false;
  }

  // We have overviews
  if(band && (band->GetOverviewCount() > 0 || band->HasArbitraryOverviews()))
    return false;

  return true;
}
// ~ bool GdalImage::needOverviews()


char getBandColorInterpret(GDALDataset* ds, int band)
{
  GDALColorInterp gci = ds->GetRasterBand(band)->GetColorInterpretation();
  switch(gci) {
    case GCI_GrayIndex:
      return 'g';
    case GCI_PaletteIndex:
      return 'p';
    case GCI_RedBand:
      return 'R';
    case GCI_GreenBand:
      return 'G';
    case GCI_BlueBand:
      return 'B';
    case GCI_AlphaBand:
      return 'A';
    case GCI_HueBand:
      return 'H';
    case GCI_LightnessBand:
      return 'L';
    case GCI_SaturationBand:
      return 'S';
    case GCI_CyanBand:
      return 'C';
    case GCI_MagentaBand:
      return 'M';
    case GCI_YellowBand:
      return 'Y';
    case GCI_BlackBand:
      return 'K';
    case GCI_YCbCr_YBand:
      return 'Y';
    case GCI_YCbCr_CbBand:
      return 'b';
    case GCI_YCbCr_CrBand:
      return 'r';
  }
  return '*';
}
// ~ char getBandColorInterpret(GDALDataset* ds, int band)

void GdalImage::loadImageInfo( int seq, int ang ) throw(file_error)
{
  if(_ds == NULL)
    throw file_error("GDAL :: loadImageInfo() :: missing dataset!");
//   jp2_channels j2k_channels;
//   jp2_palette j2k_palette;
//   jp2_resolution j2k_resolution;
//   jp2_colour j2k_colour;
//   kdu_coords layer_size;
//
//   jpx_layer_source jpx_layer = jpx_input.access_layer(0);
//
//   j2k_channels = jpx_layer.access_channels();
//   j2k_resolution = jpx_layer.access_resolution();
//   j2k_colour = jpx_layer.access_colour(0);
//   layer_size = jpx_layer.get_layer_size();
//
//   int cmp, plt, stream_id;
//   j2k_channels.get_colour_mapping(0,cmp,plt,stream_id);
//   j2k_palette = jpx_stream.access_palette();

  unsigned int w = _ds->GetRasterXSize();
  unsigned int h = _ds->GetRasterYSize();

  image_widths.push_back(w);
  image_heights.push_back(h);
  channels = _ds->GetRasterCount();
  if(channels < 1)
    throw file_error("GDAL :: loadImageInfo() :: missing raster bands!");
  GDALRasterBand *band = _ds->GetRasterBand(1);

  switch(band->GetRasterDataType()) {
    case GDT_Byte:
      bpc = 8;
      break;
    case GDT_UInt16:
    case GDT_Int16:
      bpc = 16;
      break;
    case GDT_UInt32:
    case GDT_Int32:
    case GDT_Float32:
      bpc = 32;
      break;
    default:
      throw file_error("GDAL :: loadImageInfo() :: Unsupported raster band data type");
  }

  numResolutions = 0;
#ifdef DEBUG
    logfile << "GDAL :: Resolution : " << w << "x" << h << endl;
#endif
  while(w > tile_width || h > tile_width) {
    w = floor( w/2.0 );
    h = floor( h/2.0 );
    image_widths.push_back(w);
    image_heights.push_back(h);
#ifdef DEBUG
    logfile << "GDAL :: Resolution : " << w << "x" << h << endl;
#endif
    ++numResolutions;
  }

#ifdef DEBUG
  logfile << "GDAL :: DWT Levels: " << numResolutions << endl;
#endif

  // TODO We should check overviews and detect virtual levels vs physical overviews
  virtual_levels = numResolutions - 1;
//   if( n > numResolutions ) virtual_levels = n-numResolutions-1;
//   numResolutions = n;

  if( channels == 1 )
  // Set our colour space - we need transform others colourspaces into RGB
    colourspace = GREYSCALE;
  else
    colourspace = sRGB;

  string cs;
  for(int i=1; i<=_ds->GetRasterCount(); ++i)
    cs.append(1, getBandColorInterpret(_ds, i));

  if(cs == "p")
    cs = "Palette";
  else if(cs == "g")
    cs = "Grayscale";
  else if(cs == "Ybr")
    cs = "YCbCr";
#ifdef DEBUG
  logfile << "GDAL :: " << bpc << " bit data" << endl
    << "GDAL :: " << channels << " channels" << endl
    << "GDAL :: colour space: " << cs << endl;
#endif

  // For bilevel images, force channels to 1 as we sometimes come across such images which claim 3 channels
  if( bpc == 1 ) channels = 1;

  // Get the max and min values for our data type
  //double sminvalue[4], smaxvalue[4];
  for( unsigned int i=0; i<channels; i++ ){
    min.push_back( 0.0 );
    if( bpc > 16 && bpc <= 32 ) max.push_back( 4294967295.0 );
    else if( bpc > 8 && bpc <= 16 ) max.push_back( 65535.0 );
    else max.push_back( 255.0 );
  }

  isSet = true;
}


// Close our image descriptors
void GdalImage::closeImage()
{
#ifdef DEBUG
  Timer timer;
  timer.start();
#endif

  if(_ds != NULL) {
    GDALClose(_ds);
    _ds = NULL;
  }

#ifdef DEBUG
  logfile << "GDAL :: closeImage() :: " << timer.getTime() << " microseconds" << endl;
#endif
}


// Get an invidual tile
RawTile GdalImage::getTile( int seq, int ang, unsigned int res, int layers, unsigned int tile ) throw (file_error)
{

  // Scale up our output bit depth to the nearest factor of 8
  unsigned obpc = bpc;
  if( bpc <= 16 && bpc > 8 ) obpc = 16;
  else if( bpc <= 8 ) obpc = 8;

#ifdef DEBUG
  Timer timer;
  timer.start();
#endif

  if( res > numResolutions ){
    ostringstream tile_no;
    tile_no << "GDAL :: Asked for non-existant resolution: " << res;
    throw file_error( tile_no.str() );
  }

  int vipsres = ( numResolutions - 1 ) - res;

  unsigned int tw = tile_width;
  unsigned int th = tile_height;


  // Get the width and height for last row and column tiles
  unsigned int rem_x = image_widths[vipsres] % tile_width;
  unsigned int rem_y = image_heights[vipsres] % tile_height;


  // Calculate the number of tiles in each direction
  unsigned int ntlx = (image_widths[vipsres] / tw) + (rem_x == 0 ? 0 : 1);
  unsigned int ntly = (image_heights[vipsres] / th) + (rem_y == 0 ? 0 : 1);


  if( tile >= ntlx*ntly ){
    ostringstream tile_no;
    tile_no << "GDAL :: Asked for non-existant tile: " << tile;
    throw file_error( tile_no.str() );
  }

  // Alter the tile size if it's in the last column
  if( ( tile % ntlx == ntlx - 1 ) && ( rem_x != 0 ) ) {
    tw = rem_x;
  }

  // Alter the tile size if it's in the bottom row
  if( ( tile / ntlx == ntly - 1 ) && rem_y != 0 ) {
    th = rem_y;
  }


  // Calculate the pixel offsets for this tile
  int xoffset = (tile % ntlx) * tile_width;
  int yoffset = (unsigned int) floor((double)(tile/ntlx)) * tile_height;

#ifdef DEBUG
  logfile << "GDAL :: Tile size: " << tw << "x" << th << "@" << channels << endl;
#endif


  // Create our Rawtile object and initialize with data
  RawTile rawtile( tile, res, seq, ang, tw, th, channels, obpc );


  // Create our raw tile buffer and initialize some values
  if( obpc == 16 ) rawtile.data = new unsigned short[tw*th*channels];
  else if( obpc == 8 ) rawtile.data = new unsigned char[tw*th*channels];
  else throw file_error( "GDAL :: Unsupported number of bits" );

  rawtile.dataLength = tw*th*channels*obpc/8;
  rawtile.filename = getImagePath();
  rawtile.timestamp = timestamp;

  // Process the tile
  process( res, layers, xoffset, yoffset, tw, th, rawtile.data );


#ifdef DEBUG
  //logfile << "GDAL :: bytes parsed: " << codestream.get_total_bytes(true) << endl;
  logfile << "GDAL :: getTile() :: " << timer.getTime() << " microseconds" << endl;
#endif

  return rawtile;

}


// Get an entire region and not just a tile
RawTile GdalImage::getRegion( int seq, int ang, unsigned int res, int layers, int x, int y, unsigned int w, unsigned int h ) throw (file_error)
{
  // Scale up our output bit depth to the nearest factor of 8
  unsigned int obpc = bpc;
  if( bpc <= 16 && bpc > 8 ) obpc = 16;
  else if( bpc <= 8 ) obpc = 8;

#ifdef DEBUG
  Timer timer;
  timer.start();
#endif

  RawTile rawtile( 0, res, seq, ang, w, h, channels, obpc );

  if( obpc == 16 ) rawtile.data = new unsigned short[w*h*channels];
  else if( obpc == 8 ) rawtile.data = new unsigned char[w*h*channels];
  else throw file_error( "GDAL :: Unsupported number of bits" );

  rawtile.dataLength = w*h*channels*obpc/8;
  rawtile.filename = getImagePath();
  rawtile.timestamp = timestamp;

  process( res, layers, x, y, w, h, rawtile.data );

#ifdef DEBUG
  logfile << "GDAL :: getRegion() :: " << timer.getTime() << " microseconds" << endl;
#endif

  return rawtile;

}


// Main processing function
void GdalImage::process( unsigned int res, int layers, int xoffset, int yoffset,
                         unsigned int tw, unsigned int th, void *d ) throw (file_error)
{
  // Scale up our output bit depth to the nearest factor of 8
  unsigned int obpc = bpc;
  if( bpc <= 16 && bpc > 8 ) obpc = 16;
  else if( bpc <= 8 ) obpc = 8;
/*
  /// Detect req area outside image area
  // width of tile outside raster width
  if( (rxoffset + rtw) > _ds->GetRasterXSize() )
  {
    int tx = tw;
    tw = _img->getWidth() - _x_offset;
    rtw = tw
  }
  // height of tile outside raster width
  if( (_y_offset + _y_size) > _img->getHeight() )
  {
    int ty = _y_size;
    _y_size = _img->getHeight() - _y_offset;
    _read_height = ceil(((float)_tile_height* _y_size) / ty);
  }*/

  // Get read area for requested resolution
  unsigned int rtw = tw << (numResolutions - res);
  unsigned int rth = th << (numResolutions - res);
  int rxoffset = xoffset << (numResolutions - res);
  int ryoffset = yoffset << (numResolutions - res);

//   int vipsres = ( numResolutions - 1 ) - res;
//
//   // Handle virtual resolutions
//   if( res < virtual_levels ){
//     unsigned int factor = 1 << (virtual_levels-res);
//     xoffset *= factor;
//     yoffset *= factor;
//     tw *= factor;
//     th *= factor;
//     vipsres = numResolutions - 1 - virtual_levels;
// #ifdef DEBUG
//   logfile << "GDAL :: using smallest existing resolution " << virtual_levels << endl;
// #endif
//   }

  // Size for read RGBA data from File
//   _band_size = _read_width * _read_height * sizeof(GByte);
  GUIntBig pixelSpace = (obpc/8) * _ds->GetRasterCount();
  GUIntBig lineSpace = tw * pixelSpace;
  GUIntBig bandSpace = (obpc/8);

//   int timeStartInit = GetMilliCount();
//   int partly = !had_alpha && _write_width == _read_width && _write_height == _read_height;
//   DEBUG2(TID_STR" is partly %d \n", TID_ARG, partly);
//   // Prepare and initialize output buffer
//   for(int i=0; i<_band_count; ++i) {
//     // rgba[0] was malloced above
//     if(i > 0)
//       _readBuff[i] = _readBuff[i-1] + _band_size;
//     for(GUIntBig j=0; j<_band_size; ++j) {
//       // initialize array - white for every band, or 00 for alpha (transparency)
//       _readBuff[i][j] = (i == (_alpha_band-1) && had_alpha) ? 0x00 : 0xFF;
//       // Set full transparency outside read image (if not had alpha)
//       if(partly) {
//         int r = j % _tile_width;
//         int c = j / _tile_width;
//         if(r >= _read_width || c >= _read_height)
//           _readBuff[i][j] = (_alpha_band-1 == i) ? 0x00 : 0xFF;
//       }
//     }
//   }
//   DEBUG3(TID_STR" RGBA init took %d ms\n",TID_ARG, GetMilliSpan( timeStartInit ));

  GDALDataType dt;
  if(obpc == 16)
    dt = GDT_UInt16;
  else
    dt = GDT_Byte;

#ifdef DEBUG
  logfile << "GDAL :: process: " << res << ", " << layers << ", "
        << xoffset << ", " << yoffset << ", " << tw << ", " << th << endl;
  logfile << "GDAL :: RasterIO " << rxoffset << ", " << ryoffset << ", "
        << rtw << ", " << rth << ", " << tw << ", " << th << ", "
        << _ds->GetRasterCount()
        << "\n     " << pixelSpace << ", " << lineSpace << ", " << bandSpace << endl;
#endif

//   int timeStart = GetMilliCount();
//   DEBUG2(TID_STR" RasterIO(%d, %d, %d, %d, buff, %d, %d, bands %d)\n",
//          TID_ARG, _x_offset, _y_offset, _x_size, _y_size,
//           _read_width, _read_height, _img->getDatasetP()->GetRasterCount());
  CPLErr rc = _ds->RasterIO(GF_Read,
      rxoffset, ryoffset, rtw, rth,
      d, tw, th, dt,
      _ds->GetRasterCount(), NULL,
      pixelSpace, lineSpace, bandSpace);
//   DEBUG3(TID_STR" ReadRaster took %d ms [%d]\n",TID_ARG, GetMilliSpan( timeStart ), rc);

  // We cannot continue, free memory
  if(rc != CE_None)
  {
//     DEBUG2("QGTile::readTile RasterIO failed "TID_STR"!\n", TID_ARG);
//     DEBUG2(" - error %s \n", CPLGetLastErrorMsg());
//     ErrorSet("QGTile::readTile: RasterIO failed "TID_STR"!", TID_ARG);
    throw file_error("GDAL :: process() :: Requested region failed to read raster");
  }

#ifdef DEBUG
//     logfile << "GDAL :: decompressor starting" << endl;
//
//     logfile << "GDAL :: requested region on high resolution canvas: position: "
// 	    << image_dims.pos.x << "x" << image_dims.pos.y
// 	    << ". size: " << image_dims.size.x << "x" << image_dims.size.y << endl;
//
//     logfile << "GDAL :: mapped resolution region size: " << comp_dims.size.x << "x" << comp_dims.size.y << endl;
//     logfile << "GDAL :: About to pull stripes" << endl;
#endif


#ifdef DEBUG
    logfile << "GDAL :: raster completed" << endl;
#endif

}


// Delete our buffers
void GdalImage::delete_buffer( void* buffer ){
  if( buffer ){
//     if( bpc <= 16 && bpc > 8 ) delete[] (kdu_uint16*) buffer;
//     else if( bpc<=8 ) delete[] (kdu_byte*) buffer;
  }


}
