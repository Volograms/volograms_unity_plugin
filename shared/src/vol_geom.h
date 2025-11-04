/**  @file vol_geom.h
 * Volograms Geometry Decoding API
 *
 * vol_geom  | .vol Geometry Decoding API
 * --------- | ---------------------
 * Version   | 0.13
 * Authors   | Anton Gerdelan     <anton@volograms.com>
 *           | Patrick Geoghegan  <patrick@volograms.com>
 *           | Jan Ondrej         <jan@volograms.com>
 * Copyright | 2021 - 2025, Volograms (http://volograms.com/)
 * Language  | C99
 * Files     | 2
 * Licence   | The MIT License. See LICENSE.md for details.
 *
 * Core library code that reads geometry data for a VOL sequence.
 * These functions are to be built into an application/engine and called from engine code.
 *
 * Eventually
 * ----------
 * - allow custom allocator
 * - removes statics to make thread-safe
 * 
 * History
 * -------
 * - 0.13.0 (2025/10/30) - Enhanced streaming with circular buffer, configurable buffer size, and automatic mode selection.
 * - 0.12.0 (2024/11/02) - Support for streaming, incomplete files, and frame skipping.
 * - 0.11.0 (2022/04/01)   - Support for reading single-file volograms.
 * - 0.10.0 (2022/03/22) - Support added for reading >2GB volograms.
 * - 0.9.0  (2022/03/22) - Version bump for parity with vol_av.
 * - 0.7.1  (2021/01/24) - New option streaming_mode paramter to vol_geom_create_file_info().
 *                        Set to false for typical phone captures to pre-load the sequence file to reduce disk I/O at run-time.
 * - 0.7.0  (2021/01/20) - Added customisable debug callback.
 * - 0.6.1  (2021/11/25) - Patched 0.6 to add file memory size validation vulnerabilities reported by fuzzer.
 * - 0.6    (2021/11/24) - Better memory allocation and management - improved performance & simpler API should reduce risk of accidental memory leaks.
 * - 0.5    (2021/11/15) - Better platform consistency with specified byte-size type.
 * - 0.4    (2021/10/15) - Updated licence and copyright information.
 * - 0.3    (2021/08/25) - Moved some assertions out and into test program.
 * - 0.2    (2021/08/23) - Added declspec API exports, prefixed types and function names properly with vol_geom_
 * - 0.1.1               - Renamed from vol_read to vol_geom.
 */

#pragma once

#ifdef _WIN32
/** If building a library with Visual Studio, we need to explicitly 'export' symbols. This generates a .lib file to go with the .dll dynamic library file. */
#define VOL_GEOM_EXPORT __declspec( dllexport )
#else
/** If building a library with Visual Studio, we need to explicitly 'export' symbols. This generates a .lib file to go with the .dll dynamic library file. */
#define VOL_GEOM_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif /* CPP */

#include <stdbool.h>
#include <stdint.h>

/** Using a specified-size type instead of size_t for better platform consistency. */
typedef int64_t vol_geom_size_t; // Note that signed int64 should be compatible with off_t.

/** Helper struct to store Unity-style strings from VOL file. */
VOL_GEOM_EXPORT typedef struct vol_geom_short_str_t {
  /// Bytes of string.
  char bytes[128];
  /// Size of string in bytes.
  uint8_t sz;
} vol_geom_short_str_t;

