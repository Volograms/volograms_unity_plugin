// <copyright file=VolPlayerEditor company=Volograms>
// Copyright (c) 2022 All Rights Reserved
// </copyright>
// <author>Patrick Geoghegan</author>
// <date>18/02/22</date>
// <summary>Custom inspector for the VolPlayer component</summary>

using System.IO;
using UnityEditor;
using UnityEngine;

[CustomEditor(typeof(VolPlayer))]
public class VolPlayerEditor : Editor
{
    private VolPlayer _target;
    private bool _showPathHelp;
    private bool _debugFoldout;

    private const string OpenVolFolderFileCacheId = "VolPlayer_Editor_VolFolderFileOpenCache";
    private const string OpenVideoFileCacheId = "VolPlayer_Editor_VideoFileOpenCache";
    private const string OpenVolsFileCacheId = "VolPlayer_Editor_VolsFileOpenCache";
    
    private readonly string[] _openVideoFileFilters = {"Video Files", "mp4,MP4"};
    private readonly string[] _openVolsFileFilters = {"Vols Files", "vols"};

    private readonly GUIContent _pathHelpBoxGUIContent = new GUIContent(
        @"The 'Path Type' enum corresponds to Unity's path shortcuts:
Absolute: string.Empty
Streaming: Application.streamingAssetsPath
Persistent: Application.persistentDataPath
Data: Application.dataPath

The string entered beneath the dropdown is appended to the base path to create the full path"
    );

    private readonly GUIContent _debugLoggingHelpBoxGUIContent = new GUIContent(
        @"** FOR DEBUGGING, RECOMMEND DISABLING FOR RELEASE BUILDS **

Enables logging from the different components of the native plugin to Unity

Interface: Enables logging of native plugin-bridging code
AV: Enables logging of video-related native code
Geom: Enables logging of geometry-related native code"
    );

    private void Awake()
    {
        _target = (VolPlayer) target;
    }

    private void DrawPathHelpBox()
    {
        EditorGUILayout.HelpBox(_pathHelpBoxGUIContent);
    }
    
    private void DrawDebugLoggingHelpBox()
    {
        EditorGUILayout.HelpBox(_debugLoggingHelpBoxGUIContent);
    }

    private void DrawVideoPathSection() 
    {
        GUILayout.Label("Vol Folder", EditorStyles.label, GUILayout.ExpandWidth(true));

        _target.volFolderPathType = (VolEnums.PathType) EditorGUILayout.EnumPopup("Path Type:", _target.volFolderPathType, EditorStyles.popup);
        GUILayout.Label("Path:");
        _target.volFolder = EditorGUILayout.TextField(_target.volFolder, EditorStyles.textField);
        
        if (GUILayout.Button("Open New Vol Folder"))
        {
            string cached = PlayerPrefs.GetString(OpenVolFolderFileCacheId, string.Empty);
            string openedFolder = EditorUtility.OpenFolderPanel("Open Vol Folder", cached, string.Empty);
            if (!string.IsNullOrEmpty(openedFolder))
            {
                VolEnums.PathType selectedPathType = VolEnums.DeterminePathType(openedFolder);
                string selectedPath = selectedPathType == VolEnums.PathType.Absolute 
                    ? openedFolder 
                    : openedFolder.Remove(0, selectedPathType.ToPath().Length + 1);
                _target.volFolderPathType = selectedPathType;
                _target.volFolder = selectedPath;
                PlayerPrefs.SetString(OpenVolFolderFileCacheId, openedFolder);
                EditorUtility.SetDirty(target);
            }
        }
        
        if (GUILayout.Button("Reveal Vols Path in Finder"))
        {
            EditorUtility.RevealInFinder(_target.volFolderPathType.ResolvePath(_target.volFolder));
        }
        
        EditorGUILayout.Separator();
        GUILayout.Label("Video Texture", EditorStyles.label);
        _target.volVideoTexturePathType = (VolEnums.PathType) EditorGUILayout.EnumPopup("Path Type:", _target.volVideoTexturePathType, EditorStyles.popup);
        GUILayout.Label("Path:");
        _target.volVideoTexture = EditorGUILayout.TextField(_target.volVideoTexture, EditorStyles.textField);

        if (GUILayout.Button("Open New Video File"))
        {
            string cached = PlayerPrefs.GetString(OpenVideoFileCacheId, string.Empty);
            string openedFile = EditorUtility.OpenFilePanelWithFilters("Open Video Texture", cached, _openVideoFileFilters);
            if (!string.IsNullOrEmpty(openedFile))
            {
                VolEnums.PathType selectedPathType = VolEnums.DeterminePathType(openedFile);
                string selectedPath = selectedPathType == VolEnums.PathType.Absolute 
                    ? openedFile 
                    : openedFile.Remove(0, selectedPathType.ToPath().Length + 1);
                _target.volVideoTexturePathType = selectedPathType;
                _target.volVideoTexture = selectedPath;
                PlayerPrefs.SetString(OpenVideoFileCacheId, Path.GetDirectoryName(openedFile) ?? string.Empty);
                EditorUtility.SetDirty(target);
            }
        }
        
        if (GUILayout.Button("Reveal Video File in Finder"))
        {
            EditorUtility.RevealInFinder(_target.volVideoTexturePathType.ResolvePath(_target.volVideoTexture));
        }
    }

