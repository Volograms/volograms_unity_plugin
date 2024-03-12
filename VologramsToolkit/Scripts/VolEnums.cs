// <copyright file=VolEnums company=Volograms>
// Copyright (c) 2022 All Rights Reserved
// </copyright>
// <author>Patrick Geoghegan</author>
// <date>18/02/22</date>
// <summary>Enums used in the VologramsToolkit</summary>

using System;
using System.IO;
using UnityEngine;

/// <summary>
/// Class containing all the enums used in the Volograms Unity SDK
/// </summary>
public static class VolEnums
{
    /// <summary>
    /// Enums referring to two Vols format versions
    /// </summary>
    [Serializable]
    public enum VolFormat
    {
        Video,  // Texture encoded as a video
        BasisU  //  All data (mesh + texture) in a single file 
    }

    /// <summary>
    /// Enums referring to the different paths made available through Unity
    /// Enables cross-platform paths
    /// </summary>
    [Serializable]
    public enum PathType
    {
        Absolute,   // = string.Empty
        Persistent, // = Application.persistentDataPath
        Streaming,  // = Application.streamingAssetsPath
        Data        // = Application.dataPath
    }
    

    /// <summary>
    /// Creates a full path from the enum path base and relative path
    /// </summary>
    /// <param name="pathType">The path base</param>
    /// <param name="path">The relative path</param>
    /// <returns>Full absolute path</returns>
    public static string ResolvePath(this PathType pathType, string path)
    {
        switch (pathType)
        {
            case PathType.Persistent:
                return Path.Combine(Application.persistentDataPath, path);
            case PathType.Streaming:
                return Path.Combine(Application.streamingAssetsPath, path);
            case PathType.Data:
                return Path.Combine(Application.dataPath, path);
            case PathType.Absolute:
            default:
                return path;
        }
    }

    /// <summary>
    /// Converts the PathType enum to its corresponding string path
    /// </summary>
    /// <param name="pathType">Enum to convert</param>
    /// <returns>String path</returns>
    public static string ToPath(this PathType pathType)
    {
        switch (pathType)
        {
            case PathType.Persistent:
                return Application.persistentDataPath;
            case PathType.Streaming:
                return Application.streamingAssetsPath;
            case PathType.Data:
                return Application.dataPath;
            case PathType.Absolute:
            default:
                return string.Empty;
        }
    }

    /// <summary>
    /// Returns the PathType enum of the given input path
    /// </summary>
    /// <param name="fullPath">Input full absolute path</param>
    /// <returns>The PathType enum</returns>
    public static PathType DeterminePathType(string fullPath)
    {
        if (fullPath.StartsWith(PathType.Persistent.ToPath()))
        {
            return PathType.Persistent;
        }
        
        if (fullPath.StartsWith(PathType.Streaming.ToPath()))
        {
            return PathType.Streaming;
        }

        if (fullPath.StartsWith(PathType.Data.ToPath()))
        {
            return PathType.Data;
        }

        return PathType.Absolute;
    }

    /// <summary>
    /// Refers to the type of log messages that the native code sends to Unity
    /// Is aligned with the `vol_geom_log_type_t` and `vol_av_log_type_t` enums
    /// </summary>
    [Flags, Serializable]
    public enum LoggingLevel
    {
        None = 0,
        Info = 1, 
        Debug = 1 << 1, 
        Warning = 1 << 2, 
        Error = 1 << 3,
        All = ~0
    }
}
