/****************************************************************************
 **
 ** This file is part of the project QtGDAL
 **
 ** Author: Martin Mikita <martin@mikita.eu>
 **
 ** School:   FIT VUT Brno [2009-2015]
 ** Copyright (c) 2014 Martin Mikita & Klokan Technologies GmbH.
 **           All rights reserved.
 **
 ****************************************************************************/
/**
 * File struct with methods
 *
 *  - Opening file with GDAL
 *  - Reading tile
 */

#include "GdalImage.h"

#ifdef DEBUG
extern std::ofstream logfile;
#endif

#include <cstdio>
#include <cstring>
#include <cpl_conv.h>
#include <gdal_priv.h>
#include <ogr_api.h>

#define BUFFER_SIZE       4096

/**
 * Check existence of the file in the directory
 */
static int file_exists_in(char* file, char* dir)
{
  FILE* test_file;
  char* test_path;
  int exists;
  test_path = (char*)CPLMalloc(strlen(dir) + strlen(file) + 5);
  strcpy(test_path, dir);
#ifdef _WIN32
  strcat(test_path, "\\");
#else
  strcat(test_path, "/");
#endif  // ~ !_WIN32
  strcat(test_path, file);

  test_file = fopen(test_path, "r");
  exists = (test_file != NULL);
  CPLFree(test_path);
  if(exists)
    fclose(test_file);
  return exists;
}
// ~ static int file_exists_in(char* file, char* dir)

/// ========== Windows specific functions ==========
#ifdef _WIN32

#include <Windows.h>

/**
 * Intialize Windows specific
 */
static int initialize_win()
{
  char* exec_path;
  char* dir_path;
  char* env;
  char* etmp;

  exec_path = (char*) CPLMalloc(BUFFER_SIZE);
  if (!CPLGetExecPath(exec_path, BUFFER_SIZE - 1))
  {
    CPLFree(exec_path);
    throw file_error( "GDAL :: Can't get path to the executable.");
  }

  dir_path = (char*) CPLGetDirname(exec_path);
  env = (char*) CPLMalloc(strlen(dir_path) + 48);
  etmp = (char*) CPLMalloc(strlen(dir_path) + 48);

  /// Setting GDAL_DATA
  strcpy(env, dir_path);
  strcat(env, "\\gdal-data");
  // Set GDAL_DATA only if there exists epsg.wkt file!
  if(file_exists_in((char*)"epsg.wkt", env))
    CPLSetConfigOption("GDAL_DATA", env);

  /// Setting GDAL_DRIVER_PATH as plugins for GDAL
  strcpy(env, dir_path);
  strcat(env, "\\gdal-plugins");
  // Set GDAL_DRIVER_PATH only if there exists gdal_MrSID.dll file!
  // TODO some another file for testing?
  if(file_exists_in((char*)"gdal_MrSID.dll", env))
    CPLSetConfigOption("GDAL_DRIVER_PATH", env);

  /// Setting PROJ_LIB with putenv
  // WARNING !!! PROJ_LIB has to be set with _putenv !!!
  strcpy(etmp, dir_path);
  strcat(etmp, "\\proj-data");
  strcpy(env, "PROJ_LIB=");
  strcat(env, etmp);
  // Set PROJ_LIB only if there exists epsg file!
  if(file_exists_in((char*)"epsg", etmp))
    _putenv(CPLStrdup(env));

  /// Setting GDAL_SKIP to skip drivers
#ifdef GDAL_USE_KAKADU_JP2
  CPLFree(env);
  env = (char*) CPLMalloc(150);
  // Ignore every JPEG2000 if used KAKADU library
  strcpy(env, "JP2MrSID JP2ECW ");
  strcat(env, "JPEG2000 JP2OpenJPEG");
  CPLSetConfigOption("GDAL_SKIP", env);
#endif

  // free memory, strings were copied inside functions
  CPLFree(env);
  CPLFree(etmp);
  CPLFree(exec_path);

  return 0;
}
// ~ static int initialize_win()

#endif  // ~ _WIN32

/// ========== Mac OS X specific functions ==========
#ifdef __APPLE__

