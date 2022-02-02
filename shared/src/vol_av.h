/** @file vol_av.h
 * Volograms Audio-Video Decoding API
 *
 * vol_av    | Audio-Video Decoding API
 * --------- | ----------
 * Version   | 0.8
 * Authors   | Anton Gerdelan <anton@volograms.com>
 * Copyright | 2021, Volograms (http://volograms.com/)
 * Language  | C99
 * Files     | 2
 * Licence   | The MIT License. See LICENSE.md for details.
 * Notes     | Internally this uses FFmpeg to stream audio/video from a webm file or other media.
 *
 * Current Limitations
 * -----------
 * * Only video is currently processed, audio is ignored.
 * * Seek frame is not implemented.
 * * Reverse play is not implemented.
 * * Network streaming is not implemented.
 *
 * References
 * -----------
 * - A decent libav/ffmpeg implementation reference material is this tutorial series: https://github.com/mpenkov/ffmpeg-tutorial
 *
 * History
 * -----------
 * - 0.8.0 (2021/01/20) - Added customisable debug callback.
 * - 0.7.1 (2021/12/10) - Tidied comments.
 * - 0.7   (2021/10/15) - Updated copyright and licence notice.
 * - 0.6   (2021/09/17) - Fixes to memory leaks and delete-after-free found in testing and fuzzing.
 * - 0.5   (2021/09/02) - Fixed a memory leak when a file failed to load and vol_av_open() returned false.
 * - 0.4   (2021/08/23) - Tidied comments for Doxygen. Put experimental `seek` functionality behind a macro.
 * - 0.2   (2021/06/04) - Update to use latest FFmpeg API.
 * - 0.1   (2021/05/26) - First version with number and repo.
 */

#pragma once

#ifdef _WIN32
/** If building a library with Visual Studio, we need to explicitly 'export' symbols. This generates a .lib file to go with the .dll dynamic library file. */
#define VOL_AV_EXPORT __declspec( dllexport )
#else
/** If building a library with Visual Studio, we need to explicitly 'export' symbols. This generates a .lib file to go with the .dll dynamic library file. */
#define VOL_AV_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif /* CPP */

#include <stdbool.h>
#include <stdint.h>

/** Forward-declaration of internal video context struct type. */
VOL_AV_EXPORT typedef struct vol_av_internal_t vol_av_internal_t;

/** Context variables for an opened video stream.
Have one copy of this struct in your app per opened mp4 file.
Zero the memory for instances of this struct before use.
*/
VOL_AV_EXPORT typedef struct vol_av_video_t {
  /** Internal context state. Must start == NULL. Should not need to be accessed by the application. */
  vol_av_internal_t* _context_ptr;

  /** Pointer to decoded frame's tightly-packed 3-channel RGB image data. */
  uint8_t* pixels_ptr;
  /** Dimensions of image in `pixels_ptr`. */
  int w, h;
} vol_av_video_t;

/** In your application these enum values can be used to filter out or categorise messages given by vol_av_log_callback. */
typedef enum vol_av_log_type_t {
  VOL_AV_LOG_TYPE_INFO = 0, //
  VOL_AV_LOG_TYPE_DEBUG,
  VOL_AV_LOG_TYPE_WARNING,
  VOL_AV_LOG_TYPE_ERROR,
  VOL_AV_LOG_STR_MAX_LEN // Not an error type, just used to count the error types.
} vol_av_log_type_t;

VOL_AV_EXPORT void vol_av_set_log_callback( void ( *user_function_ptr )( vol_av_log_type_t log_type, const char* message_str ) );
VOL_AV_EXPORT void vol_av_reset_log_callback( void );

/** Open a video file given by `filename`.
 * @param filename File path to the movie file to open. Must not be NULL.
 * @param info_ptr This function populates the struct pointed to with context data about the file. Must not be NULL.
 * @return         False on error. If info_ptr points to a struct where _context_ptr is not initialised to NULL this function will fail and return false.
 */
VOL_AV_EXPORT bool vol_av_open( const char* filename, vol_av_video_t* info_ptr );

/** Close a video file.
 * @param info_ptr The context data for the file to close. Must not be NULL.
 * @return         False on error.
 */
VOL_AV_EXPORT bool vol_av_close( vol_av_video_t* info_ptr );

/**
 * @param info_ptr The context data for the file. Must not be NULL.
 * @param w        Pointer to variable this function will write the video's width in pixels. Must not be NULL.
 * @param h        Pointer to variable this function will write the video's height in pixels. Must not be NULL.
 */
VOL_AV_EXPORT void vol_av_dimensions( const vol_av_video_t* info_ptr, int* w, int* h );

/** Get the frame rate of an opened video file.
 * @param info_ptr The context data for the file. Must not be NULL.
 * @return         The frequency in Hz (frames per second).
 */
VOL_AV_EXPORT double vol_av_frame_rate( const vol_av_video_t* info_ptr );

/** This function returns the number of frames in the file.
 *
 * @warning This function needs improvement. Because libav doesn't usually know the frame count, this then returns the calculated number of frame _durations_
 * not the number of image frames, which is probably this +1. Don't use this function yet except for debugging.
 *
 * @param info_ptr The context data for the file. Must not be NULL.
 * @return         The number of frames in the movie.
 */
VOL_AV_EXPORT int64_t vol_av_frame_count( const vol_av_video_t* info_ptr );

/** Get the duration of an opened video file.
 * @param info_ptr The context data for the file. Must not be NULL.
 * @return         The duration of the video in seconds.
 */
VOL_AV_EXPORT double vol_av_duration_s( const vol_av_video_t* info_ptr );

/** Construct the next frame from an opened video stream.
* @param info_ptr The context data for the file. Must not be NULL.
* @return          False on error or end of file.
* EXAMPLE:
*
while ( next_frame > frame ) {
  if ( !codec_read_next_frame( &video_info ) ) {
    printf( "frames end\n" );
    break;
  }
  frame++;
  if ( next_frame == frame ) {
    // where this function copies bytes of an RGB 3-channel image into an engine-appropriate texture
    gfx_update_texture( &texture, video_info.pixels_ptr, video_info.info_ptr->width, video_info.info_ptr->height, 3 );
  }
}
*/
VOL_AV_EXPORT bool vol_av_read_next_frame( vol_av_video_t* info_ptr );

#ifdef __cplusplus
}
#endif /* CPP */
