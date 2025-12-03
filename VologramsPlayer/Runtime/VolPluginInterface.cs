// <copyright file=VolPluginInterface company=Volograms>
// Copyright (c) 2022 All Rights Reserved
// </copyright>
// <author>Patrick Geoghegan</author>
// <date>18/02/22</date>
// <summary>Interface bridging C# scripts and native C code</summary>

using System;
using System.Runtime.InteropServices;
using AOT;
using UnityEngine;

namespace Volograms
{
    public class VolPluginInterface
    {
#if UNITY_STANDALONE_OSX || UNITY_EDITOR_OSX
    private const string DLL = "volplayer";
#elif UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN
        private const string DLL = "vol_unity_lib_win";
#elif UNITY_IOS && !UNITY_EDITOR
    private const string DLL = "__Internal";
#elif UNITY_ANDROID && !UNITY_EDITOR
    private const string DLL = "volplayer";
#else
    private const string DLL = "UnityPlugin";
#endif

        public static VolEnums.LoggingLevel interfaceLoggingLevel = VolEnums.LoggingLevel.Info;
        public static VolEnums.LoggingLevel avLoggingLevel = VolEnums.LoggingLevel.Info;
        public static VolEnums.LoggingLevel geomLoggingLevel = VolEnums.LoggingLevel.Info;

        // Native per-instance context
        [DllImport(DLL, EntryPoint = "native_vol_context_create")]
        public static extern IntPtr VolContextCreate();

        [DllImport(DLL, EntryPoint = "native_vol_context_destroy")]
        public static extern void VolContextDestroy(IntPtr ctx);

        [StructLayout(LayoutKind.Sequential)]
        public struct VolGeometryData
        {
            public IntPtr blockDataPtr;
            public ulong blockDataSize;
            public ulong verticesOffset;
            public int verticesSize;
            public ulong normalOffset;
            public int normalSize;
            public ulong indicesOffset;
            public int indicesSize;
            public ulong uvOffset;
            public int uvSize;
            public ulong textureOffset;
            public int textureSize;

