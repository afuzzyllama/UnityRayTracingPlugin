//using System;
//using System.Runtime.InteropServices;

//using AOT;

//using UnityEngine;
//#if UNITY_EDITOR
//using UnityEditor;
//#endif

//namespace PixelsForGlory
//{

//#if UNITY_EDITOR
//    // Had to do it this way to prevent MonoPInvokeCallback from hanging on Application.Reload

//    // ensure class initializer is called whenever scripts recompile
//    [InitializeOnLoad]
//    public static class DebugFromPluginPlayModeStateChanged
//    {
//        // register an event handler when the class is initialized
//        static DebugFromPluginPlayModeStateChanged()
//        {
//            EditorApplication.playModeStateChanged += ManageDebugCallbackPlayMode;

//            AssemblyReloadEvents.afterAssemblyReload += ManageDebugCallbackAfterAssemblyReload;
//            AssemblyReloadEvents.beforeAssemblyReload += ManageDebugCallbackBeforeAssemblyReload;

//        }

//        private static void ManageDebugCallbackAfterAssemblyReload()
//        {
//            Debug.Log("ManageDebugCallbackAfterAssemblyReload");
//            DebugFromPlugin.Register();
//        }

//        private static void ManageDebugCallbackBeforeAssemblyReload()
//        {
//            Debug.Log("ManageDebugCallbackBeforeAssemblyReload");
//            DebugFromPlugin.Unregister();
//        }

//        private static void ManageDebugCallbackPlayMode(PlayModeStateChange state)
//        {
//            if (state == PlayModeStateChange.EnteredEditMode || state == PlayModeStateChange.EnteredPlayMode)
//            {
//                Debug.Log("ManageDebugCallbackPlayMode Entered");
//                DebugFromPlugin.Register();
//            }

//            if (state == PlayModeStateChange.ExitingEditMode || state == PlayModeStateChange.ExitingPlayMode)
//            {
//                Debug.Log("ManageDebugCallbackPlayMode Exiting");
//                DebugFromPlugin.Unregister();
//            }
//        }
//    }
//#endif

//    class DebugFromPlugin 
//    {
//        public const string PLUGIN_NAME = "RayTracingPlugin";

//        private static bool Registered = false;

//        public static void Register()
//        {
//            if (!Registered)
//            {
//                RegisterDebugCallback(OnDebugCallback);
//                Registered = true;
//            }
//        }

//        public static void Unregister()
//        {
//            if (Registered)
//            {
//                ClearDebugCallback();
//                Registered = false;
//            }
//        }

//        [DllImport(PLUGIN_NAME)]
//        private static extern void RegisterDebugCallback(DebugCallback callback);

//        [DllImport(PLUGIN_NAME)]
//        private static extern void ClearDebugCallback();

//        //Create string param callback delegate
//        public delegate void DebugCallback(IntPtr request, int level, int size);

//        [MonoPInvokeCallback(typeof(DebugCallback))]
//        public static void OnDebugCallback(IntPtr request, int level, int size)
//        {
//            //Ptr to string
//            string debug_string = Marshal.PtrToStringAnsi(request, size);

//            switch (level)
//            {
//                case 0:
//                    UnityEngine.Debug.Log($"{PLUGIN_NAME}: {debug_string}");
//                    break;

//                case 1:
//                    UnityEngine.Debug.LogWarning($"{PLUGIN_NAME}: {debug_string}");
//                    break;

//                case 2:
//                    UnityEngine.Debug.LogError($"{PLUGIN_NAME}: {debug_string}");
//                    break;

//                default:
//                    UnityEngine.Debug.LogError($"Unsupported log level {level}");
//                    break;
//            }
//        }
//    }
//}