// <copyright file=VolPostBuild company=Volograms>
// Copyright (c) 2022 All Rights Reserved
// </copyright>
// <author>Patrick Geoghegan</author>
// <date>18/02/22</date>
// <summary>Utility and convenience functions for post Unity builds</summary>

using UnityEditor;
using UnityEditor.Callbacks;
#if UNITY_IOS
using UnityEditor.iOS.Xcode;
#endif // UNITY_IOS

/// <summary>
/// Class for post Unity-build functions
/// </summary>
public class VolPostBuild
{
#if UNITY_IOS
    private const string VideoToolbox = "VideoToolbox.framework";
    private const string LibZ = "libz.tbd";
    private const string LibBz2 = "libbz2.tbd";
    
    /// <summary>
    /// Function that is called after a successful Unity build
    /// </summary>
    /// <param name="target">Platform for which the build was done</param>
    /// <param name="pathToBuiltProject">Path to the output build</param>
    [PostProcessBuild]
    private static void OnPostBuild(BuildTarget target, string pathToBuiltProject)
    {
        if (target == BuildTarget.iOS)
        {
            PBXProject project = new PBXProject();
            string projectPath = PBXProject.GetPBXProjectPath(pathToBuiltProject);
            project.ReadFromFile(projectPath);
            string guid = project.GetUnityFrameworkTargetGuid();
            
            AddFrameworks(project, guid, VideoToolbox, LibZ, LibBz2);
            
            project.WriteToFile(projectPath);
        }
    }

    /// <summary>
    /// Adds Frameworks and Libs to Xcode project after iOS or Mac build
    /// </summary>
    /// <param name="project">Xcode project struct</param>
    /// <param name="guid">ID of the UnityFramework target</param>
    /// <param name="frameworks">List of Frameworks or libs to be added</param>
    private static void AddFrameworks(PBXProject project, string guid, params string[] frameworks)
    {
        foreach (string framework in frameworks)
        {
            if (!project.ContainsFramework(guid, framework))
            {
                project.AddFrameworkToProject(guid, framework, false);
            }
        }
    }
#endif // UNITY_IOS
}
