/** @file vol_geom.c
 * Volograms Geometry Decoding API
 *
 * vol_geom  | .vol Geometry Decoding API
 * --------- | ---------------------
 * Version   | 0.11.1
 * Authors   | See matching header file.
 * Copyright | 2021, Volograms (http://volograms.com/)
 * Language  | C99
 * Files     | 2
 * Licence   | The MIT License. See LICENSE.md for details.
 */

#include "vol_geom.h"
#include <assert.h>
#include <inttypes.h> // 64-bit printfs (PRId64 for integer, PRIu64 for unsigned int, PRIx64 for hex)
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h> // Used for reading file sizes.
#include <sys/types.h>

// NOTE: ftello() and fseeko() are replace ftell(), fseek(), and their Windows equivalents, to support 64-bit indices to >2GB files.
#if defined( _WIN32 ) || defined( _WIN64 )
#define vol_geom_stat64 _stat64
#define vol_geom_stat64_t __stat64
#define vol_geom_fseeko _fseeki64
#define vol_geom_ftello _ftelli64
#else
#define vol_geom_stat64 stat
#define vol_geom_stat64_t stat
#define vol_geom_fseeko fseeko
#define vol_geom_ftello ftello
#endif

#define VOL_GEOM_LOG_STR_MAX_LEN 512 // Careful - this is stored on the stack to be thread and memory-safe so don't make it too large.
/// File header section size in bytes. Used in sanity checks to test for corrupted files that are below minimum sizes expected.
#define VOL_GEOM_FILE_HDR_V10_MIN_SZ 24 /// "VOLS" (4 bytes) + 4 string length bytes + 4 ints in v10 hdr.
/// File header section size in bytes. Used in sanity checks to test for corrupted files that are below minimum sizes expected.
#define VOL_GEOM_FRAME_MIN_SZ 17 /// 3 ints, 1 byte, 1 int inside vertices array. the rest are optional

/// Helper struct to refer to an entire file loaded from disk via `_read_file()`.
typedef struct vol_geom_file_record_t {
  uint8_t* byte_ptr;  // Pointer to contents of file.
  vol_geom_size_t sz; // Size of file in bytes.
} vol_geom_file_record_t;

static void _default_logger( vol_geom_log_type_t log_type, const char* message_str ) {
  FILE* stream_ptr = ( VOL_GEOM_LOG_TYPE_ERROR == log_type || VOL_GEOM_LOG_TYPE_WARNING == log_type ) ? stderr : stdout;
  fprintf( stream_ptr, "%s", message_str );
}

static void ( *_logger_ptr )( vol_geom_log_type_t log_type, const char* message_str ) = _default_logger;

// This function is used in this file as a printf-style logger. It converts that format to a simple string and passes it to _logger_ptr.
static void _vol_loggerf( vol_geom_log_type_t log_type, const char* message_str, ... ) {
  if ( !_logger_ptr ) { return; }
  char log_str[VOL_GEOM_LOG_STR_MAX_LEN];
  log_str[0] = '\0';
  va_list arg_ptr; // using va_args lets us make sure any printf-style formatting values are properly written into the string.
  va_start( arg_ptr, message_str );
  vsnprintf( log_str, VOL_GEOM_LOG_STR_MAX_LEN - 1, message_str, arg_ptr );
  va_end( arg_ptr );
  _logger_ptr( log_type, log_str );
}

/** Helper function to check if a path is a file (i.e. is not a directory). */
static bool _is_file( const char* path ) {
  struct vol_geom_stat64_t path_stat;
  if ( 0 != vol_geom_stat64( path, &path_stat ) ) { return false; }
#ifdef _MSC_VER
  return path_stat.st_mode & _S_IFREG;
#else /* POSIX */
  return S_ISREG( path_stat.st_mode );
#endif
}

/** Helper function to check the actual size of a file on disk.
 * @param filename Input: path to a file.
 * @param sz_ptr   Output: size of the file in bytes.
 * @return         true if a valid file was found and a size could be obtained.
 */
static bool _get_file_sz( const char* filename, vol_geom_size_t* sz_ptr ) {
  struct vol_geom_stat64_t stbuf;
  if ( !_is_file( filename ) ) { return false; }
  if ( 0 != vol_geom_stat64( filename, &stbuf ) ) { return false; }
  *sz_ptr = stbuf.st_size;
  return true;
}

/** Helper function to read an entire file into an array of bytes within struct pointed to by `fr_ptr`.
 * @param max_bytes If zero then read the entire file, otherwise read up to max_bytes into memory.
 */
