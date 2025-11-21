/** @file vol_interface.c
 * Volograms SDK Audio-Video Decoding API
 *
 * Version:   0.2 \n
 * Authors:   Patrick Geoghegan <patrick@volograms.com> \n
 *            Anton Gerdelan <anton@volograms.com> \n
 *            Jan Ondrej <jan@volograms.com> \n
 * Copyright: 2021-2025, Volograms (http://volograms.com/) \n
 * Language:  C99 \n
 * Licence:   The MIT License. See LICENSE.md for details. \n
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h> // include uint types
#include <stddef.h>/* size_t */
#include <string.h> // include memcpy()
#ifdef _WIN32
#include <malloc.h> // include alloca()
#include <windows.h> /* for backtraces and timers */
#else
#include <unistd.h> // Added only for debugging, should be removed for builds
#include <alloca.h> // include alloca()
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#include "vol_av.h"
#include "vol_geom.h"
#include "vol_basis.h"

#ifdef _WIN32
#define DllExport __declspec (dllexport)
#else
#define DllExport __attribute__(( visibility("default") ))
#endif

#if __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <math.h> // TODO: Remove when test function `Plasma` is removed
    
#ifdef ENABLE_UNITY_RENDER_FUNCS
#include "IUnityRenderingExtensions.h"
#endif

/**
 * Per-instance native context to support multiple volograms in one process.
 * This replaces previous global/static state.
 */
typedef struct vol_context_t {
    // Geometry
    vol_geom_info_t geom_file_info;
    vol_geom_frame_data_t geom_frame_data;

    // Basis decode temporary buffer
    uint8_t* output_blocks_ptr;

    // Video
    vol_av_video_t video_file_ptr;
    int vid_w;
    int vid_h;
    double vid_dur;
    int64_t vid_num_frms;
    int vid_frm_size;

    // Streaming configuration/state
    vol_geom_streaming_config_t streaming_config;
} vol_context_t;

// Default singleton context to preserve backward compatibility with legacy exports.
static vol_context_t* _default_ctx = 0;
static vol_context_t* _get_default_ctx( void ) {
    if ( !_default_ctx ) {
        _default_ctx = (vol_context_t*)malloc( sizeof( vol_context_t ) );
        if ( _default_ctx ) {
            memset( _default_ctx, 0, sizeof( vol_context_t ) );
        }
    }
    return _default_ctx;
}

DllExport vol_context_t* native_vol_context_create( void ) {
    vol_context_t* ctx = (vol_context_t*)malloc( sizeof( vol_context_t ) );
    if ( !ctx ) { return 0; }
    memset( ctx, 0, sizeof( vol_context_t ) );
    return ctx;
}

DllExport void native_vol_context_destroy( vol_context_t* ctx ) {
    if ( !ctx ) { return; }
    if ( ctx->output_blocks_ptr ) {
        free( ctx->output_blocks_ptr );
        ctx->output_blocks_ptr = 0;
    }
    if ( ctx->video_file_ptr._context_ptr ) {
        vol_av_close( &ctx->video_file_ptr );
    }
    if ( ctx->geom_file_info.hdr.frame_count > 0 || ctx->geom_file_info.frames_directory_ptr ) {
        vol_geom_free_file_info( &ctx->geom_file_info );
    }
    memset( ctx, 0, sizeof( vol_context_t ) );
    free( ctx );
}

//  Unity logging taken from: https://stackoverflow.com/questions/43732825/use-debug-log-from-c 
/** Unity logging callback type */
typedef void( *vol_interface_log_callback )( int type, const char* message );
typedef void( *vol_geom_log_callback )( vol_geom_log_type_t type, const char* message );
typedef void( *vol_av_log_callback )( vol_av_log_type_t type, const char* message ); 

/** Print message to log file
 @param str     Message to print
 @return        If the operation was successful
 */
static bool _str_to_logfile( const char* str ) {
    FILE* f_ptr = fopen( "log.txt", "a" );
    if ( !f_ptr ) { return false; }
    fprintf( f_ptr, "%s\n", str );
    if ( 0 != fclose( f_ptr ) ) { return false; }
    return true;
}

/** Default debug logging function
 @param message     Message to log
 @param color       Color of the text
 @param size        Size of the message in bytes
 */
