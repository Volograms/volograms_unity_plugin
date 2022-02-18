/** @file vol_av.c
 * Volograms SDK Audio-Video Decoding API
 *
 * Version:   0.8.0 \n
 * Authors:   Anton Gerdelan <anton@volograms.com> \n
 * Copyright: 2021, Volograms (http://volograms.com/) \n
 * Language:  C99 \n
 * Licence:   The MIT License. See LICENSE.md for details. \n
 */

#include "vol_av.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h> // av_image_get_buffer_size()
#include <libswscale/swscale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VOL_AV_LOG_STR_MAX_LEN 512 // Careful - this is stored on the stack to be thread and memory-safe so don't make it too large.

/** Internal ffmepg-specific context variables. This struct lives inside the vol_av_video_t interface struct. */
struct vol_av_internal_t {
  // Video File Codec Context
  AVFormatContext* fmt_ctx_ptr;  /** Holds the header information from the format (Container). http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html */
  AVCodec* codec_ptr;            /** Video codec - decoder found for file format. http://ffmpeg.org/doxygen/trunk/structAVCodec.html */
  AVCodecContext* codec_ctx_ptr; /** Video codec context. https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html */
  int video_stream_idx;          /** The valid video stream index we found by looping over the stream ptrs. */

  // Current Decoded Frame Output
  AVFrame* output_frame_ptr;     /** Decoded frame in native format. // https://ffmpeg.org/doxygen/trunk/structAVFrame.html */
  AVFrame* output_frame_rgb_ptr; /** Conversion of `output_frame_ptr` to a RGB format for use in engines. */
  uint8_t* internal_buffer_ptr;  /** Temporary decoding storage. */

  // Tools
  struct SwsContext* sws_conv_ctx_ptr; /** Scaling/image conversion context. */

  int w, h; /** Dimensions of `output_frame_rgb_ptr`. */
};

static void _default_logger( vol_av_log_type_t log_type, const char* message_str ) {
  FILE* stream_ptr = ( VOL_AV_LOG_TYPE_ERROR == log_type || VOL_AV_LOG_TYPE_WARNING == log_type ) ? stderr : stdout;
  fprintf( stream_ptr, "%s", message_str );
}

static void ( *_logger_ptr )( vol_av_log_type_t log_type, const char* message_str ) = &_default_logger;

// This function is used in this file as a printf-style logger. It converts that format to a simple string and passes it to _logger_ptr.
static void _vol_loggerf( vol_av_log_type_t log_type, const char* message_str, ... ) {
  char log_str[VOL_AV_LOG_STR_MAX_LEN];
  log_str[0] = '\0';
  va_list arg_ptr; // using va_args lets us make sure any printf-style formatting values are properly written into the string.
  va_start( arg_ptr, message_str );
  vsnprintf( log_str, VOL_AV_LOG_STR_MAX_LEN - 1, message_str, arg_ptr );
  va_end( arg_ptr );
  int len = strlen( log_str );
  _logger_ptr( log_type, log_str );
}