#include <mach-o/dyld.h>

/**
 * Intialize Mac OS X specific
 */
static int initialize_mac()
{
  char bundle_path[2048];
  uint32_t size = 2048;
  char* dir_path;
  char* env;
  char* etmp;

  // Get required size for buffer
  // _NSGetExecutablePath(NULL,  &size);
  // bundle_path = (char*) CPLMalloc(size+2);
  if (_NSGetExecutablePath(bundle_path,  &size) != 0) {
    // CPLFree(bundle_path);
    throw file_error( "Can't get path to the executable. Required space not satisfied!");
  }

  // Not bundled: dir/exec
  dir_path = (char*) CPLGetDirname(bundle_path);
#ifdef MAC_APP_BUNDLED
  // Bundled: dir.app/Contents/MacOS/exec
  dir_path = (char*) CPLGetDirname(dir_path);
#endif // ~ MAC_APP_BUNDLED
  env = (char*) CPLMalloc(strlen(dir_path) + 128);
  etmp = (char*) CPLMalloc(strlen(dir_path) + 128);

  /// Setting GDAL_DATA
  strcpy(env, dir_path);
#ifdef MAC_APP_BUNDLED
  strcat(env, "/Frameworks");
#endif // ~ MAC_APP_BUNDLED
  strcat(env, "/GDAL.framework/Resources/gdal");
  // Set GDAL_DATA only if there exists epsg.wkt file!
  if(file_exists_in("epsg.wkt", env))
    CPLSetConfigOption("GDAL_DATA", env);

  /// Setting GDAL_DRIVER_PATH as plugins for GDAL
  strcpy(env, dir_path);
#ifdef MAC_APP_BUNDLED
  strcat(env, "/Frameworks");
#endif // ~ MAC_APP_BUNDLED
  strcat(env, "/GDAL.framework/Versions/Current/PlugIns");
  // Set GDAL_DRIVER_PATH only if there exists gdal_MrSID.dylib file!
  if(file_exists_in("gdal_MrSID.dylib", env))
    CPLSetConfigOption("GDAL_DRIVER_PATH", env);

  /// Setting PROJ_LIB with putenv
  // WARNING !!! PROJ_LIB has to be set with _putenv !!!
  strcpy(etmp, dir_path);
#ifdef MAC_APP_BUNDLED
  strcat(etmp, "/Frameworks");
#endif // ~ MAC_APP_BUNDLED
  strcat(etmp, "/PROJ.framework/Resources/proj");
  strcpy(env, "PROJ_LIB=");
  strcat(env, etmp);
  // Set PROJ_LIB only if there exists epsg file!
  if(file_exists_in("epsg", etmp))
    putenv(CPLStrdup(env));

  /// Setting GDAL_SKIP to skip drivers
#ifdef GDAL_USE_KAKADU_JP2
  CPLFree(env);
  env = (char*) CPLMalloc(150);
  // Ignore every JPEG2000 if used KAKADU library
  strcpy(env, "JP2MrSID JP2ECW ");
  strcat(env, "JPEG2000 JP2OpenJPEG");
  CPLSetConfigOption("GDAL_SKIP", env);
#endif

  // free memory, strings were copied inside functions
  CPLFree(env);
  CPLFree(etmp);
  //CPLFree(bundle_path);

  return 0;
}
// ~ static int initialize_mac()

#endif  // ~ __APPLE__


/**
 * Initialise GDAL Library on each supported platform
 */
int GdalImage::InitialiseLibrary()
{
#ifdef _WIN32
  if(initialize_win() != 0)
    return -1;
#endif  // ~ _WIN32

#ifdef __APPLE__
  if(initialize_mac() != 0)
    return -1;
#endif  // ~ __APPLE__

  GDALAllRegister();
  OGRRegisterAll();
  CPLSetErrorHandler(CPLQuietErrorHandler);

#ifdef DEBUG
    logfile << "GDAL :: InitialiseLibrary() succeeded" << endl;
#endif

  return 0;
}
// ~ int GdalImage::InitialiseLibrary()
