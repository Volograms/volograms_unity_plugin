using System;
using System.IO;
using System.Runtime.InteropServices;
using Unity.Collections;
using UnityEngine;
using UnityEngine.Experimental.Rendering;

[RequireComponent(typeof(MeshFilter))]
[RequireComponent(typeof(MeshRenderer))]
public class VolPlayer : MonoBehaviour
{
    public enum PathType
    {
        Absolute,
        Persistent,
        Streaming,
        Data
    }

    [Header("Paths")]
    public PathType volFolderPathType;
    public string volFolder;

    public PathType volVideoTexturePathType;
    public string volVideoTexture;

    [Header("Playback Settings")]
    public bool playOnStart = true;
    public bool isLooping = true;
    
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
    [SerializeField]
    private Texture2D _voloTexture;
    private IntPtr _colorPtr;
    private VolPluginInterface.VolGeometryData _geometryData;
    private byte[] _meshData;
    private static readonly int VMainTex = Shader.PropertyToID("_MainTex");

    public bool IsOpen { get; private set; }
    public bool IsPlaying { get; private set; }
    
    private void Start()
    {
        print("Start VolPlayer");
        VolPluginInterface.SetUpDebugging();
        Application.targetFrameRate = 60;
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
        
        if (playOnStart)
        {
            Open();
            Play();
        }
    }

    void Update()
    {
        //print($"Playing {IsPlaying}, numFrames {_numFrames}, nextFrame {VolPluginInterface.VolGeomGetNextFrameIndex()}, timeTracker {_timeTracker}");
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

        _hasVideoTexture = !string.IsNullOrEmpty(volVideoTexture);
        _fullVideoPath = ResolvePath(volVideoTexturePathType, volVideoTexture);

        if (_hasVideoTexture)
        {
            bool openedVideo = VolPluginInterface.VolOpenFile(_fullVideoPath);
            if (!openedVideo)
            {
                IsOpen = false;
                return false;
            }
        }
        
        print(VolPluginInterface.VolGetVideoWidth());
        print(VolPluginInterface.VolGetVideoHeight());
        print(VolPluginInterface.VolGetNumFrames());
        print(VolPluginInterface.VolGetDuration());
        print(VolPluginInterface.VolGetFrameRate());
        print(VolPluginInterface.VolGetFrameSize());


        _fullGeomPath = ResolvePath(volFolderPathType, volFolder);
        string headerFile = Path.Combine(_fullGeomPath, "header.vols");
        string sequenceFile = Path.Combine(_fullGeomPath, "sequence_0.vols");
        bool geomOpened = VolPluginInterface.VolGeomOpenFile(headerFile, sequenceFile);
        print($"Geometry opened: {geomOpened}");
        
        if (!geomOpened)
        {
            if (_hasVideoTexture)
                VolPluginInterface.VolCloseFile();
            IsOpen = false;
            return false;
        }
        
        print(VolPluginInterface.VolGeomGetFrameCount());
        print(VolPluginInterface.VolGeomGetPtrData());

        _frameCount = 0;
        _numFrames = VolPluginInterface.VolGeomGetFrameCount(); 
        _secondsPerFrame = 1f / 60f;

        _voloTexture = new Texture2D(
            VolPluginInterface.VolGetVideoWidth(),
            VolPluginInterface.VolGetVideoHeight(), 
            TextureFormat.RGB24, false, false);
        
#if UNITY_EDITOR
        _meshRenderer.sharedMaterial.SetTexture(VMainTex, _voloTexture);
#else
        _meshRenderer.material.SetTexture(VMainTex, _voloTexture);
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
        bool geomOpened = VolPluginInterface.VolGeomOpenFile(headerFile, sequenceFile);
        
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

    private void ReadNextFrame()
    {
        if (_frameCount >= _numFrames)
            return;

        //IntPtr ptr = VolPluginInterface.VolReadNextFrame();
        //VolPluginInterface.UpdateTexture(_voloTexture);
        //_frameCount++;
        //return;
        
        _colorPtr = VolPluginInterface.VolReadNextFrame();
        _voloTexture.LoadRawTextureData(_colorPtr, (int)VolPluginInterface.VolGetFrameSize());
        _voloTexture.Apply();
        
#if UNITY_EDITOR
        _meshRenderer.sharedMaterial.SetTexture(VMainTex, _voloTexture);
#else
        _meshRenderer.material.SetTexture(VMainTex, _voloTexture);
#endif
        _frameCount++;
    }

    private void ReadNextGeom()
    {
        string sequenceFile = Path.Combine(_fullGeomPath, "sequence_0.vols");
        bool success = VolPluginInterface.VolGeomReadNextFrame(sequenceFile);
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
#if UNITY_EDITOR
        _meshRenderer.sharedMaterial = newMaterial;
#else
        _meshRenderer.material = newMaterial;
#endif
    }

    private string ResolvePath(PathType type, string path)
    {
        switch (type)
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
}
