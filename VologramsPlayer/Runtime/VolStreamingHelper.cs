// <copyright file=VolStreamingHelper company=Volograms>
// Copyright (c) 2025 All Rights Reserved
// </copyright>
// <author>Jan Ondrej</author>
// <date>06/11/25</date>
// <summary></summary>

using UnityEngine;
using System;
using System.Collections;
using UnityEngine.Networking;
using System.IO;


namespace Volograms
{
    public static class VolStreamingHelper
    {
        public static IEnumerator StreamToFile(
            string sourceUrl,
            string destPath,
            Action onHeaderOpen,
            Action<float> onProgress = null
            )
        {
            using (var request = UnityWebRequest.Get(sourceUrl))
            {
                request.downloadHandler = new StreamingDownloadHandler(destPath);
                request.SendWebRequest();

                bool isOpened = false;
                string header = "";

                while (!request.isDone)
                {
                    float progress = request.downloadProgress;
                    onProgress?.Invoke(progress);
                    //Debug.Log("Download progress: " + progress);

                    if (!isOpened && request.downloadedBytes > 1024*1024*4)
                    {
                        // Open file 
                        isOpened = VolPluginInterface.VolGeomOpenFile(header, destPath, false);
                        if (isOpened)
                        {
                            Debug.Log("Streame opened: " + isOpened);
                            onHeaderOpen?.Invoke();
                        }
                    }
                    yield return null; // Continue next frame
                }

                // Final update
                if (!isOpened)
                {
                    VolPluginInterface.VolGeomOpenFile(header, destPath, false);
                    onHeaderOpen?.Invoke();
                }
                Debug.Log("Download is done.");
                //VolPluginInterface.VolGeomUpdateFramesDirectory(destPath, VolPluginInterface.VolGeomGetFrameCount()-1);
                //onFramesAvailable?.Invoke(VolPluginInterface.VolGeomGetFrameCount());
            }
        }

        public static IEnumerator StreamToBuffer(string sourceUrl, Action onHeaderOpen)
        {
            // TODO: Implement circular buffer version
            yield break;
        }
    }
}