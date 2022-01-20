using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using AOT;
using Unity.Collections;
using UnityEngine;
using UnityEngine.Rendering;

public class VolPluginInterface
{
#if UNITY_STANDALONE_OSX || UNITY_EDITOR_OSX
    private const string DLL = "vol_unity_macos";
#elif UNITY_IOS && !UNITY_EDITOR
    private const string DLL = "__Internal";
#elif UNITY_ANDROID && !UNITY_EDITOR
    private const string DLL = "volplayer";
#else
    private const string DLL = "UnityPlugin";
#endif
    
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

    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public delegate void DebugDelegate(int type, IntPtr request, int size);

    [DllImport(DLL, EntryPoint = "register_debug_callback", CallingConvention = CallingConvention.StdCall)]
    private static extern void RegisterDebugCallback(DebugDelegate cb);

    //Geometry file functions
    [DllImport(DLL, EntryPoint = "native_vol_open_geom_file")]
    public static extern bool VolGeomOpenFile(string headerFile, string sequenceFile);

    [DllImport(DLL, EntryPoint = "native_vol_free_geom_data")]
    public static extern bool VolFreeGeomData();

    [DllImport(DLL, EntryPoint = "native_vol_get_geom_frame_count")]
    public static extern int VolGeomGetFrameCount();

    [DllImport(DLL, EntryPoint = "native_vol_read_next_geom_frame")]
    public static extern bool VolGeomReadNextFrame(string sequenceFile);

    [DllImport(DLL, EntryPoint = "native_vol_get_next_geom_frame_index")]
    public static extern int VolGeomGetNextFrameIndex();

    [DllImport(DLL, EntryPoint = "native_vol_get_geom_ptr_data")]
    public static extern VolGeometryData VolGeomGetPtrData();

    // Video file functions
    [DllImport(DLL, EntryPoint = "native_vol_open_video_file")]
    public static extern bool VolOpenFile(string filename);

    [DllImport(DLL, EntryPoint = "native_vol_close_video_file")]
    public static extern bool VolCloseFile();

    [DllImport(DLL, EntryPoint = "native_vol_get_video_width")]
    public static extern int VolGetVideoWidth();

    [DllImport(DLL, EntryPoint = "native_vol_get_video_height")]
    public static extern int VolGetVideoHeight();

    [DllImport(DLL, EntryPoint = "native_vol_get_video_frame_rate")]
    public static extern double VolGetFrameRate();

    [DllImport(DLL, EntryPoint = "native_vol_get_video_frame_count")]
    public static extern long VolGetNumFrames();

    [DllImport(DLL, EntryPoint = "native_vol_get_video_duration")]
    public static extern double VolGetDuration();

    [DllImport(DLL, EntryPoint = "native_vol_get_video_frame_size")]
    public static extern long VolGetFrameSize();

    [DllImport(DLL, EntryPoint = "native_vol_read_next_frame")]
    public static extern IntPtr VolReadNextFrame();

    //[DllImport(DLL, EntryPoint = "get_texture_update_callback")]
    //private static extern System.IntPtr GetTextureUpdateCallback();

    private static CommandBuffer _commandBuffer;

    [MonoPInvokeCallback(typeof(DebugDelegate))]
    private static void DebugCallbackFunction(int logType, IntPtr request, int size)
    {
        string debugString = Marshal.PtrToStringAnsi(request, size);
        switch (logType)
        {
            case 0: 
                Debug.Log($"INFO:: {debugString}");
                break;
            case 1:
                Debug.Log($"DEBUG:: {debugString}");
                break;
            case 2:
                Debug.LogWarning($"WARN:: {debugString}");
                break;
            case 3:
                Debug.LogError($"ERROR:: {debugString}");
                break;
            default:
                Debug.Log(debugString);
                break;
        }
    }
    
    public static void SetUpDebugging()
    {
        Debug.Log(DLL);
        RegisterDebugCallback(DebugCallbackFunction);
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
