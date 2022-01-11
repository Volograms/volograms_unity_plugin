/** @file vol_geom.c
 * Volograms Geometry Decoding API
 *
 * vol_geom  | .vol Geometry Decoding API
 * --------- | ---------------------
 * Version   | 0.6.1
 * Authors   | Anton Gerdelan <anton@volograms.com>
 * Copyright | 2021, Volograms (http://volograms.com/)
 * Language  | C99
 * Files     | 2
 * Licence   | The MIT License. See LICENSE.md for details.
 */

#include "vol_geom.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/// File header section size in bytes. Used in sanity checks to test for corrupted files that are below minimum sizes expected.
#define VOL_GEOM_FILE_HDR_V10_MIN_SZ 24 /// "VOLS" (4 bytes) + 4 string length bytes + 4 ints in v10 hdr.
/// File header section size in bytes. Used in sanity checks to test for corrupted files that are below minimum sizes expected.
#define VOL_GEOM_FRAME_MIN_SZ 17 /// 3 ints, 1 byte, 1 int inside vertices array. the rest are optional

/** To suppress compiler warnings. */
#define VOL_GEOM_UNUSED( x ) (void)( x )

/** Printf an error message to stderr, but only if VOL_GEOM_DEBUG is set.
 * @param line   Use `__LINE__` here to print the line number of the error message.
 * @param fmt    A formatted string in `printf()` variable-argument, (supporting `%%i`, `%%f` etc.). \n
 * Example:      `_dbprinterr(__LINE__, "testing thing %i\n", 10 );` \n
 * That will output: `ERROR vol_av.c:64 testing thing 10`
 */
static void _dbprinterr( int line, const char* fmt, ... ) {
#ifdef VOL_GEOM_DEBUG
  va_list args;
  fprintf( stderr, "ERROR vol_geom.c:%i ", line );
  va_start( args, fmt );
  vfprintf( stderr, fmt, args );
  va_end( args );
#else
  VOL_GEOM_UNUSED( fmt );
  VOL_GEOM_UNUSED( line );
#endif
}

/** Printf an error message to stdout, but only if `VOL_GEOM_DEBUG` is set.
 * @param fmt    A formatted string in `printf()` variable-argument, (supporting `%%i`, `%%f` etc.). \n
 * Example:      `_dbprint("testing thing %i\n", 10 );` \n
 * That will output: `vol_geom: testing thing 10`
 */
static void _dbprint( const char* fmt, ... ) {
#ifdef VOL_GEOM_DEBUG
  va_list args;
  fprintf( stdout, "vol_geom: " );
  va_start( args, fmt );
  vfprintf( stdout, fmt, args );
  va_end( args );
#else
  VOL_GEOM_UNUSED( fmt );
#endif
}

/// Helper struct to refer to an entire file loaded from disk via `_read_entire_file()`.
typedef struct vol_geom_file_record_t {
  /// Pointer to contents of file.
  uint8_t* byte_ptr;
  /// Size of file in bytes.
  vol_geom_size_t sz;
} vol_geom_file_record_t;

/******************************************************************************
  BASIC API
******************************************************************************/

/** Helper function to read an entire file into an array of bytes within struct pointed to by `fr_ptr`.
 * @warning        This function allocates memory that the caller must manually free after use.
 * @param filename Pointer to nul-terminated file path string. Must not be NULL.
 * @param fr_ptr   File contents and size are written to a structure pointed to by `fr_ptr`. Must not be NULL.
 * @return         False on any error.
 */
static bool _read_entire_file( const char* filename, vol_geom_file_record_t* fr_ptr ) {
  if ( !filename || !fr_ptr ) { return false; }

  FILE* f_ptr = fopen( filename, "rb" );
  if ( !f_ptr ) { return false; }
  fseek( f_ptr, 0L, SEEK_END );
  fr_ptr->sz = (vol_geom_size_t)ftell( f_ptr );
  _dbprint( "Allocating %u bytes for reading file\n", (uint32_t)fr_ptr->sz );
  fr_ptr->byte_ptr = malloc( (size_t)fr_ptr->sz );
  if ( !fr_ptr->byte_ptr ) {
    fclose( f_ptr );
    return false;
  }
  rewind( f_ptr );
  vol_geom_size_t nr = fread( fr_ptr->byte_ptr, fr_ptr->sz, 1, f_ptr );
  fclose( f_ptr );
  if ( 1 != nr ) { return false; }

  return true;
}

