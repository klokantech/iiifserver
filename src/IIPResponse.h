/*
    IIP Response Handler Class

    Copyright (C) 2003-2012 Ruven Pillay.

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


#ifndef _IIPRESPONSE_H
#define _IIPRESPONSE_H

#ifndef VERSION
#define VERSION "0.9.9.9"
#endif

// Fix missing snprintf in Windows
#if _MSC_VER
#define snprintf _snprintf
#endif


#include <string>


/// Class to handle non-image IIP responses including errors

class IIPResponse{


 private:

  std::string server;              // Server header
  std::string modified;            // Last modified header
  std::string cache;               // Cache control header
  std::string mimeType;            // Mime type header
  std::string eof;                 // End of response delimitter eg "\r\n"
  std::string protocol;            // IIP protocol version
  std::string responseBody;        // The main response
  std::string error;               // Error message
  std::string corsAccessControl;   // Access-Control-Allow-Origin header
  std::string status;              // Status code of HTTP response (e.g. 200, 404, etc.)
  std::string contentDisposition;  // Content-Disposition header
  std::string transferEncoding;    // Transfer-Encoding header
  int contentLength;               // Content-Length header
  bool sent;                       // Indicate whether a response has been sent


 public:

  /// Constructor
  IIPResponse();


  /// Set the IIP protocol version
  /** @param p IIP protocol version */
  void setProtocol( const std::string& p ) { protocol = p; };

  /// Returns the IIP protocol version
  std::string getProtocol() { return protocol; };


  ///Set the Cache-Control header (pass only value, without 'Cache-Control:')
  /** @param c Cache-Control mechanism as a HTTP RFC 2616 (e.g. no-cache) */
  void setCacheControl( const std::string& c ) { cache = c; };

  /// Return the Cache-Control header value (without 'Cache-Control:')
  std::string getCacheControl() { return cache; };


  /// Set the Content-Type header (pass only value, without 'Content-Type:')
  /** @param mime mime type of sent data as a HTTP RFC  2045-2047, 2049, 4288 or 4289 (e.g. text/html) */
  void setContentType( const std::string& mime ) { mimeType = mime; };

  /// Return the Content-Type header value (without 'Content-Type:')
  std::string getContentType() { return mimeType; };


  /// Set the Content-Length header (pass only value, without 'Content-Length:')
  /** @param cl length of the request body in octets (8-bit bytes) */
  void setContentLength(int cl) { contentLength = cl; };

  /// Return the Content-Length header value (without 'Content-Length:')
  int getContentLength() { return contentLength; };


  /// Set the Content-Disposition header (pass only value, without 'Content-Disposition:')
  /** @param cd Content-Disposition of HTTP response as a HTTP RFC  2616 */
  void setContentDisposition( const std::string& cd ) {
    contentDisposition = cd;
  };

  ///Return the Content-Disposition header value (without 'Content-Disposition:')
  std::string getContentDisposition() { return contentDisposition; };


  ///Set the Access-Control-Allow-Origin header (pass only value, without 'Access-Control-Allow-Origin:')
  /** @param cors Content-Disposition of HTTP response as a HTTP RFC  2616 */
  void setAccessControlAllowOrigin(const std::string& cors) {
    corsAccessControl = cors;
  };

  ///Return the Access-Control-Allow-Origin header (without 'Access-Control-Allow-Origin:')
  std::string getAccessControlAllowOrigin() { return corsAccessControl; };


  ///Set the Status header
  /** @param s status code of HTTP response as a HTTP RFC  2616 (e.g. 200,404...) */
  void setStatus(const std::string& s) { status = s; };

  ///Return the Status header value (without 'Status:')
  std::string getStatus() { return status; };

  /// Set the Last-Modified header (without 'Last-Modified:')
  /** @param m Last-modifed date as a HTTP RFC 1123 formatted timestamp */
  void setLastModified( const std::string& m ) { modified = m; };

  ///Return the Last-Modified header value (without 'Last-Modified:')
  std::string getLastModified() { return modified; };


  /// Set the Transfer-Encoding header (without 'Transfer-Encoding:')
  /** @param te form of encoding used to safely transfer the entity to the user */
  void setTransferEncoding( const std::string& te ) { transferEncoding = te; };

  ///Return the Transfer-Encoding header value (without 'Transfer-Encoding:')
  std::string getTransferEncoding() { return transferEncoding; };

  /// Set an error
  /** @param code error code
      @param arg the argument supplied by the client
  */
  void setError( const std::string& code, const std::string& arg );

  /// Indicate whether we have an error message
  bool errorIsSet(){
    if( error.length() ) return true;
    else return false;
  }

  /// Set the sent flag indicating that some sort of response has been sent
  void setImageSent() { sent = true; };

  /// Indicate whether a response has been sent
  bool imageSent() { return sent; };

    /// Indicate whether this object has had any arguments passed to it
  bool isSet(){
    if( error.length() || responseBody.length() || protocol.length() ) return true;
    else return false;
  }


  /// Add a response string
  /** @param r response string */
  void addResponse( const std::string& r ); 


  /// Add a response string
  /** @param c response string */
  void addResponse( const char* c );


  /// Add a response string
  /** @param c response string
      @param a integer value
   */
  void addResponse( const char* c, int a );


  /// Add a response string
  /** @param c response string
      @param a string reply
   */
  void addResponse( std::string c, const std::string& a );


  /// Add a response string
  /** @param c response string
      @param a integer value
      @param b another integer value
   */
  void addResponse( const char* c, int a, int b );


  /// Get a formatted string to send back
  std::string formatResponse();


  /// Display our advertising banner ;-)
  /** @param version server version */
  std::string getAdvert( const std::string& version );


};


#endif
