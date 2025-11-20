// <copyright file=VolPlayer company=Volograms>
// Copyright (c) 2022 All Rights Reserved
// </copyright>
// <author>Patrick Geoghegan</author>
// <date>18/02/22</date>
// <summary>Controls for vologram playback</summary>

using Codice.Utils;
using System;
using System.Collections;
using System.IO;
using System.Runtime.InteropServices;
using Unity.Collections;
using UnityEngine;
using UnityEngine.Networking;
using UnityEngine.Video;


namespace Volograms
{
    [Serializable]
    [RequireComponent(typeof(MeshFilter))]
    [RequireComponent(typeof(MeshRenderer))]
    public class VolPlayer : MonoBehaviour
    {
        [Header("Streaming Settings (Single-file format only)")]
        [Tooltip("File: Stream to disk (mobile-friendly). Buffer: Stream to memory (faster seeking).")]
        public VolEnums.StreamingMode streamingMode;

        // Private streaming state
        private bool _isStreaming = false;
        private bool _isBuffering = false;
        // _isSeek is used when playOnStart is not set to show frame 0 mesh on open or restart
        private bool _isSeek = false;
        private Coroutine _downloadCoroutine;

        [Header("Buffer Settings (Buffer Mode Only)")]
        [Tooltip("Buffer size in MB")]
        public int bufferSizeMB = 60;
        [Tooltip("Seconds of video to keep ahead")]
        public float bufferAheadSeconds = 2.0f;

        public float StreamingProgress { get; private set; } = 0f;
        private VolStreamingSession _streamingSession;

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
        public bool audioOn = true;

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
        private double _framesPerSecond;
        private MeshFilter _meshFilter;
        private MeshRenderer _meshRenderer;
        private ushort[] _keyShortIndices;
        private Vector2[] _keyUvs;
        private Texture2D _voloTexture;
        private IntPtr _colorPtr;
        private VolPluginInterface.VolGeometryData _geometryData;
        private byte[] _meshData;
        private int _textureId;
        private VideoPlayer _audioPlayerVideo;
        private AudioSource _audioPlayerVols;


        public bool IsOpen { get; private set; }
        public bool IsPlaying { get; private set; }
        //public int Frame => _currentFrameIndex; // TODO(Anton) have i broken something here?
        //public bool IsMuted => audioOn;

        /// <summary>
        /// Unity's Start function - called on the first frame
        /// </summary>
        private void Start()
        {
            //streamingMode = VolEnums.StreamingMode.Buffer;
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
            if (!TryGetComponent<VideoPlayer>(out _audioPlayerVideo))
            {
                _audioPlayerVideo = gameObject.AddComponent<VideoPlayer>();
            }
            if (!TryGetComponent<AudioSource>(out _audioPlayerVols))
            {
                _audioPlayerVols = gameObject.AddComponent<AudioSource>();
            }

            Open();
        }

        private bool isBuffering(int desiredFrameIndex)
        {
            int lookaheadFrameIndex = desiredFrameIndex;
            bool isFrameAvailable = false;

            if (_isBuffering)
            {
                // Wait until we have enough frames buffered ahead
                if (streamingMode == VolEnums.StreamingMode.Buffer)
                {
                    float bufferHealthSeconds = VolPluginInterface.VolGetBufferHealthSeconds((float)_framesPerSecond);
                    if (bufferHealthSeconds < bufferAheadSeconds)
                        return true;
                }
                else
                {
                    // During file streaming we just look ahead a few seconds
                    lookaheadFrameIndex = Math.Min((int)(desiredFrameIndex + _framesPerSecond * bufferAheadSeconds), _numFrames - 1);
                }
            }

            if (streamingMode == VolEnums.StreamingMode.File)
                isFrameAvailable = VolPluginInterface.VolGeomUpdateFramesDirectory(_fullGeomPath, lookaheadFrameIndex);
            else // Buffer mode
                isFrameAvailable = VolPluginInterface.VolIsFrameAvailableInBuffer(desiredFrameIndex);

            if (!isFrameAvailable)
            {
                // Still downloading - pause plauyback until we have enough data.
                //Debug.Log($"Buffering... waiting for frame {desiredFrameIndex}");
                BufferingPause();
                _isBuffering = true;

                return true;
            }
            else
            {
                if (_isBuffering)
                {
                    //Debug.Log("Resuming playback after buffering.");
                    BufferingResume();
                }
                _isBuffering = false;
            }

            return false;
        }

