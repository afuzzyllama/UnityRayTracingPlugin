using System;
using System.Runtime.InteropServices;

using AOT;

using UnityEngine;

namespace PixelsForGlory
{
    [ExecuteInEditMode]
    class DebugFromPlugin : MonoBehaviour
    {
        const string PLUGIN_NAME = "RayTracingPlugin";

        [DllImport(PLUGIN_NAME, CallingConvention = CallingConvention.Cdecl)]
        public static extern void RegisterDebugCallback(DebugCallback callback);

        //Create string param callback delegate
        public delegate void DebugCallback(IntPtr request, int level, int size);

        [MonoPInvokeCallback(typeof(DebugCallback))]
        public static void OnDebugCallback(IntPtr request, int level, int size)
        {
            //Ptr to string
            string debug_string = Marshal.PtrToStringAnsi(request, size);

            switch (level)
            {
                case 0:
                    UnityEngine.Debug.Log($"{PLUGIN_NAME}: {debug_string}");
                    break;

                case 1:
                    UnityEngine.Debug.LogWarning($"{PLUGIN_NAME}: {debug_string}");
                    break;

                case 2:
                    UnityEngine.Debug.LogError($"{PLUGIN_NAME}: {debug_string}");
                    break;

                default:
                    UnityEngine.Debug.LogError($"Unsupported log level {level}");
                    break;
            }
        }

        private void Awake()
        {
            RegisterDebugCallback(OnDebugCallback);
        }
    }
}