static bool _read_file( const char* filename, vol_geom_file_record_t* fr_ptr, vol_geom_size_t max_bytes ) {
  FILE* f_ptr = NULL;

  if ( !filename || !fr_ptr ) { goto vol_geom_read_file_failed; }
  if ( !_get_file_sz( filename, &fr_ptr->sz ) ) { goto vol_geom_read_file_failed; }
  fr_ptr->sz = ( 0 == max_bytes || fr_ptr->sz < max_bytes ) ? fr_ptr->sz : max_bytes;

  _vol_loggerf( VOL_GEOM_LOG_TYPE_DEBUG, "Allocating %" PRId64 " bytes for reading file\n", fr_ptr->sz );
  fr_ptr->byte_ptr = NULL;
  fr_ptr->byte_ptr = malloc( (size_t)fr_ptr->sz );
  if ( !fr_ptr->byte_ptr ) { goto vol_geom_read_file_failed; }

  f_ptr = fopen( filename, "rb" );
  if ( !f_ptr ) { goto vol_geom_read_file_failed; }
  vol_geom_size_t nr = fread( fr_ptr->byte_ptr, fr_ptr->sz, 1, f_ptr );
  if ( 1 != nr ) { goto vol_geom_read_file_failed; }
  fclose( f_ptr );

  return true;
vol_geom_read_file_failed:
  if ( f_ptr ) { fclose( f_ptr ); }
  if ( fr_ptr ) {
    if ( fr_ptr->byte_ptr ) {
      free( fr_ptr->byte_ptr );
      fr_ptr->byte_ptr = NULL;
    }
    fr_ptr->sz = 0;
  }
  return false;
}

/** Helper function to read Unity-style strings, specified in VOL format, from a loaded file. */
static bool _read_short_str( const uint8_t* data_ptr, uint32_t data_sz, vol_geom_size_t offset, vol_geom_short_str_t* sstr ) {
  if ( !data_ptr || !sstr ) { return false; }
  if ( offset >= data_sz ) { return false; } // OOB

  sstr->sz = data_ptr[offset];               // assumes the 1-byte length
  if ( sstr->sz > 127 ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: string length %i given is > 127\n", (int)sstr->sz );
    return false;
  }
  if ( offset + sstr->sz >= data_sz ) { return false; } // OOB
  memcpy( sstr->bytes, &data_ptr[offset + 1], sstr->sz );
  sstr->bytes[sstr->sz] = '\0';

  return true;
}

static bool _read_vol_frame( const vol_geom_info_t* info_ptr, uint32_t frame_idx, vol_geom_frame_data_t* frame_data_ptr ) {
  assert( info_ptr && info_ptr->preallocated_frame_blob_ptr && frame_data_ptr );
  if ( !info_ptr || !info_ptr->preallocated_frame_blob_ptr || !frame_data_ptr ) { return false; }
  if ( frame_idx >= info_ptr->hdr.frame_count ) { return false; }

  *frame_data_ptr = ( vol_geom_frame_data_t ){ .block_data_sz = 0 };

  uint8_t* byte_blob_ptr         = (uint8_t*)info_ptr->preallocated_frame_blob_ptr;
  frame_data_ptr->block_data_ptr = &byte_blob_ptr[info_ptr->frames_directory_ptr[frame_idx].hdr_sz];
  frame_data_ptr->block_data_sz  = info_ptr->frames_directory_ptr[frame_idx].corrected_payload_sz;

  {
    // start within the frame's memory but after its frame header and at the start of mesh data
    vol_geom_size_t curr_offset = 0;

    { // vertices
      if ( frame_data_ptr->block_data_sz < ( curr_offset + (vol_geom_size_t)sizeof( uint32_t ) + (vol_geom_size_t)frame_data_ptr->vertices_sz ) ) {
        return false;
      }

      memcpy( &frame_data_ptr->vertices_sz, &frame_data_ptr->block_data_ptr[curr_offset], sizeof( uint32_t ) );
      curr_offset += (vol_geom_size_t)sizeof( uint32_t );
      frame_data_ptr->vertices_offset = curr_offset;
      curr_offset += (vol_geom_size_t)frame_data_ptr->vertices_sz;
    }

    // normals
    if ( info_ptr->hdr.normals && info_ptr->hdr.version >= 11 ) {
      if ( frame_data_ptr->block_data_sz < ( curr_offset + (vol_geom_size_t)sizeof( uint32_t ) + (vol_geom_size_t)frame_data_ptr->normals_sz ) ) {
        return false;
      }

      memcpy( &frame_data_ptr->normals_sz, &frame_data_ptr->block_data_ptr[curr_offset], sizeof( uint32_t ) );
      curr_offset += (vol_geom_size_t)sizeof( uint32_t );
      frame_data_ptr->normals_offset = curr_offset;
      curr_offset += (vol_geom_size_t)frame_data_ptr->normals_sz;
    }

    // indices and UVs
    if ( info_ptr->frame_headers_ptr[frame_idx].keyframe == 1 || ( info_ptr->hdr.version >= 12 && info_ptr->frame_headers_ptr[frame_idx].keyframe == 2 ) ) {
      { // indices
        if ( frame_data_ptr->block_data_sz < ( curr_offset + (vol_geom_size_t)sizeof( uint32_t ) + (vol_geom_size_t)frame_data_ptr->indices_sz ) ) {
          return false;
        }

        memcpy( &frame_data_ptr->indices_sz, &frame_data_ptr->block_data_ptr[curr_offset], sizeof( uint32_t ) );
        curr_offset += (vol_geom_size_t)sizeof( uint32_t );
        frame_data_ptr->indices_offset = curr_offset;
        curr_offset += (vol_geom_size_t)frame_data_ptr->indices_sz;
      }

      { // UVs
        if ( frame_data_ptr->block_data_sz < ( curr_offset + (vol_geom_size_t)sizeof( uint32_t ) + (vol_geom_size_t)frame_data_ptr->uvs_sz ) ) { return false; }

        memcpy( &frame_data_ptr->uvs_sz, &frame_data_ptr->block_data_ptr[curr_offset], sizeof( uint32_t ) );
        curr_offset += (vol_geom_size_t)sizeof( uint32_t );
        frame_data_ptr->uvs_offset = curr_offset;
        curr_offset += (vol_geom_size_t)frame_data_ptr->uvs_sz;
      }
    } // endif indices & UVs

    // texture
    // NOTE(Anton) not tested since we aren't using embedded textures at the moment.
    if ( info_ptr->hdr.version >= 11 && info_ptr->hdr.textured ) {
      if ( frame_data_ptr->block_data_sz < ( curr_offset + (vol_geom_size_t)sizeof( uint32_t ) + (vol_geom_size_t)frame_data_ptr->texture_sz ) ) {
        return false;
      }

      memcpy( &frame_data_ptr->texture_sz, &frame_data_ptr->block_data_ptr[curr_offset], sizeof( uint32_t ) );
      curr_offset += (vol_geom_size_t)sizeof( uint32_t );
      frame_data_ptr->texture_offset = curr_offset;
      curr_offset += (vol_geom_size_t)frame_data_ptr->texture_sz;
    }
  } // endread data sections

  return true;
}

