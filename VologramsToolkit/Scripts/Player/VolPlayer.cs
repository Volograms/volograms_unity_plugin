// <copyright file=VolPlayer company=Volograms>
// Copyright (c) 2022 All Rights Reserved
// </copyright>
// <author>Patrick Geoghegan</author>
// <date>18/02/22</date>
// <summary>Controls for vologram playback</summary>

using System;
using System.IO;
using System.Runtime.InteropServices;
using Unity.Collections;
using UnityEngine;
using UnityEngine.Experimental.Rendering;
using UnityEngine.Video;

[Serializable]
[RequireComponent(typeof(MeshFilter))]
[RequireComponent(typeof(MeshRenderer))]
public class VolPlayer : MonoBehaviour
{
    [Header("Paths")]
    public VolEnums.PathType volFolderPathType;
    public string volFolder;

    public VolEnums.PathType volFilePathType;
    public string volFile;

    public VolEnums.PathType volVideoTexturePathType;
    public string volVideoTexture;

    public VolEnums.VolFormat volFormat;

    [Header("Playback Settings")]
    public bool playOnStart = true;
    public bool isLooping = true;
    public bool audioOn = false;

    [Header("Rendering Settings")] 
    public Material material;
    public string textureShaderId = "_MainTex";

    [Header("Debug Logging Options")]
    public VolEnums.LoggingLevel interfaceLoggingLevel = VolEnums.LoggingLevel.None;
    public VolEnums.LoggingLevel avLoggingLevel = VolEnums.LoggingLevel.None;
    public VolEnums.LoggingLevel geomLoggingLevel = VolEnums.LoggingLevel.None;
    
    private string _fullGeomPath;
    private string _fullVideoPath;
    private int _currentlyLoadedFrameIndex; // Start at -1 so after loading first frame it gets set to 0.
    private int _numFrames;
    private bool _hasVideoTexture;
    // When an animation starts this value is 0. When the last frame is played it is == video duration. On loop it resets to zero.
    private double _animationAccumulatedSeconds;
    private double _secondsPerFrame;
    private MeshFilter _meshFilter;
    private MeshRenderer _meshRenderer;
    private ushort[] _keyShortIndices;
    private Vector2[] _keyUvs;
    private Texture2D _voloTexture;
    private IntPtr _colorPtr;
    private VolPluginInterface.VolGeometryData _geometryData;
    private byte[] _meshData;
    private int _textureId;
    private VideoPlayer _audioPlayer;

    public bool IsOpen { get; private set; }
    public bool IsPlaying { get; private set; }
    //public int Frame => _currentFrameIndex; // TODO(Anton) have i broken something here?
    public bool IsMuted => audioOn && _audioPlayer != null && _audioPlayer.GetDirectAudioMute(0);
    
    /// <summary>
    /// Unity's Start function - called on the first frame
    /// </summary>
    private void Start()
    {
#if UNITY_EDITOR
        if (_meshFilter.sharedMesh == null)
        {
            _meshFilter.sharedMesh = new Mesh();
        }
#else        
        if (_meshFilter.mesh == null)
        {
            _meshFilter.mesh = new Mesh();
        }
#endif
        
#if UNITY_ANDROID
        audioOn = false;
#endif
        
        if (audioOn)
        {
            if (!TryGetComponent<VideoPlayer>(out _audioPlayer))
            {
                _audioPlayer = gameObject.AddComponent<VideoPlayer>();
            }
        }
        
        Open();
        
        if (playOnStart)
        {
            
            Play();
        }
    }

    /// <summary>
    /// Unity's Update function - called every frame
    /// </summary>
    private void Update()
    {
        if (!IsPlaying) return;
        
        // Work out the frame index to play based on elapsed animation time. This lets us skip to the correct frame when the player is going slowly.
        _animationAccumulatedSeconds += Time.deltaTime;
        int desiredFrameIndex = (int)(_animationAccumulatedSeconds / _secondsPerFrame);
        // Not enough time has passed to advance to the next frame yet.
        if ( desiredFrameIndex == _currentlyLoadedFrameIndex ) { return; }

        if (desiredFrameIndex >= _numFrames )
        {
            if (isLooping)
            {
                Restart();
            } else {
                IsPlaying = false;
                Close();
            }
            return;
        }
        // --VIDEO TEXTURE--
        // Always skip video frames to desired frame.
        if(_hasVideoTexture)
            ReadVideoFrame(_currentlyLoadedFrameIndex, desiredFrameIndex);

        { // --GEOMETRY--
            int previousKeyframeIndex = VolPluginInterface.VolGeomFindPreviousKeyframe(desiredFrameIndex);
            bool desiredIsKeyframe = VolPluginInterface.VolGeomIsKeyframe(desiredFrameIndex);
            // If our desired frame would jump over its proceeding keyframe, we need to stop and load that first,
            // unless it is a keyframe itself.
            bool needToLoadKeyframe = (_currentlyLoadedFrameIndex < previousKeyframeIndex) && !desiredIsKeyframe;
            if ( needToLoadKeyframe ) { ReadGeomFrame( previousKeyframeIndex); }
            ReadGeomFrame( desiredFrameIndex);
        }

        // Advance frame
        _currentlyLoadedFrameIndex = desiredFrameIndex;
    }

