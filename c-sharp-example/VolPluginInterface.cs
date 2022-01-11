using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using AOT;
using Unity.Collections;
using UnityEngine;
public class VolPluginInterface
{
#if UNITY_IOS || UNITY_STANDALONE_OSX || UNITY_EDITOR_OSX
    private const string DLL = "__Internal";
#elif UNITY_ANDROID && !UNITY_EDITOR
    private const string DLL = "vol";
#else
    private const string DLL = "UnityPlugin";
#endif
    
    enum Color { none, red, green, blue, black, white, yellow, orange };

    [StructLayout(LayoutKind.Sequential)]
    public struct VolGeometryData
    {
        public IntPtr bytePtr;
        public ulong bytesSize;
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
            return $"Bytes size: {bytesSize}\n" +
                   $"Vertices: {verticesOffset} -> {verticesSize}\n" +
                   $"Normals: {normalOffset} -> {normalSize}\n" +
                   $"Indices: {indicesOffset} -> {indicesSize}\n" +
                   $"UVs: {uvOffset} -> {uvSize}\n" +
                   $"Texture: {textureOffset} -> {textureSize}\n";
        }
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public delegate void DebugDelegate(IntPtr request, int color, int size);

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

    [MonoPInvokeCallback(typeof(DebugDelegate))]
    private static void DebugCallbackFunction(IntPtr request, int color, int size)
    {
        string debugString = Marshal.PtrToStringAnsi(request, size);
        Debug.Log((Color) color == Color.none
            ? $"Native:: {debugString}"
            : $"Native:: <color={((Color) color).ToString()}>{debugString}</color>");
    }

    public void SetUpDebugging()
    {
        RegisterDebugCallback(DebugCallbackFunction);
    }
}
