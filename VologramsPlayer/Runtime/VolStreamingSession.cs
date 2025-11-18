// VolStreamingSession.cs
// Instance session that can stream to file OR to your native circular buffer via VolPluginInterface.

using System;
using System.Collections;
using System.Reflection;
using System.Runtime.InteropServices;
using Unity.IO.LowLevel.Unsafe;
using UnityEngine;
using UnityEngine.Networking;
using Volograms;

public sealed class VolStreamingSession
{
    public enum Target { File, Buffer }

    private readonly MonoBehaviour _runner;

    // Common
    private UnityWebRequest _active = null;
    private bool _paused;
    private bool _stopped;
    private FileStreamingDownloadHandler _fileHandler;

    // File target
    private string _destPath;

    // Buffer/Range target
    private bool _loop;
    private bool _seeking;
    private long _pos;
    private long _seekLocation;
    private long _fileSize;          // if known; last byte = _fileSize - 1
    private int  _windowBytes;       // per range window
    private long _downloadedBytes;
    private long _donwloadAdvanced;

    public bool IsRunning { get; private set; }
    public bool IsPaused => _paused;

    public VolStreamingSession(MonoBehaviour runner) { _runner = runner; }

    public void Stop()
    {
        _stopped = true;
        _paused = false;
        _seeking = false;
        // Abort download
        if (IsRunning)
        {
            if(!_stopped)
                _active?.Abort();
            _active?.Dispose();
        }
        // Dispose handler and close file
        _fileHandler?.Dispose();
        _fileHandler = null;
        IsRunning = false;
    }

    public void PauseDownload()  { _paused = true; }
    public void ResumeDownload() { _paused = false; }
    public void SetLoopStreaming(bool enable) { _loop = enable; }
    public void RestartFromStart() 
    { 
        _seekLocation = VolPluginInterface.VolGetFrameBodyStart();
        _seeking = true; 
        _paused = false; 
    }

    // FILE MODE: single GET streamed into a local file using your StreamingDownloadHandler.
    public IEnumerator RunFileStreaming(
        string sourceUrl,
        string destPath,
        Action<bool> onHeaderOpen,
        Action<float> onProgress = null,
        Action<string> onError = null)
    {
        _destPath = destPath;
        _stopped = false;
        _paused = false;
        IsRunning = true;

        try
        {
            using (_active = UnityWebRequest.Get(sourceUrl))
            {
                // Uses your existing file handler (FileShare.Read + Flush) for native reader compatibility.
                _fileHandler = new FileStreamingDownloadHandler(destPath);
                _active.downloadHandler = _fileHandler;

                var op = _active.SendWebRequest();
                bool isOpened = false;
                string headerPath = ""; // same as your current code

                while (!op.isDone)
                {
                    if (_stopped) { _active.Abort(); break; }

                    // Progress
                    onProgress?.Invoke(_active.downloadProgress);

                    // Open header when enough bytes are locally available
                    if (!isOpened && _active.downloadedBytes > 4L * 1024 * 1024)
                    {
                        isOpened = VolPluginInterface.VolGeomOpenFile(headerPath, destPath, false);
                        if (isOpened) onHeaderOpen?.Invoke(isOpened);
                    }

                    yield return null;
                }

                // Errors (corrected logical test)
                if (_active.result == UnityWebRequest.Result.ConnectionError ||
                    _active.result == UnityWebRequest.Result.ProtocolError)
                {
                    onError?.Invoke(_active.error);
                }
                else
                {
                    if (!isOpened)
                    {
                        isOpened = VolPluginInterface.VolGeomOpenFile(headerPath, destPath, false);
                        onHeaderOpen?.Invoke(isOpened);
                    }
                }
            }
        }
        finally
        {
            IsRunning = false;
            Stop();
        }
    }