void default_print( int type, const char* message )
{
    _str_to_logfile(message);
}

/** Debug logging function */
static vol_interface_log_callback log_callback = default_print;

/** Register a new debug logging function
 @param cb      New debug logging callback function
 */
DllExport void register_debug_callback( vol_interface_log_callback cb ) {
    log_callback = cb;
}

DllExport void register_geom_log_callback( vol_geom_log_callback cb ) {
    vol_geom_set_log_callback( cb );
}

DllExport void register_av_log_callback( vol_av_log_callback cb ) {
    vol_av_set_log_callback( cb );
}

DllExport void clear_logging_functions( void ) {
    log_callback = default_print;
    vol_av_reset_log_callback();
    vol_geom_reset_log_callback();
}

#ifdef VOL_TEST_TIMERS
static uint64_t _frequency = 1000000, _offset;

void apg_time_init( void ) {
#ifdef _WIN32
  uint64_t counter;
  _frequency = 1000; // QueryPerformanceCounter default
  QueryPerformanceFrequency( (LARGE_INTEGER*)&_frequency );
  QueryPerformanceCounter( (LARGE_INTEGER*)&_offset );
#elif __APPLE__
  mach_timebase_info_data_t info;
  mach_timebase_info( &info );
  _frequency       = ( info.denom * 1e9 ) / info.numer;
  _offset          = mach_absolute_time();
#else
  _frequency = 1000000000; // nanoseconds
  struct timespec ts;
  clock_gettime( CLOCK_MONOTONIC, &ts );
  _offset = (uint64_t)ts.tv_sec * (uint64_t)_frequency + (uint64_t)ts.tv_nsec;
#endif
}

double apg_time_s( void ) {
#ifdef _WIN32
  uint64_t counter = 0;
  QueryPerformanceCounter( (LARGE_INTEGER*)&counter );
  return (double)( counter - _offset ) / _frequency;
#elif __APPLE__
  uint64_t counter = mach_absolute_time();
  return (double)( counter - _offset ) / _frequency;
#else
  struct timespec ts;
  clock_gettime( CLOCK_MONOTONIC, &ts );
  uint64_t counter = (uint64_t)ts.tv_sec * (uint64_t)_frequency + (uint64_t)ts.tv_nsec;
  return (double)( counter - _offset ) / _frequency;
#endif
}
#endif // VOL_TEST_TIMERS

/**
 * Geometry file
 */

/** Structs moved into per-instance context (see vol_context_t) */

/** Open the geometry file
 @param hdr_filename    Path to the header file
 @param seq_filename    Path to the sequence file
 @returns               If the operation was successful
 */
DllExport bool native_vol_open_geom_file_ctx( vol_context_t* ctx, const char* hdr_filename, const char* seq_filename, bool streaming_mode )
{
    if ( !ctx ) { return false; }
    memset( &ctx->geom_file_info, 0, sizeof( vol_geom_info_t ) );
    bool opened = false;
    if ( hdr_filename && hdr_filename[0] != '\0' )
        opened = vol_geom_create_file_info( hdr_filename, seq_filename, &ctx->geom_file_info, streaming_mode );
    else 
        opened = vol_geom_create_file_info_from_file( seq_filename, &ctx->geom_file_info );
    
    if ( !opened )
        return false;
        
    memset( &ctx->geom_frame_data, 0, sizeof(vol_geom_frame_data_t));
    
    return true;
}
DllExport bool native_vol_open_geom_file(const char* hdr_filename, const char* seq_filename, bool streaming_mode)
{
    return native_vol_open_geom_file_ctx( _get_default_ctx(), hdr_filename, seq_filename, streaming_mode );
}

/** Clears the loaded geometry data
 @returns   `true` if the file closed successfully, `false` otherwise
 */
DllExport bool native_vol_free_geom_data_ctx( vol_context_t* ctx )
{
    if ( !ctx ) { return false; }
    bool ret = vol_geom_free_file_info( &ctx->geom_file_info );
    return ret;
}
DllExport bool native_vol_free_geom_data(void)
{
    return native_vol_free_geom_data_ctx( _get_default_ctx() );
}