// .vols file header supporting 1.0 to 1.3.
VOL_GEOM_EXPORT typedef struct vol_geom_file_hdr_t {
  vol_geom_short_str_t format;      // Removed in v1.3. Must be leading byte of decimal value 4, then "VOLS".
  uint32_t version;                 // 10->v1.0, 11->v1.1 etc.
  uint32_t compression;             // 0 (> 0 if we use quantisation of some of the mesh data).
  vol_geom_short_str_t mesh_name;   // Removed in v1.3.
  vol_geom_short_str_t material;    // Removed in v1.3.
  vol_geom_short_str_t shader;      // Removed in v1.3.
  uint32_t topology;                // Removed in v1.3.
  uint32_t frame_count;             //
  bool normals;                     // Added in v1.2.
  bool textured;                    // Added in v1.2.
  uint8_t texture_compression;      // Added in v1.3.
  uint8_t texture_container_format; // Added in v1.3.
  uint32_t texture_width;           // Changed from int16_t to uint32_t in v1.3.
  uint32_t texture_height;          // Changed from int16_t to uint32_t in v1.3.
  float fps;                        // Added in v1.3. Most volograms are 30.0, but allows 29.97 and similar.
  uint32_t audio;                   // Added in v1.3. ( 0: no audio ).
  uint32_t audio_start;             // Added in v1.3. Start of audio chunk (containing size+data) as byte offset from start of file. Should be 44 in v1.3.
  uint32_t frame_body_start;        // Added in v1.3. Byte offset from start of file. Without audio = 48, otherwise 48 + audio file size;
  uint16_t texture_format;          // Removed in v1.3. Follows UnityEngine.TextureFormat enum.
  float translation[3];             // Removed in v1.3. Added in v1.2.
  float rotation[4];                // Removed in v1.3. Added in v1.2. w, x, y, z. where [1,0,0,0] is identity, face towards +z and +y being upwards.
  float scale;                      // Removed in v1.3. Added in v1.2.
} vol_geom_file_hdr_t;

/** Items from the actual frame header (excluding the trailing size item, which is a repeat of mesh_data_sz but after the data). */
VOL_GEOM_EXPORT typedef struct vol_geom_frame_hdr_t {
  uint32_t frame_number;
  /* V10, 11 = Size of Vertices Data + Normals Data + Indices Data + UVs Data + Texture Data.
   V12 = Size of Vertices Data + Normals Data + Indices Data + UVs Data + Texture Data + 4 Bytes for each "Size of Array" in Vertices, Normals, Indices, UVs
   and Texture (if present)
   */
  /// Mesh data size in bytes.
  uint32_t mesh_data_sz;
  /// id of a keyframe. For keyframe this is the same as frame_number.
  uint32_t keyframe_number;
  /// 0 = tracked frame, 1 = first/key frame, 2 = last tracked frame (for backward traversal, only if Version >= 12)
  uint8_t keyframe;
} vol_geom_frame_hdr_t;

/** Directory entry for a frame's byte offsets and sizes. */
VOL_GEOM_EXPORT typedef struct vol_geom_frame_directory_entry_t {
  /// Addresses of where the header part of this frame starts in the file.
  vol_geom_size_t offset_sz;
  /// Total size of the frame in bytes. =frame_hdr_sz+correctd_payload_sz+4 (+4 is a trailing integer after mesh data)
  vol_geom_size_t total_sz;
  /// Size of the stuff in vol_geom_frame_hdr_t before the mesh data i.e. use offset_sz + hdr_sz to get to the mesh data.
  vol_geom_size_t hdr_sz;
  /// mesh_data_sz + everything not accounted for by mesh_data_sz in older versions of spec.
  vol_geom_size_t corrected_payload_sz;
} vol_geom_frame_directory_entry_t;

/** Configuration structure for streaming buffer behavior.
 * This structure defines how the streaming buffer system operates for large files.
 * Default values are set by vol_geom_init_streaming_config().
 */
 VOL_GEOM_EXPORT typedef struct vol_geom_streaming_config_t {
  /// Maximum size of the circular buffer in bytes. Default: 200MB (200 * 1024 * 1024).
  vol_geom_size_t max_buffer_size;
  /// Minimum buffer size before switching to full download mode. Default: 50MB.
  vol_geom_size_t min_buffer_size;
  /// Reserved space for first frame and keyframes for loop support. Default: 10MB.
  vol_geom_size_t reserved_space_size;
  /// If true, automatically select streaming vs full download based on file size. Default: true.
  bool auto_select_mode;
  /// If true, force streaming mode even for small files. Default: false.
  bool force_streaming_mode;
  /// How many seconds of content to buffer ahead of current playback position. Default: 2.0.
  float lookahead_seconds;
} vol_geom_streaming_config_t;

