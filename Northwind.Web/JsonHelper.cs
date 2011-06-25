using System;
using System.Net;
using System.IO;
using System.Runtime.Serialization.Json;
using System.Collections.Generic;
using System.Text;


	public static class JsonHelper
	{
		public static string JSON_stringify(this object objectToSerialize)
		{
			using (MemoryStream ms = new MemoryStream())
			{
				DataContractJsonSerializer serializer =
						new DataContractJsonSerializer(objectToSerialize.GetType());
				serializer.WriteObject(ms, objectToSerialize);
				ms.Position = 0;

				using (StreamReader reader = new StreamReader(ms))
				{
					return reader.ReadToEnd();
				}
			}
		}

		public static T Deserialize<T>(Stream s)
		{
			DataContractJsonSerializer serializer =
					new DataContractJsonSerializer(typeof(T));
			return (T)serializer.ReadObject(s);
		}

		public static T ParseJSON<T>(this string json)
		{
			using (MemoryStream ms = new MemoryStream(Encoding.Unicode.GetBytes(json)))
			{
				DataContractJsonSerializer serializer =
						new DataContractJsonSerializer(typeof(T));
				return (T)serializer.ReadObject(ms);
			}
		}
	}