/** Get the number of frames in the geometry file
 @returns   Number of geometry frames in the file
 */
DllExport int native_vol_get_geom_frame_count_ctx( vol_context_t* ctx )
{
    if ( !ctx ) { return 0; }
    return ctx->geom_file_info.hdr.frame_count;
}
DllExport int native_vol_get_geom_frame_count(void)
{
    return native_vol_get_geom_frame_count_ctx( _get_default_ctx() );
}

/** Reads the specified geometry frame
 @param seq_filename    The path to the geometry file
 @param frame           Index of the frame you want to read
 @returns               If the operation was a success
 */
DllExport bool native_vol_read_geom_frame_ctx(const char* seq_filename, int frame, vol_context_t* ctx) 
{
    if ( !ctx ) { return false; }
    if ( frame >= (int32_t) ctx->geom_file_info.hdr.frame_count )
        return false;

    bool ret = vol_geom_read_frame( seq_filename, &ctx->geom_file_info, frame, &ctx->geom_frame_data );
    return ret; 
}
DllExport bool native_vol_read_geom_frame(const char* seq_filename, int frame) 
{
    return native_vol_read_geom_frame_ctx( seq_filename, frame, _get_default_ctx() );
}

/**
 * @returns Returns true if the given frame_idx is valid and is also a keyframe, in the currently opened vologram's geometry.
 */
DllExport bool native_vol_geom_is_keyframe_ctx( int frame_idx, vol_context_t* ctx ) {
    if ( !ctx ) { return false; }
    return vol_geom_is_keyframe( &ctx->geom_file_info, frame_idx );
}
DllExport bool native_vol_geom_is_keyframe( int frame_idx ) {
    return native_vol_geom_is_keyframe_ctx( frame_idx, _get_default_ctx() );
}

/**
 * @returns Returns the index of the keyframe prior to frame_idx, in the currently opened vologram's geometry.
 */
DllExport int native_vol_geom_find_previous_keyframe_ctx( int frame_idx, vol_context_t* ctx ) {
    if ( !ctx ) { return -1; }
    return vol_geom_find_previous_keyframe( &ctx->geom_file_info, frame_idx );
}
DllExport int native_vol_geom_find_previous_keyframe( int frame_idx ) {
    return native_vol_geom_find_previous_keyframe_ctx( frame_idx, _get_default_ctx() );
}

/** Get the geometry data of the current loaded frame
 @returns   Struct containing details of the geometry data
 */
DllExport vol_geom_frame_data_t native_vol_get_geom_ptr_data_ctx(vol_context_t* ctx)
{
    if ( !ctx ) { vol_geom_frame_data_t zero; memset( &zero, 0, sizeof zero ); return zero; }
    return ctx->geom_frame_data;
}
DllExport vol_geom_frame_data_t native_vol_get_geom_ptr_data(void)
{
    return native_vol_get_geom_ptr_data_ctx( _get_default_ctx() );
}

/** Gets the geom info struct including the data of the last loaded mesh
 @returns   Struct containing the geometry info
 */
DllExport vol_geom_info_t native_vol_get_geom_info_ctx(vol_context_t* ctx)
{
    if ( !ctx ) { vol_geom_info_t zero; memset( &zero, 0, sizeof zero ); return zero; }
    return ctx->geom_file_info;
}
DllExport vol_geom_info_t native_vol_get_geom_info(void)
{
    return native_vol_get_geom_info_ctx( _get_default_ctx() );
}

/** Check if audio data is present in the vologram
 * @returns   true if audio data is present, false otherwise
 */
DllExport bool native_vol_has_audio_ctx( vol_context_t* ctx ) {
    if ( !ctx ) { return false; }
    vol_geom_info_t vol_info = native_vol_get_geom_info_ctx( ctx );
    return (bool) vol_info.hdr.audio;
}
DllExport bool native_vol_has_audio(void) {
    return native_vol_has_audio_ctx( _get_default_ctx() );
}