/** Simple frame information for dual buffer streaming.
 * This structure tracks frame locations within a linear buffer - much simpler than circular buffer approach.
 */
// Note: vol_geom_buffer_frame_info_t removed - using unified vol_geom_frame_directory_entry_t



/** Internal state for managing the dual-buffer streaming system.
 * This structure tracks the current state of the streaming buffers and is managed internally.
 * Do not manually modify these values - use the provided API functions.
 */
VOL_GEOM_EXPORT typedef struct vol_geom_buffer_state_t {
  /// Single linear buffer used as a sliding window over the stream.
  uint8_t* ring_buffer;
  /// Total capacity of the buffer in bytes.
  vol_geom_size_t ring_capacity;
  /// Number of valid bytes currently stored in the buffer [0, ring_capacity].
  vol_geom_size_t data_size;
  /// Offset of the logical start (head) of valid data within ring [0, ring_capacity).
  vol_geom_size_t head_offset;
  /// Parsing cursor within [0, data_size] used to discover new frames.
  vol_geom_size_t parse_pos;

  // Note: Frame directory unified with frames_directory_ptr in vol_geom_info_t

  /// Current position in the remote file being downloaded (byte offset).
  vol_geom_size_t file_pos;
  /// Monotonic file position at the current ring head (bytes already evicted logically).
  vol_geom_size_t head_file_pos;
  /// Total size of the remote file, if known. 0 if unknown.
  vol_geom_size_t file_size;
  /// True if currently using streaming mode.
  bool is_streaming_mode;

  /// Copy of the streaming configuration used for this buffer.
  vol_geom_streaming_config_t config;
  /// Last playback frame observed (for eviction/compaction decisions).
  uint32_t last_playback_frame;
  /// Running average of parsed frame sizes (bytes), used for resume heuristics.
  vol_geom_size_t avg_frame_size;
} vol_geom_buffer_state_t;

/** Meta-data about the whole Vologram sequence. Load this once with `vol_geom_create_file_info()` before using the Vologram. */
VOL_GEOM_EXPORT typedef struct vol_geom_info_t {
  vol_geom_file_hdr_t hdr;
  /// Size of the audio data in bytes. This value is from the start of the audio chunk, before the data.
  uint32_t audio_data_sz;
  /// Pointer to the audio data, if it exists, otherwise NULL.
  uint8_t* audio_data_ptr;
  /// Vologram's directory of blob contents. NOTE(Anton) - this could really be stored the binary file spec right after the header similar to IFF.
  vol_geom_frame_directory_entry_t* frames_directory_ptr;
  /// Pointer to frame header structs for each frame.
  vol_geom_frame_hdr_t* frame_headers_ptr;
  /// This is a pre-allocated block of memory, large enough to store the data of any frame in the vologram sequence. Do not manually allocate or free this memory!
  uint8_t* preallocated_frame_blob_ptr;
  /// This is the maximum size of the buffer pointed to by preallocated_frame_blob_ptr.
  vol_geom_size_t biggest_frame_blob_sz;
  /// If streaming_mode was not set then sequence file is read to a blob pointed to by this pointer. Otherwise it is NULL and file I/O occurs on every frame read.
  uint8_t* sequence_blob_byte_ptr;
  /// Byte offset of the sequence chunk from the start of file. For separated hdr/seq files this will be 0.
  vol_geom_size_t sequence_offset;
  /// Frame number that was read the last and is a keayframe. Useful during seek or when skipping frames. 
  uint32_t last_keyframe;
  /// Pointer to streaming buffer state. NULL if not using streaming mode. Do not manually allocate or free this memory!
  vol_geom_buffer_state_t* streaming_buffer_ptr;
} vol_geom_info_t;