//
//
bool vol_av_open( const char* filename, vol_av_video_t* info_ptr ) {
  if ( !filename || !info_ptr || info_ptr->_context_ptr != NULL ) { return false; }

  _vol_loggerf( VOL_AV_LOG_TYPE_INFO, "opening URL `%s`...\n", filename );

  memset( info_ptr, 0, sizeof( vol_av_video_t ) );
  info_ptr->_context_ptr = calloc( 1, sizeof( vol_av_internal_t ) );
  if ( !info_ptr->_context_ptr ) {
    _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: calloc() failed to allocate memory for internal pointer\n" );
    return false;
  }
  vol_av_internal_t* p = info_ptr->_context_ptr;

  { // Open the file and read its header. The codecs are not opened. -- note that if first param is NULL then this allocates memory.
    if ( avformat_open_input( &p->fmt_ctx_ptr, filename, NULL, NULL ) < 0 ) { // NOTE(Anton) the second param is `url` and we can try a web stream.
      _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: Failed to open input file.\n" );
      return false;
    }
    _vol_loggerf( VOL_AV_LOG_TYPE_INFO, "format: %s, duration: %lld us, bit_rate: %lld\n", p->fmt_ctx_ptr->iformat->name, p->fmt_ctx_ptr->duration,
      p->fmt_ctx_ptr->bit_rate );

    // Read packets from the AVFormatContext to get stream information. this function populates p->fmt_ctx_ptr->streams
    if ( avformat_find_stream_info( p->fmt_ctx_ptr, NULL ) < 0 ) {
      _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: Failed to get stream info.\n" );
      return false;
    }

#ifdef VOL_AV_DEBUG
    // Dump debug information about file onto standard error
    av_dump_format( p->fmt_ctx_ptr, 0, filename, 0 );
#endif

    AVCodecParameters* codec_params_ptr = NULL; // https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html
    p->video_stream_idx                 = -1;
    // Now p->fmt_ctx_ptr->streams is just an array of pointers, so let's walk through it until we find a video stream.
    for ( unsigned int i = 0; i < p->fmt_ctx_ptr->nb_streams; ++i ) {
      _vol_loggerf( VOL_AV_LOG_TYPE_DEBUG, "AVStream->time_base before open coded %d/%d\n", p->fmt_ctx_ptr->streams[i]->time_base.num,
        p->fmt_ctx_ptr->streams[i]->time_base.den );
      _vol_loggerf( VOL_AV_LOG_TYPE_DEBUG, "AVStream->r_frame_rate before open coded %d/%d\n", p->fmt_ctx_ptr->streams[i]->r_frame_rate.num,
        p->fmt_ctx_ptr->streams[i]->r_frame_rate.den );
      _vol_loggerf( VOL_AV_LOG_TYPE_DEBUG, "AVStream->start_time %lld\n", p->fmt_ctx_ptr->streams[i]->start_time );
      _vol_loggerf( VOL_AV_LOG_TYPE_DEBUG, "AVStream->duration %lld\n", vol_av_duration_s( info_ptr ) );
      // NOTE(Anton) this is an update from using deprecated codec pointer (p->fmt_ctx_ptr->streams[i]->codec->codec_type).
      AVCodecParameters* tmp_codec_params_ptr = p->fmt_ctx_ptr->streams[i]->codecpar;
      if ( !tmp_codec_params_ptr ) {
        _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: unsupported codec parameters!\n" );
        continue;
      }
      // find proper decoder
      const AVCodec* tmp_codec_ptr = avcodec_find_decoder( tmp_codec_params_ptr->codec_id ); // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
      if ( !tmp_codec_ptr ) {
        _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: unsupported codec!\n" );
        continue;
      }

      if ( tmp_codec_params_ptr->codec_type == AVMEDIA_TYPE_VIDEO ) {
        if ( p->video_stream_idx == -1 ) {
          p->video_stream_idx = i;
          p->codec_ptr        = (AVCodec*)tmp_codec_ptr;
          codec_params_ptr    = tmp_codec_params_ptr;
        }
        _vol_loggerf( VOL_AV_LOG_TYPE_DEBUG, "Video Codec: resolution %dx%d\n", tmp_codec_params_ptr->width, tmp_codec_params_ptr->height );
      } else if ( tmp_codec_params_ptr->codec_type == AVMEDIA_TYPE_AUDIO ) {
        _vol_loggerf( VOL_AV_LOG_TYPE_DEBUG, "Audio Codec: %d channels, sample rate %d\n", tmp_codec_params_ptr->channels, tmp_codec_params_ptr->sample_rate );
      }

      // print its name, id and bitrate
      _vol_loggerf( VOL_AV_LOG_TYPE_DEBUG, "\tCodec %s ID %d bit_rate %lld\n", tmp_codec_ptr->name, tmp_codec_ptr->id, tmp_codec_params_ptr->bit_rate );
    }

    if ( p->video_stream_idx == -1 ) {
      _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: Failed to find video stream.\n" );
      return false;
    }

    p->codec_ctx_ptr = avcodec_alloc_context3( p->codec_ptr );
    if ( !p->codec_ctx_ptr ) {
      _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: failed to allocate memory for AVCodecContext\n" );
      return false;
    }
    // Fill the codec context based on the values from the supplied codec parameters
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
    if ( avcodec_parameters_to_context( p->codec_ctx_ptr, codec_params_ptr ) < 0 ) {
      _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: failed to copy codec params to codec context\n" );
      return false;
    }

    // Initialise the AVCodecContext to use the given AVCodec.
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
    if ( avcodec_open2( p->codec_ctx_ptr, p->codec_ptr, NULL ) < 0 ) {
      _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: failed to open codec through avcodec_open2\n" );
      return false;
    }
  } // endblock Video Codec Context

  { // Allocate Frame Storage
    p->output_frame_ptr     = av_frame_alloc();
    p->output_frame_rgb_ptr = av_frame_alloc();
    if ( !p->output_frame_ptr || !p->output_frame_rgb_ptr ) {
      _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: Failed to allocate frame storage.\n" );
      return false;
    }
    p->output_frame_rgb_ptr->format = AV_PIX_FMT_RGB24;
    p->output_frame_rgb_ptr->width  = p->codec_ctx_ptr->width;
    p->output_frame_rgb_ptr->height = p->codec_ctx_ptr->height;

    // NOTE(Anton) This API is quite confusing
    // The allocated image buffer has to be freed by using av_freep(&pointers[0]).
    int align = 32;                        // NOTE(Anton) I haven no idea if this is correct!!
    int ret   = av_image_alloc(            //
      p->output_frame_rgb_ptr->data,     // ubyte*[4]	pointers
      p->output_frame_rgb_ptr->linesize, //  int[4]	linesizes NOTE(Anton) should be 32
      p->codec_ctx_ptr->width,           //  int	w
      p->codec_ctx_ptr->height,          //  int h
      AV_PIX_FMT_RGB24,                  // AVPixelFormat pix_fmt
      align                              // int	align_
    );
    if ( ret < 0 ) {
      _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: failed to allocate and set up output image buffer.\n" );
      return false;
    }
    /* "Setup the data pointers and linesizes based on the specified image parameters and the provided array.
    The fields of the given image are filled in by using the srcaddress which points to the image data buffer.
    Depending on thespecified pixel format, one or multiple image data pointers andline sizes will be set.
    If a planar format is specified, several pointers will be set pointing to the different picture planes and
    the line sizes of the different planes will be stored in thelines_sizes array. Call with src == NULL to get the required
    size for the src buffer.
    To allocate the buffer and fill in the dst_data and dst_linesize in one call, use av_image_alloc()." */
    // av_image_fill_arrays(); // NOTE(Anton) so i guess this means i don't need to call this function?
  } // endblock Allocate Frame Storage

  { // init SWS context for software scaling
    // This function can crash if it gets bad params so let's check for those first
    if ( AV_PIX_FMT_NONE == p->codec_ctx_ptr->pix_fmt ) {
      _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: failed to get SWS context - pixel format of stream was NONE.\n" );
      return false;
    }

    p->sws_conv_ctx_ptr = sws_getContext( //
      p->codec_ctx_ptr->width,            // src w
      p->codec_ctx_ptr->height,           // src h
      p->codec_ctx_ptr->pix_fmt,          // src format
      p->codec_ctx_ptr->width,            // dst w
      p->codec_ctx_ptr->height,           // dst h
      AV_PIX_FMT_RGB24,                   // dst format
      SWS_BILINEAR,                       // scaling flags
      NULL, NULL, NULL                    // filters and param
    );
  } // endblock init SWS context
  return true;
}

