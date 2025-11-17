using System;
using Unity.Android.Gradle.Manifest;
using UnityEngine;
using UnityEngine.Networking;

public class BufferStreamingDownloadHandler : DownloadHandlerScript
{
    public int contentLength
    {
        get { return (int)(_totalBytesReceived > _contentLength ? _totalBytesReceived : _contentLength); }
    }

    private long _totalBytesReceived = 0;
    private int _contentLength;
    private bool _isCancelled = false;
    private Action<byte[], int> _onDataReceived;

    public long TotalBytesReceived => _totalBytesReceived;
    public bool IsCancelled => _isCancelled;

    public BufferStreamingDownloadHandler(Action<byte[], int> onDataReceived) : base(new byte[512 * 1024]) // 512KB chunks
    {
        _onDataReceived = onDataReceived;
    }

    public void Cancel()
    {
        _isCancelled = true;
    }

    protected override float GetProgress()
    {
        return contentLength <= 0 ? 0 : Mathf.Clamp01(((float)_totalBytesReceived / (float)contentLength)% contentLength);
    }

    protected override void ReceiveContentLengthHeader(ulong contentLength)
    {
        _contentLength = (int)contentLength;
    }

    protected override bool ReceiveData(byte[] data, int dataLength)
    {
        if (_isCancelled || data == null || dataLength == 0)
            return false;

        try
        {
            _onDataReceived?.Invoke(data, dataLength);
            _totalBytesReceived += dataLength;
            return true;
        }
        catch (Exception e)
        {
            Debug.LogError($"Error processing buffer data: {e.Message}");
            return false;
        }
    }
}