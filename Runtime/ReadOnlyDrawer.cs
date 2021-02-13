using UnityEditor;
using UnityEngine;

namespace PixelsForGlory.RayTracing
{
    [CustomPropertyDrawer(typeof(ReadOnlyAttribute))]
    public class ReadOnlyDrawer : PropertyDrawer
    {
        public override void OnGUI(Rect position, SerializedProperty property, GUIContent label)
        {
            string val;
            switch (property.propertyType)
            {
                case SerializedPropertyType.Integer:
                    val = property.intValue.ToString();
                    break;

                case SerializedPropertyType.Float:
                    val = property.floatValue.ToString();
                    break;

                case SerializedPropertyType.String:
                    val = property.stringValue.ToString();
                    break;

                case SerializedPropertyType.Boolean:
                    val = property.boolValue.ToString();
                    break;

                default:
                    val = "Unsupported type";
                    break;

            }

            EditorGUI.LabelField(position, string.Format("{0}: {1}", label.text, val));
        }
    }
}