        /// <summary>
        /// Unity's Update function - called every frame
        /// </summary>
        private void Update()
        {
            double deltaTime = Time.deltaTime;

            if (!_isSeek)
            {
                if (!IsOpen || !IsPlaying)
                    return;
            }
            
            if (IsPlaying && !_isBuffering)
            {
                // Work out the frame index to play based on elapsed animation time. This lets us skip to the correct frame when the player is going slowly.
                _animationAccumulatedSeconds += deltaTime;
            }

            int desiredFrameIndex = (int)(_animationAccumulatedSeconds / _secondsPerFrame);

            // Not enough time has passed to advance to the next frame yet.
            if (desiredFrameIndex == _currentlyLoadedFrameIndex || desiredFrameIndex < 0) { return; }

            //Debug.Log("Desired Frame Index: " + desiredFrameIndex);

            if (!_isSeek && desiredFrameIndex >= _numFrames)
            {
                if (isLooping)
                {
                    // Restart triggers play only if playOnStart is TRUE, we need to call Play() here
                    Restart();
                    Play();
                }
                else
                {
                    Stop();
                }
                return;
            }

            // Check if we are waiting for frames to download during streaming
            if (_isStreaming && isBuffering(desiredFrameIndex)) return;

            // If we are buffering, we skip the actual frame loading until we have enough data.
            if (!_isBuffering)
            {
                // --VIDEO TEXTURE--
                // Always skip video frames to desired frame.
                if (volFormat == VolEnums.VolFormat.Video)
                {
                    ReadVideoFrame(_currentlyLoadedFrameIndex, desiredFrameIndex);
                }

                // --GEOMETRY--
                int previousKeyframeIndex = VolPluginInterface.VolGeomFindPreviousKeyframe(desiredFrameIndex);
                bool desiredIsKeyframe = VolPluginInterface.VolGeomIsKeyframe(desiredFrameIndex);
                // If our desired frame would jump over its proceeding keyframe, we need to stop and load that first,
                // unless it is a keyframe itself.
                bool needToLoadKeyframe = (_currentlyLoadedFrameIndex < previousKeyframeIndex) && !desiredIsKeyframe;
                if (needToLoadKeyframe)
                {
                    if (!ReadGeomFrame(previousKeyframeIndex)) return;
                }
                if (!ReadGeomFrame(desiredFrameIndex)) return;

                // --BASISU TEXTURE--
                if (volFormat == VolEnums.VolFormat.BasisU)
                {
                    // Swap the order - mesh first, texture next
                    ReadTextureFrame(_currentlyLoadedFrameIndex, desiredFrameIndex);
                }

                // Advance frame
                _currentlyLoadedFrameIndex = desiredFrameIndex;
            }

            // Update buffer streaming session
            if (_isStreaming && streamingMode == VolEnums.StreamingMode.Buffer)
            {
                _streamingSession.SetLoopStreaming(isLooping);

                if (VolPluginInterface.VolShouldResumeDownload(_currentlyLoadedFrameIndex, (float)_framesPerSecond))
                    _streamingSession.ResumeDownload();

            }
            _isSeek = false;
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
        ///  
        /// </summary>
        /// <param name="url"></param>
        /// <returns></returns>
        public IEnumerator LoadAudio(string url)
        {
            using (UnityWebRequest www = UnityWebRequestMultimedia.GetAudioClip(url, AudioType.MPEG))
            {
                yield return www.SendWebRequest();

                if (www.result == UnityEngine.Networking.UnityWebRequest.Result.Success)
                {
                    AudioClip clip = UnityEngine.Networking.DownloadHandlerAudioClip.GetContent(www);
                    clip.LoadAudioData();
                    _audioPlayerVols.clip = clip;
                    //Debug.Log("Audio Loaded");
                }
                else
                {
                    Debug.LogError("Failed to load audio: " + www.error);
                }
            }
        }

        /// <summary>
        /// Public method to Open the given vologram files
        /// </summary>
        /// 
        public void Open()
        {
            Open(
                onComplete: () =>
                {
                    if (playOnStart)
                        Play();
                    else
                        // To load first frame, otherwise it wil not show the mesh
                        _isSeek = true;
                },
                onProgress: progress =>
                {
                    StreamingProgress = progress;
                }
            );
        }

        /// <summary>
        ///  Private version of Open with callbacks
        /// </summary>
        /// <param name="onComplete"></param>
        /// <param name="onProgress"></param>
        private void Open(System.Action onComplete = null, System.Action<float> onProgress = null)
        {
            StartCoroutine(OpenCoroutine(onComplete, onProgress));
        }

        /// <summary>
        ///     
        /// </summary>
        /// <param name="onComplete"></param>
        /// <returns></returns>
        private IEnumerator StartStreamingMode(System.Action onComplete, System.Action<float> onProgress = null, Action<string> onError = null)
        {
            _isStreaming = true;
            string streamURL = volFilePathType.ResolvePath(volFile);
            bool headerOpened = false;

            if (_streamingSession != null)
            {
                _streamingSession.Stop();
            }
            else
            {
                _streamingSession = new VolStreamingSession(this);
            }

            System.Action<bool> onHeaderOpen = isOpened =>
            {
                //Debug.Log("Streaming: Header opened: " + isOpened);
                headerOpened = true;
            };

            if (streamingMode == VolEnums.StreamingMode.File)
            {
                string filename = Path.GetFileName(streamURL);
                _fullGeomPath = Path.GetFullPath(Path.GetFullPath(Path.Combine(Application.temporaryCachePath, filename)));
                _downloadCoroutine = StartCoroutine(_streamingSession.RunFileStreaming(streamURL, _fullGeomPath, onHeaderOpen, onProgress, onError));
            }
            else
            {
                _downloadCoroutine = StartCoroutine(_streamingSession.RunBufferStreaming(streamURL, isLooping, onHeaderOpen, onProgress, onError, bufferSizeMB));
            }

            yield return new WaitUntil(() => headerOpened); // Wait for the header to be opened
        }

        /// <summary>
        /// Open a video sequence vologram
        /// </summary>
        /// <param name="volVideoTexture"></param>
        /// <param name="volFolder"></param>
        /// 
        /// <returns></returns>
        private IEnumerator OpenVideoSequence(string volVideoTexture, string volFolder, System.Action<string> onError)
        {
            bool geomOpened = false;
            _hasVideoTexture = !string.IsNullOrEmpty(volVideoTexture);
            _fullVideoPath = volVideoTexturePathType.ResolvePath(volVideoTexture);

            if (_hasVideoTexture)
            {
                bool openedVideo = VolPluginInterface.VolOpenFile(_fullVideoPath);
                Debug.Log("Opened vologram video texture from: " + _fullVideoPath + " and " + openedVideo);
                if (openedVideo)
                {
                    // Audio
                    if (audioOn)
                    {
                        _audioPlayerVideo.Stop();
                        _audioPlayerVideo.sendFrameReadyEvents = true;
                        _audioPlayerVideo.source = VideoSource.Url;
                        _audioPlayerVideo.url = _fullVideoPath;

                        _audioPlayerVideo.frameReady -= AudioVideoPlayerOnFrameReady;
                        _audioPlayerVideo.frameReady += AudioVideoPlayerOnFrameReady;
                        _audioPlayerVideo.loopPointReached -= AudioVideoPlayerOnLoopPointReached;
                        _audioPlayerVideo.loopPointReached += AudioVideoPlayerOnLoopPointReached;
                        _audioPlayerVideo.errorReceived -= AudioVideoPlayerOnErrorReceived;
                        _audioPlayerVideo.errorReceived += AudioVideoPlayerOnErrorReceived;

                        _audioPlayerVideo.renderMode = VideoRenderMode.APIOnly;
                        _audioPlayerVideo.audioOutputMode = VideoAudioOutputMode.Direct;
                        _audioPlayerVideo.EnableAudioTrack(0, true);
                        _audioPlayerVideo.SetDirectAudioVolume(0, 1f);
                        _audioPlayerVideo.SetDirectAudioMute(0, false);
                        _audioPlayerVideo.controlledAudioTrackCount = 1;
                        _audioPlayerVideo.Prepare();
                        yield return new WaitUntil(() => _audioPlayerVideo.isPrepared);
                    }

                    // Mesh
                    _fullGeomPath = volFolderPathType.ResolvePath(volFolder);
                    string headerFile = Path.Combine(_fullGeomPath, "header.vols");
                    string sequenceFile = Path.Combine(_fullGeomPath, "sequence_0.vols");
                    geomOpened = VolPluginInterface.VolGeomOpenFile(headerFile, sequenceFile, true);

                    if (geomOpened)
                    {
                        int texWidth = VolPluginInterface.VolGetVideoWidth();
                        int texHeight = VolPluginInterface.VolGetVideoHeight();

                        // Texture
                        _voloTexture = new Texture2D(
                        texWidth,
                        texHeight,
                        TextureFormat.RGB24, false, false);
                    }
                }
            }
            if(!geomOpened)
            {
                onError?.Invoke("Failed to open video sequence vologram.");
            }
        }

        /// <summary>
        /// Open texture and audio for a single-file vologram    
        /// </summary>
        /// <param name="geomOpened"></param>
        /// <returns></returns>
        private IEnumerator OpenSingleFileSequence(bool geomOpened)
        {
            bool initBasis = VolPluginInterface.VolInitBasisDecoder();
            if (!initBasis)
            {
                Debug.LogError("Failed to initialize Bassis texteure decoder.");
            }
            else
            {
                //Debug.Log("Basis Initialized: " + initBasis);
#if UNITY_ANDROID
                        TextureFormat textureFormat = TextureFormat.ETC_RGB4;
                        //TextureFormat textureFormat = TextureFormat.ETC2_RGBA8;
#elif UNITY_IOS
                        TextureFormat textureFormat = TextureFormat.ASTC_4x4;
#else
                TextureFormat textureFormat = TextureFormat.DXT1;
#endif
                try
                {
                    int texWidth = VolPluginInterface.VolGetTextureWidth();
                    int texHeight = VolPluginInterface.VolGetTextureHeight();

                    if (texHeight > 0 && texWidth > 0)
                    {
                        _voloTexture = new Texture2D(
                            texWidth,
                            texHeight,
                            textureFormat, false, false, true); // TODO(Jan): Set the texture format based on the platform
                    }
                }
                catch (Exception e)
                {
                    _voloTexture = null;
                    Debug.LogError("Failed to create vologram texture. " + e.Message);
                }
            }
            if (audioOn && VolPluginInterface.VolHasAudio())
            {
                //Debug.Log("Loading Audio");
                int audioSize;
                IntPtr audioData = VolPluginInterface.VolGetAudio(out audioSize);
                if (audioData != IntPtr.Zero && audioSize > 0)
                {
                    byte[] audioBytes = new byte[audioSize];
                    Marshal.Copy(audioData, audioBytes, 0, audioSize);

                    // temporary local path for audio file so we can load it via UnityWebRequest
                    string tempPath = System.IO.Path.Combine(Application.temporaryCachePath, System.Guid.NewGuid().ToString() + "-audio.mp3");
                    bool fileExists = false;
                    try
                    {
                        System.IO.File.WriteAllBytes(tempPath, audioBytes);
                        fileExists = System.IO.File.Exists(tempPath);
                    }
                    catch (Exception e)
                    {
                        Debug.LogError("Failed to write audio file to temp path: " + e.Message);
                    }
                    if (fileExists)
                    {
                        string uri = "file://" + tempPath;
                        yield return LoadAudio(uri);

                        // cleanup
                        System.IO.File.Delete(tempPath);
                    }
                }
            }
        }

        /// <summary>
        /// A coroutine to open the vologram files
        /// </summary>
        /// <param name="onComplete"></param>
        /// <param name="onProgress"></param>
        /// <returns></returns>
        public IEnumerator OpenCoroutine(System.Action onComplete = null, System.Action<float> onProgress = null)
        {
            if (IsOpen)
            {
                Debug.LogWarning("Cannot open a vologram while another is open");
                onComplete?.Invoke();
                yield break;
            }

            VolPluginInterface.interfaceLoggingLevel = interfaceLoggingLevel;
            VolPluginInterface.avLoggingLevel = avLoggingLevel;
            VolPluginInterface.geomLoggingLevel = geomLoggingLevel;
            VolPluginInterface.EnableInterfaceLogging();
            VolPluginInterface.EnableAvLogging();
            VolPluginInterface.EnableGeomLogging();

            bool geomOpened = false;

            if (VolEnums.VolFormat.Video == volFormat)
            {
                bool errorOccurred = false;

                System.Action<string> onError = errorMessage =>
                {
                    Debug.LogError("Error during streaming: " + errorMessage);
                    errorOccurred = true;
                };

                yield return OpenVideoSequence(volVideoTexture, volFolder, onError);
                if (!errorOccurred)
                {
                    geomOpened = true;
                }
            }
            else if (VolEnums.VolFormat.BasisU == volFormat)
            {
                bool shouldStream = volFormat == VolEnums.VolFormat.BasisU && volFilePathType == VolEnums.PathType.URL;
                Debug.Log("Opening vologram geometry from: " + volFilePathType.ResolvePath(volFile) + (shouldStream ? " with streaming." : "."));

                if (shouldStream)
                {
                    bool errorOccurred = false;

                    System.Action<string> onError = errorMessage =>
                    {
                        Debug.LogError("Error during streaming: " + errorMessage);
                        errorOccurred = true;
                    };

                    yield return StartStreamingMode(onComplete, onProgress, onError);
                    if (!errorOccurred)
                    {
                        geomOpened = true;
                    }
                }
                else
                {
                    _fullGeomPath = volFilePathType.ResolvePath(volFile);
                    geomOpened = VolPluginInterface.VolGeomOpenFile("", _fullGeomPath, false);
                }

                if (geomOpened)
                {
                    yield return OpenSingleFileSequence(geomOpened);
                }
            }

            if (!geomOpened)
            {
                if (_hasVideoTexture)
                    VolPluginInterface.VolCloseFile();
                IsOpen = false;
                Close();
                onComplete?.Invoke();
                yield break;
            }

            _currentlyLoadedFrameIndex = -1;
            _animationAccumulatedSeconds = 0f;
            _numFrames = VolPluginInterface.VolGeomGetFrameCount();
            _framesPerSecond = VolPluginInterface.VolGetFrameRate();
            if (0.0 == _framesPerSecond) { _framesPerSecond = 30.0; }
            _secondsPerFrame = 1f / _framesPerSecond; // TODO(Anton) -- we should fetch this from vol_av rather than rely on 30fps.

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

            if (_voloTexture)
            {
#if UNITY_EDITOR
                _meshRenderer.sharedMaterial.SetTexture(_textureId, _voloTexture);
#else
                _meshRenderer.material.SetTexture(_textureId, _voloTexture);
#endif
            }
            _currentlyLoadedFrameIndex = -1;
            _animationAccumulatedSeconds = 0f;

            IsOpen = true;
            onComplete?.Invoke();
        }

        /// <summary>
        /// Closes the open vologram files 
        /// </summary>
        /// <returns>True if successful</returns>
        public bool Close()
        {
            Stop();

            IsOpen = false;

            if (_isStreaming)
            {
                // Stop streaming
                if (_streamingSession != null)
                {
                    _streamingSession.Stop();
                    _streamingSession = null;
                }

                if (_downloadCoroutine != null)
                {
                    StopCoroutine(_downloadCoroutine);
                    _downloadCoroutine = null;
                }
            }

            bool closedVideo = true;
            if (_hasVideoTexture)
                closedVideo = VolPluginInterface.VolCloseFile();
            bool freedGeom = VolPluginInterface.VolFreeGeomData();

            // Clean up temporary streaming file
            if (_isStreaming)
            {
                bool deleteTempFile = volFormat == VolEnums.VolFormat.BasisU && volFilePathType == VolEnums.PathType.URL;
                bool isTempFile = File.Exists(_fullGeomPath) && _fullGeomPath.StartsWith(Path.GetFullPath(Application.temporaryCachePath));
                if (isTempFile && deleteTempFile)
                {
                    try
                    {
                        File.Delete(_fullGeomPath);
                        Debug.Log("Deleted streaming temp file");
                    }
                    catch (Exception e)
                    {
                        Debug.LogWarning($"Could not delete streaming file: {e.Message}");
                    }
                }
                _isStreaming = false;
            }

            // Remove mesh
#if UNITY_EDITOR
            if (_meshFilter.sharedMesh != null)
                _meshFilter.sharedMesh.Clear();
#else
            if(_meshFilter.mesh != null) 
                _meshFilter.mesh.Clear();
#endif

            VolPluginInterface.ClearLoggingFunctions();

            return closedVideo && freedGeom;
        }

        private void BufferingPause()
        {
            if (!IsOpen || !IsPlaying)
                return;

            // Pause playback during buffering
            if (audioOn && _audioPlayerVols != null) _audioPlayerVols.Pause();
        }

        private void BufferingResume()
        {
            if (!IsOpen || !IsPlaying)
                return;

            if (audioOn && _audioPlayerVols != null) _audioPlayerVols.Play();
        }

        private IEnumerator PlayCoroutine()
        {

            if (audioOn && _audioPlayerVideo != null)
            {
                _audioPlayerVideo.Prepare();
                yield return new WaitUntil(() => _audioPlayerVideo.isPrepared);
                _audioPlayerVideo.Play();

            }
            if (audioOn && _audioPlayerVols != null) _audioPlayerVols.Play();

            IsPlaying = true;
        }

        /// <summary>
        /// Play the vologram
        /// </summary>
        public void Play()
        {
            if (!IsOpen)
                return;

            StartCoroutine(PlayCoroutine());
        }

        /// <summary>
        /// Pauses the vologram
        /// </summary>
        public void Pause()
        {
            if (!IsOpen)
                return;

            IsPlaying = false;

            if (audioOn && _audioPlayerVideo != null) _audioPlayerVideo.Pause();
            if (audioOn && _audioPlayerVols != null) _audioPlayerVols.Pause();
        }

        public void Stop()
        {
            if (!IsOpen)
                return;

            IsPlaying = false;
            //IsOpen = false;

            if (audioOn && _audioPlayerVideo != null) _audioPlayerVideo.Stop();
            if (audioOn && _audioPlayerVols != null) _audioPlayerVols.Stop();

            //bool closedVideo = true;
            //if (_hasVideoTexture)
            //    closedVideo = VolPluginInterface.VolCloseFile();
            //bool freedGeom = VolPluginInterface.VolFreeGeomData();
        }


        /// <summary>
        /// Closes the vologram and re-opens it
        /// </summary>
        /// <returns>True if successful</returns>
        public bool Restart()
        {
            if (!IsOpen)
                return false;

            Stop();

            if (_hasVideoTexture)
            {
                bool openedVideo = VolPluginInterface.VolOpenFile(_fullVideoPath);
                if (!openedVideo)
                {
                    IsOpen = false;
                    return false;
                }
            }

            // Handle a special case where we need a first frame after a restart and it is not in the buffer anymore.
            if (_isStreaming && streamingMode == VolEnums.StreamingMode.Buffer && !VolPluginInterface.VolIsFrameAvailableInBuffer(0))
                _streamingSession.RestartFromStart();

            _currentlyLoadedFrameIndex = -1;
            _animationAccumulatedSeconds = 0f;

            // For looping we call play in the Update() function
            if (playOnStart)
                Play();
            else
                _isSeek = true;
            
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
            if (desiredFrameIndex >= _numFrames)
            {
                if (isLooping)
                {
                    Restart();
                }
                return;
            }
            // Always skip video frames to desired frame.
            if (volFormat == VolEnums.VolFormat.Video)
                ReadVideoFrame(_currentlyLoadedFrameIndex, desiredFrameIndex);
            else
                ReadTextureFrame(_currentlyLoadedFrameIndex, desiredFrameIndex);
            ReadGeomFrame(desiredFrameIndex);
        }

        /// <summary>
        /// Mute or unmute the vologram's audio
        /// </summary>
        /// <param name="mute">Value for mute</param>
        public void SetMute(bool mute)
        {
            if (audioOn && _audioPlayerVideo != null) _audioPlayerVideo.SetDirectAudioMute(0, mute);
            if (audioOn && _audioPlayerVols != null) _audioPlayerVols.mute = mute;

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
        /// Read a desired video texture frame
        /// </summary>
        /// <param name="currentFrameIndex">The frame we last played.</param>
        /// <param name="desiredFrameIndex">The frame we want to retrieve and upload to the current texture.</param>
        private void ReadVideoFrame(int currentFrameIndex, int desiredFrameIndex)
        {
            if (desiredFrameIndex >= _numFrames || currentFrameIndex >= desiredFrameIndex) { return; }

            // Always skip ahead to desired frame. (This is a workaround until we get better video decoder seek behaviour).
            for (int videoFrameIndex = _currentlyLoadedFrameIndex; videoFrameIndex < desiredFrameIndex - 1; videoFrameIndex++)
            {
                _colorPtr = VolPluginInterface.VolReadNextVideoFrame(false);
            }
            // This is the frame we want, and we vertically flip this too.
            _colorPtr = VolPluginInterface.VolReadNextVideoFrame(true);
            if(_voloTexture)
            { // Upload only the texture from the desired frame to the GPU via Unity.
                _voloTexture.LoadRawTextureData(_colorPtr, (int)VolPluginInterface.VolGetFrameSize());
                _voloTexture.Apply();
#if UNITY_EDITOR
                _meshRenderer.sharedMaterial.SetTexture(_textureId, _voloTexture);
#else
                _meshRenderer.material.SetTexture(_textureId, _voloTexture);
#endif
            }
        }

        /// <summary>
        /// Read a desired Basis texture frame
        /// </summary>
        /// <param name="currentFrameIndex">The frame we last played.</param>
        /// <param name="desiredFrameIndex">The frame we want to retrieve and upload to the current texture.</param>
        private void ReadTextureFrame(int currentFrameIndex, int desiredFrameIndex)
        {
#if UNITY_ANDROID
        // TextureFormat.ETC_RGB4 = cTFETC1_RGB = 0;
        // TextureFormat.ETC2_RGBA8 = cTFETC2_RGBA = 1;
        int textureFormat = 0;
#elif UNITY_IOS
        // TextureFormat.ASTC_4x4 == cTFASTC_4x4_RGBA = 10;
        int textureFormat = 10;
#else
            // TextureFormat.DXT1 == cTFBC1_RGB = 2;
            // TextureFormat.DXT5 == cTFBC3_RGBA = 3,
            int textureFormat = 2;
#endif
            _colorPtr = VolPluginInterface.VolReadNextTextureFrame(textureFormat);
            if (_voloTexture && _colorPtr != IntPtr.Zero)
            { // Upload only the texture from the desired frame to the GPU via Unity.
                _voloTexture.LoadRawTextureData(_colorPtr, (int)VolPluginInterface.VolGetTextureSize());
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
        private bool ReadGeomFrame(int frame)
        {
            if (frame >= _numFrames) { return false; }

            bool isKeyframe = VolPluginInterface.VolGeomIsKeyframe(frame);
            if (_isStreaming && streamingMode == VolEnums.StreamingMode.Buffer)
            {
                if (!VolPluginInterface.VolReadFrameStreaming(frame))
                {
                    Debug.LogError("Error loading geometry frame");
                    return false;
                }
            }
            else 
            {
                string sequenceFile;

                if (volFormat == VolEnums.VolFormat.Video)
                    sequenceFile = Path.Combine(_fullGeomPath, "sequence_0.vols");
                else
                    sequenceFile = _fullGeomPath;

                if (!VolPluginInterface.VolGeomReadFrame(sequenceFile, frame))
                {
                    Debug.LogError("Error loading geometry frame");
                    return false;
                }
            }

            _geometryData = VolPluginInterface.VolGeomGetPtrData();

            if (_geometryData.blockDataSize == 0)
                return false;

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
                    .Slice((int)_geometryData.normalOffset, _geometryData.normalSize).SliceConvert<Vector3>();
#if UNITY_EDITOR
                _meshFilter.sharedMesh.SetNormals(normalsSlice.ToArray());
#else
            _meshFilter.mesh.SetNormals(normalsSlice.ToArray());
#endif
            }
            // TODO(Anton) - Patrick please check this, I was hoping to skip some updates here
            isKeyframe = true; // HACK(Anton) ... but it didn't work.

            if (isKeyframe && _geometryData.indicesSize > 0)
            {
                NativeSlice<ushort> indicesSlice =
                    nativeMeshData.Slice((int)_geometryData.indicesOffset, _geometryData.indicesSize).SliceConvert<ushort>();
                _keyShortIndices = indicesSlice.ToArray();
            }

            if (isKeyframe)
            {
#if UNITY_EDITOR
                _meshFilter.sharedMesh.SetIndices(_keyShortIndices, MeshTopology.Triangles, 0);
#else
            _meshFilter.mesh.SetIndices(_keyShortIndices, MeshTopology.Triangles, 0);
#endif
            }

            if (isKeyframe && _geometryData.uvSize > 0)
            {
                NativeSlice<Vector2> uvSlice = nativeMeshData.Slice((int)_geometryData.uvOffset, _geometryData.uvSize)
                    .SliceConvert<Vector2>();
                _keyUvs = uvSlice.ToArray();

                // Need to flip UVs becasue flipping compressed textuer is hard. 
                // TODO(Jan): hide it inside pluggin and update format in the future to avoid doingthis. 
                // Check if this is needed on all platforms
                if (volFormat == VolEnums.VolFormat.BasisU)
                {
                    Vector2 s = new Vector2(1, -1);
                    for (int i = 0; i < _keyUvs.Length; i++)    //ITERATE ALL VALUES IN VECTORS
                    {
                        _keyUvs[i] = Vector3.Scale(_keyUvs[i], s);
                    }
                }
            }

#if UNITY_EDITOR
            if (isKeyframe) { _meshFilter.sharedMesh.SetUVs(0, _keyUvs); } // TODO(Anton) check this if statement makes sense.
            _meshFilter.sharedMesh.RecalculateBounds();
            _meshFilter.sharedMesh.MarkModified();
#else
        if (isKeyframe) { _meshFilter.mesh.SetUVs(0, _keyUvs); } // TODO(Anton) check this if statement makes sense.
        _meshFilter.mesh.RecalculateBounds();
        _meshFilter.mesh.MarkModified();
#endif

            nativeMeshData.Dispose();
            return true;
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

        private void AudioVideoPlayerOnLoopPointReached(VideoPlayer source)
        {
            Restart();
        }

        private void AudioVideoPlayerOnFrameReady(VideoPlayer source, long frameidx)
        {

        }
    }
}