/** Meta-data for each from of the Vologram sequence. */
VOL_GEOM_EXPORT typedef struct vol_geom_frame_data_t {
  /// Points into the data offset of vol_geom_info_t->preallocated_frame_blob_ptr.
  /// After calling vol_geom_read_frame() this pointer points into that frame's data section inside vol_geom_info_t->preallocated_frame_blob_ptr.
  /// Do not manually allocate or free this memory!
  uint8_t* block_data_ptr;

  /// Size of the relevant data pointed to by data_ptr (inclusive of vertices and normals etc and the integers giving their sizes).
  vol_geom_size_t block_data_sz;

  // DIRECTORY: offsets, in bytes, of various bits into data_ptr

  /// To access the address of the vertex data:
  /// &data_ptr[vertices_offset]
  /// and this will be vertices_sz bytes worth of tightly-packed vertex data.
  vol_geom_size_t vertices_offset;
  uint32_t vertices_sz;

  /// Only if version >= 11.
  vol_geom_size_t normals_offset;
  uint32_t normals_sz;

  /// If Keyframe == 1 || 2
  vol_geom_size_t indices_offset;
  uint32_t indices_sz;

  /// If Keyframe == 1 || 2
  vol_geom_size_t uvs_offset;
  uint32_t uvs_sz;

  /// Only if Version >= 11. Not applicable if `textured` is not true in the file header.
  vol_geom_size_t texture_offset;
  uint32_t texture_sz;
} vol_geom_frame_data_t;

/** In your application these enum values can be used to filter out or categorise messages given by vol_geom_log_callback. */
typedef enum vol_geom_log_type_t {
  VOL_GEOM_LOG_TYPE_DEBUG = 0, //
  VOL_GEOM_LOG_TYPE_INFO,
  VOL_GEOM_LOG_TYPE_WARNING,
  VOL_GEOM_LOG_TYPE_ERROR,
  VOL_GEOM_LOG_STR_MAX_LEN // Not an error type, just used to count the error types.
} vol_geom_log_type_t;

/** To silence log output or pipe log messages into a user function.
 * @param user_function_ptr
 */
VOL_GEOM_EXPORT void vol_geom_set_log_callback( void ( *user_function_ptr )( vol_geom_log_type_t log_type, const char* message_str ) );

VOL_GEOM_EXPORT void vol_geom_reset_log_callback( void );

/** Read a header from the top of a .vols blob in memory. */
VOL_GEOM_EXPORT bool vol_geom_read_hdr_from_mem( const uint8_t* data_ptr, uint32_t data_sz, vol_geom_file_hdr_t* hdr_ptr, vol_geom_size_t* hdr_sz_ptr );

/** Read a header from the top of a .vols file. */
VOL_GEOM_EXPORT bool vol_geom_read_hdr_from_file( const char* filename, vol_geom_file_hdr_t* hdr_ptr, vol_geom_size_t* hdr_sz_ptr );

/** As vol_geom_create_file_info, but for volograms where the contents { header, sequence } are all in one .vols file. */
VOL_GEOM_EXPORT bool vol_geom_create_file_info_from_file( const char* vols_filename, vol_geom_info_t* info_ptr );

/** Update missing items in frames directory initially created by vol_geom_create_file_info_from_file. */
VOL_GEOM_EXPORT bool vol_geom_update_frames_directory(  const char* seq_filename, vol_geom_info_t* info_ptr, uint32_t frame_idx );

/** Call this function before playing a vologram sequence.
 * It will build a directory of file and frame information about the VOL sequence, and pre-allocate memory.
 * You only need to call this function once per Vologram - you can keep the vol_geom_info_t struct in memory and re-use it during playback.
 * After you have finished using a Vologram, call `vol_geom_free_file_info()` to free memory allocated by this function.
 * This is also true when changing volograms - call `vol_geom_free_file_info()` first before opening a new vologram.
 * @param hdr_filename   Pointer to a char array containing the file path to the Vologram header file. Must not be NULL.
 * @param seq_filename   Pointer to a char array containing the file path to the Vologram sequence file. Must not be NULL.
 * @param info_ptr       Pointer to a `vol_geom_info_t` struct in your application that will be populated by this function. Must not be NULL.
 * @param streaming_mode If set then sequence file is not pre-loaded to memory.
 *                       This allows support of very large or streamed files, but may introduce file I/O performance issues.
 * @returns              Returns false on any error such as not finding the files specified, or failing to read from a file.
 * On failure, any allocated memory will be cleaned up by this function first,
 * so there is no need to call `vol_geom_free_file_info()` when this function returns `false`.
 */
