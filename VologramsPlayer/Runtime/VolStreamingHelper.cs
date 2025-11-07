// <copyright file=VolStreamingHelper company=Volograms>
// Copyright (c) 2025 All Rights Reserved
// </copyright>
// <author>Jan Ondrej</author>
// <date>06/11/25</date>
// <summary></summary>

using System;
using System.Collections;
using System.IO;
using UnityEditor.PackageManager.Requests;
using UnityEngine;
using UnityEngine.Networking;


namespace Volograms
{
    public static class VolStreamingHelper
    {
        private static UnityWebRequest _activeDownloadRequest;
        private static StreamingDownloadHandler _downloadHandler;

        public static void Stop()
        {
            _downloadHandler?.Dispose();
            _activeDownloadRequest?.Dispose();
            _activeDownloadRequest = null;
            _downloadHandler = null;

        }

        public static IEnumerator StreamToFile(
            string sourceUrl,
            string destPath,
            Action<bool> onHeaderOpen,
            Action<float> onProgress = null,
            Action<string> onError = null
            )
        {
            try
            {
                using (_activeDownloadRequest = UnityWebRequest.Get(sourceUrl))
                {
                    _downloadHandler = new StreamingDownloadHandler(destPath);
                    _activeDownloadRequest.downloadHandler = _downloadHandler;
                    _activeDownloadRequest.SendWebRequest();

                    bool isOpened = false;
                    string header = "";

                    while (!_activeDownloadRequest.isDone)
                    {
                        float progress = _activeDownloadRequest.downloadProgress;
                        onProgress?.Invoke(progress);
                        //Debug.Log("Download progress: " + progress);

                        if (!isOpened && _activeDownloadRequest.downloadedBytes > 1024 * 1024 * 4)
                        {
                            // Open file 
                            isOpened = VolPluginInterface.VolGeomOpenFile(header, destPath, false);
                            if (isOpened)
                            {
                                onHeaderOpen?.Invoke(isOpened);
                            }
                        }
                        yield return null; // Continue next frame
                    }
                    bool error = false;
                    if (_activeDownloadRequest.result != UnityWebRequest.Result.ConnectionError || _activeDownloadRequest.result != UnityWebRequest.Result.ProtocolError)
                    {
                        error = true;
                        onError?.Invoke(_activeDownloadRequest.error);
                    }

                    // Final update
                    if (!error && !isOpened)
                    {
                        isOpened = VolPluginInterface.VolGeomOpenFile(header, destPath, false);
                        onHeaderOpen?.Invoke(isOpened);
                    }
                    Debug.Log("Download is done.");
                }
            }
            finally
            {
                Stop();
            }
        }

        public static IEnumerator StreamToBuffer(
            string sourceUrl, 
            Action<bool> onHeaderOpen,
            Action<float> onProgress = null,
            Action<string> onError = null
            )
        {
            // TODO: Implement circular buffer version
            onError?.Invoke("Function not implemented");
            yield break;
        }
    }
}