    private void DrawBasisUPathSection() 
    {       
        EditorGUILayout.Separator();
        GUILayout.Label("Vols File", EditorStyles.label);
        _target.volFilePathType = (VolEnums.PathType) EditorGUILayout.EnumPopup("Path Type:", _target.volFilePathType, EditorStyles.popup);
        GUILayout.Label("Path:");
        _target.volFile = EditorGUILayout.TextField(_target.volFile, EditorStyles.textField);

        if (GUILayout.Button("Open New Vols File"))
        {
            string cached = PlayerPrefs.GetString(OpenVolsFileCacheId, string.Empty);
            string openedFile = EditorUtility.OpenFilePanelWithFilters("Open Vols File", cached, _openVolsFileFilters);
            if (!string.IsNullOrEmpty(openedFile))
            {
                VolEnums.PathType selectedPathType = VolEnums.DeterminePathType(openedFile);
                string selectedPath = selectedPathType == VolEnums.PathType.Absolute 
                    ? openedFile 
                    : openedFile.Remove(0, selectedPathType.ToPath().Length + 1);
                _target.volFilePathType = selectedPathType;
                _target.volFile = selectedPath;
                PlayerPrefs.SetString(OpenVolsFileCacheId, Path.GetDirectoryName(openedFile) ?? string.Empty);
                EditorUtility.SetDirty(target);
            }
        }
        
        if (GUILayout.Button("Reveal Vols File in Finder"))
        {
            EditorUtility.RevealInFinder(_target.volFilePathType.ResolvePath(_target.volFile));
        }
    }

    public override void OnInspectorGUI()
    {
        EditorGUI.BeginChangeCheck();
        
        GUILayout.Label("Paths", EditorStyles.boldLabel);
        _showPathHelp = EditorGUILayout.Foldout(_showPathHelp, "Path Help", EditorStyles.foldout);
        if (_showPathHelp)
        {
            DrawPathHelpBox();
        }
        
        EditorGUILayout.Separator();
        _target.volFormat = (VolEnums.VolFormat)  EditorGUILayout.EnumPopup("Format:", _target.volFormat, EditorStyles.popup);
        if(_target.volFormat == VolEnums.VolFormat.Video) {
            DrawVideoPathSection();
        } else {
            DrawBasisUPathSection();
        }

        EditorGUILayout.Separator();
        GUILayout.Label("Playback Settings", EditorStyles.boldLabel);

        _target.playOnStart = EditorGUILayout.Toggle("Play On Start", _target.playOnStart);
        _target.isLooping = EditorGUILayout.Toggle("Is Looping", _target.isLooping);
        _target.audioOn = EditorGUILayout.Toggle("Audio On", _target.audioOn);
        
        EditorGUILayout.Separator();
        GUILayout.Label("Rendering Settings", EditorStyles.boldLabel);
        _target.material = EditorGUILayout.ObjectField("Material", _target.material, typeof(Material), false) as Material;
        _target.textureShaderId = EditorGUILayout.TextField("Texture Shader ID", _target.textureShaderId);
        
        EditorGUILayout.Separator();
        _debugFoldout = EditorGUILayout.Foldout(_debugFoldout, "Debug Logging Options", EditorStyles.foldoutHeader);
        if (_debugFoldout)
        {
            DrawDebugLoggingHelpBox();
            _target.interfaceLoggingLevel = (VolEnums.LoggingLevel) EditorGUILayout.EnumFlagsField("Interface", _target.interfaceLoggingLevel);
            _target.avLoggingLevel =(VolEnums.LoggingLevel) EditorGUILayout.EnumFlagsField("AV", _target.avLoggingLevel);
            _target.geomLoggingLevel = (VolEnums.LoggingLevel) EditorGUILayout.EnumFlagsField("Geom", _target.geomLoggingLevel);
        }

        if (EditorGUI.EndChangeCheck())
        {
            EditorUtility.SetDirty(target);
        }

        EditorGUILayout.Separator();

        if (EditorApplication.isPlaying)
        {
            EditorGUILayout.Separator();
            
            if (_target.IsOpen)
            {
                if (GUILayout.Button("Close"))
                {
                    _target.Close();
                }
            }
            else
            {
                if (GUILayout.Button("Open"))
                {
                    _target.Open();
                }
            }
            
            if (_target.IsPlaying)
            {
                if (GUILayout.Button("Pause"))
                {
                    _target.Pause();
                }
            }
            else
            {
                if (GUILayout.Button("Play"))
                {
                    _target.Play();
                }
            }

            if (GUILayout.Button("Restart"))
            {
                _target.Restart();
            }
            
        }

        #if UNITY_ANDROID 
        if (_target.audioOn)
        {
            EditorGUILayout.HelpBox("Audio in Android is still a work in progress. " +
                                    "Audio is disabled in Android builds.", MessageType.Warning, true);
        }
        #endif
    }
    
}