VOL_GEOM_EXPORT bool vol_geom_create_file_info( const char* hdr_filename, const char* seq_filename, vol_geom_info_t* info_ptr, bool streaming_mode );

/** Call this function to free memory allocated by a call to `vol_geom_create_file_info()` and reset struct to defaults.
 * @param info_ptr       Pointer to a `vol_geom_info_t` struct in your application that will be populated by this function. Must not be NULL.
 * @returns              False error such as NULL pointers where allocated memory was expected.
 */
VOL_GEOM_EXPORT bool vol_geom_free_file_info( vol_geom_info_t* info_ptr );

/** This function can be used to determine if a frame can be skipped or has essential keyframe data.
 * @param info_ptr       Collected VOL sequence information created by `vol_geom_create_file_info()`. Must not be NULL.
 * @param frame_idx      Index number of the frame to query within the sequence, starting at 0.
 * @returns              True if frame number `frame_idx` has keyframe value 1 or 2.
 *                       If the frame is a tracked frame (0) or not in the valid range then the function returns false.
 */
VOL_GEOM_EXPORT bool vol_geom_is_keyframe( const vol_geom_info_t* info_ptr, uint32_t frame_idx );

/** Look backwards from a frame to find the previous keyframe. This is useful when seeking to an intermediate frame, so we can load any base data first.
 * @param info_ptr       Pointer to vologram meta-data loaded by a call to vol_geom_create_file_info().
 * @param frame_idx      Index of the current frame to start looking back from. If this frame is a keyframe then the function will return this index.
 * @returns              The index of the first keyframe found going backwards from current frame to 0, inclusive. Returns -1 on error or if no keyframe is found.
 */
VOL_GEOM_EXPORT int vol_geom_find_previous_keyframe( const vol_geom_info_t* info_ptr, uint32_t frame_idx );

/** Read a single frame from a Vologram sequence file.
 * @param seq_filename   Pointer to a char array containing the file path to the Vologram sequence file. Must not be NULL.
 * @param info_ptr       Pointer to a `vol_geom_info_t` struct in your application as populated by a previous call `vol_geom_create_file_info()`.
 * @param frame_idx      Index of the frame you wish to read. Frames start at index 0.
 * @param frame_data_ptr Pointer to a `vol_geom_frame_data_t` struct in your application that this function will populate with data.
 * @returns              False on any error including `frame_idx` range validation, File I/O, and memory allocation.
 */
VOL_GEOM_EXPORT bool vol_geom_read_frame( const char* seq_filename, vol_geom_info_t* info_ptr, uint32_t frame_idx, vol_geom_frame_data_t* frame_data_ptr );

VOL_GEOM_EXPORT int vol_geom_get_sequence_offset( const vol_geom_info_t* info_ptr );

// Reset unified frame directory (invalidate all entries). Useful on loop/seek discontinuities.
VOL_GEOM_EXPORT void vol_geom_reset_frame_directory( vol_geom_info_t* info_ptr );

//
// ===== STREAMING BUFFER API =====
// These functions provide enhanced streaming capabilities with circular buffer support.
// They are designed to work alongside the existing API and provide automatic mode selection.
//

/** Initialize a streaming configuration structure with default values.
 * This function sets up reasonable defaults for streaming buffer behavior.
 * @param config_ptr Pointer to a vol_geom_streaming_config_t struct to be initialized. Must not be NULL.
 * @returns          True on success, false if config_ptr is NULL.
 */
VOL_GEOM_EXPORT bool vol_geom_init_streaming_config( vol_geom_streaming_config_t* config_ptr );