/** Get pointer to the audio data
* @returns   Pointer to the audio data
*/
DllExport uint8_t* native_vol_get_audio_ctx(vol_context_t* ctx, int* outSize)
{
    if ( !ctx ) { return 0; }
    vol_geom_info_t vol_info = native_vol_get_geom_info_ctx( ctx );
    if (!vol_info.hdr.audio) {
        return 0;
    }
    if ( outSize ) { *outSize = vol_info.audio_data_sz; }
    return vol_info.audio_data_ptr;
}
DllExport uint8_t* native_vol_get_audio(int* outSize)
{
    return native_vol_get_audio_ctx( _get_default_ctx(), outSize );
}

/** Update missing items in frames directory initially created by vol_geom_create_file_info_from_file.
 @param seq_filename    Path to the sequence file
 @param frame           Index of the frame you want to update
 @returns               If the operation was a success
 */
DllExport bool native_vol_update_frames_directory_ctx(vol_context_t* ctx, const char* seq_filename, int frame) {
    if ( !ctx ) { return false; }
    vol_geom_info_t vol_info = native_vol_get_geom_info_ctx( ctx );
    return vol_geom_update_frames_directory(seq_filename, &vol_info, frame);
}
DllExport bool native_vol_update_frames_directory(const char* seq_filename, int frame) {
    return native_vol_update_frames_directory_ctx( _get_default_ctx(), seq_filename, frame );
}

/**
 * Basis Texture from Vols File
 */

DllExport bool native_vol_basis_init_ctx( vol_context_t* ctx )
{
    if ( !ctx ) { return false; }
    bool res = vol_basis_init();
    if (!res) {
        log_callback(4, "basis_init - vol_basis_init failed\n");
        return false;
    }
    return true;
}
DllExport bool native_vol_basis_init(void)
{
    return native_vol_basis_init_ctx( _get_default_ctx() );
}

/** Read the next frame of the video
 @returns   Pointer to the video frame pixel data
 */
DllExport uint8_t * native_vol_read_next_texture_frame_ctx( vol_context_t* ctx, int format )
{
    if ( !ctx ) { return 0; }
    vol_geom_info_t vol_info = native_vol_get_geom_info_ctx( ctx );
    if(vol_info.hdr.textured) {
        
        uint32_t texture_size = vol_info.hdr.texture_width * vol_info.hdr.texture_height*3;

        if(ctx->output_blocks_ptr == 0)
            ctx->output_blocks_ptr = (uint8_t*)malloc( texture_size );

        vol_geom_frame_data_t vols_frame_data = native_vol_get_geom_ptr_data_ctx( ctx );

        int w = 0, h = 0;
        uint8_t* vols_texture_ptr = (uint8_t*)&vols_frame_data.block_data_ptr[vols_frame_data.texture_offset];
        int32_t vols_texture_sz = vols_frame_data.texture_sz;

        if (!vol_basis_transcode(format, vols_texture_ptr, vols_texture_sz, ctx->output_blocks_ptr, texture_size, &w, &h)) {
            log_callback(3, "Decoding basis texture failed!");
            return 0;
        }
        return ctx->output_blocks_ptr;
    } else {
        return 0;
    }
}
DllExport uint8_t * native_vol_read_next_texture_frame( int format )
{
    return native_vol_read_next_texture_frame_ctx( _get_default_ctx(), format );
}

/** Get the size of a video frame in bytes
 @returns   The number of bytes in a video frame
 */
DllExport int64_t native_vol_get_texture_frame_size_ctx( vol_context_t* ctx )
{
    if ( !ctx ) { return 0; }
    vol_geom_info_t vol_info = native_vol_get_geom_info_ctx( ctx );
    return vol_info.hdr.texture_width * vol_info.hdr.texture_height*3;
}
DllExport int64_t native_vol_get_texture_frame_size(void)
{
    return native_vol_get_texture_frame_size_ctx( _get_default_ctx() );
}


/** Get the width in pixels of the video
 @returns   The pixel width of the video
 */
DllExport int native_vol_get_texture_width_ctx( vol_context_t* ctx )
{
    if ( !ctx ) { return 0; }
    vol_geom_info_t vol_info = native_vol_get_geom_info_ctx( ctx );
    return vol_info.hdr.texture_width;
}
DllExport int native_vol_get_texture_width(void)
{
    return native_vol_get_texture_width_ctx( _get_default_ctx() );
}

/** Get the height in pixels of the video
 @returns   The pixel height of the video
 */