/** Helper function to check the actual size of a file on disk.
 * @param filename Pointer to nul-terminated file path string. Must not be NULL.
 * @param sz_ptr   Size of the file, in bytes, is written to address pointed to by sz_ptr. Must not be NULL. Not written when returning false.
 * @return         False on any error, including bad parameters or file not found. Otherwise true on success, where sz_ptr is set.
 */
static bool _get_file_sz( const char* filename, vol_geom_size_t* sz_ptr ) {
  if ( !filename || !sz_ptr ) { return false; }

  FILE* f_ptr = fopen( filename, "rb" );
  if ( !f_ptr ) { return false; }
  {
    fseek( f_ptr, 0L, SEEK_END );
    *sz_ptr = (vol_geom_size_t)ftell( f_ptr );
  }
  fclose( f_ptr );

  return true;
}

/** Helper function to read Unity-style strings, specified in VOL format, from a loaded file.
 * @warning      The file's string format is ambiguous so insecure assumptions are made here.
 * @param fr_ptr Pointer to a file record loaded with a call to `_read_entire_file()`. Must not be NULL.
 * @param offset Offset, in bytes, of the string's location within the file record.
 * @param sstr   Pointer to a struct to write the string's contents. Must not be NULL.
 * @return       False on any error.
 */
static bool _read_short_str( const vol_geom_file_record_t* fr_ptr, vol_geom_size_t offset, vol_geom_short_str_t* sstr ) {
  if ( !fr_ptr || !sstr ) { return false; }
  if ( offset >= fr_ptr->sz ) { return false; } // OOB

  sstr->sz = fr_ptr->byte_ptr[offset]; // assumes the 1-byte length
  if ( sstr->sz > 127 ) {
    _dbprinterr( __LINE__, "ERROR: string length %i given is > 127\n", (int)sstr->sz );
    return false;
  }
  if ( offset + sstr->sz >= fr_ptr->sz ) { return false; } // OOB
  memcpy( sstr->bytes, &fr_ptr->byte_ptr[offset + 1], sstr->sz );
  sstr->bytes[sstr->sz] = '\0';

  return true;
}

