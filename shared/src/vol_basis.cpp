/** @file vol_basis.cpp
 * Volograms Basis Universal Wrapper API
 *
 * vol_basis | .basis Transcoding Wrapper.
 * --------- | ---------------------
 * Version   | 0.1
 * Authors   | See matching header file.
 * Copyright | 2023, Volograms (http://volograms.com/)
 * Language  | C++
 * Files     | 2
 * Licence   | The MIT License. See LICENSE.md for details.
 */

#include "vol_basis.h"
#include "basis_universal/transcoder/basisu_transcoder.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

bool vol_basis_init( void ) {
  basist::basisu_transcoder_init();
  return true;
}

bool vol_basis_transcode( //
  int format,
  void* data_ptr,             // Input: Basis-compressed data from sequence frame.
  uint32_t data_sz,           // Input: Data size in bytes from sequence frame.
  uint8_t* output_blocks_ptr, // Output: Transcoded compressed texture data to use.
  uint32_t output_blocks_sz,  //
  int* w_ptr, int* h_ptr      // Output: Dimensions of texture.
) {
  if ( !data_ptr || 0 == data_sz || !output_blocks_ptr || 0 == output_blocks_sz || !w_ptr || !h_ptr ) {
    fprintf( stderr, "ERROR vol_basis_transcode invalid params {%p,%i,%p,%i,%p,%p}\n", (void*)data_ptr, data_sz, (void*)output_blocks_ptr, output_blocks_sz, (void*)w_ptr, (void*)h_ptr ); // TODO: remove
    return false;
  }

  basist::basisu_transcoder trans;
  if ( !trans.start_transcoding( data_ptr, data_sz ) ) {
    fprintf( stderr, "ERROR vol_basis_transcode transcoding failed\n" ); // TODO: remove
    return false;
  }
  // basist::basis_texture_type basisTextureType = trans.get_texture_type( data_ptr, data_sz );
  // uint32_t n_images                           = trans.get_total_images( data_ptr, data_sz );
  basist::basisu_image_info image_info;
  uint32_t image_index = 0;
  trans.get_image_info( data_ptr, data_sz, image_info, image_index );
  basist::basisu_image_level_info level_info;
  uint32_t level_index = 0;
  trans.get_image_level_info( data_ptr, data_sz, level_info, image_index, level_index );
  // VERY IMPORTANT! These need to match! https://github.com/BinomialLLC/basis_universal/wiki/OpenGL-texture-format-enums-table
  basist::transcoder_texture_format fmt = (basist::transcoder_texture_format)format;
  if ( !trans.get_ready_to_transcode() ) {
    fprintf( stderr, "ERROR vol_basis_transcode not ready to transcode.\n" );
    return false;
  }
  if ( !trans.transcode_image_level( data_ptr, data_sz, image_index, level_index, output_blocks_ptr, output_blocks_sz, fmt ) ) {
    fprintf( stderr, "ERROR vol_basis_transcode transcoding image level failed.\n" );
    return false;
  }
  *w_ptr = level_info.m_width;
  *h_ptr = level_info.m_height;

  return true;
}