/** Find out the size and offset of every frame and store in the info struct pointed to by info_ptr.
 * @param chunk_offset If the sequence chunk is offset into the file then this offset can be provided here, in bytes from the start of the file.
 */
static bool _build_frames_directory_from_file( const char* seq_filename, vol_geom_info_t* info_ptr, vol_geom_size_t chunk_offset ) {
  FILE* f_ptr = NULL;
  if ( !seq_filename || !info_ptr ) { return false; }

  { // Allocate memory for frame headers and frames directory.
    vol_geom_size_t frame_headers_sz = info_ptr->hdr.frame_count * sizeof( vol_geom_frame_hdr_t );
    _vol_loggerf( VOL_GEOM_LOG_TYPE_DEBUG, "Allocating %" PRId64 " bytes for frame headers.\n", frame_headers_sz );
    info_ptr->frame_headers_ptr = calloc( 1, (size_t)frame_headers_sz );
    if ( !info_ptr->frame_headers_ptr ) {
      _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: OOM allocating frames headers.\n" );
      goto bfdff_fail;
    }

    vol_geom_size_t frames_directory_sz = info_ptr->hdr.frame_count * sizeof( vol_geom_frame_directory_entry_t );
    _vol_loggerf( VOL_GEOM_LOG_TYPE_DEBUG, "Allocating %" PRId64 " bytes for frames directory.\n", frames_directory_sz );
    info_ptr->frames_directory_ptr = calloc( 1, (size_t)frames_directory_sz );
    if ( !info_ptr->frames_directory_ptr ) {
      _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: OOM allocating frames directory.\n" );
      goto bfdff_fail;
    }
  }

  vol_geom_size_t sequence_file_sz = 0;
  if ( !_get_file_sz( seq_filename, &sequence_file_sz ) ) { goto bfdff_fail; }
  _vol_loggerf( VOL_GEOM_LOG_TYPE_DEBUG, "Sequence file is %" PRId64 " bytes.\n", sequence_file_sz );

  f_ptr = fopen( seq_filename, "rb" );
  if ( !f_ptr ) { goto bfdff_fail; }
  if ( 0 != vol_geom_fseeko( f_ptr, chunk_offset, SEEK_SET ) ) { goto bfdff_fail; }

  for ( uint32_t i = 0; i < info_ptr->hdr.frame_count; i++ ) {
    vol_geom_frame_hdr_t frame_hdr = ( vol_geom_frame_hdr_t ){ .mesh_data_sz = 0 };

    vol_geom_size_t frame_start_offset = vol_geom_ftello( f_ptr );
    if ( -1LL == frame_start_offset ) { goto bfdff_fail; }

    if ( !fread( &frame_hdr.frame_number, sizeof( uint32_t ), 1, f_ptr ) ) {
      _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: frame_number at frame %i in sequence file was out of file size range.\n", i );
      goto bfdff_fail;
    }
    if ( frame_hdr.frame_number != i ) {
      _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: frame_number was %i at frame %i in sequence file.\n", frame_hdr.frame_number, i );
      goto bfdff_fail;
    }
    if ( !fread( &frame_hdr.mesh_data_sz, sizeof( uint32_t ), 1, f_ptr ) ) {
      _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: mesh_data_sz %i was out of file size range in sequence file.\n", frame_hdr.mesh_data_sz );
      goto bfdff_fail;
    }
    if ( (vol_geom_size_t)frame_hdr.mesh_data_sz > sequence_file_sz ) {
      _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: frame %i has mesh_data_sz %i, which is invalid. Sequence file is %" PRId64 " bytes.\n", i,
        frame_hdr.mesh_data_sz, sequence_file_sz );
      goto bfdff_fail;
    }
    if ( !fread( &frame_hdr.keyframe, sizeof( uint8_t ), 1, f_ptr ) ) {
      _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: keyframe (type) was out of file size range in sequence file.\n" );
      goto bfdff_fail;
    }

    vol_geom_size_t frame_current_offset = vol_geom_ftello( f_ptr );
    if ( -1LL == frame_current_offset ) { goto bfdff_fail; }
    info_ptr->frames_directory_ptr[i].hdr_sz = (vol_geom_size_t)frame_current_offset - (vol_geom_size_t)frame_start_offset;

    // in version 12 mesh_data_sz includes array sizes, but earlier versions need to add that to payload size
    info_ptr->frames_directory_ptr[i].corrected_payload_sz = frame_hdr.mesh_data_sz;
    if ( info_ptr->hdr.version < 12 ) {
      // keyframe value 2 only exists in v12 plus but value 1 exists.
      if ( 1 == frame_hdr.keyframe ) {
        info_ptr->frames_directory_ptr[i].corrected_payload_sz += 8; // indices and UVs size
      }
      // version 10 doesn't have normals/texture, but 11 can do.
      if ( 11 == info_ptr->hdr.version ) {
        info_ptr->frames_directory_ptr[i].corrected_payload_sz += 4;   // normals sz
        if ( info_ptr->hdr.textured ) {
          info_ptr->frames_directory_ptr[i].corrected_payload_sz += 4; // texture sz
        }
      }
    }
    if ( info_ptr->frames_directory_ptr[i].corrected_payload_sz > sequence_file_sz ) {
      _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: frame %i corrected_payload_sz %" PRId64 " bytes was too large for a sequence of %" PRId64 " bytes.\n", i,
        info_ptr->frames_directory_ptr[i].corrected_payload_sz, sequence_file_sz );
      goto bfdff_fail;
    }

    // Seek past mesh data and past the final integer "frame data size". See if file is big enough.
    if ( 0 != vol_geom_fseeko( f_ptr, info_ptr->frames_directory_ptr[i].corrected_payload_sz + 4, SEEK_CUR ) ) {
      _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: not enough memory in sequence file for frame %i contents.\n", i );
      goto bfdff_fail;
    }
    frame_current_offset = vol_geom_ftello( f_ptr );
    if ( -1LL == frame_current_offset ) { goto bfdff_fail; }

    // Update frame directory and store frame header.
    info_ptr->frames_directory_ptr[i].offset_sz = (vol_geom_size_t)frame_start_offset;
    info_ptr->frames_directory_ptr[i].total_sz  = (vol_geom_size_t)frame_current_offset - (vol_geom_size_t)frame_start_offset;
    info_ptr->frame_headers_ptr[i]              = frame_hdr;
    if ( info_ptr->frames_directory_ptr[i].total_sz > sequence_file_sz ) {
      _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: frame %i total_sz %" PRId64 " bytes was too large for a sequence of %" PRId64 " bytes.\n", i,
        info_ptr->frames_directory_ptr[i].total_sz, sequence_file_sz );
      goto bfdff_fail;
    }

    if ( info_ptr->frames_directory_ptr[i].total_sz > info_ptr->biggest_frame_blob_sz ) {
      info_ptr->biggest_frame_blob_sz = info_ptr->frames_directory_ptr[i].total_sz;
    }
  }
  fclose( f_ptr );
  f_ptr = NULL; // this is checked later, so make = NULL
  return true;

