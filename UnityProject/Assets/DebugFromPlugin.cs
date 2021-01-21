using System;
using System.Runtime.InteropServices;

using AOT;

using UnityEngine;

namespace PixelsForGlory
{
    class DebugFromPlugin : MonoBehaviour
    {
        [DllImport("RayTracingPlugin", CallingConvention = CallingConvention.Cdecl)]
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
                    UnityEngine.Debug.Log(debug_string);
                    break;

                case 1:
                    UnityEngine.Debug.LogWarning(debug_string);
                    break;

                case 2:
                    UnityEngine.Debug.LogError(debug_string);
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