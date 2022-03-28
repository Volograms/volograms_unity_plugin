/** @file vol_interface.c
 * Volograms SDK Audio-Video Decoding API
 *
 * Version:   0.1 \n
 * Authors:   Patrick Geoghegan <patrick@volograms.com> \n
 *            Anton Gerdelan <anton@volograms.com> \n
 * Copyright: 2021, Volograms (http://volograms.com/) \n
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
#include "vol_util.h"

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

typedef enum vol_interface_log_type_t {
  VOL_INTERFACE_LOG_TYPE_INFO = 0, //
  VOL_INTERFACE_LOG_TYPE_DEBUG,
  VOL_INTERFACE_LOG_TYPE_WARNING,
  VOL_INTERFACE_LOG_TYPE_ERROR,
  VOL_INTERFACE_LOG_STR_MAX_LEN // Not an error type, just used to count the error types.
} vol_interface_log_type_t;

//  Unity logging taken from: https://stackoverflow.com/questions/43732825/use-debug-log-from-c 
/** Unity logging callback type */
typedef void( *vol_interface_log_callback )( vol_interface_log_type_t type, const char* message );
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
void default_print( vol_interface_log_type_t type, const char* message )
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
    log_callback( VOL_INTERFACE_LOG_TYPE_INFO, "Set interface logging function" );
}

DllExport void register_geom_log_callback( vol_geom_log_callback cb ) {
    vol_geom_set_log_callback( cb );
}

DllExport void register_av_log_callback( vol_av_log_callback cb ) {
    vol_av_set_log_callback( cb );
}

DllExport void clear_logging_functions( void ) {
    log_callback( VOL_INTERFACE_LOG_TYPE_INFO, "Clearing interface logging function" );
    log_callback = default_print;
    vol_av_reset_log_callback();
    vol_geom_reset_log_callback();
}

/**
 * Geometry file
 */

/** Struct containing details of the opened geometry file */
static vol_geom_info_t geom_file_ptr;
/** Struct containing read geometry data */
static vol_geom_frame_data_t geom_frame_data;
/** Track the index number of the loaded geometry frame */
static int curr_geom_frame;

/** Open the geometry file
 @param hdr_filename    Path to the header file
 @param seq_filename    Path to the sequence file
 @returns               If the operation was successful
 */
DllExport bool native_vol_open_geom_file(const char* hdr_filename, const char* seq_filename, bool streaming_mode)
{
    memset(&geom_file_ptr, 0, sizeof(vol_geom_info_t));
    bool opened = vol_geom_create_file_info(hdr_filename, seq_filename, &geom_file_ptr, streaming_mode);
    
    if ( !opened )
        return opened;
        
    memset( &geom_frame_data, 0, sizeof(vol_geom_frame_data_t));
    curr_geom_frame = 0;
    apg_time_init();

    return true;
}

/** Clears the loaded geometry data
 @returns   `true` if the file closed successfully, `false` otherwise
 */
DllExport bool native_vol_free_geom_data(void)
{
    bool ret = vol_geom_free_file_info( &geom_file_ptr );
    return ret;
}

/** Get the number of frames in the geometry file
 @returns   Number of geometry frames in the file
 */
DllExport int native_vol_get_geom_frame_count(void)
{
    return geom_file_ptr.hdr.frame_count;
}

/** Reads the next geometry frame
 @param seq_filename    The path to the geometry file
 @param frame           Index of the frame you want to read
 @returns               If the operation was a success
 */
DllExport bool native_vol_read_geom_frame(const char* seq_filename, int frame) 
{
    if ( frame >= geom_file_ptr.hdr.frame_count )
        return false;

    bool ret = vol_geom_read_frame( seq_filename, &geom_file_ptr, frame, &geom_frame_data );
    curr_geom_frame = frame;
    return ret; 
}

/** Reads the next geometry frame
 @param seq_filename    The path to the geometry file
 @returns               If the operation was a success
 */
DllExport bool native_vol_read_next_geom_frame(const char* seq_filename)
{
    if ( curr_geom_frame >= geom_file_ptr.hdr.frame_count )
        return false;

    char log_string[100];
    double read_time_start = apg_time_s();

    bool ret = vol_geom_read_frame( seq_filename, &geom_file_ptr, curr_geom_frame, &geom_frame_data );
    curr_geom_frame++;

    sprintf( log_string, "read geom frame time: %f ms", ( apg_time_s() - read_time_start ) * 1000.0f );
    log_callback(VOL_INTERFACE_LOG_TYPE_DEBUG, log_string);

    return ret;
}