bfdff_fail:
  if ( f_ptr ) { fclose( f_ptr ); }
  if ( info_ptr->frame_headers_ptr ) {
    free( info_ptr->frame_headers_ptr );
    info_ptr->frame_headers_ptr = NULL;
  }
  if ( info_ptr->frames_directory_ptr ) {
    free( info_ptr->frames_directory_ptr );
    info_ptr->frames_directory_ptr = NULL;
  }
  return false;
}

/******************************************************************************
  BASIC API
******************************************************************************/

void vol_geom_set_log_callback( void ( *user_function_ptr )( vol_geom_log_type_t log_type, const char* message_str ) ) { _logger_ptr = user_function_ptr; }

void vol_geom_reset_log_callback( void ) { _logger_ptr = _default_logger; }

bool vol_geom_read_hdr_from_mem( const uint8_t* data_ptr, uint32_t data_sz, vol_geom_file_hdr_t* hdr_ptr, vol_geom_size_t* hdr_sz_ptr ) {
  if ( !data_ptr || !hdr_ptr || !hdr_sz_ptr || data_sz < VOL_GEOM_FILE_HDR_V10_MIN_SZ ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "vol_geom_read_hdr_from_mem: invalid parameters\n" );
    return false;
  }

  vol_geom_size_t offset = 0;
  memset( hdr_ptr, 0, sizeof( vol_geom_file_hdr_t ) );

  // Support either Unity format "VOLS" string, or IFF-style first-4-bytes "VOLS" magic file numbers.
  if ( data_ptr[0] == 'V' && data_ptr[1] == 'O' && data_ptr[2] == 'L' && data_ptr[3] == 'S' ) {
    memcpy( hdr_ptr->format.bytes, data_ptr, 4 );
    hdr_ptr->format.sz = 4;
    _vol_loggerf( VOL_GEOM_LOG_TYPE_DEBUG, "vol_geom_read_hdr_from_mem: format: %c, %c, %c, %c\n", data_ptr[0], data_ptr[1], data_ptr[2], data_ptr[3] );
    offset += 4;
  } else {
    if ( !_read_short_str( data_ptr, data_sz, 0, &hdr_ptr->format ) ) {
      _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "vol_geom_read_hdr_from_mem: failed to read format\n" );
      return false;
    }
    if ( strncmp( "VOLS", hdr_ptr->format.bytes, 4 ) != 0 ) {
      _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "vol_geom_read_hdr_from_mem: failed format check\n" );
      return false;
    } // Format check.
    offset += ( hdr_ptr->format.sz + 1 );
  }
  if ( offset + 4 * (vol_geom_size_t)sizeof( uint32_t ) + 3 >= data_sz ) { return false; } // OOB
  memcpy( &hdr_ptr->version, &data_ptr[offset], sizeof( uint32_t ) );
  offset += (vol_geom_size_t)sizeof( uint32_t );
  _vol_loggerf( VOL_GEOM_LOG_TYPE_DEBUG, "vol_geom_read_hdr_from_mem: detected header version %d", hdr_ptr->version );
  if ( hdr_ptr->version < 10 || hdr_ptr->version > 13 ) { return false; } // Version check.
  memcpy( &hdr_ptr->compression, &data_ptr[offset], sizeof( uint32_t ) );
  offset += (vol_geom_size_t)sizeof( uint32_t );
  if ( hdr_ptr->version < 13 ) { // V1.3 removed strings & topology field from header.
    if ( !_read_short_str( data_ptr, data_sz, offset, &hdr_ptr->mesh_name ) ) { return false; }
    offset += ( hdr_ptr->mesh_name.sz + 1 );
    if ( offset + 2 * (vol_geom_size_t)sizeof( uint32_t ) + 2 >= data_sz ) { return false; } // OOB
    if ( !_read_short_str( data_ptr, data_sz, offset, &hdr_ptr->material ) ) { return false; }
    offset += ( hdr_ptr->material.sz + 1 );
    if ( offset + 2 * (vol_geom_size_t)sizeof( uint32_t ) + 1 >= data_sz ) { return false; } // OOB
    if ( !_read_short_str( data_ptr, data_sz, offset, &hdr_ptr->shader ) ) { return false; }
    offset += ( hdr_ptr->shader.sz + 1 );
    if ( offset + 2 * (vol_geom_size_t)sizeof( uint32_t ) >= data_sz ) { return false; } // OOB
    memcpy( &hdr_ptr->topology, &data_ptr[offset], sizeof( uint32_t ) );
    offset += (vol_geom_size_t)sizeof( uint32_t );
  }
  memcpy( &hdr_ptr->frame_count, &data_ptr[offset], sizeof( uint32_t ) );
  offset += (vol_geom_size_t)sizeof( uint32_t );
  // Parse v1.1 part of header.
  if ( hdr_ptr->version < 11 ) { goto vol_geom_rhfmem_success; }
  const vol_geom_size_t v11_section_sz = (vol_geom_size_t)( 3 * sizeof( uint16_t ) + 2 * sizeof( uint8_t ) );
  if ( offset + v11_section_sz > data_sz ) { return false; } // OOB
  hdr_ptr->normals  = (bool)data_ptr[offset++];
  hdr_ptr->textured = (bool)data_ptr[offset++];

  if ( hdr_ptr->version >= 13 ) {                           // v1.3 added texture compression fields.
    hdr_ptr->texture_compression      = data_ptr[offset++]; // { 0 = mp4, 1 = ETC1S, 2 = UASTC }
    hdr_ptr->texture_container_format = data_ptr[offset++]; // { 0 = raw, 1 = basis, 2 = KTX2 }
    memcpy( &hdr_ptr->texture_width, &data_ptr[offset], sizeof( uint32_t ) );
    offset += (vol_geom_size_t)sizeof( uint32_t );
    memcpy( &hdr_ptr->texture_height, &data_ptr[offset], sizeof( uint32_t ) );
    offset += (vol_geom_size_t)sizeof( uint32_t );
    memcpy( &hdr_ptr->fps, &data_ptr[offset], sizeof( float ) );
    offset += (vol_geom_size_t)sizeof( float );
    memcpy( &hdr_ptr->audio, &data_ptr[offset], sizeof( uint32_t ) );
    offset += (vol_geom_size_t)sizeof( uint32_t );
    memcpy( &hdr_ptr->audio_start, &data_ptr[offset], sizeof( uint32_t ) );
    offset += (vol_geom_size_t)sizeof( uint32_t );
    memcpy( &hdr_ptr->frame_body_start, &data_ptr[offset], sizeof( uint32_t ) );
    offset += (vol_geom_size_t)sizeof( uint32_t );
    if ( offset != 44 ) { return false; }
    goto vol_geom_rhfmem_success; // End of header for v1.3 here.
  }
  uint16_t w = 0, h = 0;
  memcpy( &w, &data_ptr[offset], sizeof( uint16_t ) );
  offset += (vol_geom_size_t)sizeof( uint16_t );
  memcpy( &h, &data_ptr[offset], sizeof( uint16_t ) );
  offset += (vol_geom_size_t)sizeof( uint16_t );
  hdr_ptr->texture_width  = (uint32_t)w;
  hdr_ptr->texture_height = (uint32_t)h;
  memcpy( &hdr_ptr->texture_format, &data_ptr[offset], sizeof( uint16_t ) );
  offset += (vol_geom_size_t)sizeof( uint16_t );
  // Parse v1.2 part of header.
  if ( hdr_ptr->version < 12 ) { goto vol_geom_rhfmem_success; }
  const vol_geom_size_t v12_section_sz = 8 * sizeof( float );
  if ( offset + v12_section_sz > data_sz ) { return false; } // OOB
  memcpy( hdr_ptr->translation, &data_ptr[offset], 3 * sizeof( float ) );
  offset += 3 * sizeof( float );
  memcpy( hdr_ptr->rotation, &data_ptr[offset], 4 * sizeof( float ) );
  offset += 4 * sizeof( float );
  memcpy( &hdr_ptr->scale, &data_ptr[offset], sizeof( float ) );
  offset += sizeof( float );

