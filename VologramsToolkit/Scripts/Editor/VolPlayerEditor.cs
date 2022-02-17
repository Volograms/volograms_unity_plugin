using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

[CustomEditor(typeof(VolPlayer))]
public class VolPlayerEditor : Editor
{
    private VolPlayer _target;
    private bool _showPathHelp;
    private bool _debugFoldout;

    private const string OpenVolFolderFileCacheId = "VolPlayer_Editor_VolFolderFileOpenCache";
    private const string OpenVideoFileCacheId = "VolPlayer_Editor_VideoFileOpenCache";
    
    private readonly string[] _openVideoFileFilters = {"Video Files", "mp4,MP4"};

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

    public override void OnInspectorGUI()
    {
        //base.OnInspectorGUI();
        
        EditorGUI.BeginChangeCheck();
        
        GUILayout.Label("Paths", EditorStyles.boldLabel);
        _showPathHelp = EditorGUILayout.Foldout(_showPathHelp, "Path Help", EditorStyles.foldout);
        if (_showPathHelp)
        {
            DrawPathHelpBox();
        }
        
        EditorGUILayout.Separator();
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
                string selectedPath = openedFolder.Remove(0, selectedPathType.ToPath().Length + 1);
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
                string selectedPath = openedFile.Remove(0, selectedPathType.ToPath().Length + 1);
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

        EditorGUILayout.Separator();
        GUILayout.Label("Playback Settings", EditorStyles.boldLabel);

        _target.playOnStart = EditorGUILayout.Toggle("Play On Start", _target.playOnStart);
        _target.isLooping = EditorGUILayout.Toggle("Is Looping", _target.isLooping);
        _target.audioOn = EditorGUILayout.Toggle("Audio On", _target.audioOn);
        
        EditorGUILayout.Separator();
        GUILayout.Label("Rendering Settings", EditorStyles.boldLabel);
        _target.material = EditorGUILayout.ObjectField("Material", _target.material, typeof(Material)) as Material;
        _target.textureShaderId = EditorGUILayout.TextField("Texture Shader ID", _target.textureShaderId);
        
        EditorGUILayout.Separator();
        _debugFoldout = EditorGUILayout.Foldout(_debugFoldout, "Debug Logging Options", EditorStyles.foldoutHeader);
        if (_debugFoldout)
        {
            DrawDebugLoggingHelpBox();
            _target.enableInterfaceLogging = EditorGUILayout.Toggle("Interface", _target.enableInterfaceLogging);
            _target.enableAvLogging = EditorGUILayout.Toggle("AV", _target.enableAvLogging);
            _target.enableGeomLogging = EditorGUILayout.Toggle("Geom", _target.enableGeomLogging);
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