static bool _read_vol_file_hdr( const vol_geom_file_record_t* fr, vol_geom_file_hdr_t* hdr, vol_geom_size_t* hdr_sz ) {
  if ( !fr || !hdr || !hdr_sz || fr->sz < VOL_GEOM_FILE_HDR_V10_MIN_SZ ) { return false; }

  vol_geom_size_t offset = 0;

  // parse v10 part of header
  if ( !_read_short_str( fr, 0, &hdr->format ) ) { return false; }
  if ( strncmp( "VOLS", hdr->format.bytes, 4 ) != 0 ) { return false; } // format check
  offset += ( hdr->format.sz + 1 );
  if ( offset + 4 * sizeof( int32_t ) + 3 >= fr->sz ) { return false; } // OOB
  memcpy( &hdr->version, &fr->byte_ptr[offset], sizeof( int32_t ) );
  offset += sizeof( int32_t );
  if ( hdr->version != 10 && hdr->version != 11 && hdr->version != 12 ) { return false; } // version check
  memcpy( &hdr->compression, &fr->byte_ptr[offset], sizeof( int32_t ) );
  offset += sizeof( int32_t );
  if ( !_read_short_str( fr, offset, &hdr->mesh_name ) ) { return false; }
  offset += ( hdr->mesh_name.sz + 1 );
  if ( offset + 2 * sizeof( int32_t ) + 2 >= fr->sz ) { return false; } // OOB
  if ( !_read_short_str( fr, offset, &hdr->material ) ) { return false; }
  offset += ( hdr->material.sz + 1 );
  if ( offset + 2 * sizeof( int32_t ) + 1 >= fr->sz ) { return false; } // OOB
  if ( !_read_short_str( fr, offset, &hdr->shader ) ) { return false; }
  offset += ( hdr->shader.sz + 1 );
  if ( offset + 2 * sizeof( int32_t ) >= fr->sz ) { return false; } // OOB
  memcpy( &hdr->topology, &fr->byte_ptr[offset], sizeof( int32_t ) );
  offset += sizeof( int32_t );
  memcpy( &hdr->frame_count, &fr->byte_ptr[offset], sizeof( int32_t ) );
  offset += sizeof( int32_t );

  // parse v11 part of header
  if ( hdr->version < 11 ) {
    *hdr_sz = offset;
    return true;
  }
  const vol_geom_size_t v11_section_sz = 3 * sizeof( uint16_t ) + 2 * sizeof( uint8_t );
  if ( offset + v11_section_sz > fr->sz ) { return false; } // OOB
  hdr->normals  = (bool)fr->byte_ptr[offset++];
  hdr->textured = (bool)fr->byte_ptr[offset++];
  memcpy( &hdr->texture_width, &fr->byte_ptr[offset], sizeof( uint16_t ) );
  offset += sizeof( uint16_t );
  memcpy( &hdr->texture_height, &fr->byte_ptr[offset], sizeof( uint16_t ) );
  offset += sizeof( uint16_t );
  memcpy( &hdr->texture_format, &fr->byte_ptr[offset], sizeof( uint16_t ) );
  offset += sizeof( uint16_t );

  // parse v12 part of header
  if ( hdr->version < 12 ) {
    *hdr_sz = offset;
    return true;
  }
  const vol_geom_size_t v12_section_sz = 8 * sizeof( float );
  if ( offset + v12_section_sz > fr->sz ) { return false; } // OOB
  memcpy( hdr->translation, &fr->byte_ptr[offset], 3 * sizeof( float ) );
  offset += 3 * sizeof( float );
  memcpy( hdr->rotation, &fr->byte_ptr[offset], 4 * sizeof( float ) );
  offset += 4 * sizeof( float );
  memcpy( &hdr->scale, &fr->byte_ptr[offset], sizeof( float ) );
  offset += sizeof( float );

  *hdr_sz = offset;
  return true;
}