vol_geom_rhfmem_success:
  *hdr_sz_ptr = offset;
  return true;
}

bool vol_geom_read_hdr_from_file( const char* filename, vol_geom_file_hdr_t* hdr_ptr, vol_geom_size_t* hdr_sz_ptr ) {
  vol_geom_file_record_t record = ( vol_geom_file_record_t ){ .byte_ptr = NULL };
  if ( !filename || !hdr_ptr || !hdr_sz_ptr ) { return false; }
  if ( !_read_file( filename, &record, sizeof( vol_geom_file_hdr_t ) ) ) { goto rhff_fail; }
  if ( !vol_geom_read_hdr_from_mem( record.byte_ptr, record.sz, hdr_ptr, hdr_sz_ptr ) ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: vol_geom_read_hdr_from_file: Failed to read header from file `%s`.\n", filename );
    goto rhff_fail;
  }
  if ( record.byte_ptr != NULL ) { free( record.byte_ptr ); }
  return true;
rhff_fail:
  if ( record.byte_ptr != NULL ) { free( record.byte_ptr ); }
  return false;
}

bool vol_geom_read_audio_from_file( const char* vols_filename, vol_geom_info_t* info_ptr ) {
  FILE* f_ptr = NULL;
  if ( !vols_filename || !info_ptr || info_ptr->hdr.version < 13 ) { goto vgraff_fail; }

  f_ptr = fopen( vols_filename, "rb" );
  if ( !f_ptr ) { goto vgraff_fail; }
  if ( 0 != vol_geom_fseeko( f_ptr, info_ptr->hdr.audio_start, SEEK_SET ) ) { goto vgraff_fail; }
  if ( 1 != fread( &info_ptr->audio_data_sz, sizeof( uint32_t ), 1, f_ptr ) ) { goto vgraff_fail; }
  info_ptr->audio_data_ptr = malloc( info_ptr->audio_data_sz );
  if ( !info_ptr->audio_data_ptr ) { goto vgraff_fail; }
  if ( 1 != fread( info_ptr->audio_data_ptr, info_ptr->audio_data_sz, 1, f_ptr ) ) { goto vgraff_fail; }
  fclose( f_ptr );
  return true;
vgraff_fail:
  if ( f_ptr ) { fclose( f_ptr ); }
  return false;
}