DllExport int native_vol_get_texture_height_ctx( vol_context_t* ctx )
{
    if ( !ctx ) { return 0; }
    vol_geom_info_t vol_info = native_vol_get_geom_info_ctx( ctx );
    return vol_info.hdr.texture_height;
}
DllExport int native_vol_get_texture_height(void)
{
    return native_vol_get_texture_height_ctx( _get_default_ctx() );
}

/**
 * Video File
 */

/** Open the video texture file for a vologram
 @param filename    Path to the video texture file
 @returns           `true` if file was opened sucessfully, `false` otherwise
 */
DllExport bool native_vol_open_video_file_ctx( vol_context_t* ctx, const char* filename )
{
    if ( !ctx ) { return false; }
    memset( &ctx->video_file_ptr, 0, sizeof(vol_av_video_t));
    bool ret = vol_av_open(filename, &ctx->video_file_ptr);
#ifdef VOL_TEST_TIMERS
    apg_time_init();
#endif
    if ( ret ) {
        vol_av_dimensions( &ctx->video_file_ptr, &ctx->vid_w, &ctx->vid_h );
        ctx->vid_num_frms = vol_av_frame_count( &ctx->video_file_ptr );
        ctx->vid_dur = vol_av_duration_s( &ctx->video_file_ptr );
        ctx->vid_frm_size = ctx->vid_w * ctx->vid_h * 3;
    }
    
    return ret;
}
DllExport bool native_vol_open_video_file(const char* filename)
{
    return native_vol_open_video_file_ctx( _get_default_ctx(), filename );
}

/** Close the video texture file
 @returns   `true` if the file was closed sucessfully, `false` otherwise
 */
DllExport bool native_vol_close_video_file_ctx( vol_context_t* ctx )
{
    if ( !ctx ) { return false; }
    ctx->vid_w = 0;
    ctx->vid_h = 0;
    ctx->vid_dur = 0.0;
    ctx->vid_num_frms = 0;
    ctx->vid_frm_size = 0;
    return vol_av_close( &ctx->video_file_ptr );
}
DllExport bool native_vol_close_video_file(void)
{
    return native_vol_close_video_file_ctx( _get_default_ctx() );
}

/** Get the width in pixels of the video
 @returns   The pixel width of the video
 */
DllExport int native_vol_get_video_width_ctx( vol_context_t* ctx )
{
    if ( !ctx ) { return 0; }
    return ctx->vid_w;
}
DllExport int native_vol_get_video_width(void)
{
    return native_vol_get_video_width_ctx( _get_default_ctx() );
}

/** Get the height in pixels of the video
 @returns   The pixel height of the video
 */
DllExport int native_vol_get_video_height_ctx( vol_context_t* ctx )
{
    if ( !ctx ) { return 0; }
    return ctx->vid_h;
}
DllExport int native_vol_get_video_height(void)
{
    return native_vol_get_video_height_ctx( _get_default_ctx() );
}

/** Get the rate of playback in frames per second of the video
 @returns   The frame rate of the video
 */
DllExport double native_vol_get_video_frame_rate_ctx( vol_context_t* ctx )
{
    if ( !ctx ) { return 0.0; }
    // It's safer to check on demand because this can change during playback.
    return vol_av_frame_rate( &ctx->video_file_ptr );
}
DllExport double native_vol_get_video_frame_rate(void)
{
    return native_vol_get_video_frame_rate_ctx( _get_default_ctx() );
}

/** Get the number of frames in the video
 @returns   The frame count of the video
 */
DllExport int64_t native_vol_get_video_frame_count_ctx( vol_context_t* ctx )
{
    if ( !ctx ) { return 0; }
    return ctx->vid_num_frms;
}
DllExport int64_t native_vol_get_video_frame_count(void)
{
    return native_vol_get_video_frame_count_ctx( _get_default_ctx() );
}

/** Get the length of the video in seconds
 @returns   The duration of the video
 */
DllExport double native_vol_get_video_duration_ctx( vol_context_t* ctx )
{
    if ( !ctx ) { return 0.0; }
    return ctx->vid_dur;
}
DllExport double native_vol_get_video_duration(void)
{
    return native_vol_get_video_duration_ctx( _get_default_ctx() );
}