static bool _read_vol_frame( const vol_geom_info_t* info_ptr, int frame_idx, vol_geom_frame_data_t* frame_data_ptr ) {
  assert( info_ptr && info_ptr->preallocated_frame_blob_ptr && frame_data_ptr );
  if ( !info_ptr || !info_ptr->preallocated_frame_blob_ptr || !frame_data_ptr ) { return false; }
  if ( frame_idx < 0 || frame_idx >= info_ptr->hdr.frame_count ) { return false; }

  *frame_data_ptr = ( vol_geom_frame_data_t ){ .block_data_sz = 0 };

  uint8_t* byte_blob_ptr         = (uint8_t*)info_ptr->preallocated_frame_blob_ptr;
  frame_data_ptr->block_data_ptr = &byte_blob_ptr[info_ptr->frames_directory_ptr[frame_idx].hdr_sz];
  frame_data_ptr->block_data_sz  = info_ptr->frames_directory_ptr[frame_idx].corrected_payload_sz;

  {
    // start within the frame's memory but after its frame header and at the start of mesh data
    vol_geom_size_t curr_offset = 0;

    { // vertices
      if ( frame_data_ptr->block_data_sz < ( curr_offset + sizeof( int32_t ) + (vol_geom_size_t)frame_data_ptr->vertices_sz ) ) { return false; }

      memcpy( &frame_data_ptr->vertices_sz, &frame_data_ptr->block_data_ptr[curr_offset], sizeof( int32_t ) );
      curr_offset += sizeof( int32_t );
      frame_data_ptr->vertices_offset = curr_offset;
      curr_offset += (vol_geom_size_t)frame_data_ptr->vertices_sz;
    }

    // normals
    if ( info_ptr->hdr.normals && info_ptr->hdr.version >= 11 ) {
      if ( frame_data_ptr->block_data_sz < ( curr_offset + sizeof( int32_t ) + (vol_geom_size_t)frame_data_ptr->normals_sz ) ) { return false; }

      memcpy( &frame_data_ptr->normals_sz, &frame_data_ptr->block_data_ptr[curr_offset], sizeof( int32_t ) );
      curr_offset += sizeof( int32_t );
      frame_data_ptr->normals_offset = curr_offset;
      curr_offset += (vol_geom_size_t)frame_data_ptr->normals_sz;
    }

    // indices and UVs
    if ( info_ptr->frame_headers_ptr[frame_idx].keyframe == 1 || ( info_ptr->hdr.version >= 12 && info_ptr->frame_headers_ptr[frame_idx].keyframe == 2 ) ) {
      { // indices
        if ( frame_data_ptr->block_data_sz < ( curr_offset + sizeof( int32_t ) + (vol_geom_size_t)frame_data_ptr->indices_sz ) ) { return false; }

        memcpy( &frame_data_ptr->indices_sz, &frame_data_ptr->block_data_ptr[curr_offset], sizeof( int32_t ) );
        curr_offset += sizeof( int32_t );
        frame_data_ptr->indices_offset = curr_offset;
        curr_offset += (vol_geom_size_t)frame_data_ptr->indices_sz;
      }

      { // UVs
        if ( frame_data_ptr->block_data_sz < ( curr_offset + sizeof( int32_t ) + (vol_geom_size_t)frame_data_ptr->uvs_sz ) ) { return false; }

        memcpy( &frame_data_ptr->uvs_sz, &frame_data_ptr->block_data_ptr[curr_offset], sizeof( int32_t ) );
        curr_offset += sizeof( int32_t );
        frame_data_ptr->uvs_offset = curr_offset;
        curr_offset += (vol_geom_size_t)frame_data_ptr->uvs_sz;
      }
    } // endif indices & UVs

    // texture
    // NOTE(Anton) not tested since we aren't using embedded textures at the moment.
    if ( info_ptr->hdr.version >= 11 && info_ptr->hdr.textured ) {
      if ( frame_data_ptr->block_data_sz < ( curr_offset + sizeof( int32_t ) + (vol_geom_size_t)frame_data_ptr->texture_sz ) ) { return false; }

      memcpy( &frame_data_ptr->texture_sz, &frame_data_ptr->block_data_ptr[curr_offset], sizeof( int32_t ) );
      curr_offset += sizeof( int32_t );
      frame_data_ptr->texture_offset = curr_offset;
      curr_offset += (vol_geom_size_t)frame_data_ptr->texture_sz;
    }
  } // endread data sections

  return true;
}