bool vol_geom_create_file_info_from_file( const char* vols_filename, vol_geom_info_t* info_ptr ) {
  if ( !vols_filename || !info_ptr || !_is_file( vols_filename ) ) { return false; }

  vol_geom_size_t hdr_sz = 0;
  if ( !vol_geom_read_hdr_from_file( vols_filename, &info_ptr->hdr, &hdr_sz ) ) { goto cfiff_fail; }
  _vol_loggerf( VOL_GEOM_LOG_TYPE_INFO, "Vologram header v%i.%i\n", info_ptr->hdr.version / 10, info_ptr->hdr.version % 10 );

  if ( info_ptr->hdr.audio && !vol_geom_read_audio_from_file( vols_filename, info_ptr ) ) { goto cfiff_fail; }

  // v1.3 introduced a header offset field for this. Preceding versions are immediately after the header.
  info_ptr->sequence_offset = info_ptr->hdr.frame_body_start ? info_ptr->hdr.frame_body_start : hdr_sz;

  if ( !_build_frames_directory_from_file( vols_filename, info_ptr, info_ptr->sequence_offset ) ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: vol_geom_create_file_info_from_file(): Failed to create frames directory.\n" );
    goto cfiff_fail;
  }

  _vol_loggerf( VOL_GEOM_LOG_TYPE_DEBUG, "Allocating preallocated_frame_blob_ptr bytes %" PRId64 "\n", info_ptr->biggest_frame_blob_sz );
  if ( info_ptr->biggest_frame_blob_sz >= 1024 * 1024 * 1024 ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: extremely high frame size %" PRId64 " reported - assuming error.\n", info_ptr->biggest_frame_blob_sz );
    goto cfiff_fail;
  }
  info_ptr->preallocated_frame_blob_ptr = calloc( 1, info_ptr->biggest_frame_blob_sz );
  if ( !info_ptr->preallocated_frame_blob_ptr ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: out of memory allocating frame blob reserve.\n" );
    goto cfiff_fail;
  }

  return true;

cfiff_fail:
  vol_geom_free_file_info( info_ptr );
  return false;
}

