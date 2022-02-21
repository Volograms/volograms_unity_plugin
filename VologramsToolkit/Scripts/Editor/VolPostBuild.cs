// <copyright file=VolPostBuild company=Volograms>
// Copyright (c) 2022 All Rights Reserved
// </copyright>
// <author>Patrick Geoghegan</author>
// <date>18/02/22</date>
// <summary>Utility and convenience functions for post Unity builds</summary>

using UnityEditor;
using UnityEditor.Callbacks;
using UnityEditor.iOS.Xcode;

public class VolPostBuild
{
    private const string VideoToolbox = "VideoToolbox.framework";
    private const string LibZ = "libz.tbd";
    private const string LibBz2 = "libbz2.tbd";
    
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
}
