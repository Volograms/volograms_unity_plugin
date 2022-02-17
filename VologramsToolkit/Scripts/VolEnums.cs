using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

public static class VolEnums
{
    public enum PathType
    {
        Absolute,
        Persistent,
        Streaming,
        Data
    }

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
}
