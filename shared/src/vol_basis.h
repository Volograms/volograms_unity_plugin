/**  @file vol_basis.h
 * Volograms Basis Universal Wrapper API
 *
 * vol_basis | .basis Transcoding Wrapper.
 * --------- | ---------------------
 * Version   | 0.1
 * Authors   | Anton Gerdelan     <anton@volograms.com>
 *           | Patrick Geoghegan  <patrick@volograms.com>
 * Copyright | 2023, Volograms (http://volograms.com/)
 * Language  | C++ (C compatibility header).
 * Files     | 2
 * Licence   | The MIT License. See LICENSE.md for details.
 *
 * History
 * -------
 * - 0.1 (2023/04/26) - First version.
 */

#pragma once

#ifdef _WIN32
/** If building a library with Visual Studio, we need to explicitly 'export' symbols. This generates a .lib file to go with the .dll dynamic library file. */
#define VOL_BASIS_EXPORT __declspec( dllexport )
#else
/** If building a library with Visual Studio, we need to explicitly 'export' symbols. This generates a .lib file to go with the .dll dynamic library file. */
#define VOL_BASIS_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif /* CPP */

#include <stdbool.h>
#include <stdint.h>

VOL_BASIS_EXPORT bool vol_basis_init( void );

VOL_BASIS_EXPORT bool vol_basis_transcode( //
  int format,                              /// Matches transcoder_texture_format enum values from basisu_transcoder.h (cTFBC3_RGBA = 3)
  void* data_ptr,                          // Input: Basis-compressed data from sequence frame.
  uint32_t data_sz,                        // Input: Data size in bytes from sequence frame.
  uint8_t* output_blocks_ptr,              // Output: Transcoded compressed texture data to use.
  uint32_t output_blocks_sz,               //
  int* w_ptr, int* h_ptr                   // Output: Dimensions of texture.
);

#ifdef __cplusplus
}
#endif /* CPP */