bool vol_geom_read_frame( const char* seq_filename, const vol_geom_info_t* info_ptr, int frame_idx, vol_geom_frame_data_t* frame_data_ptr ) {
  assert( seq_filename && info_ptr && frame_data_ptr );
  if ( !seq_filename || !info_ptr || !frame_data_ptr ) { return false; }

  if ( frame_idx < 0 || frame_idx >= info_ptr->hdr.frame_count ) {
    _dbprinterr( __LINE__, "ERROR: frame requested (%i) is not in valid range of 0-%i for sequence\n", frame_idx, info_ptr->hdr.frame_count );
    return false;
  }

  // Get the offset of that frame and size required to allocate for it.
  vol_geom_size_t offset_sz = info_ptr->frames_directory_ptr[frame_idx].offset_sz;
  vol_geom_size_t total_sz  = info_ptr->frames_directory_ptr[frame_idx].total_sz;

  // Get file size and check for file size issues before allocating memory or reading
  vol_geom_size_t file_sz = 0;
  if ( !_get_file_sz( seq_filename, &file_sz ) ) {
    _dbprinterr( __LINE__, "ERROR: sequence file `%s` could not be opened.\n", seq_filename );
    return false;
  }
  if ( file_sz < ( offset_sz + total_sz ) ) {
    _dbprinterr( __LINE__, "ERROR: sequence file is too short to contain frame %i data.\n", frame_idx );
    return false;
  }

  if ( info_ptr->biggest_frame_blob_sz < total_sz ) {
    _dbprinterr( __LINE__, "ERROR: pre-allocated frame blob was too small for frame %i: %u/%u bytes.\n", frame_idx, info_ptr->biggest_frame_blob_sz, total_sz );
    return false;
  }

  { // Read blob from file.
    FILE* f_ptr = fopen( seq_filename, "rb" );
    if ( !f_ptr ) {
      _dbprinterr( __LINE__, "ERROR could not open file `%s` for frame data.\n", seq_filename );
      return false;
    }
    if ( -1 == fseek( f_ptr, (long)offset_sz, SEEK_SET ) ) {
      _dbprinterr( __LINE__, "ERROR seeking frame %i from sequence file - file too small for data\n", frame_idx );
      fclose( f_ptr );
      return false;
    }
    if ( !fread( info_ptr->preallocated_frame_blob_ptr, total_sz, 1, f_ptr ) ) {
      _dbprinterr( __LINE__, "ERROR reading frame %i from sequence file\n", frame_idx );
      fclose( f_ptr );
      return false;
    }
    fclose( f_ptr );
  } // end FILE i/o block

  if ( !_read_vol_frame( info_ptr, frame_idx, frame_data_ptr ) ) {
    _dbprinterr( __LINE__, "ERROR parsing frame %i\n", frame_idx );
    return false;
  }
  return true;
}