/** Get the index of the next frame to be loaded
 @returns   The index of the geometry frame to be loaded next
 */
DllExport int native_vol_get_next_geom_frame_index(void)
{
    return curr_geom_frame;
}

/** Get the geometry data of the current loaded frame
 @returns   Struct containing details of the geometry data
 */
DllExport vol_geom_frame_data_t native_vol_get_geom_ptr_data(void)
{
    return geom_frame_data;
}

/** Gets the geom info struct including the data of the last loaded mesh
 @returns   Struct containing the geometry info
 */
DllExport vol_geom_info_t native_vol_get_geom_info(void)
{
    return geom_file_ptr;
}

/**
 * Video File
 */

/** Struct containing details of the loaded video file */
static vol_av_video_t video_file_ptr;
/** The pixel width of the loaded video */
static int vid_w = -1;
/** The pixel width of the loaded video */
static int vid_h = -1;
/** The frame rate of the loaded video */
static double vid_frm_rt = -1;
/** The duration in seconds of the loaded video */
static double vid_dur = -1;
/** The number of frames in the loaded video */
static int64_t vid_num_frms = -1;
/** The number of bytes in a single from of the loaded video */
static int vid_frm_size = -1;

/** Open the video texture file for a vologram
 @param filename    Path to the video texture file
 @returns           `true` if file was opened sucessfully, `false` otherwise
 */
DllExport bool native_vol_open_video_file(const char* filename)
{
    memset( &video_file_ptr, 0, sizeof(vol_av_video_t));
    bool ret = vol_av_open(filename, &video_file_ptr);

    if ( ret ) {
        vol_av_dimensions( &video_file_ptr, &vid_w, &vid_h );
        vid_frm_rt = vol_av_frame_rate( &video_file_ptr );
        vid_num_frms = vol_av_frame_count( &video_file_ptr );
        vid_dur = vol_av_duration_s( &video_file_ptr );
        vid_frm_size = vid_w * vid_h * 3;
    }
    
    return ret;
}

/** Close the video texture file
 @returns   `true` if the file was closed sucessfully, `false` otherwise
 */
DllExport bool native_vol_close_video_file(void)
{
    vid_w = -1;
    vid_h = -1;
    vid_frm_rt = -1;
    vid_dur = -1;
    vid_num_frms = -1;
    vid_frm_size = -1;
    return vol_av_close( &video_file_ptr );
}

/** Get the width in pixels of the video
 @returns   The pixel width of the video
 */
DllExport int native_vol_get_video_width(void)
{
    return vid_w;
}

/** Get the height in pixels of the video
 @returns   The pixel height of the video
 */
DllExport int native_vol_get_video_height(void)
{
    return vid_h;
}

/** Get the rate of playback in frames per second of the video
 @returns   The frame rate of the video
 */
DllExport double native_vol_get_video_frame_rate(void)
{
    return vid_frm_rt;
}

/** Get the number of frames in the video
 @returns   The frame count of the video
 */
DllExport int64_t native_vol_get_video_frame_count(void)
{
    return vid_num_frms;
}

/** Get the length of the video in seconds
 @returns   The duration of the video
 */
DllExport double native_vol_get_video_duration(void)
{
    return vid_dur;
}

/** Get the size of a video frame in bytes
 @returns   The number of bytes in a video frame
 */
DllExport int64_t native_vol_get_video_frame_size(void)
{
    return vid_frm_size;
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

    char log_string[100];
    double read_time_start = apg_time_s();

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

    sprintf( log_string, "image flip time: %f ms", ( apg_time_s() - read_time_start ) * 1000.0f );
    log_callback(VOL_INTERFACE_LOG_TYPE_DEBUG, log_string);
}

/** Read the next frame of the video
 @returns   Pointer to the video frame pixel data
 */
DllExport uint8_t * native_vol_read_next_frame(void)
{
    char log_string[100];
    double read_time_start = apg_time_s();
    vol_av_read_next_frame( &video_file_ptr );
    sprintf( log_string, "read video frame time: %f ms", ( apg_time_s() - read_time_start ) * 1000.0f );
    log_callback(VOL_INTERFACE_LOG_TYPE_DEBUG, log_string);

    _image_flip_vertical(video_file_ptr.pixels_ptr, vid_w, vid_h, 3);
    return video_file_ptr.pixels_ptr;
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

#if __cplusplus
}
#endif
