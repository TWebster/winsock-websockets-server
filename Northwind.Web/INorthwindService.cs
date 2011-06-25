using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ServiceModel;
using System.Runtime.Serialization;
using System.Reflection;
using System.ServiceModel.Web;

namespace Northwind
{
	/// <summary>
	/// This derived interface exists because it's important to add these 
	/// KnownTypes to the ServiceContract, so that svcutil (esp. for Silverlight)
	/// will auto-generate the classes (so as not to require referencing the 
	/// actual .NET class library (i.e. NorthwindLibrary) which uses LINQ to SQL.
	/// 1. Derive from IObjectService and include the ServiceKnownTypeAttribute(s).
	/// 2. Run the service which uses this ServiceContract, 
	/// 3. Run svcutil.exe
	/// 4. svcutil.exe will generate new INorthwindService interface code.
	/// 5. Use that instead and reference the svcutil.exe from a separate class library, which
	/// can now be referenced by both the service and the client.
	/// </summary>
	[ServiceContract]

	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.OrderCollection))]
	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.OrderCollection[]))]

	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.NorthwindObject))]
	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.NorthwindObject[]))]

	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.CustomerKey))]
	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.CustomerKey[]))]

	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.ProductKey))]
	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.ProductKey[]))]

	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.Product))]
	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.Product[]))]

	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.Customer))]
	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.Customer[]))]

	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind. Supplier))]
	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind. Supplier[]))]


	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.Category))]
	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.Category[]))]

	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.Employee))]
	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.Employee[]))]

	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.Order))]
	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.Order[]))]

	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.SL.Security.SecurityHandle))]
	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.SL.Security.SecurityHandle[]))]

	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.SL.Security.SecureString))]
	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.SL.Security.SecureString[]))]

	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.SL.Security.PermissionTypeEnum))]
	[System.ServiceModel.ServiceKnownTypeAttribute(typeof(Northwind.SL.Security.PermissionTypeEnum[]))]

	//[System.ServiceModel.ServiceKnownTypeAttribute(typeof(System.Object[]))]
	//[System.ServiceModel.ServiceKnownTypeAttribute(typeof(System.Object))]
	//[System.ServiceModel.ServiceKnownTypeAttribute(typeof(NorthwindObject))]
	//[System.ServiceModel.ServiceKnownTypeAttribute(typeof(NorthwindObject[]))]	
	public interface INorthwindService
	{			
		[OperationContract]
		[WebGet(UriTemplate = "/objects",
			RequestFormat = WebMessageFormat.Json,
			ResponseFormat = WebMessageFormat.Json,
			BodyStyle = WebMessageBodyStyle.Bare)]
		object[] GetObjects();





		/// <summary>
		/// Example that shows how send JSON to a WCF REST service.
		/// GET parameters can be optional when sent from jquery.
		/// However WCF will fail to send a Supplier object back, even though
		/// [DataContract], [DataMember], [KnownType] attributes have been provided.
		/// </summary>
		[OperationContract]
		[WebGet(UriTemplate = "/supplier/get?id={id}&co={co}&city={city}",
			RequestFormat = WebMessageFormat.Json,
			ResponseFormat = WebMessageFormat.Json,
			BodyStyle = WebMessageBodyStyle.Bare)]
		 string GetSupplier(string id, string co, string city);

		/// <summary>
		/// Example that shows how send JSON to a WCF REST service.
		/// GET parameters can be optional when sent from jquery.
		/// However WCF may fail to send a Supplier object back, even though
		/// [DataContract], [DataMember], [KnownType] attributes have been provided.
		/// An error may occur outside of your service's code where WCF fails silently.
		/// </summary>
		[OperationContract]
		[WebGet(UriTemplate = "/supplier/bad/get/?id={id}&co={co}&city={city}&json={json}",
			RequestFormat = WebMessageFormat.Json,
			ResponseFormat = WebMessageFormat.Json,
			BodyStyle = WebMessageBodyStyle.Bare)]
		Supplier GetSupplierBad(string id, string co, string city, string json);

		[OperationContract]
		[WebGet(UriTemplate = "/suppliers/get?url={url}&postal={postal}",
			RequestFormat = WebMessageFormat.Json,
			ResponseFormat = WebMessageFormat.Json,
			BodyStyle = WebMessageBodyStyle.Bare)]
		string GetSuppliers(string url, string postal);
		

		/// <summary>
		/// To call from javascript after GetSupplier. 
		/// WCF will fail on HTTP POST and the request will never make it 
		/// to the WCF service's method (i.e. the debugger will never step into the method)
		/// </summary>
		/// <param name="s"></param>
		/// <returns></returns>
		[OperationContract]
		[WebInvoke(Method = "POST",
			UriTemplate = "/supplier/bad/post",
			RequestFormat = WebMessageFormat.Json,
			ResponseFormat = WebMessageFormat.Json,
			BodyStyle = WebMessageBodyStyle.Wrapped)]
		Supplier PostSupplierBad(Supplier s);


		/// <summary>
		/// returns the Supplier (that was POSTed as JSON string).
		/// </summary>
		/// <param name="json"></param>
		/// <returns></returns>
		[OperationContract]
		[WebGet(UriTemplate = "/supplier/post?json={json}",
			RequestFormat = WebMessageFormat.Json,
			ResponseFormat = WebMessageFormat.Json,
			BodyStyle = WebMessageBodyStyle.Wrapped)]
		Supplier PostSupplier(string json);

		/// <summary>
		/// POST just doesn't seem to work. At all.
		/// </summary>
		/// <param name="s"></param>
		/// <returns></returns>
		[OperationContract]
		[WebInvoke(Method = "POST",
			UriTemplate = "/post",
			RequestFormat = WebMessageFormat.Json,
			ResponseFormat = WebMessageFormat.Json,
			BodyStyle = WebMessageBodyStyle.Wrapped)]
		string PostString(string s);

	}//end interface

}