            public override string ToString()
            {
                return $"Bytes size: {blockDataSize}\n" +
                       $"Vertices: {verticesOffset} -> {verticesSize}\n" +
                       $"Normals: {normalOffset} -> {normalSize}\n" +
                       $"Indices: {indicesOffset} -> {indicesSize}\n" +
                       $"UVs: {uvOffset} -> {uvSize}\n" +
                       $"Texture: {textureOffset} -> {textureSize}\n";
            }
        }

#if UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN
        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
#else
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Auto)]
#endif
        public delegate void DebugDelegate(int type, string debugString);

        [DllImport(DLL, EntryPoint = "register_debug_callback")]
        private static extern void RegisterDebugCallback(IntPtr funcPtr);

        [DllImport(DLL, EntryPoint = "register_geom_log_callback")]
        private static extern void RegisterGeomLogCallback(IntPtr funcPtr);

        [DllImport(DLL, EntryPoint = "register_av_log_callback")]
        private static extern void RegisterAvLogCallback(IntPtr funcPtr);

        [DllImport(DLL, EntryPoint = "clear_logging_functions")]
        public static extern void ClearLoggingFunctions();

        //Geometry file functions
        [DllImport(DLL, EntryPoint = "native_vol_open_geom_file")]
        public static extern bool VolGeomOpenFile(string headerFile, string sequenceFile, bool streamingMode);

        [DllImport(DLL, EntryPoint = "native_vol_open_geom_file_ctx")]
        public static extern bool VolGeomOpenFileCtx(IntPtr ctx, string headerFile, string sequenceFile, bool streamingMode);

        [DllImport(DLL, EntryPoint = "native_vol_free_geom_data")]
        public static extern bool VolFreeGeomData();

        [DllImport(DLL, EntryPoint = "native_vol_free_geom_data_ctx")]
        public static extern bool VolFreeGeomDataCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_get_geom_frame_count")]
        public static extern int VolGeomGetFrameCount();

        [DllImport(DLL, EntryPoint = "native_vol_get_geom_frame_count_ctx")]
        public static extern int VolGeomGetFrameCountCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_read_geom_frame")]
        public static extern bool VolGeomReadFrame(string sequenceFile, int frame);

        [DllImport(DLL, EntryPoint = "native_vol_read_geom_frame_ctx")]
        public static extern bool VolGeomReadFrameCtx(string sequenceFile, int frame, IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_geom_is_keyframe")]
        public static extern bool VolGeomIsKeyframe(int frame);

        [DllImport(DLL, EntryPoint = "native_vol_geom_is_keyframe_ctx")]
        public static extern bool VolGeomIsKeyframeCtx(int frame, IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_geom_find_previous_keyframe")]
        public static extern int VolGeomFindPreviousKeyframe(int frame);

        [DllImport(DLL, EntryPoint = "native_vol_geom_find_previous_keyframe_ctx")]
        public static extern int VolGeomFindPreviousKeyframeCtx(int frame, IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_get_geom_ptr_data")]
        public static extern VolGeometryData VolGeomGetPtrData();

        [DllImport(DLL, EntryPoint = "native_vol_get_geom_ptr_data_ctx")]
        public static extern VolGeometryData VolGeomGetPtrDataCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_update_frames_directory")]
        public static extern bool VolGeomUpdateFramesDirectory(string seq_filename, int frame);

        [DllImport(DLL, EntryPoint = "native_vol_update_frames_directory_ctx")]
        public static extern bool VolGeomUpdateFramesDirectoryCtx(IntPtr ctx, string seq_filename, int frame);


        //Basis Texture from Vols File
        [DllImport(DLL, EntryPoint = "native_vol_basis_init")]
        public static extern bool VolInitBasisDecoder();

        [DllImport(DLL, EntryPoint = "native_vol_basis_init_ctx")]
        public static extern bool VolInitBasisDecoderCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_read_next_texture_frame")]
        public static extern IntPtr VolReadNextTextureFrame(int format);

        [DllImport(DLL, EntryPoint = "native_vol_read_next_texture_frame_ctx")]
        public static extern IntPtr VolReadNextTextureFrameCtx(IntPtr ctx, int format);

        [DllImport(DLL, EntryPoint = "native_vol_get_texture_frame_size")]
        public static extern long VolGetTextureSize();

        [DllImport(DLL, EntryPoint = "native_vol_get_texture_frame_size_ctx")]
        public static extern long VolGetTextureSizeCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_get_texture_width")]
        public static extern int VolGetTextureWidth();

        [DllImport(DLL, EntryPoint = "native_vol_get_texture_width_ctx")]
        public static extern int VolGetTextureWidthCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_get_texture_height")]
        public static extern int VolGetTextureHeight();

        [DllImport(DLL, EntryPoint = "native_vol_get_texture_height_ctx")]
        public static extern int VolGetTextureHeightCtx(IntPtr ctx);

        // Audio from vols file
        [DllImport(DLL, EntryPoint = "native_vol_has_audio")]
        public static extern bool VolHasAudio();

        [DllImport(DLL, EntryPoint = "native_vol_has_audio_ctx")]
        public static extern bool VolHasAudioCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_get_audio")]
        public static extern IntPtr VolGetAudio(out int outSize);

        [DllImport(DLL, EntryPoint = "native_vol_get_audio_ctx")]
        public static extern IntPtr VolGetAudioCtx(IntPtr ctx, out int outSize);


        // Video file functions
        [DllImport(DLL, EntryPoint = "native_vol_open_video_file")]
        public static extern bool VolOpenFile(string filename);

        [DllImport(DLL, EntryPoint = "native_vol_open_video_file_ctx")]
        public static extern bool VolOpenFileCtx(IntPtr ctx, string filename);

        [DllImport(DLL, EntryPoint = "native_vol_close_video_file")]
        public static extern bool VolCloseFile();

        [DllImport(DLL, EntryPoint = "native_vol_close_video_file_ctx")]
        public static extern bool VolCloseFileCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_get_video_width")]
        public static extern int VolGetVideoWidth();

        [DllImport(DLL, EntryPoint = "native_vol_get_video_width_ctx")]
        public static extern int VolGetVideoWidthCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_get_video_height")]
        public static extern int VolGetVideoHeight();

        [DllImport(DLL, EntryPoint = "native_vol_get_video_height_ctx")]
        public static extern int VolGetVideoHeightCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_get_video_frame_rate")]
        public static extern double VolGetFrameRate();

        [DllImport(DLL, EntryPoint = "native_vol_get_video_frame_rate_ctx")]
        public static extern double VolGetFrameRateCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_get_video_frame_count")]
        public static extern long VolGetNumFrames();

        [DllImport(DLL, EntryPoint = "native_vol_get_video_frame_count_ctx")]
        public static extern long VolGetNumFramesCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_get_video_duration")]
        public static extern double VolGetDuration();

        [DllImport(DLL, EntryPoint = "native_vol_get_video_duration_ctx")]
        public static extern double VolGetDurationCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_get_video_frame_size")]
        public static extern long VolGetFrameSize();

        [DllImport(DLL, EntryPoint = "native_vol_get_video_frame_size_ctx")]
        public static extern long VolGetFrameSizeCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_read_next_video_frame")]
        public static extern IntPtr VolReadNextVideoFrame(bool flipVertical);

        [DllImport(DLL, EntryPoint = "native_vol_read_next_video_frame_ctx")]
        public static extern IntPtr VolReadNextVideoFrameCtx(IntPtr ctx, bool flipVertical);

        //[DllImport(DLL, EntryPoint = "get_texture_update_callback")]
        //private static extern System.IntPtr GetTextureUpdateCallback();


        // Circular Buffer Streaming functions
        // Streaming buffer configuration
        [DllImport(DLL, EntryPoint = "native_vol_init_streaming_config")]
        public static extern bool VolInitStreamingConfig();

        [DllImport(DLL, EntryPoint = "native_vol_init_streaming_config_ctx")]
        public static extern bool VolInitStreamingConfigCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_should_use_streaming_mode")]
        public static extern bool VolShouldUseStreamingMode(long fileSize);

        [DllImport(DLL, EntryPoint = "native_vol_should_use_streaming_mode_ctx")]
        public static extern bool VolShouldUseStreamingModeCtx(IntPtr ctx, long fileSize);

        [DllImport(DLL, EntryPoint = "native_vol_create_streaming_buffer")]
        public static extern bool VolCreateStreamingBuffer();

        [DllImport(DLL, EntryPoint = "native_vol_create_streaming_buffer_ctx")]
        public static extern bool VolCreateStreamingBufferCtx(IntPtr ctx);

        // Add data to buffer
        [DllImport(DLL, EntryPoint = "native_vol_add_data_to_buffer")]
        public static extern bool VolAddDataToBuffer(IntPtr dataPtr, long dataSize);

        [DllImport(DLL, EntryPoint = "native_vol_add_data_to_buffer_ctx")]
        public static extern bool VolAddDataToBufferCtx(IntPtr ctx, IntPtr dataPtr, long dataSize);

        // Frame directory management
        [DllImport(DLL, EntryPoint = "native_vol_update_buffer_frame_directory")]
        public static extern bool VolUpdateBufferFrameDirectory();

        [DllImport(DLL, EntryPoint = "native_vol_update_buffer_frame_directory_ctx")]
        public static extern bool VolUpdateBufferFrameDirectoryCtx(IntPtr ctx);

        // Frame reading
        [DllImport(DLL, EntryPoint = "native_vol_read_frame_streaming")]
        public static extern bool VolReadFrameStreaming(int frameIdx);

        [DllImport(DLL, EntryPoint = "native_vol_read_frame_streaming_ctx")]
        public static extern bool VolReadFrameStreamingCtx(IntPtr ctx, int frameIdx);

        [DllImport(DLL, EntryPoint = "native_vol_is_frame_available_in_buffer")]
        public static extern bool VolIsFrameAvailableInBuffer(int frameIdx);

        [DllImport(DLL, EntryPoint = "native_vol_is_frame_available_in_buffer_ctx")]
        public static extern bool VolIsFrameAvailableInBufferCtx(IntPtr ctx, int frameIdx);

        // Buffer management
        [DllImport(DLL, EntryPoint = "native_vol_update_buffer_state")]
        public static extern bool VolUpdateBufferState();

        [DllImport(DLL, EntryPoint = "native_vol_update_buffer_state_ctx")]
        public static extern bool VolUpdateBufferStateCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_is_download_buffer_full")]
        public static extern bool VolIsDownloadBufferFull();

        [DllImport(DLL, EntryPoint = "native_vol_is_download_buffer_full_ctx")]
        public static extern bool VolIsDownloadBufferFullCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_should_resume_download")]
        public static extern bool VolShouldResumeDownload(int currentFrame, float fps);

        [DllImport(DLL, EntryPoint = "native_vol_should_resume_download_ctx")]
        public static extern bool VolShouldResumeDownloadCtx(IntPtr ctx, int currentFrame, float fps);

        // Buffer health monitoring
        [DllImport(DLL, EntryPoint = "native_vol_get_buffer_health_seconds")]
        public static extern float VolGetBufferHealthSeconds(float fps);

        [DllImport(DLL, EntryPoint = "native_vol_get_buffer_health_seconds_ctx")]
        public static extern float VolGetBufferHealthSecondsCtx(IntPtr ctx, float fps);

        // Streaming file info creation (for buffer mode)
        [DllImport(DLL, EntryPoint = "native_vol_create_streaming_file_info")]
        public static extern bool VolCreateStreamingFileInfo();

        [DllImport(DLL, EntryPoint = "native_vol_create_streaming_file_info_ctx")]
        public static extern bool VolCreateStreamingFileInfoCtx(IntPtr ctx);

        // Configuration setters
        [DllImport(DLL, EntryPoint = "native_vol_set_max_buffer_size")]
        public static extern void VolSetMaxBufferSize(long bytes);

        [DllImport(DLL, EntryPoint = "native_vol_set_max_buffer_size_ctx")]
        public static extern void VolSetMaxBufferSizeCtx(IntPtr ctx, long bytes);

        [DllImport(DLL, EntryPoint = "native_vol_set_lookahead_seconds")]
        public static extern void VolSetLookaheadSeconds(float seconds);

        [DllImport(DLL, EntryPoint = "native_vol_set_lookahead_seconds_ctx")]
        public static extern void VolSetLookaheadSecondsCtx(IntPtr ctx, float seconds);

        [DllImport(DLL, EntryPoint = "native_vol_get_header_frame_body_start")]
        public static extern int VolGetFrameBodyStart();

        [DllImport(DLL, EntryPoint = "native_vol_get_header_frame_body_start_ctx")]
        public static extern int VolGetFrameBodyStartCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_reset_frame_directory")]
        public static extern void VolResetFrameDirectory();

        [DllImport(DLL, EntryPoint = "native_vol_reset_frame_directory_ctx")]
        public static extern void VolResetFrameDirectoryCtx(IntPtr ctx);

        [DllImport(DLL, EntryPoint = "native_vol_get_playback_buffer_size")]
        public static extern int VolGetUsedBufferSize();

        [DllImport(DLL, EntryPoint = "native_vol_get_playback_buffer_size_ctx")]
        public static extern int VolGetUsedBufferSizeCtx(IntPtr ctx);


        // Thin per-instance wrapper for convenience
        public sealed class VolNativeContext : IDisposable
        {
            public IntPtr Handle { get; private set; }

            public VolNativeContext()
            {
                Handle = VolContextCreate();
            }

            public void Dispose()
            {
                if (Handle != IntPtr.Zero)
                {
                    VolContextDestroy(Handle);
                    Handle = IntPtr.Zero;
                }
            }

            // Geometry
            public bool OpenGeom(string headerFile, string sequenceFile, bool streamingMode) => VolGeomOpenFileCtx(Handle, headerFile, sequenceFile, streamingMode);
            public bool FreeGeom() => VolFreeGeomDataCtx(Handle);
            public int GetGeomFrameCount() => VolGeomGetFrameCountCtx(Handle);
            public bool ReadGeomFrame(string sequenceFile, int frame) => VolGeomReadFrameCtx(sequenceFile, frame, Handle);
            public bool IsKeyframe(int frame) => VolGeomIsKeyframeCtx(frame, Handle);
            public int FindPrevKeyframe(int frame) => VolGeomFindPreviousKeyframeCtx(frame, Handle);
            public VolGeometryData GetGeomPtrData() => VolGeomGetPtrDataCtx(Handle);
            public bool UpdateFramesDirectory(string seqFile, int frame) => VolGeomUpdateFramesDirectoryCtx(Handle, seqFile, frame);

            // Basis
            public bool InitBasis() => VolInitBasisDecoderCtx(Handle);
            public IntPtr ReadNextTextureFrame(int format) => VolReadNextTextureFrameCtx(Handle, format);
            public long GetTextureSize() => VolGetTextureSizeCtx(Handle);
            public int GetTextureWidth() => VolGetTextureWidthCtx(Handle);
            public int GetTextureHeight() => VolGetTextureHeightCtx(Handle);

            // Audio
            public bool HasAudio() => VolHasAudioCtx(Handle);
            public IntPtr GetAudio(out int outSize) => VolGetAudioCtx(Handle, out outSize);

            // Video
            public bool OpenVideo(string filename) => VolOpenFileCtx(Handle, filename);
            public bool CloseVideo() => VolCloseFileCtx(Handle);
            public int GetVideoWidth() => VolGetVideoWidthCtx(Handle);
            public int GetVideoHeight() => VolGetVideoHeightCtx(Handle);
            public double GetFrameRate() => VolGetFrameRateCtx(Handle);
            public long GetNumFrames() => VolGetNumFramesCtx(Handle);
            public double GetDuration() => VolGetDurationCtx(Handle);
            public long GetFrameSize() => VolGetFrameSizeCtx(Handle);
            public IntPtr ReadNextVideoFrame(bool flipVertical) => VolReadNextVideoFrameCtx(Handle, flipVertical);

            // Streaming
            public bool InitStreamingConfig() => VolInitStreamingConfigCtx(Handle);
            public bool ShouldUseStreamingMode(long fileSize) => VolShouldUseStreamingModeCtx(Handle, fileSize);
            public bool CreateStreamingBuffer() => VolCreateStreamingBufferCtx(Handle);
            public bool AddDataToBuffer(IntPtr dataPtr, long dataSize) => VolAddDataToBufferCtx(Handle, dataPtr, dataSize);
            public bool UpdateBufferFrameDirectory() => VolUpdateBufferFrameDirectoryCtx(Handle);
            public bool ReadFrameStreaming(int frameIdx) => VolReadFrameStreamingCtx(Handle, frameIdx);
            public bool IsFrameAvailableInBuffer(int frameIdx) => VolIsFrameAvailableInBufferCtx(Handle, frameIdx);
            public bool UpdateBufferState() => VolUpdateBufferStateCtx(Handle);
            public bool IsDownloadBufferFull() => VolIsDownloadBufferFullCtx(Handle);
            public bool ShouldResumeDownload(int currentFrame, float fps) => VolShouldResumeDownloadCtx(Handle, currentFrame, fps);
            public float GetBufferHealthSeconds(float fps) => VolGetBufferHealthSecondsCtx(Handle, fps);
            public bool CreateStreamingFileInfo() => VolCreateStreamingFileInfoCtx(Handle);
            public void SetMaxBufferSize(long bytes) => VolSetMaxBufferSizeCtx(Handle, bytes);
            public void SetLookaheadSeconds(float seconds) => VolSetLookaheadSecondsCtx(Handle, seconds);
            public int GetFrameBodyStart() => VolGetFrameBodyStartCtx(Handle);
            public void ResetFrameDirectory() => VolResetFrameDirectoryCtx(Handle);
            public int GetUsedBufferSize() => VolGetUsedBufferSizeCtx(Handle);
        }



        //private static CommandBuffer _commandBuffer;

        [MonoPInvokeCallback(typeof(DebugDelegate))]
        private static void DebugCallbackFunction(int logType, string debugString)
        {
            switch (logType)
            {
                case 0:
                    if (interfaceLoggingLevel.HasFlag(VolEnums.LoggingLevel.Info))
                        Debug.Log($"VOL_LIB {debugString}");
                    break;
                case 1:
                    if (interfaceLoggingLevel.HasFlag(VolEnums.LoggingLevel.Debug))
                        Debug.Log($"VOL_LIB {debugString}");
                    break;
                case 2:
                    if (interfaceLoggingLevel.HasFlag(VolEnums.LoggingLevel.Warning))
                        Debug.LogWarning($"VOL_LIB {debugString}");
                    break;
                case 3:
                    if (interfaceLoggingLevel.HasFlag(VolEnums.LoggingLevel.Error))
                        Debug.LogError($"VOL_LIB {debugString}");
                    break;
                default:
                    Debug.Log(debugString);
                    break;
            }
        }

        [MonoPInvokeCallback(typeof(DebugDelegate))]
        private static void DebugGeomCallbackFunction(int logType, string debugString)
        {
            switch (logType)
            {
                case 0:
                    if (geomLoggingLevel.HasFlag(VolEnums.LoggingLevel.Info))
                        Debug.Log($"VOL_GEOM {debugString}");
                    break;
                case 1:
                    if (geomLoggingLevel.HasFlag(VolEnums.LoggingLevel.Debug))
                        Debug.Log($"VOL_GEOM {debugString}");
                    break;
                case 2:
                    if (geomLoggingLevel.HasFlag(VolEnums.LoggingLevel.Warning))
                        Debug.LogWarning($"VOL_GEOM {debugString}");
                    break;
                case 3:
                    if (geomLoggingLevel.HasFlag(VolEnums.LoggingLevel.Error))
                        Debug.LogError($"VOL_GEOM {debugString}");
                    break;
                default:
                    Debug.Log(debugString);
                    break;
            }
        }

        [MonoPInvokeCallback(typeof(DebugDelegate))]
        private static void DebugAvCallbackFunction(int logType, string debugString)
        {
            switch (logType)
            {
                case 0:
                    if (avLoggingLevel.HasFlag(VolEnums.LoggingLevel.Info))
                        Debug.Log($"VOL_AV {debugString}");
                    break;
                case 1:
                    if (avLoggingLevel.HasFlag(VolEnums.LoggingLevel.Debug))
                        Debug.Log($"VOL_AV {debugString}");
                    break;
                case 2:
                    if (avLoggingLevel.HasFlag(VolEnums.LoggingLevel.Warning))
                        Debug.LogWarning($"VOL_AV {debugString}");
                    break;
                case 3:
                    if (avLoggingLevel.HasFlag(VolEnums.LoggingLevel.Error))
                        Debug.LogError($"VOL_AV {debugString}");
                    break;
                default:
                    Debug.Log(debugString);
                    break;
            }
        }


        public static void EnableInterfaceLogging()
        {
            DebugDelegate debugDelegate = DebugCallbackFunction;
            IntPtr delegatePtr = Marshal.GetFunctionPointerForDelegate(debugDelegate);
            RegisterDebugCallback(delegatePtr);
        }

        public static void EnableGeomLogging()
        {
            DebugDelegate debugGeomDelegate = DebugGeomCallbackFunction;
            IntPtr delegateGeomPtr = Marshal.GetFunctionPointerForDelegate(debugGeomDelegate);
            RegisterGeomLogCallback(delegateGeomPtr);
        }

        public static void EnableAvLogging()
        {
            DebugDelegate debugAvDelegate = DebugAvCallbackFunction;
            IntPtr delegateAvPtr = Marshal.GetFunctionPointerForDelegate(debugAvDelegate);
            RegisterAvLogCallback(delegateAvPtr);
        }

        /* -- TODO: REIMPLEMENT 
        public static void InitCommandBuffer()
        {
            _commandBuffer = new CommandBuffer();
        }

        public static void UpdateTexture(Texture texture)
        {
            _commandBuffer.IssuePluginCustomTextureUpdateV2(GetTextureUpdateCallback(), texture, (uint)Time.time * 60);
            Graphics.ExecuteCommandBuffer(_commandBuffer);
            _commandBuffer.Clear();
        }*/
    }
}