bool vol_geom_create_file_info( const char* hdr_filename, const char* seq_filename, vol_geom_info_t* info_ptr, bool streaming_mode ) {
  if ( !hdr_filename || !seq_filename || !info_ptr ) { return false; }

  vol_geom_file_record_t record = ( vol_geom_file_record_t ){ .sz = 0 };
  vol_geom_size_t hdr_sz        = 0;
  *info_ptr                     = ( vol_geom_info_t ){ .biggest_frame_blob_sz = 0 };
  info_ptr->sequence_offset     = 0; // Using separate files here, so there is no offset.

  if ( !vol_geom_read_hdr_from_file( hdr_filename, &info_ptr->hdr, &hdr_sz ) ) { goto cfi_fail; }

  if ( !_build_frames_directory_from_file( seq_filename, info_ptr, info_ptr->sequence_offset ) ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: vol_geom_create_file_info_from_file(): Failed to create frames directory.\n" );
    goto cfi_fail;
  }

  _vol_loggerf( VOL_GEOM_LOG_TYPE_DEBUG, "Allocating preallocated_frame_blob_ptr bytes %" PRId64 "\n", info_ptr->biggest_frame_blob_sz );
  if ( info_ptr->biggest_frame_blob_sz >= 1024 * 1024 * 1024 ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: extremely high frame size %" PRId64 " reported - assuming error.\n", info_ptr->biggest_frame_blob_sz );
    goto cfi_fail;
  }
  info_ptr->preallocated_frame_blob_ptr = calloc( 1, info_ptr->biggest_frame_blob_sz );
  if ( !info_ptr->preallocated_frame_blob_ptr ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: out of memory allocating frame blob reserve.\n" );
    goto cfi_fail;
  }

  // If not dealing with huge sequence files - preload the whole thing to memory to avoid file I/O problems.
  if ( !streaming_mode ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_DEBUG, "Reading entire sequence file to blob memory\n" );
    vol_geom_file_record_t seq_blob = ( vol_geom_file_record_t ){ .sz = 0 };
    if ( !_read_file( seq_filename, &seq_blob, 0 ) ) { goto cfi_fail; }
    info_ptr->sequence_blob_byte_ptr = (uint8_t*)seq_blob.byte_ptr;
  }

  return true;