/** Get the size of a video frame in bytes
 @returns   The number of bytes in a video frame
 */
DllExport int64_t native_vol_get_video_frame_size_ctx( vol_context_t* ctx )
{
    if ( !ctx ) { return 0; }
    return ctx->vid_frm_size;
}
DllExport int64_t native_vol_get_video_frame_size(void)
{
    return native_vol_get_video_frame_size_ctx( _get_default_ctx() );
}

/** Vertically mirror image memory by swapping the top half of rows with the bottom half.
 * This function directly modifies the original memory. That is, bytes_ptr is input and output.
 * eg tightly packged RGB image memory for a 512x512 image would be
 * _image_flip_vertical( bytes_ptr, 512, 512, 3 );
 * RGBA would be
 * _image_flip_vertical( bytes_ptr, 512, 512, 4 );
 * @param bytes_ptr         Pointer to the image pixel data
 * @param width             Pixel width of the image
 * @param height            Pixel height of the image
 * @param bytes_per_pixel   Number of bytes in a single pixel
 */
static void _image_flip_vertical( uint8_t* bytes_ptr, int width, int height, int bytes_per_pixel ) {
  if ( !bytes_ptr || 0 == height ) { return; } // invalid image
  int row_stride = width * bytes_per_pixel;
  // probably an invalid param/massive image - this could cause stack overflow in alloca().
  if ( row_stride <= 0 || row_stride > 1024 * 1024 ) { return; }
  // allocate fast *stack* memory for doing the copy
  uint8_t* tmp_row_ptr = (uint8_t *) alloca( row_stride );
  // go half way down image and swap with opposing row
  for ( int i = 0; i < height / 2; i++ ) {                       // so if height == 5 only go to 0 and 1, and ignore 3.
    int mirror_i            = height - 1 - i;                    // index of row we want to swap with
    uint8_t* row_ptr        = &bytes_ptr[i * row_stride];        // address of our row in memory
    uint8_t* mirror_row_ptr = &bytes_ptr[mirror_i * row_stride]; // address of opposite row (bottom half)
    memcpy( tmp_row_ptr, row_ptr, row_stride );                  // our row -> tmp
    memcpy( row_ptr, mirror_row_ptr, row_stride );               // row on other side -> our row
    memcpy( mirror_row_ptr, tmp_row_ptr, row_stride );           // tmp -> row on other side
  }
}

/** Read the next frame of the video
 @returns   Pointer to the video frame pixel data
 */
DllExport uint8_t * native_vol_read_next_video_frame_ctx( vol_context_t* ctx, bool flip_vertical )
{
    if ( !ctx ) { return 0; }
    vol_av_read_next_frame( &ctx->video_file_ptr );
    if ( flip_vertical ) { _image_flip_vertical(ctx->video_file_ptr.pixels_ptr, ctx->vid_w, ctx->vid_h, 3); }
    return ctx->video_file_ptr.pixels_ptr;
}
DllExport uint8_t * native_vol_read_next_video_frame( bool flip_vertical )
{
    return native_vol_read_next_video_frame_ctx( _get_default_ctx(), flip_vertical );
}
    
#ifdef ENABLE_UNITY_RENDER_FUNCS
/**
 UNITY RENDERING FUNCTIONS
 */
    
    // FOR TESTING REMOVE
    uint32_t Plasma(int x, int y, int width, int height, unsigned int frame)
    {
        float px = (float)x / width;
        float py = (float)y / height;
        float time = frame / 60.0f;

        float l = sinf(px * sinf(time * 1.3f) + sinf(py * 4 + time) * sinf(time));

        uint32_t r = sinf(l *  6) * 127 + 127;
        uint32_t g = sinf(l *  7) * 127 + 127;
        uint32_t b = sinf(l * 10) * 127 + 127;

        return r + (g << 8) + (b << 16) + 0xff000000u;
    }
    