bool vol_geom_create_file_info( const char* hdr_filename, const char* seq_filename, vol_geom_info_t* info_ptr ) {
  if ( !hdr_filename || !seq_filename || !info_ptr ) { return false; }

  FILE* f_ptr = NULL; // this is checked later so declare & init up top.
  // Read file header.
  vol_geom_file_record_t record = ( vol_geom_file_record_t ){ .sz = 0 };
  vol_geom_size_t hdr_sz        = 0;
  info_ptr->hdr                 = ( vol_geom_file_hdr_t ){ .version = 0 };
  {
    if ( !_read_entire_file( hdr_filename, &record ) ) { goto failed_to_read_info; }
    if ( !_read_vol_file_hdr( &record, &info_ptr->hdr, &hdr_sz ) ) { goto failed_to_read_info; }

    // done with file record so tidy-up memory
    if ( record.byte_ptr != NULL ) {
      _dbprint( "Freeing record.byte_ptr\n" );
      free( record.byte_ptr );
      record.byte_ptr = NULL; // this is checked later, so make = NULL
    }
    _dbprint( "hdr sz was %u. %u bytes in file\n", (uint32_t)hdr_sz, (uint32_t)record.sz );
  }

  { // allocate memory for frame headers and frames directory
    vol_geom_size_t frame_headers_sz = info_ptr->hdr.frame_count * sizeof( vol_geom_frame_hdr_t );
    _dbprint( "Allocating %u bytes for frame headers.\n", (unsigned int)frame_headers_sz );
    info_ptr->frame_headers_ptr = calloc( 1, (size_t)frame_headers_sz );
    if ( !info_ptr->frame_headers_ptr ) {
      _dbprinterr( __LINE__, "ERROR: OOM allocating frames headers\n" );
      return false;
    }

    vol_geom_size_t frames_directory_sz = info_ptr->hdr.frame_count * sizeof( vol_geom_frame_directory_entry_t );
    _dbprint( "Allocating %u bytes for frames directory.\n", (unsigned int)frames_directory_sz );
    info_ptr->frames_directory_ptr = calloc( 1, (size_t)frames_directory_sz );
    if ( !info_ptr->frames_directory_ptr ) {
      _dbprinterr( __LINE__, "ERROR: OOM allocating frames directory\n" );
      return false;
    }
  }

  info_ptr->biggest_frame_blob_sz = 0;
  int biggest_frame_idx           = -1;

  // find out the size and offset of every frame
  { // fetch frame from sequence file
    vol_geom_size_t sequence_file_sz = 0;

    f_ptr = fopen( seq_filename, "rb" );
    if ( !f_ptr ) {
      _dbprinterr( __LINE__, "ERROR: Could not open file `%s`\n", seq_filename );
      goto failed_to_read_info;
    }

    { // get file size to do sanity test of other sizes with
      if ( 0 != fseek( f_ptr, 0L, SEEK_END ) ) { goto failed_to_read_info; }
      sequence_file_sz = (vol_geom_size_t)ftell( f_ptr );
      _dbprint( "Sequence file is %u bytes\n", (uint32_t)sequence_file_sz );
      if ( 0 != fseek( f_ptr, 0L, SEEK_SET ) ) { goto failed_to_read_info; }
    }

    // loop to get each frame's details
    for ( int32_t i = 0; i < info_ptr->hdr.frame_count; i++ ) {
      vol_geom_frame_hdr_t frame_hdr = ( vol_geom_frame_hdr_t ){ .mesh_data_sz = 0 };

      long frame_start_offset = ftell( f_ptr );

      if ( !fread( &frame_hdr.frame_number, sizeof( int32_t ), 1, f_ptr ) ) {
        _dbprinterr( __LINE__, "ERROR: frame_number at frame %i in sequence file was out of file size range\n", i );
        goto failed_to_read_info;
      }
      if ( frame_hdr.frame_number != i ) {
        _dbprinterr( __LINE__, "ERROR: frame_number was %i at frame %i in sequence file\n", frame_hdr.frame_number, i );
        goto failed_to_read_info;
      }
      if ( !fread( &frame_hdr.mesh_data_sz, sizeof( int32_t ), 1, f_ptr ) ) {
        _dbprinterr( __LINE__, "ERROR: mesh_data_sz %i was out of file size range in sequence file\n", frame_hdr.mesh_data_sz );
        goto failed_to_read_info;
      }
      if ( frame_hdr.mesh_data_sz < 0 || (vol_geom_size_t)frame_hdr.mesh_data_sz > sequence_file_sz ) {
        _dbprinterr( __LINE__, "ERROR: mesh_data_sz %i was invalid for a sequence of %u bytes\n", frame_hdr.mesh_data_sz, (uint32_t)sequence_file_sz );
        goto failed_to_read_info;
      }
      if ( !fread( &frame_hdr.keyframe, sizeof( uint8_t ), 1, f_ptr ) ) {
        _dbprinterr( __LINE__, "ERROR: keyframe (type) was out of file size range in sequence file\n" );
        goto failed_to_read_info;
      }

      long frame_current_offset                = ftell( f_ptr );
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
          info_ptr->frames_directory_ptr[i].corrected_payload_sz += 4; // normals sz
          if ( info_ptr->hdr.textured ) {
            info_ptr->frames_directory_ptr[i].corrected_payload_sz += 4; // texture sz
          }
        }
      }
      if ( info_ptr->frames_directory_ptr[i].corrected_payload_sz > sequence_file_sz ) {
        _dbprinterr( __LINE__, "ERROR: frame %i corrected_payload_sz %i bytes was too large for a sequence of %u bytes\n", i,
          info_ptr->frames_directory_ptr[i].corrected_payload_sz, (uint32_t)sequence_file_sz );
        goto failed_to_read_info;
      }

      // seek past mesh data and past the final integer "frame data size". see if file is big enough
      if ( -1 == fseek( f_ptr, (long)info_ptr->frames_directory_ptr[i].corrected_payload_sz + 4, SEEK_CUR ) ) {
        _dbprinterr( __LINE__, "ERROR: not enough memory in sequence file for frame %i contents\n", i );
        goto failed_to_read_info;
      }
      frame_current_offset = ftell( f_ptr );

      // update frame directory and store frame header
      info_ptr->frames_directory_ptr[i].offset_sz = (vol_geom_size_t)frame_start_offset;
      info_ptr->frames_directory_ptr[i].total_sz  = (vol_geom_size_t)frame_current_offset - (vol_geom_size_t)frame_start_offset;
      info_ptr->frame_headers_ptr[i]              = frame_hdr;
      if ( info_ptr->frames_directory_ptr[i].total_sz > sequence_file_sz ) {
        _dbprinterr( __LINE__, "ERROR: frame %i total_sz %i bytes was too large for a sequence of %u bytes\n", i, info_ptr->frames_directory_ptr[i].total_sz,
          (uint32_t)sequence_file_sz );
        goto failed_to_read_info;
      }

      if ( info_ptr->frames_directory_ptr[i].total_sz > info_ptr->biggest_frame_blob_sz ) {
        info_ptr->biggest_frame_blob_sz = info_ptr->frames_directory_ptr[i].total_sz;
        biggest_frame_idx               = i;
      }
    }

    fclose( f_ptr );
    f_ptr = NULL; // this is checked later, so make = NULL
  }

  _dbprint( "Allocating preallocated_frame_blob_ptr bytes %u (frame %i)\n", (uint32_t)info_ptr->biggest_frame_blob_sz, biggest_frame_idx );
  if ( info_ptr->biggest_frame_blob_sz >= 1024 * 1024 * 1024 ) {
    _dbprinterr( __LINE__, "ERROR: extremely high frame size %u reported - assuming error.\n", (uint32_t)info_ptr->biggest_frame_blob_sz );
    goto failed_to_read_info;
  }
  info_ptr->preallocated_frame_blob_ptr = calloc( 1, info_ptr->biggest_frame_blob_sz );
  if ( !info_ptr->preallocated_frame_blob_ptr ) {
    _dbprinterr( __LINE__, "ERROR: out of memory allocating frame blob reserve.\n" );
    goto failed_to_read_info;
  }

  return true;