    /// <summary>
    /// Unity's OnEnable function - called when the VolPlayer becomes enabled and active
    /// </summary>
    private void OnEnable()
    {
        _meshFilter = GetComponent<MeshFilter>();
        _meshRenderer = GetComponent<MeshRenderer>();
    }

    /// <summary>
    /// Unity's OnDisable function - called when the VolPlayer becomes disabled and inactive
    /// </summary>
    private void OnDisable()
    {
        Close();
    }

    /// <summary>
    /// Open the given vologram files
    /// </summary>
    /// <returns>True if successful</returns>
    public bool Open()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot open a vologram while another is open");
            return false;
        }

        VolPluginInterface.interfaceLoggingLevel = interfaceLoggingLevel;
        VolPluginInterface.avLoggingLevel = avLoggingLevel;
        VolPluginInterface.geomLoggingLevel = geomLoggingLevel;
        VolPluginInterface.EnableInterfaceLogging();
        VolPluginInterface.EnableAvLogging();
        VolPluginInterface.EnableGeomLogging();

        _hasVideoTexture = !string.IsNullOrEmpty(volVideoTexture);
        _fullVideoPath = volVideoTexturePathType.ResolvePath(volVideoTexture);

        if (_hasVideoTexture)
        {
            bool openedVideo = VolPluginInterface.VolOpenFile(_fullVideoPath);
            if (!openedVideo)
            {
                IsOpen = false;
                Close();
                return false;
            }
        }

        if (audioOn)
        {
            _audioPlayer.Stop();
            _audioPlayer.sendFrameReadyEvents = true;
            _audioPlayer.source = VideoSource.Url;
            _audioPlayer.url = _fullVideoPath;
            
            _audioPlayer.frameReady -= AudioVideoPlayerOnFrameReady;
            _audioPlayer.frameReady += AudioVideoPlayerOnFrameReady;
            _audioPlayer.loopPointReached -= AudioVideoPlayerOnLoopPointReached;
            _audioPlayer.loopPointReached += AudioVideoPlayerOnLoopPointReached;
            _audioPlayer.prepareCompleted -= AudioVideoPlayerOnPrepareCompleted;
            _audioPlayer.prepareCompleted += AudioVideoPlayerOnPrepareCompleted;
            _audioPlayer.errorReceived -= AudioVideoPlayerOnErrorReceived;
            _audioPlayer.errorReceived += AudioVideoPlayerOnErrorReceived;
            
            _audioPlayer.renderMode = VideoRenderMode.APIOnly;
            _audioPlayer.audioOutputMode = VideoAudioOutputMode.Direct;
            _audioPlayer.EnableAudioTrack(0, true);
            _audioPlayer.SetDirectAudioVolume(0, 1f);
            _audioPlayer.SetDirectAudioMute(0, false);
            _audioPlayer.controlledAudioTrackCount = 1;
            _audioPlayer.Prepare();
        }
        bool geomOpened;
        if(volFormat == VolEnums.VolFormat.Video) {
            _fullGeomPath = volFolderPathType.ResolvePath(volFolder);
            string headerFile = Path.Combine(_fullGeomPath, "header.vols");
            string sequenceFile = Path.Combine(_fullGeomPath, "sequence_0.vols");
            geomOpened = VolPluginInterface.VolGeomOpenFile(headerFile, sequenceFile, true);
        } else {
            string headerFile = "";
            _fullGeomPath = volFilePathType.ResolvePath(volFile);
            geomOpened = VolPluginInterface.VolGeomOpenFile(headerFile, _fullGeomPath, true);
        }
        if (!geomOpened)
        {
            if (_hasVideoTexture)
                VolPluginInterface.VolCloseFile();
            IsOpen = false;
            Close();
            return false;
        }

        _currentlyLoadedFrameIndex = -1;
        _animationAccumulatedSeconds = 0f;
        _numFrames = VolPluginInterface.VolGeomGetFrameCount();
        double fps = VolPluginInterface.VolGetFrameRate();
        if ( 0.0 == fps ) { fps = 30.0; }
        _secondsPerFrame = 1f / fps; // TODO(Anton) -- we should fetch this from vol_av rather than rely on 30fps.

        _voloTexture = new Texture2D(
            VolPluginInterface.VolGetVideoWidth(),
            VolPluginInterface.VolGetVideoHeight(), 
            TextureFormat.RGB24, false, false);

        _textureId = Shader.PropertyToID(textureShaderId);

        if (material == null)
        {
            Debug.LogWarning("Material in VolPlayer component is empty. Checking MeshRenderer");

#if UNITY_EDITOR
            if (_meshRenderer.sharedMaterial == null)
            {
                Debug.LogWarning("Material in MeshRenderer is empty, creating a new one with Shader \"Unlit/Texture\"");
                _meshRenderer.sharedMaterial = new Material(Shader.Find("Unlit/Texture"));
            }
            else
            {
                Debug.LogWarning("Using Material in MeshRenderer");
            }
#else
            if (_meshRenderer.material == null)
            {
                Debug.LogWarning("Material in MeshRenderer is empty, creating a new one with Shader \"Unlit/Texture\"");
                _meshRenderer.material = new Material(Shader.Find("Unlit/Texture"));
            }
            else
            {
                Debug.LogWarning("Using Material in MeshRenderer");
            }
#endif
        }
        else
        {
#if UNITY_EDITOR
            _meshRenderer.sharedMaterial = material;
#else
            _meshRenderer.material = material;
#endif
        }
        