void _texture_update_callback( int event_id, void *data)
{
    if (event_id == kUnityRenderingExtEventUpdateTextureBeginV2)
    {
        // UpdateTextureBegin: Generate and return texture image data.
        UnityRenderingExtTextureUpdateParamsV2 *params = data;
                        
        //uint32_t *img = malloc(params->width * params->height * 4);
        //memcpy(img, video_file_ptr.pixels_ptr, params->width * params->height * 3);
        
        //params->format = kUnityRenderingExtFormatR8G8B8_UInt;
        params->texData = video_file_ptr.pixels_ptr;
    }
    else if (event_id == kUnityRenderingExtEventUpdateTextureEndV2)
    {
        // UpdateTextureEnd: Free up the temporary memory.
        UnityRenderingExtTextureUpdateParamsV2 *params = data;
        if ( params->texData )
        {
            free(params->texData);
        }
    }
}
    
UnityRenderingEventAndData UNITY_INTERFACE_API get_texture_update_callback(void)
{
    return _texture_update_callback;
}

#endif

/**
 * Circular Streaming Buffer
 */

// Configuration
DllExport bool native_vol_init_streaming_config_ctx( vol_context_t* ctx ) {
    if ( !ctx ) { return false; }
    return vol_geom_init_streaming_config(&ctx->streaming_config);
}
DllExport bool native_vol_init_streaming_config() {
    return native_vol_init_streaming_config_ctx( _get_default_ctx() );
}

DllExport bool native_vol_should_use_streaming_mode_ctx( vol_context_t* ctx, int64_t file_size ) {
    if ( !ctx ) { return false; }
    return vol_geom_should_use_streaming_mode(file_size, &ctx->streaming_config);
}
DllExport bool native_vol_should_use_streaming_mode(int64_t file_size) {
    return native_vol_should_use_streaming_mode_ctx( _get_default_ctx(), file_size );
}

DllExport void native_vol_set_max_buffer_size_ctx( vol_context_t* ctx, int64_t bytes ) {
    if ( !ctx ) { return; }
    ctx->streaming_config.max_buffer_size = bytes;
}
DllExport void native_vol_set_max_buffer_size(int64_t bytes) {
    native_vol_set_max_buffer_size_ctx( _get_default_ctx(), bytes );
}

DllExport void native_vol_set_lookahead_seconds_ctx( vol_context_t* ctx, float seconds ) {
    if ( !ctx ) { return; }
    ctx->streaming_config.lookahead_seconds = seconds;
}
DllExport void native_vol_set_lookahead_seconds(float seconds) {
    native_vol_set_lookahead_seconds_ctx( _get_default_ctx(), seconds );
}

DllExport bool native_vol_create_streaming_buffer_ctx( vol_context_t* ctx ) {
    
    if ( !ctx ) { return false; }
    memset(&ctx->geom_file_info, 0, sizeof(vol_geom_info_t));
    
    return vol_geom_create_streaming_buffer(&ctx->geom_file_info, &ctx->streaming_config);

    memset(&ctx->geom_frame_data, 0, sizeof(vol_geom_frame_data_t));
}
DllExport bool native_vol_create_streaming_buffer() {
    return native_vol_create_streaming_buffer_ctx( _get_default_ctx() );
}

// Add data
DllExport bool native_vol_add_data_to_buffer_ctx( vol_context_t* ctx, const uint8_t* data_ptr, int64_t data_size ) {
	// pass in directly as we are changing it. 
    if ( !ctx ) { return false; }
    return vol_geom_add_data_to_buffer(&ctx->geom_file_info, data_ptr, data_size);
}
DllExport bool native_vol_add_data_to_buffer(const uint8_t* data_ptr, int64_t data_size) {
    return native_vol_add_data_to_buffer_ctx( _get_default_ctx(), data_ptr, data_size );
}

// Frame directory
DllExport bool native_vol_update_buffer_frame_directory_ctx( vol_context_t* ctx ) {
    if ( !ctx ) { return false; }
    return vol_geom_update_buffer_frame_directory(&ctx->geom_file_info);
}
DllExport bool native_vol_update_buffer_frame_directory() {
    return native_vol_update_buffer_frame_directory_ctx( _get_default_ctx() );
}

