using System;
using System.IO;
using System.Runtime.InteropServices;
using Unity.Collections;
using UnityEngine;
using UnityEngine.Experimental.Rendering;
using UnityEngine.Video;

[RequireComponent(typeof(MeshFilter))]
[RequireComponent(typeof(MeshRenderer))]
public class VolPlayer : MonoBehaviour
{
    [Header("Paths")]
    public VolEnums.PathType volFolderPathType;
    public string volFolder;

    public VolEnums.PathType volVideoTexturePathType;
    public string volVideoTexture;

    [Header("Playback Settings")]
    public bool playOnStart = true;
    public bool isLooping = true;
    public bool audioOn = false;

    [Header("Rendering Settings")] 
    public Material material;
    public string textureShaderId = "_MainTex";

    [Header("Debug Logging Options")]
    public bool enableInterfaceLogging = false;
    public bool enableAvLogging = false;
    public bool enableGeomLogging = false;
    
    private string _fullGeomPath;
    private string _fullVideoPath;
    private int _frameCount;
    private int _numFrames;
    private bool _hasVideoTexture;
    private float _timeTracker;
    private float _secondsPerFrame;
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

    public int Frame => _frameCount;
    
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

    private void Update()
    {
        if (!IsPlaying) return;
        
        if (VolPluginInterface.VolGeomGetNextFrameIndex() >= _numFrames)
        {
            if (isLooping)
            {
                Restart();
                return;
            }
            IsPlaying = false;
            Close();
            return;
        }
        
        _timeTracker += Time.deltaTime;
        
        if (!(_timeTracker > _secondsPerFrame)) 
            return;
        
        ReadNextFrame();
        ReadNextGeom();
        _timeTracker -= _secondsPerFrame;
    }

    private void OnEnable()
    {
        _meshFilter = GetComponent<MeshFilter>();
        _meshRenderer = GetComponent<MeshRenderer>();
    }

    private void OnDisable()
    {
        Close();
    }