failed_to_read_info:

  _dbprinterr( __LINE__, "ERROR: Failed to parse info from vologram geometry files.\n" );
  if ( f_ptr ) { fclose( f_ptr ); }
  if ( record.byte_ptr ) {
    _dbprint( "Freeing record.byte_ptr\n" );
    free( record.byte_ptr );
  }
  vol_geom_free_file_info( info_ptr );

  return false;
}

bool vol_geom_free_file_info( vol_geom_info_t* info_ptr ) {
  if ( !info_ptr ) { return false; }

  if ( info_ptr->preallocated_frame_blob_ptr ) {
    _dbprint( "Freeing preallocated_frame_blob_ptr\n" );
    free( info_ptr->preallocated_frame_blob_ptr );
  }
  if ( info_ptr->frame_headers_ptr ) {
    _dbprint( "Freeing frame_headers_ptr\n" );
    free( info_ptr->frame_headers_ptr );
  }
  if ( info_ptr->frames_directory_ptr ) {
    _dbprint( "Freeing frames_directory_ptr\n" );
    free( info_ptr->frames_directory_ptr );
  }
  *info_ptr = ( vol_geom_info_t ){ .hdr.frame_count = 0 };

  return true;
}

bool vol_geom_is_keyframe( const vol_geom_info_t* info_ptr, int frame_idx ) {
  assert( info_ptr );
  if ( !info_ptr ) { return false; }
  if ( frame_idx < 0 || frame_idx >= info_ptr->hdr.frame_count ) { return false; }
  if ( 0 == info_ptr->frame_headers_ptr[frame_idx].keyframe ) { return false; }
  return true;
}

int vol_geom_find_previous_keyframe( const vol_geom_info_t* info_ptr, int frame_idx ) {
  assert( info_ptr );
  if ( !info_ptr ) { return -1; }
  if ( frame_idx < 0 || frame_idx >= info_ptr->hdr.frame_count ) { return false; }
  for ( int i = frame_idx; i >= 0; i-- ) {
    if ( vol_geom_is_keyframe( info_ptr, i ) ) { return i; }
  }
  return -1;
}