cfi_fail:

  _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: Failed to parse info from vologram geometry files.\n" );
  if ( record.byte_ptr ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_DEBUG, "Freeing record.byte_ptr\n" );
    free( record.byte_ptr );
  }
  vol_geom_free_file_info( info_ptr );

  return false;
}

bool vol_geom_free_file_info( vol_geom_info_t* info_ptr ) {
  if ( !info_ptr ) { return false; }

  _vol_loggerf( VOL_GEOM_LOG_TYPE_DEBUG, "Freeing allocated vol_geom info_ptr memory.\n" );
  if ( info_ptr->audio_data_ptr ) { free( info_ptr->audio_data_ptr ); }
  if ( info_ptr->sequence_blob_byte_ptr ) { free( info_ptr->sequence_blob_byte_ptr ); }
  if ( info_ptr->preallocated_frame_blob_ptr ) { free( info_ptr->preallocated_frame_blob_ptr ); }
  if ( info_ptr->frame_headers_ptr ) { free( info_ptr->frame_headers_ptr ); }
  if ( info_ptr->frames_directory_ptr ) { free( info_ptr->frames_directory_ptr ); }
  *info_ptr = ( vol_geom_info_t ){ .hdr.frame_count = 0 };

  return true;
}

bool vol_geom_is_keyframe( const vol_geom_info_t* info_ptr, uint32_t frame_idx ) {
  assert( info_ptr );
  if ( !info_ptr ) { return false; }
  if ( frame_idx >= info_ptr->hdr.frame_count ) { return false; }
  if ( 0 == info_ptr->frame_headers_ptr[frame_idx].keyframe ) { return false; }
  return true;
}

int vol_geom_find_previous_keyframe( const vol_geom_info_t* info_ptr, uint32_t frame_idx ) {
  assert( info_ptr );
  if ( !info_ptr ) { return -1; }
  if ( frame_idx >= info_ptr->hdr.frame_count ) { return false; }
  for ( int i = frame_idx; i >= 0; i-- ) {
    if ( vol_geom_is_keyframe( info_ptr, i ) ) { return i; }
  }
  return -1;
}

bool vol_geom_read_frame( const char* seq_filename, const vol_geom_info_t* info_ptr, uint32_t frame_idx, vol_geom_frame_data_t* frame_data_ptr ) {
  assert( seq_filename && info_ptr && frame_data_ptr );
  if ( !seq_filename || !info_ptr || !frame_data_ptr ) { return false; }

  if ( frame_idx >= info_ptr->hdr.frame_count ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: frame requested (%i) is not in valid range of 0-%i for sequence\n", frame_idx, info_ptr->hdr.frame_count );
    return false;
  }

  // Get the offset of that frame and size required to allocate for it.
  vol_geom_size_t offset_sz = info_ptr->frames_directory_ptr[frame_idx].offset_sz;
  vol_geom_size_t total_sz  = info_ptr->frames_directory_ptr[frame_idx].total_sz;

  // Get file size and check for file size issues before allocating memory or reading
  vol_geom_size_t file_sz = 0;
  if ( !_get_file_sz( seq_filename, &file_sz ) ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: sequence file `%s` could not be opened.\n", seq_filename );
    return false;
  }
  if ( file_sz < ( offset_sz + total_sz ) ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: sequence file is too short to contain frame %i data.\n", frame_idx );
    return false;
  }

  if ( info_ptr->biggest_frame_blob_sz < total_sz ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR: pre-allocated frame blob was too small for frame %i: %" PRId64 "/%" PRId64 " bytes.\n", frame_idx,
      info_ptr->biggest_frame_blob_sz, total_sz );
    return false;
  }

  // Find frame section within sequence file blob if it was pre-loaded.
  if ( info_ptr->sequence_blob_byte_ptr ) {
    memcpy( info_ptr->preallocated_frame_blob_ptr, &info_ptr->sequence_blob_byte_ptr[offset_sz], total_sz );

    // Read frame blob from file.
  } else {
    FILE* f_ptr = fopen( seq_filename, "rb" );
    if ( !f_ptr ) {
      _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR could not open file `%s` for frame data.\n", seq_filename );
      return false;
    }
    if ( 0 != vol_geom_fseeko( f_ptr, offset_sz, SEEK_SET ) ) {
      _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR seeking frame %i from sequence file - file too small for data.\n", frame_idx );
      fclose( f_ptr );
      return false;
    }
    if ( !fread( info_ptr->preallocated_frame_blob_ptr, total_sz, 1, f_ptr ) ) {
      _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR reading frame %i from sequence file\n", frame_idx );
      fclose( f_ptr );
      return false;
    }
    fclose( f_ptr );
  } // end FILE i/o block

  if ( !_read_vol_frame( info_ptr, frame_idx, frame_data_ptr ) ) {
    _vol_loggerf( VOL_GEOM_LOG_TYPE_ERROR, "ERROR parsing frame %i\n", frame_idx );
    return false;
  }
  return true;
}
