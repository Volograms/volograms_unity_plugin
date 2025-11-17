using UnityEngine;
using UnityEngine.Networking;
using System.IO;

public class FileStreamingDownloadHandler : DownloadHandlerScript
{

    public int contentLength { get { return _received > _contentLength ? _received : _contentLength; } }

    private FileStream _fileStream;
    private int _contentLength;
    private int _received;
   

    public FileStreamingDownloadHandler(string filePath) : base(new byte[1024*1024])
    {
        const int bufferSize = 1024 * 8;
        string directory = Path.GetDirectoryName(filePath);
        _contentLength = -1;
        _received = 0;

        if (!Directory.Exists(directory)) Directory.CreateDirectory(directory);

        _fileStream = new FileStream(filePath, FileMode.Create, FileAccess.Write, FileShare.Read, bufferSize, useAsync: false);
    }

    protected override float GetProgress()
    {
        return contentLength <= 0 ? 0 : Mathf.Clamp01((float)_received / (float)contentLength);
    }

    protected override void ReceiveContentLengthHeader(ulong contentLength)
    {
        _contentLength = (int)contentLength;
    }

    protected override bool ReceiveData(byte[] data, int dataLength)
    {
        if (data == null || data.Length == 0) return false;

        ;
        _fileStream.Write(data, 0, dataLength);
        _fileStream.Flush(flushToDisk: true); // Force OS to write immediately
        _received += dataLength;

        return true;
    }

    protected override void CompleteContent()
    {
        CloseStream();
    }

    public new void Dispose()
    {
        CloseStream();
        base.Dispose();
    }

    private void CloseStream()
    {
        if (_fileStream != null)
        {
            _fileStream.Dispose();
            _fileStream = null;
        }
    }

}