using UnityEditor;
using UnityEngine;

namespace PixelsForGlory.RayTracing
{
    public class RayTracingEditorWindow : EditorWindow
    {
        [MenuItem("Window/Analysis/Pixels for Glory/Ray Tracing")]
        public static void ShowWindow()
        {
            var window = EditorWindow.GetWindow(typeof(RayTracingEditorWindow));
            window.titleContent.text = "Ray Tracing Data";
        }

        private void OnInspectorUpdate()
        {
            Repaint();
        }

        void OnGUI()
        {
            var stats = PixelsForGlory.RayTracing.RayTracingPlugin.GetRayTracerStatistics();

            GUILayout.Label("Statistics", EditorStyles.boldLabel);
            EditorGUILayout.LabelField($"Registered Lights: {stats.RegisteredLights}");
            EditorGUILayout.LabelField($"Registered Shared Meshes: {stats.RegisteredSharedMeshes}");
            EditorGUILayout.LabelField($"Registered Mesh Instances: {stats.RegisteredMeshInstances}");
            EditorGUILayout.LabelField($"Registered Materials: {stats.RegisteredMaterials}");
            EditorGUILayout.LabelField($"Registered Textures: {stats.RegisteredTextures}");
            EditorGUILayout.LabelField($"Registered Render Targets: {stats.RegisteredRenderTargets}");
            EditorGUILayout.Space();
            EditorGUILayout.LabelField($"Descriptor Set Count: {stats.DescriptorSetCount}");
            EditorGUILayout.LabelField($"Acceleration Stucture Count: {stats.AccelerationStuctureCount}");
            EditorGUILayout.LabelField($"Uniform Buffer Count: {stats.UniformBufferCount}");
            EditorGUILayout.LabelField($"Storage Image Count: {stats.StorageImageCount}");
            EditorGUILayout.LabelField($"Storage Buffer Count: {stats.StorageBufferCount}");
            EditorGUILayout.LabelField($"Combined Image Sampler Count: {stats.CombinedImageSamplerCount}");
        }
    }
}