//
//
bool vol_av_close( vol_av_video_t* info_ptr ) {
  if ( !info_ptr || !info_ptr->_context_ptr ) { return false; }

  _vol_loggerf( VOL_AV_LOG_TYPE_INFO, "Releasing all the resources...\n" );

  vol_av_internal_t* p = info_ptr->_context_ptr;

  if ( p->fmt_ctx_ptr ) { avformat_close_input( &p->fmt_ctx_ptr ); }
  if ( p->output_frame_ptr ) { av_frame_free( &p->output_frame_ptr ); }
  if ( p->output_frame_rgb_ptr ) {
    av_freep( &p->output_frame_rgb_ptr->data[0] );
    av_frame_free( &p->output_frame_rgb_ptr );
  }
  if ( p->codec_ctx_ptr ) { avcodec_free_context( &p->codec_ctx_ptr ); }

  // tools
  if ( p->sws_conv_ctx_ptr ) { sws_freeContext( p->sws_conv_ctx_ptr ); }

  free( info_ptr->_context_ptr );                  // this is our internal struct we allocated
  memset( info_ptr, 0, sizeof( vol_av_video_t ) ); // wipe for subsequent use

  return true;
}

//
//
static void _save_rgb_frame( vol_av_video_t* info_ptr ) {
  vol_av_internal_t* p = info_ptr->_context_ptr;

  info_ptr->w = p->output_frame_ptr->width;
  info_ptr->h = p->output_frame_ptr->height;
  //   printf("[vol_av] DEBUG - frame wxh %ix%i linesize %i\n", info_ptr->w, info_ptr->h, p->output_frame_rgb_ptr->linesize[0] );
  // Convert the image from its native format to RGB
  sws_scale( p->sws_conv_ctx_ptr,                     // context.
    (uint8_t const* const*)p->output_frame_ptr->data, // src slice. NOTE(Anton) Pedantic const cast required.
    p->output_frame_ptr->linesize,                    // src stride.
    0,                                                // slice y.
    info_ptr->h,                                      // slice h.
    p->output_frame_rgb_ptr->data,                    // dst.
    p->output_frame_rgb_ptr->linesize                 // dst stride.
  );
  // Remember that you can cast an AVFrame pointer to an AVPicture pointer.
  // can now save or use this data and increment frame counter
  info_ptr->pixels_ptr = p->output_frame_rgb_ptr->data[0]; // [0] is the first (red) channel. output usually has 3 but can have 4 channels.
}