/** Determine whether to use streaming mode based on file size and configuration.
 * This function implements the auto-selection logic for choosing between full download and streaming modes.
 * @param file_size  Size of the file in bytes. Use 0 if size is unknown (will return true for streaming).
 * @param config_ptr Pointer to configuration structure. Must not be NULL.
 * @returns          True if streaming mode should be used, false for full download mode.
 */
VOL_GEOM_EXPORT bool vol_geom_should_use_streaming_mode( vol_geom_size_t file_size, const vol_geom_streaming_config_t* config_ptr );

/** Create and initialize a streaming buffer for the vologram.
 * This function allocates the circular buffer and initializes streaming state.
 * Call this after reading the header but before starting to download sequence data.
 * @param info_ptr   Pointer to vol_geom_info_t struct. Must not be NULL.
 * @param config_ptr Pointer to streaming configuration. Must not be NULL.
 * @returns          True on success, false on memory allocation failure or invalid parameters.
 */
VOL_GEOM_EXPORT bool vol_geom_create_streaming_buffer( vol_geom_info_t* info_ptr, const vol_geom_streaming_config_t* config_ptr );

/** Add downloaded data to the circular streaming buffer.
 * This function manages the circular buffer, handling wraparound and maintaining frame boundaries.
 * @param info_ptr  Pointer to vol_geom_info_t with initialized streaming buffer. Must not be NULL.
 * @param data_ptr  Pointer to new data to add to the buffer. Must not be NULL.
 * @param data_size Size of the data to add in bytes.
 * @returns         True on success, false on buffer overflow or invalid parameters.
 */
VOL_GEOM_EXPORT bool vol_geom_add_data_to_buffer( vol_geom_info_t* info_ptr, const uint8_t* data_ptr, vol_geom_size_t data_size );

// /** Parse a frame header from buffer data.
//  * This function reads and validates frame header information from a buffer location.
//  * @param buffer_ptr     Pointer to buffer containing frame data. Must not be NULL.
//  * @param offset         Byte offset in buffer where frame header starts.
//  * @param header_ptr     Pointer to vol_geom_frame_hdr_t struct to populate. Must not be NULL.  
//  * @param header_size_ptr Pointer to store the size of the parsed header. Must not be NULL.
//  * @returns              True if header was successfully parsed and is valid, false otherwise.
//  */
// VOL_GEOM_EXPORT bool vol_geom_parse_frame_header_from_buffer( const uint8_t* buffer_ptr, vol_geom_size_t offset, vol_geom_frame_hdr_t* header_ptr, vol_geom_size_t* header_size_ptr );

/** Update the frame directory with newly available frames from the buffer.
 * This function scans the buffer for complete frames and updates the frame directory accordingly.
 * It should be called after adding new data to the buffer.
 * @param info_ptr Pointer to vol_geom_info_t with streaming buffer. Must not be NULL.
 * @returns        True on success, false on parsing errors or invalid parameters.
 */
VOL_GEOM_EXPORT bool vol_geom_update_buffer_frame_directory( vol_geom_info_t* info_ptr );
VOL_GEOM_EXPORT bool vol_geom_update_single_buffer_frames( vol_geom_info_t* info_ptr, uint8_t* buffer_to_parse, 
                                          vol_geom_size_t buffer_data_size, 
                                          void* unused_frame_directory, 
                                          uint32_t* unused_frame_count, const char* buffer_name );

/** Read a frame from the streaming buffer instead of a file.
 * This function reads frame data from the circular buffer rather than performing file I/O.
 * @param info_ptr       Pointer to vol_geom_info_t with streaming buffer. Must not be NULL.
 * @param frame_idx      Index of the frame to read.
 * @param frame_data_ptr Pointer to vol_geom_frame_data_t struct to populate. Must not be NULL.
 * @returns              True on success, false if frame is not available or on error.
 */
VOL_GEOM_EXPORT bool vol_geom_read_frame_streaming( vol_geom_info_t* info_ptr, uint32_t frame_idx, vol_geom_frame_data_t* frame_data_ptr );