    public bool Open()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot open a vologram while another is open");
            return false;
        }
        
        if (enableInterfaceLogging)
            VolPluginInterface.EnableInterfaceLogging();
        
        if (enableAvLogging)
            VolPluginInterface.EnableAvLogging();
        
        if (enableGeomLogging)
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

        _fullGeomPath = volFolderPathType.ResolvePath(volFolder);
        string headerFile = Path.Combine(_fullGeomPath, "header.vols");
        string sequenceFile = Path.Combine(_fullGeomPath, "sequence_0.vols");
        bool geomOpened = VolPluginInterface.VolGeomOpenFile(headerFile, sequenceFile, true);
        
        if (!geomOpened)
        {
            if (_hasVideoTexture)
                VolPluginInterface.VolCloseFile();
            IsOpen = false;
            Close();
            return false;
        }

        _frameCount = 0;
        _numFrames = VolPluginInterface.VolGeomGetFrameCount(); 
        _secondsPerFrame = 1f / 30f;

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
            _meshRenderer.material = volMaterial;
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
    
    public bool Close()
    {
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

    public void Play()
    {
        IsPlaying = true;
    }

    public void Pause()
    {
        IsPlaying = false;
    }

    private bool _isSkipping = false;
    public void SkipTo(int frame)
    {
        if (_isSkipping || frame <= _frameCount)
            return;

        _isSkipping = true;
        Pause();

        while (_frameCount < frame)
        {
            ReadNextFrame(true);
        }
        
        ReadNextFrame();
        ReadNextGeom(frame);
        _isSkipping = false;
    }

    public bool Restart()
    {
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

        string headerFile = Path.Combine(_fullGeomPath, "header.vols");
        string sequenceFile = Path.Combine(_fullGeomPath, "sequence_0.vols");
        bool geomOpened = VolPluginInterface.VolGeomOpenFile(headerFile, sequenceFile, true);
        
        if (!geomOpened)
        {
            if (_hasVideoTexture)
                VolPluginInterface.VolCloseFile();
            IsOpen = false;
            return false;
        }

        _frameCount = 0;

        IsOpen = true;
        if (playOnStart)
            Play();
        return true;
    }

    public void Step()
    {
        playOnStart = false;
        if (VolPluginInterface.VolGeomGetNextFrameIndex() >= _numFrames)
        {
            Restart();
        }
        ReadNextFrame();
        ReadNextGeom();
    }

    public void SetMute(bool mute)
    {
        if (audioOn)
        {
            _audioPlayer.SetDirectAudioMute(0, mute);
        }
    }

    public int GetVideoWidth()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot get the width of the video, call Open() first");
            return -1;
        }
        
        return VolPluginInterface.VolGetVideoWidth();
    }

    public int GetVideoHeight()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot get the height of the video, call Open() first");
            return -1;
        }

        return VolPluginInterface.VolGetVideoHeight();
    }

    public double GetVideoFrameRate()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot get the frame rate of the video, call Open() first");
            return -1.0;
        }

        return VolPluginInterface.VolGetFrameRate();
    }

    public long GetVideoNumberOfFrames()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot get the frame rate of the video, call Open() first");
            return -1L;
        }

        return VolPluginInterface.VolGetNumFrames();
    }

    public double GetVideoDuration()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot get the number of frames in the video, call Open() first");
            return -1.0;
        }

        return VolPluginInterface.VolGetDuration();
    }

    public long GetVideoFrameSize()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot get the frame size of the video, call Open() first");
            return -1L;
        }

        return VolPluginInterface.VolGetFrameSize();
    }

    private VolPluginInterface.VolGeometryData? GetFrameData()
    {
        if (IsOpen)
        {
            Debug.LogWarning("Cannot get the geometry data, call Open() first");
            return null;
        }

        return VolPluginInterface.VolGeomGetPtrData();
    }

    private void ReadNextFrame(bool dispose = false)
    {
        if (_frameCount >= _numFrames)
            return;

        _colorPtr = VolPluginInterface.VolReadNextFrame();

        if (!dispose)
        {
            _voloTexture.LoadRawTextureData(_colorPtr, (int) VolPluginInterface.VolGetFrameSize());
            _voloTexture.Apply();

#if UNITY_EDITOR
            _meshRenderer.sharedMaterial.SetTexture(_textureId, _voloTexture);
#else
            _meshRenderer.material.SetTexture(VMainTex, _voloTexture);
#endif
        }

        _frameCount++;
    }

    private void ReadNextGeom(int frame = -1)
    {
        string sequenceFile = Path.Combine(_fullGeomPath, "sequence_0.vols");
        bool success = frame == -1 
            ? VolPluginInterface.VolGeomReadNextFrame(sequenceFile) 
            : VolPluginInterface.VolGeomReadFrame(sequenceFile, frame);
        if (!success)
        {
            Debug.LogError("Error loading geometry frame");
            return;
        }
        
        _geometryData = VolPluginInterface.VolGeomGetPtrData();

        if (_geometryData.blockDataSize == 0)
            return;

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

        if (_geometryData.indicesSize > 0)
        {
            NativeSlice<ushort> indicesSlice =
                nativeMeshData.Slice((int) _geometryData.indicesOffset, _geometryData.indicesSize).SliceConvert<ushort>();
            _keyShortIndices = indicesSlice.ToArray();
        }

#if UNITY_EDITOR
        _meshFilter.sharedMesh.SetIndices(_keyShortIndices, MeshTopology.Triangles, 0);
#else
        _meshFilter.mesh.SetIndices(_keyShortIndices, MeshTopology.Triangles, 0);
#endif

        if (_geometryData.uvSize > 0)
        {
            NativeSlice<Vector2> uvSlice = nativeMeshData.Slice((int) _geometryData.uvOffset, _geometryData.uvSize)
                .SliceConvert<Vector2>();
            _keyUvs = uvSlice.ToArray();
        }
        
#if UNITY_EDITOR
        _meshFilter.sharedMesh.SetUVs(0, _keyUvs);
        _meshFilter.sharedMesh.RecalculateBounds();
        _meshFilter.sharedMesh.MarkModified();
#else 
        _meshFilter.mesh.SetUVs(0, _keyUvs);
        _meshFilter.mesh.RecalculateBounds();
        _meshFilter.mesh.MarkModified();
#endif

        nativeMeshData.Dispose();
    }

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
        if (frameidx - Frame > 15)
        {
            SkipTo((int)frameidx);
        }
    }
}