//
//
static int _decode_packet( vol_av_video_t* info_ptr, AVPacket* packet_ptr ) {
  vol_av_internal_t* p = info_ptr->_context_ptr;

  /*
  * Important Note:
  *
  * Return codes such as AVERROR_EOF and AVERROR( EAGAIN ) are negative return codes, but not necessarily errors. We still need to decode packets left behind
  after finding EOF, and there may be additional trailing packets that need processing after finding EOF.
  * All /other/ negative values are errors.
  * The first few frame reads often produce EAGAIN codes, meaning we need to call frame read again to buffer the full frame of packets (otherwise the first few
  frames of the video sequence are blank).
  * This is IMO an error-prone error handling API design and lots of examples/tutorials misunderstand this and end up dropping the last few frames in a video.
  * A better video processing API would handle these states internally, not require the programmer to call 'read_frame' an unpredictable number of times in
  order to read a frame, and then loop over unknown numbers of packets that spit out unreliably.
  * -- Anton.
  */

  // Supply raw packet data as input to a decoder
  int response = avcodec_send_packet( p->codec_ctx_ptr, packet_ptr ); // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3
  if ( response < 0 && response != AVERROR_EOF ) {
    _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: while sending a packet to the decoder: %s\n", av_err2str( response ) );
    return response;
  }

  // Counter to escape infinite loop if API requests a frame off the end of the video and it doesn't exist.
  const int overflow_retry_limit = 16; // NOTE(Anton) I have no idea how many overflow frames there can be - assume it's <5 !
  int overflow_retry_count       = 0;

  while ( ( response >= 0 || response == AVERROR_EOF ) && ( overflow_retry_count < overflow_retry_limit ) ) {
    // Return decoded output data (into a frame) from a decoder
    response = avcodec_receive_frame( p->codec_ctx_ptr, p->output_frame_ptr ); // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
    if ( response == AVERROR( EAGAIN ) ) {                                     //|| response == AVERROR_EOF
      return response;
    } else if ( response < 0 && response != AVERROR_EOF ) {
      _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: while receiving a frame from the decoder: %s\n", av_err2str( response ) );
      return response;
    }
    // NOTE(Anton) i think this is always true >=0 at this point.
    if ( response >= 0 ) {
      _vol_loggerf( VOL_AV_LOG_TYPE_DEBUG, "Frame %d (type=%c, size=%d bytes, format=%d) pts %d key_frame %d [DTS %d]\n", p->codec_ctx_ptr->frame_number,
        av_get_picture_type_char( p->output_frame_ptr->pict_type ), p->output_frame_ptr->pkt_size, p->output_frame_ptr->format, p->output_frame_ptr->pts,
        p->output_frame_ptr->key_frame, p->output_frame_ptr->coded_picture_number );
      _save_rgb_frame( info_ptr );
      return response;
    }
    overflow_retry_count++;
  } // endwhile

  return response;
}

