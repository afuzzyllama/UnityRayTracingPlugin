using UnityEngine;

namespace Assets
{
    class RayTracingSuckCode : MonoBehaviour
    {
        void Start()
        {
            PixelsForGlory.RayTracingPlugin.BuildTlas();
            PixelsForGlory.RayTracingPlugin.Prepare(Screen.width, Screen.height);
        }

    }
}
