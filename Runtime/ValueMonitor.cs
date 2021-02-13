using System;
using System.Collections.Generic;
using System.Reflection;

namespace PixelsForGlory.RayTracing
{
    class ValueMonitor
    {
        private class MonitorValue
        {
            public object obj;
            public PropertyInfo propertyInfo;
            public FieldInfo FieldInfo;
            public object lastValue;
        }

        private List<MonitorValue> _values = new List<MonitorValue>();

        public void AddProperty(object obj, Type type, string propertyName, object lastValue)
        {
            _values.Add(new MonitorValue { obj = obj, propertyInfo = type.GetProperty(propertyName), lastValue = lastValue });
        }

        public void AddField(object obj, Type type, string fieldName, object lastValue)
        {
            _values.Add(new MonitorValue { obj = obj, FieldInfo = type.GetField(fieldName), lastValue = lastValue });
        }

        public bool CheckForUpdates()
        {
            bool update = false;
            
            // Make sure to run over all values just in case there are multiple updates
            for (int i = 0; i < _values.Count; ++i)
            {
                object value;
                if(_values[i].propertyInfo != null)
                {
                    value = _values[i].propertyInfo.GetValue(_values[i].obj);
                }
                else
                {
                    value = _values[i].FieldInfo.GetValue(_values[i].obj);
                }

                if (value != _values[i].lastValue)
                {
                    update = true;
                }
                _values[i].lastValue = value;
            }

            return update;
        }
    }
}