//
//
bool vol_av_read_next_frame( vol_av_video_t* info_ptr ) {
  if ( !info_ptr || !info_ptr->_context_ptr ) {
    _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: info_ptr || !info_ptr->_context_ptr NULL.\n" );
    return false;
  }

  vol_av_internal_t* p = info_ptr->_context_ptr;

  AVPacket* packet_ptr = av_packet_alloc(); // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
  if ( !packet_ptr ) {
    _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: Failed to allocated memory for AVPacket.\n" );
    return false;
  }
  int packet_response = -1;
  // fill the Packet with data from the Stream
  // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61

  // Try a few times, in case there are dangling packets at the beginning or end of file to clean up. -- Anton.
  for ( int i = 0; i < 8; i++ ) {
    if ( av_read_frame( p->fmt_ctx_ptr, packet_ptr ) >= 0 ) {
      // if it's the video stream
      if ( packet_ptr->stream_index == p->video_stream_idx ) {
        packet_response = _decode_packet( info_ptr, packet_ptr );
        if ( packet_response == AVERROR( EAGAIN ) || packet_response == AVERROR_EOF ) {
          av_packet_unref( packet_ptr );
          continue;
        }
        // https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2
        break;
      }      // endwhile
    } else { // maybe there are some leftover packets from last read
      if ( packet_ptr->stream_index == p->video_stream_idx ) {
        packet_response = _decode_packet( info_ptr, packet_ptr );
        if ( packet_response == AVERROR( EAGAIN ) || packet_response == AVERROR_EOF ) {
          av_packet_unref( packet_ptr );
          continue;
        }
        break;
      } // endwhile
    }
  }

  av_packet_unref( packet_ptr );
  av_packet_free( &packet_ptr );

  if ( packet_response < 0 && packet_response != AVERROR_EOF && packet_response != AVERROR( EAGAIN ) ) {
    _vol_loggerf( VOL_AV_LOG_TYPE_ERROR, "ERROR: packet response was %i.\n", packet_response );
    return false;
  }

  return true;
}

//
//
void vol_av_dimensions( const vol_av_video_t* info_ptr, int* w, int* h ) {
  if ( !info_ptr || !info_ptr->_context_ptr || !w || !h ) { return; }

  vol_av_internal_t* p = info_ptr->_context_ptr;
  *w                   = p->codec_ctx_ptr->width;
  *h                   = p->codec_ctx_ptr->height;
}

//
//
double vol_av_frame_rate( const vol_av_video_t* info_ptr ) {
  if ( !info_ptr || !info_ptr->_context_ptr ) { return 0.0; }

  vol_av_internal_t* p = info_ptr->_context_ptr;
  int v_idx            = p->video_stream_idx;
  AVStream* v_strm     = p->fmt_ctx_ptr->streams[v_idx];
  AVRational avfr      = v_strm->avg_frame_rate;
  if ( avfr.den <= 0 ) { return 0.0; } // Safety catch.
  double frame_rate = (double)avfr.num / (double)avfr.den;
  return frame_rate;
}

//
//
int64_t vol_av_frame_count( const vol_av_video_t* info_ptr ) {
  if ( !info_ptr || !info_ptr->_context_ptr ) { return 0; }

  vol_av_internal_t* p = info_ptr->_context_ptr;
  int v_idx            = p->video_stream_idx;
  AVStream* v_strm     = p->fmt_ctx_ptr->streams[v_idx];
  // this variable is 0 if nb_frames "is not known" by libav
  int64_t n_frames = v_strm->nb_frames;
  if ( 0 != n_frames ) { return n_frames; }

  // if so i'll try to calculate it NB sir fred is 1799 (frames 1 to 1800) so i think i need a +1 here in calculations that would otherwise start at 0
  double duration_s   = vol_av_duration_s( info_ptr );
  double framerate_hz = vol_av_frame_rate( info_ptr );
  if ( framerate_hz <= 0.0 ) { return 0; }
  double seconds_per_frame = 1.0f / framerate_hz;
  n_frames                 = (int64_t)( duration_s / seconds_per_frame ) + 1;
  return n_frames;
}

//
//
double vol_av_duration_s( const vol_av_video_t* info_ptr ) {
  if ( !info_ptr || !info_ptr->_context_ptr ) { return 0.0; }
  if ( AV_TIME_BASE <= 0 ) { return 0.0; } // Safety catch for div by zero.
  vol_av_internal_t* p = info_ptr->_context_ptr;
  int64_t duration     = p->fmt_ctx_ptr->duration; // eg. output.mpg 5394000.  Duration: 00:00:59.93. in AV_TIME_BASE units.
  double duration_s    = duration / (double)AV_TIME_BASE;
  return duration_s;
}

//
//
void vol_av_set_log_callback( void ( *user_function_ptr )( vol_av_log_type_t log_type, const char* message_str ) ) {
  _logger_ptr = user_function_ptr;
}

void vol_av_reset_log_callback( void ) { 
  _logger_ptr = &_default_logger;
}