/** Check if a specific frame is currently available in the streaming buffer.
 * This function can be used to determine if a frame can be read without waiting for more data.
 * @param info_ptr  Pointer to vol_geom_info_t with streaming buffer. Must not be NULL.
 * @param frame_idx Index of the frame to check.
 * @returns         True if frame is available in buffer, false otherwise.
 */
VOL_GEOM_EXPORT bool vol_geom_is_frame_available_in_buffer( const vol_geom_info_t* info_ptr, uint32_t frame_idx );

// /** Get the current buffer health in bytes.
//  * This function returns how many bytes of valid data are currently in the buffer.
//  * @param info_ptr Pointer to vol_geom_info_t with streaming buffer. Must not be NULL.
//  * @returns        Number of bytes of valid data in buffer, or 0 on error.
//  */
// VOL_GEOM_EXPORT vol_geom_size_t vol_geom_get_buffer_health_bytes( const vol_geom_info_t* info_ptr );

/** Get the current buffer health in seconds of content.
 * This function estimates how many seconds of playback content are available in the buffer.
 * @param info_ptr Pointer to vol_geom_info_t with streaming buffer. Must not be NULL.
 * @param fps      Frames per second for the vologram sequence.
 * @returns        Estimated seconds of content available in buffer, or 0.0 on error.
 */
VOL_GEOM_EXPORT float vol_geom_get_buffer_health_seconds( const vol_geom_info_t* info_ptr, float fps );

/** Determine if download should be resumed based on buffer health and current playback position.
 * This function implements the lookahead logic to decide when to resume downloading.
 * @param info_ptr      Pointer to vol_geom_info_t with streaming buffer. Must not be NULL.
 * @param current_frame Current frame being played.
 * @param fps           Frames per second for the vologram sequence.
 * @returns             True if download should be resumed, false if buffer is sufficient.
 */
VOL_GEOM_EXPORT bool vol_geom_should_resume_download( vol_geom_info_t* info_ptr, uint32_t current_frame, float fps );

/** Check if the current download buffer is full and needs to be swapped.
 * This function determines if the download buffer has reached capacity.
 * @param info_ptr Pointer to vol_geom_info_t with streaming buffer. Must not be NULL.
 * @returns        True if download buffer is full and ready for playback.
 */
VOL_GEOM_EXPORT bool vol_geom_is_download_buffer_full( const vol_geom_info_t* info_ptr );

/** Swap the roles of the two buffers (download becomes playback, playback becomes download).
 * This function implements the core buffer swapping logic for the dual-buffer system.
 * @param info_ptr Pointer to vol_geom_info_t with streaming buffer. Must not be NULL.
 * @returns        True on successful swap, false on error.
 */
VOL_GEOM_EXPORT bool vol_geom_update_buffer_state( vol_geom_info_t* info_ptr );

/** Get the current playback buffer and its size.
 * This function returns a pointer to the buffer currently being used for playback.
 * @param info_ptr    Pointer to vol_geom_info_t with streaming buffer. Must not be NULL.
 * @param buffer_size Pointer to store the size of valid data in playback buffer. Must not be NULL.
 * @returns           Pointer to current playback buffer, or NULL on error.
 */
VOL_GEOM_EXPORT const uint8_t* vol_geom_get_playback_buffer( const vol_geom_info_t* info_ptr, vol_geom_size_t* buffer_size );

/** Initialize vologram file info directly from streaming buffer data.
 * This function creates standard vol_geom_info_t structures from streaming buffer data,
 * allowing existing frame reading functions to work with streaming mode.
 * @param info_ptr Pointer to vol_geom_info_t that already has streaming buffer data. Must not be NULL.
 * @returns        True on successful initialization, false on error.
 */
VOL_GEOM_EXPORT bool vol_geom_create_streaming_file_info( vol_geom_info_t* info_ptr );



#ifdef __cplusplus
}
#endif /* CPP */
