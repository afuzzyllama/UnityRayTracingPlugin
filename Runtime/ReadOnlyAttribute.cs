using UnityEngine;

namespace PixelsForGlory.RayTracing
{
    public class ReadOnlyAttribute : PropertyAttribute
    {
        /// <summary>
        /// Writes a warning if the value of this field is null
        /// </summary>
        public readonly bool warningIfNull = false;

        public ReadOnlyAttribute(bool _warningIfNull = false)
        {
            warningIfNull = _warningIfNull;
        }
    }
}