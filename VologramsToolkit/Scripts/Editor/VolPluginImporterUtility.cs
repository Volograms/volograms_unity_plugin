// <copyright file=VolPluginImporterUtility company=Volograms>
// Copyright (c) 2022 All Rights Reserved
// </copyright>
// <author>Patrick Geoghegan</author>
// <date>18/02/22</date>
// <summary>Utility scripts for importing various plugin libs with the correct settings</summary>

using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEngine;

public static class VolPluginImporterUtility
{
    private const string RelativePathToPlugins = "Assets/VologramsToolkit/Plugins";
    
    private const string IOSPluginPath = "iOS";
    private const string MacOSPluginPath = "MacOS";
    private const string WindowsPluginPath = "Windows";

    private static readonly string[] AndroidPluginPaths =
    {
        "Android/arm64-v8a", "Android/armeabi-v7a", "Android/x86", "Android/x86_64"
    };

    public const string IOSLib = "libvolplayer.a";
    public const string MacOSLib = "volplayer.bundle";
    private static readonly string[] AndroidLibs =
    {
        "libavcodec.so", "libavdevice.so", "libavfilter.so", 
        "libavformat.so", "libavutil.so", "libswresample.so", 
        "libswscale.so", "libvolplayer.so"
    };
    
    private static readonly Dictionary<string, string> AndroidCPUEditorValues = new Dictionary<string, string>()
    {
        {"Android/arm64-v8a","ARM64"},
        {"Android/armeabi-v7a", "ARMv7"},
        {"Android/x86", "X86"},
        {"Android/x86_64", "X86_64"}
    };

    [MenuItem("Volograms/Utils/Reimport Plugins/All")]
    private static void SetAllPluginSettings()
    {
        SetAndroidPluginSettings();
        SetIOSPluginSettings();
        SetMacOSPluginSettings();
    }
    
    [MenuItem("Volograms/Utils/Reimport Plugins/Android")]
    private static void SetAndroidPluginSettings()
    {
        foreach (string androidPluginPath in AndroidPluginPaths)
        {
            string pluginFolderPath = Path.Combine(RelativePathToPlugins, androidPluginPath);
            foreach (string androidLib in AndroidLibs)
            {
                string pluginPath = Path.Combine(pluginFolderPath, androidLib);
                PluginImporter pluginImporter = (PluginImporter) AssetImporter.GetAtPath(pluginPath);
                if (pluginImporter == null)
                {
                    Debug.LogWarning($"Could not find plugin at {pluginPath}");
                    return;
                }
                pluginImporter.ClearSettings();
                pluginImporter.SetCompatibleWithAnyPlatform(false);
                pluginImporter.SetCompatibleWithPlatform(BuildTarget.Android, true);
                pluginImporter.SetPlatformData(BuildTarget.Android, "CPU", AndroidCPUEditorValues[androidPluginPath]);
                pluginImporter.SaveAndReimport();

            }
            
        }
    }
    
    [MenuItem("Volograms/Utils/Reimport Plugins/iOS")]
    private static void SetIOSPluginSettings()
    {
        string pluginPath = Path.Combine(RelativePathToPlugins, IOSPluginPath, IOSLib);
        PluginImporter pluginImporter = (PluginImporter) AssetImporter.GetAtPath(pluginPath);
        if (pluginImporter == null)
        {
            Debug.LogWarning($"Could not find plugin at {pluginPath}");
            return;
        }
        pluginImporter.ClearSettings();
        pluginImporter.SetCompatibleWithAnyPlatform(false);
        pluginImporter.SetCompatibleWithPlatform(BuildTarget.iOS, true);
        pluginImporter.SetPlatformData(BuildTarget.iOS, "CPU", "AnyCPU");
        pluginImporter.SaveAndReimport();
    }
    
    [MenuItem("Volograms/Utils/Reimport Plugins/MacOS")]
    private static void SetMacOSPluginSettings()
    {
        string pluginPath = Path.Combine(RelativePathToPlugins, MacOSPluginPath, MacOSLib);
        PluginImporter pluginImporter = (PluginImporter) AssetImporter.GetAtPath(pluginPath);
        if (pluginImporter == null)
        {
            Debug.LogWarning($"Could not find plugin at {pluginPath}");
            return;
        }
        pluginImporter.ClearSettings();
        pluginImporter.SetCompatibleWithAnyPlatform(false);
        pluginImporter.SetCompatibleWithPlatform(BuildTarget.StandaloneOSX, true);
        pluginImporter.SetPlatformData(BuildTarget.StandaloneOSX, "CPU", "AnyCPU");
        pluginImporter.SetCompatibleWithEditor(true);
        pluginImporter.SetEditorData("OS", "OSX");
        pluginImporter.SaveAndReimport();
    }
    
}
