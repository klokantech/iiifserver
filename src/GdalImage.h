// GDAL Image class Interface

/*  IIP GDAL

    Copyright (C) 2015 Klokan Technologies GmbH (http://www.klokantech.com/)
    Author: Martin Mikita <martin.mikita@klokantech.com>

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


#ifndef _GDALIMAGE_H
#define _GDALIMAGE_H

#include <cpl_vsi.h>
// from cpl_vsi.h
// typedef GUIntBig  vsi_l_offset
#include <gdal_priv.h>

#include "IIPImage.h"

#include <fstream>

#define TILESIZE 256

extern std::ofstream logfile;


/// Image class for GDAL Images: Inherits from IIPImage. Uses the GDAL library.
class GdalImage : public IIPImage {

 private:

  /// GDAL dataset
  GDALDataset * _ds;

  /**
   * Dynamic decision of need for Overviews
   *  - small images, or images with overviews
   *  - or not compressed TIF
   */
  bool needOverviews();

  /// Main processing function
  /** @param r resolution
      @param l number of quality levels to decode
      @param x x coordinate
      @param y y coordinate
      @param w width of region
      @param h height of region
      @param d buffer to fill
   */
  void process( unsigned int r, int l, int x, int y, unsigned int w, unsigned int h, void* d ) throw (file_error);

  /// Convenience function to delete allocated buffers
  /** @param b pointer to buffer
   */
  void delete_buffer( void* b );

  /// GDAL Library initialised
  static bool initialised;

 public:

   /**
    * Initialise GDAL Library on each supported platform
    */
   static int InitialiseLibrary();

   /**
    * Test if file is supported
    * @param      std::string       Path to the file
    */
   static int IsFileSupported(std::string path);

  /// Constructor
  GdalImage(): IIPImage(){
    tile_width = TILESIZE; tile_height = TILESIZE;
    _ds = NULL;
  };

  /// Constructor
  /** @param path image path
   */
  GdalImage( const std::string& path ): IIPImage( path ){
    tile_width = TILESIZE; tile_height = TILESIZE;
    _ds = NULL;
  };

  /// Copy Constructor
  /** @param image GDAL object
   */
  GdalImage( const GdalImage& image ): IIPImage( image ) {
    _ds = NULL;
  };

  /// Constructor from IIPImage object
  /** @param image IIPImage object
   */
  GdalImage( const IIPImage& image ): IIPImage( image ) {
    tile_width = TILESIZE; tile_height = TILESIZE;
    _ds = NULL;
  };

  /// Assignment Operator
  /** @param TPTImage object
   */
  GdalImage& operator = ( GdalImage image ) {
    if( this != &image ){
      closeImage();
      IIPImage::operator=(image);
    }
    return *this;
  }


  /// Destructor
  ~GdalImage() { closeImage(); };

  /// Overloaded function for opening a TIFF image
  void openImage() throw (file_error);


  /// Overloaded function for loading TIFF image information
  /** @param x horizontal sequence angle
      @param y vertical sequence angle
   */
  void loadImageInfo( int x, int y ) throw (file_error);

  /// Overloaded function for closing a JPEG2000 image
  void closeImage();

  /// Return whether this image type directly handles region decoding
  bool regionDecoding(){ return true; };

  /// Overloaded function for getting a particular tile
  /** @param x horizontal sequence angle
      @param y vertical sequence angle
      @param r resolution
      @param l number of quality layers to decode
      @param t tile number
   */
  RawTile getTile( int x, int y, unsigned int r, int l, unsigned int t ) throw (file_error);

  /// Overloaded function for returning a region for a given angle and resolution
  /** Return a RawTile object: Overloaded by child class.
      @param ha horizontal angle
      @param va vertical angle
      @param r resolution
      @param l number of quality layers to decode
      @param x x coordinate
      @param y y coordinate
      @param w width of region
      @param h height of region
      @param b buffer to fill
   */
  RawTile getRegion( int ha, int va, unsigned int r, int l, int x, int y, unsigned int w, unsigned int h ) throw (file_error);


};


#endif