// Frame reading
DllExport bool native_vol_read_frame_streaming_ctx( vol_context_t* ctx, uint32_t frame_idx ) {
    
    if ( !ctx ) { return false; }
    if (frame_idx >= (int32_t)ctx->geom_file_info.hdr.frame_count)
        return false;

    bool ret = vol_geom_read_frame_streaming(&ctx->geom_file_info, frame_idx, &ctx->geom_frame_data);
    return ret;
}
DllExport bool native_vol_read_frame_streaming(uint32_t frame_idx) {
    return native_vol_read_frame_streaming_ctx( _get_default_ctx(), frame_idx );
}

DllExport bool native_vol_is_frame_available_in_buffer_ctx( vol_context_t* ctx, uint32_t frame_idx ) {
    if ( !ctx ) { return false; }
    vol_geom_info_t g_info = native_vol_get_geom_info_ctx( ctx );
    return vol_geom_is_frame_available_in_buffer(&g_info, frame_idx);
}
DllExport bool native_vol_is_frame_available_in_buffer(uint32_t frame_idx) {
    return native_vol_is_frame_available_in_buffer_ctx( _get_default_ctx(), frame_idx );
}

// Buffer management
DllExport bool native_vol_update_buffer_state_ctx( vol_context_t* ctx ) {
    if ( !ctx ) { return false; }
    return vol_geom_update_buffer_state(&ctx->geom_file_info);
}
DllExport bool native_vol_update_buffer_state() {
    return native_vol_update_buffer_state_ctx( _get_default_ctx() );
}

DllExport bool native_vol_is_download_buffer_full_ctx( vol_context_t* ctx ) {
    if ( !ctx ) { return false; }
    vol_geom_info_t g_info = native_vol_get_geom_info_ctx( ctx );
    return vol_geom_is_download_buffer_full(&g_info);
}
DllExport bool native_vol_is_download_buffer_full() {
    return native_vol_is_download_buffer_full_ctx( _get_default_ctx() );
}

DllExport bool native_vol_should_resume_download_ctx( vol_context_t* ctx, uint32_t current_frame, float fps ) {
    if ( !ctx ) { return false; }
    return vol_geom_should_resume_download(&ctx->geom_file_info, current_frame, fps);
}
DllExport bool native_vol_should_resume_download(uint32_t current_frame, float fps) {
    return native_vol_should_resume_download_ctx( _get_default_ctx(), current_frame, fps );
}

DllExport float native_vol_get_buffer_health_seconds_ctx( vol_context_t* ctx, float fps ) {
    if ( !ctx ) { return 0.0f; }
    return vol_geom_get_buffer_health_seconds(&ctx->geom_file_info, fps);
}
DllExport float native_vol_get_buffer_health_seconds(float fps) {
    return native_vol_get_buffer_health_seconds_ctx( _get_default_ctx(), fps );
}


DllExport bool native_vol_create_streaming_file_info_ctx( vol_context_t* ctx ) {
    if ( !ctx ) { return false; }
    return vol_geom_create_streaming_file_info(&ctx->geom_file_info);
}
DllExport bool native_vol_create_streaming_file_info() {
    return native_vol_create_streaming_file_info_ctx( _get_default_ctx() );
}

DllExport int native_vol_get_header_frame_body_start_ctx( vol_context_t* ctx ) {
    if ( !ctx ) { return 0; }
    return vol_geom_get_sequence_offset(&ctx->geom_file_info);
}
DllExport int native_vol_get_header_frame_body_start(void) {
    return native_vol_get_header_frame_body_start_ctx( _get_default_ctx() );
}

DllExport void native_vol_reset_frame_directory_ctx( vol_context_t* ctx ) {
    if ( !ctx ) { return; }
    return vol_geom_reset_frame_directory(&ctx->geom_file_info);
}
DllExport void native_vol_reset_frame_directory(void) {
    return native_vol_reset_frame_directory_ctx( _get_default_ctx() );
}

DllExport int native_vol_get_playback_buffer_size_ctx( vol_context_t* ctx ) {
    if ( !ctx ) { return 0; }
    vol_geom_size_t buffer_size = 0;
    const uint8_t* buffer = vol_geom_get_playback_buffer(&ctx->geom_file_info, &buffer_size);
    return buffer ? (int)buffer_size : 0;
}
DllExport int native_vol_get_playback_buffer_size(void) {
    return native_vol_get_playback_buffer_size_ctx( _get_default_ctx() );
}


#if __cplusplus
}
#endif