#if UNITY_EDITOR
        _meshRenderer.sharedMaterial.SetTexture(_textureId, _voloTexture);
#else
        _meshRenderer.material.SetTexture(_textureId, _voloTexture);
#endif
        
        IsOpen = true;
        //VolPluginInterface.InitCommandBuffer();
        return true;
    }
    
    /// <summary>
    /// Closes the open vologram files 
    /// </summary>
    /// <returns>True if successful</returns>
    public bool Close()
    {
        if (!IsOpen) 
            return false;
        
        IsPlaying = false;
        bool closedVideo = VolPluginInterface.VolCloseFile();
        bool freedGeom = VolPluginInterface.VolFreeGeomData();
        IsOpen = false;

        if (audioOn)
        {
            _audioPlayer.Stop();
        }
        
        VolPluginInterface.ClearLoggingFunctions();
        
        return closedVideo && freedGeom;
    }

    /// <summary>
    /// Play the vologram
    /// </summary>
    public void Play()
    {
        if (!IsOpen) 
            return;
        
        IsPlaying = true;

        if (audioOn && _audioPlayer != null)
        {
            _audioPlayer.Play();
        }
    }

    /// <summary>
    /// Pauses the vologram
    /// </summary>
    public void Pause()
    {
        if (!IsOpen) 
            return;
        
        IsPlaying = false;
        
        if (audioOn && _audioPlayer != null)
        {
            _audioPlayer.Pause();
        }
    }

    /// <summary>
    /// Closes the vologram and re-opens it
    /// </summary>
    /// <returns>True if successful</returns>
    public bool Restart()
    {
        if (!IsOpen) 
            return false;
        
        bool closed = Close();
        if (!closed)
            return false;
        
        if (_hasVideoTexture)
        {
            bool openedVideo = VolPluginInterface.VolOpenFile(_fullVideoPath);
            if (!openedVideo)
            {
                IsOpen = false;
                return false;
            }
        }
        bool geomOpened;
        if(volFormat == VolEnums.VolFormat.Video) {
            string headerFile = Path.Combine(_fullGeomPath, "header.vols");
            string sequenceFile = Path.Combine(_fullGeomPath, "sequence_0.vols");
            geomOpened = VolPluginInterface.VolGeomOpenFile(headerFile, sequenceFile, true);
        } else {
            string headerFile = "";
            geomOpened = VolPluginInterface.VolGeomOpenFile(headerFile, _fullGeomPath, true);
        }
        
        if (!geomOpened)
        {
            if (_hasVideoTexture)
                VolPluginInterface.VolCloseFile();
            IsOpen = false;
            return false;
        }

        _currentlyLoadedFrameIndex = -1;
        _animationAccumulatedSeconds = 0f;

        IsOpen = true;
        if (playOnStart)
            Play();
        return true;
    }

    /// <summary>
    /// (EXPERIMENTAL) Move forward one frame
    /// </summary>
    public void Step()
    {
        if (!IsOpen) 
            return;
        
        playOnStart = false;

        int desiredFrameIndex = _currentlyLoadedFrameIndex + 1;
        if (desiredFrameIndex >= _numFrames )
        {
            if (isLooping)
            {
                Restart();
            }
            return;
        }
        // Always skip video frames to desired frame.
        ReadVideoFrame(_currentlyLoadedFrameIndex, desiredFrameIndex);
        ReadGeomFrame(desiredFrameIndex);
    }

    /// <summary>
    /// Mute or unmute the vologram's audio
    /// </summary>
    /// <param name="mute">Value for mute</param>
    public void SetMute(bool mute)
    {
        if (audioOn && _audioPlayer != null)
        {
            _audioPlayer.SetDirectAudioMute(0, mute);
        }
    }

    /// <summary>
    /// Get the width in pixels of the texture video
    /// </summary>
    /// <returns>Width in pixels of texture video</returns>
    public int GetVideoWidth()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot get the width of the video, call Open() first");
            return -1;
        }
        
        return VolPluginInterface.VolGetVideoWidth();
    }

    /// <summary>
    /// Get the height in pixels of the texture video
    /// </summary>
    /// <returns>Height in pixels of texture video</returns>
    public int GetVideoHeight()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot get the height of the video, call Open() first");
            return -1;
        }

        return VolPluginInterface.VolGetVideoHeight();
    }

    /// <summary>
    /// Get the frames per second of the texture video 
    /// </summary>
    /// <returns>Frames per second of the texture video</returns>
    public double GetVideoFrameRate()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot get the frame rate of the video, call Open() first");
            return -1.0;
        }

        return VolPluginInterface.VolGetFrameRate();
    }

    /// <summary>
    /// Get the number of image frames in the texture video
    /// </summary>
    /// <returns>The number of frames in the texture video</returns>
    public long GetVideoNumberOfFrames()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot get the frame rate of the video, call Open() first");
            return -1L;
        }

        return VolPluginInterface.VolGetNumFrames();
    }

    /// <summary>
    /// Get the duration in seconds of the texture video
    /// </summary>
    /// <returns>The duration in seconds of the texture video</returns>
    public double GetVideoDuration()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot get the number of frames in the video, call Open() first");
            return -1.0;
        }

        return VolPluginInterface.VolGetDuration();
    }

    /// <summary>
    /// Get the size in bytes of an image from the texture video
    /// </summary>
    /// <returns>The size in bytes of an image from the texture video</returns>
    public long GetVideoFrameSize()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot get the frame size of the video, call Open() first");
            return -1L;
        }

        return VolPluginInterface.VolGetFrameSize();
    }

    /// <summary>
    /// Get the geometry data of the most recent read frame
    /// </summary>
    /// <returns>Struct containing the geometry data</returns>
    private VolPluginInterface.VolGeometryData? GetFrameData()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot get the geometry data, call Open() first");
            return null;
        }

        return VolPluginInterface.VolGeomGetPtrData();
    }

    /// <summary>
    /// Read a desired texture frame
    /// </summary>
    /// <param name="currentFrameIndex">The frame we last played.</param>
    /// <param name="desiredFrameIndex">The frame we want to retrieve and upload to the current texture.</param>
    private void ReadVideoFrame(int currentFrameIndex, int desiredFrameIndex)
    {
        if (desiredFrameIndex >= _numFrames || currentFrameIndex >= desiredFrameIndex ) { return; }

        // Always skip ahead to desired frame. (This is a workaround until we get better video decoder seek behaviour).
        for (int videoFrameIndex = _currentlyLoadedFrameIndex; videoFrameIndex < desiredFrameIndex - 1; videoFrameIndex++ )
        {
            _colorPtr = VolPluginInterface.VolReadNextVideoFrame(false);
        }
        // This is the frame we want, and we vertically flip this too.
        _colorPtr = VolPluginInterface.VolReadNextVideoFrame(true);
        { // Upload only the texture from the desired frame to the GPU via Unity.
            _voloTexture.LoadRawTextureData(_colorPtr, (int) VolPluginInterface.VolGetFrameSize());
            _voloTexture.Apply();
#if UNITY_EDITOR
            _meshRenderer.sharedMaterial.SetTexture(_textureId, _voloTexture);
#else
            _meshRenderer.material.SetTexture(_textureId, _voloTexture);
#endif
        }
    }

    /// <summary>
    /// Read and process a frame's geometry data
    /// </summary>
    private void ReadGeomFrame(int frame)
    {
        if ( frame >= _numFrames ) { return; }

        bool isKeyframe = VolPluginInterface.VolGeomIsKeyframe(frame);
        string sequenceFile;

        if(volFormat == VolEnums.VolFormat.Video)
            sequenceFile = Path.Combine(_fullGeomPath, "sequence_0.vols");
        else 
            sequenceFile = _fullGeomPath;

        if (!VolPluginInterface.VolGeomReadFrame(sequenceFile, frame))
        {
            Debug.LogError("Error loading geometry frame");
            return;
        }
        
        _geometryData = VolPluginInterface.VolGeomGetPtrData();

        if (_geometryData.blockDataSize == 0)
            return;

        // TODO(Anton) maybe can remove a memcopy here with a cast/pointer? 
        _meshData = new byte[_geometryData.blockDataSize];
        Marshal.Copy(_geometryData.blockDataPtr, _meshData, 0, (int)_geometryData.blockDataSize);
        NativeArray<byte> nativeMeshData = new NativeArray<byte>(_meshData, Allocator.Temp);

#if UNITY_EDITOR
        _meshFilter.sharedMesh.Clear();
#else
        _meshFilter.mesh.Clear();
#endif
        
        NativeSlice<Vector3> verticesSlice =
            nativeMeshData.Slice((int)_geometryData.verticesOffset, _geometryData.verticesSize).SliceConvert<Vector3>();
        
#if UNITY_EDITOR
        _meshFilter.sharedMesh.SetVertices(verticesSlice.ToArray());
#else
        _meshFilter.mesh.SetVertices(verticesSlice.ToArray());
#endif

        if (_geometryData.normalSize > 0)
        {
            NativeSlice<Vector3> normalsSlice = nativeMeshData
                .Slice((int) _geometryData.normalOffset, _geometryData.normalSize).SliceConvert<Vector3>();
#if UNITY_EDITOR
            _meshFilter.sharedMesh.SetNormals(normalsSlice.ToArray());
#else
            _meshFilter.mesh.SetNormals(normalsSlice.ToArray());
#endif
        }
        // TODO(Anton) - Patrick please check this, I was hoping to skip some updates here
        isKeyframe = true; // HACK(Anton) ... but it didn't work.
 
        if ( isKeyframe && _geometryData.indicesSize > 0)
        {
            NativeSlice<ushort> indicesSlice =
                nativeMeshData.Slice((int) _geometryData.indicesOffset, _geometryData.indicesSize).SliceConvert<ushort>();
            _keyShortIndices = indicesSlice.ToArray();
        }

        if ( isKeyframe ) {
#if UNITY_EDITOR
            _meshFilter.sharedMesh.SetIndices(_keyShortIndices, MeshTopology.Triangles, 0);
#else
            _meshFilter.mesh.SetIndices(_keyShortIndices, MeshTopology.Triangles, 0);
#endif
        }

        if ( isKeyframe && _geometryData.uvSize > 0)
        {
            NativeSlice<Vector2> uvSlice = nativeMeshData.Slice((int) _geometryData.uvOffset, _geometryData.uvSize)
                .SliceConvert<Vector2>();
            _keyUvs = uvSlice.ToArray();
        }
        
#if UNITY_EDITOR
        if ( isKeyframe ) { _meshFilter.sharedMesh.SetUVs(0, _keyUvs); } // TODO(Anton) check this if statement makes sense.
        _meshFilter.sharedMesh.RecalculateBounds();
        _meshFilter.sharedMesh.MarkModified();
#else 
        if ( isKeyframe ) { _meshFilter.mesh.SetUVs(0, _keyUvs); } // TODO(Anton) check this if statement makes sense.
        _meshFilter.mesh.RecalculateBounds();
        _meshFilter.mesh.MarkModified();
#endif

        nativeMeshData.Dispose();
    }

    /// <summary>
    /// Change the vologram's material in runtime 
    /// </summary>
    /// <param name="newMaterial">New material to be applied to the vologram</param>
    public void ChangeMaterial(Material newMaterial)
    {
        material = newMaterial;
#if UNITY_EDITOR
        _meshRenderer.sharedMaterial = newMaterial;
#else
        _meshRenderer.material = newMaterial;
#endif
    }

    private void AudioVideoPlayerOnErrorReceived(VideoPlayer source, string message)
    {
        Debug.LogError(message);
    }

    private void AudioVideoPlayerOnPrepareCompleted(VideoPlayer source)
    {
        source.Play();
        Play();
    }

    private void AudioVideoPlayerOnLoopPointReached(VideoPlayer source)
    {
        Restart();
    }

    private void AudioVideoPlayerOnFrameReady(VideoPlayer source, long frameidx)
    {

    }
}