    // BUFFER/RANGE MODE: sequential Range windows into a native circular buffer via VolPluginInterface.
    public IEnumerator RunBufferStreaming(
        string sourceUrl,
        bool loop,
        Action<bool> onHeaderOpen,
        Action<float> onProgress = null,
        Action<string> onError = null,
        long bufferSize = 50, // in MB
        float lookaheadSeconds = 2.0f // in seconds
        )
    {
        //_bodyStart = 0;
        long maxSize = bufferSize * 1024 * 1024;
        _fileSize = -1;           // pass -1 if unknown
        _windowBytes = 4 * 1024 * 1024;
        _loop = loop;
        _pos = 0;
        _stopped = false;
        _paused = false;
        _seeking = false;
        _seekLocation = 0;
        IsRunning = true;
        _downloadedBytes = 0;

        bool headerOpened = false;
        int safetyBufferHeadroom = 4 * 1024;

        try
        {
            // Get file size
            UnityWebRequest uwr = UnityWebRequest.Head(sourceUrl);
            yield return uwr.SendWebRequest();
            string size = uwr.GetResponseHeader("Content-Length");

            if (uwr.result == UnityWebRequest.Result.ConnectionError ||
                uwr.result == UnityWebRequest.Result.ProtocolError)
            {
                Debug.Log("Error While Getting Length: " + uwr.error);
                onError?.Invoke(uwr.error);
                Stop();
                yield break;
            }
            _fileSize = (size != "") ? Convert.ToInt64(size) : 0;


            // Initialize your streaming buffer and config in the native plugin.
            VolPluginInterface.VolInitStreamingConfig();
            VolPluginInterface.VolSetMaxBufferSize(maxSize);
            VolPluginInterface.VolSetLookaheadSeconds(lookaheadSeconds);
            if (!VolPluginInterface.VolCreateStreamingBuffer())
            {
                onError?.Invoke("Failed to create streaming buffer");
                Stop();
                yield break;
            }

            // Stream the file in small chunks to allow us to pause the download when our buffer is full
            while (!_stopped)
            {
                //while (_paused && !_stopped) { yield return null; }
                if (_stopped) break;

                // Handle seek
                if (_seeking)
                {
                    _pos = _seekLocation;

                    // Order is important: reset frame directory first, then update buffer state.
                    VolPluginInterface.VolResetFrameDirectory();
                    VolPluginInterface.VolUpdateBufferState();
                    _seeking = false;
                }

                // Check buffer space or wait for enough free space in buffer
                long usedSize = VolPluginInterface.VolGetUsedBufferSize();
                long freeSpace = maxSize - usedSize;

                if (_paused || freeSpace < (_windowBytes + safetyBufferHeadroom))
                {
                    _paused = true;

                    // Cleanm-up buffer and check if we have enough free space
                    if (VolPluginInterface.VolUpdateBufferState())
                    {
                        usedSize = VolPluginInterface.VolGetUsedBufferSize();
                        freeSpace = maxSize - usedSize;

                        // Resume if we have enough space
                        if (freeSpace >= (_windowBytes + safetyBufferHeadroom))
                            _paused = false;
                    }
                    if (_paused)
                    {
                        yield return null;
                        continue;
                    }
                }

                // Handle end of file and looping
                if (_pos >= _fileSize)
                {
                    if (!_loop)
                    {
                        // Reached end of file, pause downloading. Otherwise we will have to open a new request on reset or looping enabled.
                        PauseDownload();
                    }
                    _pos = VolPluginInterface.VolGetFrameBodyStart();
                    yield return null;
                    continue;
                }

                // Calculate download range
                long start = _pos;
                long end = _fileSize > 0 ? Math.Min(start + _windowBytes - 1, _fileSize) : start + _windowBytes - 1;

                // Start Range request
                using (_active = UnityWebRequest.Get(sourceUrl))
                {
                    _active.SetRequestHeader("Range", $"bytes={start}-{end}");
                    //Debug.Log($"Requesting bytes={start}-{end}");

                    // Feed chunks into native buffer via your handler callback.
                    var handler = new BufferStreamingDownloadHandler(OnBufferDataReceived);
                    _active.downloadHandler = handler;

                    var op = _active.SendWebRequest();
                    _donwloadAdvanced = 0;

                    while (!op.isDone)
                    {
                        if (_stopped || _seeking) { _active.Abort(); break; }

                        // Buffer backpressure from native side
                        //if (VolPluginInterface.VolIsDownloadBufferFull())
                        //{
                        //    _paused = true;
                        //    yield return new WaitForSeconds(0.5f);
                        //    continue;
                        //}
                        //if (_paused && VolPluginInterface.VolShouldResumeDownload(0, 30f))
                        //{
                        //    _paused = false;
                        //}

                        // Progress (per window)
                        onProgress?.Invoke(_active.downloadProgress);

                        // Header open once enough data is buffered to parse file info
                        if (!headerOpened && _downloadedBytes > 4L * 1024 * 1024)
                        {
                            headerOpened = VolPluginInterface.VolCreateStreamingFileInfo();
                            if (headerOpened) onHeaderOpen?.Invoke(headerOpened);
                        }

                        yield return null;
                    }

                    // Validate response code (206 preferred, 200 tolerated for servers without range)
                    var rc = _active.responseCode;
                    if (rc == 206 || rc == 200)
                    {
                        _pos = start + _donwloadAdvanced;

                        if (_fileSize > 0 && _pos > _fileSize)
                        {
                            if (_loop) _pos = VolPluginInterface.VolGetFrameBodyStart();
                            else { PauseDownload(); break; }
                        }
                    }
                    else if (rc == 416)
                    {
                        if (_loop) _pos = VolPluginInterface.VolGetFrameBodyStart();
                        else { PauseDownload(); break; }
                    }
                    else if (_active.result == UnityWebRequest.Result.ConnectionError ||
                             _active.result == UnityWebRequest.Result.ProtocolError)
                    {
                        onError?.Invoke(_active.error);
                        // Backoff or break; here we break the loop to report up.
                        break;
                    }
                }

                yield return null;
            }
        }
        finally
        {
            IsRunning = false;
            Stop();
        }
    }

    // Matches your helper’s pattern: deliver each chunk to native VolAddDataToBuffer.
    private void OnBufferDataReceived(byte[] data, int dataLength)
    {
        if (_paused || _stopped || _seeking || dataLength <= 0) return;

        // Fast path: pin just for the duration of the P/Invoke call to avoid an extra heap copy.
        var handle = GCHandle.Alloc(data, GCHandleType.Pinned);
        try
        {
            IntPtr ptr = handle.AddrOfPinnedObject();
            bool ok = VolPluginInterface.VolAddDataToBuffer(ptr, dataLength);
            if (!ok) _paused = true; // let the loop's resume policy handle it
            _downloadedBytes += dataLength;
            _donwloadAdvanced += dataLength;
        }
        finally
        {
            handle.Free();
        }
    }
}
